/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btpatterneditor
 * @short_description: the pattern editor widget
 * @see_also: #BtPattern, #BtMainPagePatterns
 *
 * Provides an editor widget for #BtPattern instances.
 */

/* TODO(ensonic): block operations
 *       o copy
 *       o paste
 *       o cut
 *       o clear
 *       o expand
 *       o shrink
 */
/* TODO(ensonic): mouse handling:
 * - selections - see bt_pattern_editor_button_press/release
 * - editing:
 *   - shift + left mb click: toggle switches
 *   - shift + left mb drag: paint values in a column
 */
/* TODO(ensonic): shift + cursor selection does not really work with the
 * buzz keybindings
 */
/* TODO(ensonic): ui styling
 * - if we want separator bars for headers, look at gtkhseparator.c
 * - padding
 *   - having some 1 pixel padding left/right of groups would look better
 *     the group gap is one whole character anyway
 * - cursor
 *   - gtk uses a black bar and it is blinking
 *   - look at GtkSettings::gtk-cursor-blink{,time,timeout}
 */
/* IDEA(ensonic): use gtk_widget_error_bell (widget) when hitting borders with
 * cursor
 */
/* for more performance, we need to render to memory surfaces and blit them when
 * scolling
 * - we need to update the memory surfaces when layout or data changes
 * - do we want to handle the cursor moves like data changes?
 * rendering to memory surfaces:
 * blitting:
 */

#define BT_EDIT

#include <ctype.h>
#include <math.h>

#include "bt-edit.h"

enum
{
  /* GtkScrollable implementation */
  PROP_HADJUSTMENT = 1,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
  /* GObject properties */
  PROP_PLAY_POSITION,
  PROP_OCTAVE,
  PROP_CURSOR_GROUP,
  PROP_CURSOR_PARAM,
  PROP_CURSOR_DIGIT,
  PROP_CURSOR_ROW
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtPatternEditor, bt_pattern_editor, GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL));

typedef struct _ParamType
{
  gint chars, columns;
  gchar *(*to_string_func) (gchar * dest, gint bufsize, gfloat value, gint def);
  guint column_pos[4];
  gfloat scale;
} ParamType;

//-- helper methods

static gchar *
to_string_note (gchar * buf, gint bufsize, gfloat value, gint def)
{
  static gchar note_names[] = "C-C#D-D#E-F-F#G-G#A-A#B-????????";
  gint note = ((gint) value) & 255, octave;

  g_return_val_if_fail (bufsize >= 4, "ERR");
  if (note == def || note == 0)
    return "...";
  if (note == 255)
    return "off";

  note--;
  octave = note >> 4;
  buf[0] = note_names[2 * (note & 15)];
  buf[1] = note_names[1 + 2 * (note & 15)];
  buf[2] = '0' + octave;
  buf[3] = '\0';
  return buf;
}

static gchar *
to_string_trigger (gchar * buf, gint bufsize, gfloat value, gint def)
{
  gint v = (gint) value;

  if (v == def)
    return ".";

  snprintf (buf, bufsize, "%01X", v != 0 ? 1 : 0);
  return buf;
}

static gchar *
to_string_byte (gchar * buf, gint bufsize, gfloat value, gint def)
{
  gint v = (gint) (value + 0.5);

  if (v == def)
    return "..";

  snprintf (buf, bufsize, "%02X", v & 255);
  return buf;
}

static gchar *
to_string_word (gchar * buf, gint bufsize, gfloat value, gint def)
{
  gint v = (gint) (value + 0.5);

  if (v == def)
    return "....";

  snprintf (buf, bufsize, "%04X", v & 65535);
  return buf;
}

static gchar *
to_string_float (gchar * buf, gint bufsize, gfloat value, gint def)
{
  gint v = (gint) value;
  if (fabs (v - def) < 0.00001)
    return "........";

  snprintf (buf, bufsize, "%-8f", value);
  return buf;
}

static ParamType param_types[] = {
  {3, 2, to_string_note, {0, 2}, 1.0},
  {1, 1, to_string_trigger, {0}, 1.0},
  {2, 2, to_string_byte, {0, 1}, 1.0},
  {4, 4, to_string_word, {0, 1, 2, 3}, 1.0},
  {8, 1, to_string_float, {0}, 65535.0},
};

static gint inline
bt_pattern_editor_rownum_width (BtPatternEditor * self)
{
  return self->cw * 5;
}

static void
bt_pattern_editor_draw_rownum (BtPatternEditor * self, cairo_t * cr,
    gint x, gint y, gint max_y)
{
  PangoLayout *pl = self->pl;
  gchar buf[16];
  gint row = 0;
  gint ch = self->ch, cw = self->cw;
  gint colw1 = bt_pattern_editor_rownum_width (self);
  gint colw = colw1 - cw;

  while (y < max_y && row < self->num_rows) {
    gdk_cairo_set_source_rgba (cr, &self->bg_shade_color[row & 0x1]);
    cairo_rectangle (cr, x, y, colw, ch);
    cairo_fill (cr);

    gdk_cairo_set_source_rgba (cr, &self->text_color);
    snprintf (buf, sizeof (buf), "%04X", row);
    cairo_move_to (cr, x, y);
    pango_layout_set_text (pl, buf, 4);
    pango_cairo_show_layout (cr, pl);

    y += ch;
    row++;
  }
}

static void
bt_pattern_editor_draw_colnames (BtPatternEditor * self, cairo_t * cr,
    gint x, gint y, gint w)
{
  PangoLayout *pl = self->pl;
  gint g;
  gint cw = self->cw;

  gtk_render_background (gtk_widget_get_style_context ((GtkWidget *) self), cr,
      x, y, w, self->ch);

  gdk_cairo_set_source_rgba (cr, &self->text_color);

  for (g = 0; g < self->num_groups; g++) {
    BtPatternEditorColumnGroup *cgrp = &self->groups[g];
    gint max_chars = ((cgrp->width / cw) - 1);

    max_chars = MIN (strlen (cgrp->name), max_chars);
    cairo_move_to (cr, x, y);
    pango_layout_set_text (pl, cgrp->name, max_chars);
    pango_cairo_show_layout (cr, pl);

    x += cgrp->width;
  }
}

static void
bt_pattern_editor_draw_rowname (BtPatternEditor * self, cairo_t * cr,
    gint x, gint y)
{
  PangoLayout *pl = self->pl;

  gtk_render_background (gtk_widget_get_style_context ((GtkWidget *) self), cr,
      x, y, bt_pattern_editor_rownum_width (self), self->ch);

  if (self->num_groups) {
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_move_to (cr, x, y);
    pango_layout_set_text (pl, "Tick", 4);
    pango_cairo_show_layout (cr, pl);
  }
}

static gboolean inline
in_selection_column (BtPatternEditor * self, guint group, guint param)
{
  // check columns
  if (self->selection_mode == PESM_COLUMN)
    return (group == self->selection_group) && (param == self->selection_param);
  if (self->selection_mode == PESM_GROUP)
    return (group == self->selection_group);
  return TRUE;                  /* PESM_ALL */
}

static gboolean inline
in_selection_row (BtPatternEditor * self, guint row)
{
  // check rows
  if (row < self->selection_start)
    return FALSE;
  if (row > self->selection_end)
    return FALSE;
  return TRUE;
}

/*
 * bt_pattern_editor_draw_column:
 * @x,@y: the top left corner for the column
 * @col,@group,@param: column data, group and param to draw
 * @max_y: max-y for clipping
 */
static void
bt_pattern_editor_draw_column (BtPatternEditor * self, cairo_t * cr,
    gint x, gint y, BtPatternEditorColumn * col,
    guint group, guint param, gint max_y)
{
  PangoLayout *pl = self->pl;
  ParamType *pt = &param_types[col->type];
  gchar buf[16], *str;
  gint row = 0;
  gint cw = self->cw, ch = self->ch;
  gint col_w = cw * (pt->chars + 1);
  gint col_w2 = col_w - (param == self->groups[group].num_columns - 1 ? cw : 0);
  gfloat (*get_data_func) (gpointer pattern_data, gpointer column_data,
      guint row, guint group, guint param) = self->callbacks->get_data_func;

  gboolean is_cursor_column = (param == self->parameter
      && group == self->group);
  gboolean is_selection_column = (self->selection_enabled
      && in_selection_column (self, group, param));

  while (y < max_y && row < self->num_rows) {
    gint col_w3 = col_w2;
    gfloat pval = get_data_func (self->pattern_data, col->user_data, row, group,
        param);
    gboolean sel = (is_selection_column && in_selection_row (self, row));
    str = pt->to_string_func (buf, sizeof (buf), pval, col->def);

    /* draw background */
    gdk_cairo_set_source_rgba (cr, &self->bg_shade_color[row & 0x1]);
    if (sel) {
      /* the last space should be selected if it's a within-group "glue"
         in multiple column selection, row colour otherwise */
      if (self->selection_mode == PESM_COLUMN) {
        /* draw row-coloured "continuation" after column, unless last column in
           a group */
        col_w3 = col_w - cw;
        if (param != self->groups[group].num_columns - 1) {
          cairo_rectangle (cr, x + col_w3, y, cw, ch);
          cairo_fill (cr);
        }
      }
      /* draw selected column+continuation (unless last column, then don't draw
         continuation) */
      gdk_cairo_set_source_rgba (cr, &self->sel_color[row & 0x1]);
    }
    cairo_rectangle (cr, x, y, col_w3, ch);
    cairo_fill (cr);

    // draw value bar
    if (!sel && (str[0] != '.')) {
      gdk_cairo_set_source_rgba (cr, &self->value_color[row & 0x1]);
      cairo_rectangle (cr, x, y, (col_w - cw) * (pval / (col->max * pt->scale)),
          ch);
      cairo_fill (cr);
    }
    // draw cursor
    if (row == self->row && is_cursor_column) {
      gint cp = pt->column_pos[self->digit];
      gdk_cairo_set_source_rgba (cr, &self->cursor_color);
      cairo_rectangle (cr, x + cw * cp, y, cw, ch);
      cairo_fill (cr);
    }
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_move_to (cr, x, y);
    pango_layout_set_text (pl, str, pt->chars);
    pango_cairo_show_layout (cr, pl);

    y += ch;
    row++;
  }
}

static gint
bt_pattern_editor_get_row_width (BtPatternEditor * self)
{
  gint width = self->rowhdr_width, g;
  for (g = 0; g < self->num_groups; g++)
    width += self->groups[g].width;
  return width;
}

static gint
bt_pattern_editor_get_col_height (BtPatternEditor * self)
{
  return (self->colhdr_height + self->num_rows * self->ch);
}

static void
bt_pattern_editor_refresh_cell (BtPatternEditor * self)
{
  gint y = self->colhdr_height + (self->row * self->ch) - self->ofs_y;
  gint x = self->rowhdr_width - self->ofs_x;
  gint g, i, w;
  BtPatternEditorColumnGroup *cgrp;
  BtPatternEditorColumn *col;
  ParamType *pt;
  GtkAllocation allocation;

  for (g = 0; g < self->group; g++) {
    cgrp = &self->groups[g];
    x += cgrp->width;
  }
  cgrp = &self->groups[self->group];
  for (i = 0; i < self->parameter; i++) {
    col = &cgrp->columns[i];
    pt = &param_types[col->type];
    x += self->cw * (pt->chars + 1);
  }
  col = &cgrp->columns[self->parameter];
  pt = &param_types[col->type];
  w = self->cw * (pt->chars + 1);

  gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
  x += allocation.x;
  y += allocation.y;

  //GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",x, y, w, self->ch);
  gtk_widget_queue_draw_area (GTK_WIDGET (self), x, y, w, self->ch);
}

static void
bt_pattern_editor_refresh_cursor (BtPatternEditor * self)
{
  gint y = self->colhdr_height + (self->row * self->ch) - self->ofs_y;
  gint x = self->rowhdr_width - self->ofs_x;
  gint g, i;
  BtPatternEditorColumnGroup *cgrp;
  BtPatternEditorColumn *col;
  ParamType *pt;
  GtkAllocation allocation;

  if (!self->num_groups)
    return;

  for (g = 0; g < self->group; g++) {
    cgrp = &self->groups[g];
    x += cgrp->width;
  }
  cgrp = &self->groups[self->group];
  for (i = 0; i < self->parameter; i++) {
    col = &cgrp->columns[i];
    pt = &param_types[col->type];
    x += self->cw * (pt->chars + 1);
  }
  col = &cgrp->columns[self->parameter];
  pt = &param_types[col->type];
  x += self->cw * pt->column_pos[self->digit];

  gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
  x += allocation.x;
  y += allocation.y;

  //GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",x, y, self->cw, self->ch);
  gtk_widget_queue_draw_area (GTK_WIDGET (self), x, y, self->cw, self->ch);
}

static void
bt_pattern_editor_refresh_cursor_or_scroll (BtPatternEditor * self)
{
  GtkAllocation allocation;
  GtkRequisition requisition;
  gboolean draw = TRUE;

  gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
  gtk_widget_get_preferred_size (GTK_WIDGET (self), NULL, &requisition);

  if (self->group < self->num_groups) {
    gint g, p;
    gint xpos = 0;
    BtPatternEditorColumnGroup *grp = NULL;

    for (g = 0; g < self->group; g++)
      xpos += self->groups[g].width;
    grp = &self->groups[g];
    for (p = 0; p < self->parameter; p++)
      xpos += self->cw * (param_types[grp->columns[p].type].chars + 1);
    if (p < grp->num_columns) {
      gint cpos = self->ofs_x;
      gint w = self->cw * param_types[grp->columns[p].type].chars;      /* column width */
      gint ww = requisition.width - self->rowhdr_width; /* full width, less rowheader */
      gint pw = allocation.width - self->rowhdr_width;  /* visible (scrolled) width, less rowheader */
      /* rowheader is subtracted, because it is not taken into account in hscroll range */
      GST_INFO ("xpos = %d cpos = %d w = %d width = %d / %d", xpos, cpos, w, ww,
          pw);
      if (xpos < cpos || xpos + w > cpos + pw) {
        /* if current parameter doesn't fit, try to center the cursor */
        xpos = xpos - pw / 2;
        if (xpos + pw > ww)
          xpos = ww - pw;
        if (xpos < 0)
          xpos = 0;
        gtk_adjustment_set_value (self->hadj, xpos);
        draw = FALSE;
      }
    }
  }

  {
    gint ypos = self->row * self->ch;
    gint cpos = self->ofs_y;
    gint h = self->ch;
    gint wh = requisition.height - self->colhdr_height; /* full heigth, less colheader */
    gint ph = allocation.height - self->colhdr_height;  /* visible (scrolled) heigth, less colheader */
    /* colheader is subtracted, because it is not taken into account in vscroll range */
    GST_INFO ("ypos = %d cpos = %d h = %d height = %d / %d", ypos, cpos, h, wh,
        ph);
    if (ypos < cpos || ypos + h > cpos + ph) {
      /* if current parameter doesn't fit, try to center the cursor */
      ypos = ypos - ph / 2;
      if (ypos + ph > wh)
        ypos = wh - ph;
      if (ypos < 0)
        ypos = 0;
      gtk_adjustment_set_value (self->vadj, ypos);
      draw = FALSE;
    }
  }
  if (draw) {
    // setting the adjustments already draws
    bt_pattern_editor_refresh_cursor (self);
  }
}

static void
advance (BtPatternEditor * self)
{
  if (self->row < self->num_rows - 1) {
    // invalidate old pos
    bt_pattern_editor_refresh_cursor (self);
    self->row += self->step;
    if (self->row > self->num_rows - 1)
      self->row = self->num_rows - 1;
    bt_pattern_editor_refresh_cursor_or_scroll (self);
  }
}

static BtPatternEditorColumn *
cur_column (BtPatternEditor * self)
{
  return &self->groups[self->group].columns[self->parameter];
}

static gboolean
char_to_coords (gint charpos, BtPatternEditorColumn * columns, gint num_cols,
    gint * parameter, gint * digit)
{
  gint i, j;
  for (i = 0; i < num_cols; i++) {
    ParamType *type = &param_types[columns[i].type];
    if (charpos < type->chars) {
      for (j = 0; j < type->columns; j++) {
        gint cps = type->column_pos[j], cpe = cps;
        if (columns[i].type == PCT_NOTE && j == 0)
          cpe++;
        if (charpos >= cps && charpos <= cpe) {
          *parameter = i;
          *digit = j;
          return TRUE;
        }
      }
      return FALSE;
    }
    charpos -= type->chars + 1;
  }
  return FALSE;
}

static void
bt_pattern_editor_configure_style (BtPatternEditor * self,
    GtkStyleContext * style_ctx)
{
  gtk_style_context_lookup_color (style_ctx, "playline_color",
      &self->play_pos_color);

  gtk_style_context_lookup_color (style_ctx, "row_even_color",
      &self->bg_shade_color[0]);
  gtk_style_context_lookup_color (style_ctx, "row_odd_color",
      &self->bg_shade_color[1]);

  gtk_style_context_get_color (style_ctx, GTK_STATE_FLAG_NORMAL,
      &self->text_color);

  gtk_style_context_lookup_color (style_ctx, "selection1_color",
      &self->sel_color[0]);
  gtk_style_context_lookup_color (style_ctx, "selection2_color",
      &self->sel_color[1]);

  gtk_style_context_lookup_color (style_ctx, "cursor_color",
      &self->cursor_color);

  self->value_color[0].red = self->bg_shade_color[0].red * 0.8;
  self->value_color[0].green = self->bg_shade_color[0].green * 0.8;
  self->value_color[0].blue = self->bg_shade_color[0].blue * 0.8;
  self->value_color[0].alpha = self->bg_shade_color[0].alpha;
  self->value_color[1].red = self->bg_shade_color[1].red * 0.8;
  self->value_color[1].green = self->bg_shade_color[1].green * 0.8;
  self->value_color[1].blue = self->bg_shade_color[1].blue * 0.8;
  self->value_color[1].alpha = self->bg_shade_color[1].alpha;
}

static void
bt_pattern_editor_style_updated (GtkWidget * widget)
{
  bt_pattern_editor_configure_style (BT_PATTERN_EDITOR (widget),
      gtk_widget_get_style_context (widget));

  GTK_WIDGET_CLASS (bt_pattern_editor_parent_class)->style_updated (widget);
}

//-- constructor methods

/**
 * bt_pattern_editor_new:
 *
 * Create a new pattern editor widget. Use bt_pattern_editor_set_pattern() to
 * pass pattern data.
 *
 * Returns: the widget
 */
GtkWidget *
bt_pattern_editor_new (void)
{
  return GTK_WIDGET (g_object_new (BT_TYPE_PATTERN_EDITOR, NULL));
}

//-- class internals

static void
bt_pattern_editor_realize (GtkWidget * widget)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);
  GdkWindow *window;
  GtkAllocation allocation;
  GdkWindowAttr attributes;
  GtkStyleContext *style_ctx;
  gint attributes_mask;
  PangoFontDescription *style_pfd;
  PangoFontDescription *pfd;
  PangoContext *pc;
  PangoFontMetrics *pfm;

  g_return_if_fail (BT_IS_PATTERN_EDITOR (widget));

  gtk_widget_set_realized (widget, TRUE);

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) |
      GDK_EXPOSURE_MASK | GDK_SCROLL_MASK | GDK_KEY_PRESS_MASK |
      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  self->window = gdk_window_new (window, &attributes, attributes_mask);
#if GTK_CHECK_VERSION (3,8,0)
  gtk_widget_register_window (widget, self->window);
#else
  gdk_window_set_user_data (self->window, widget);
#endif

  style_ctx = gtk_widget_get_style_context (widget);
  gtk_style_context_add_class (style_ctx, GTK_STYLE_CLASS_VIEW);

  // setup graphic styles
  bt_pattern_editor_configure_style (self, style_ctx);

  // TODO(ensonic): can we make the font part of the css?
  gtk_style_context_get (style_ctx, GTK_STATE_FLAG_NORMAL,
      GTK_STYLE_PROPERTY_FONT, &style_pfd, NULL);

  /* copy size from default font and use default monospace font */
  GST_WARNING (" default font: size %d (is_absolute %d?), scl=%lf",
      pango_font_description_get_size (style_pfd),
      pango_font_description_get_size_is_absolute (style_pfd),
      (gdouble) PANGO_SCALE);

  /* calculate font-metrics */
  pc = gtk_widget_get_pango_context (widget);
  pfd = pango_font_description_new ();

  //pango_font_description_set_family_static (pfd, "Bitstream Vera Sans Mono,");
  //pango_font_description_set_absolute_size (pfd, 12 * PANGO_SCALE);
  pango_font_description_set_family_static (pfd, "monospace,");
  if (pango_font_description_get_size_is_absolute (style_pfd)) {
    pango_font_description_set_absolute_size (pfd,
        pango_font_description_get_size (style_pfd));
  } else {
    pango_font_description_set_size (pfd,
        pango_font_description_get_size (style_pfd));
  }
  pango_font_description_free (style_pfd);
  pango_context_load_font (pc, pfd);

  pfm = pango_context_get_metrics (pc, pfd, NULL);
  self->cw = pango_font_metrics_get_approximate_digit_width (pfm) / PANGO_SCALE;
  self->ch =
      (pango_font_metrics_get_ascent (pfm) +
      pango_font_metrics_get_descent (pfm)) / PANGO_SCALE;
  pango_font_metrics_unref (pfm);

  self->pl = pango_layout_new (pc);
  pango_layout_set_font_description (self->pl, pfd);
  pango_font_description_free (pfd);

  /* static layout variables */
  self->rowhdr_width = bt_pattern_editor_rownum_width (self) + self->cw;
  self->colhdr_height = self->ch;

  GST_INFO ("char size: %d x %d", self->cw, self->ch);
}

static void
bt_pattern_editor_unrealize (GtkWidget * widget)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);

  g_object_unref (self->pl);
  self->pl = NULL;

  if (self->hadj) {
    g_object_unref (self->hadj);
    self->hadj = NULL;
  }
  if (self->vadj) {
    g_object_unref (self->vadj);
    self->vadj = NULL;
  }
#if GTK_CHECK_VERSION (3,8,0)
  gtk_widget_unregister_window (widget, self->window);
#else
  gdk_window_set_user_data (self->window, NULL);
#endif
  gdk_window_destroy (self->window);
  self->window = NULL;

  GTK_WIDGET_CLASS (bt_pattern_editor_parent_class)->unrealize (widget);
}

static void
bt_pattern_editor_map (GtkWidget * widget)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);

  GTK_WIDGET_CLASS (bt_pattern_editor_parent_class)->map (widget);

  gdk_window_show (self->window);
}

static void
bt_pattern_editor_unmap (GtkWidget * widget)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);

  gdk_window_hide (self->window);

  GTK_WIDGET_CLASS (bt_pattern_editor_parent_class)->unmap (widget);
}


/* TODO(ensonic): speedup
 * - refactor layout and redrawing
 *   - calculate layout in bt_pattern_editor_size_allocate() and cache (what exactly)
 *   - only redraw in expose
 * - do columns in two passes to avoid changing the color for the text
 */
static gboolean
bt_pattern_editor_draw (GtkWidget * widget, cairo_t * cr)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);
  GtkStyleContext *style;
  GtkAllocation allocation;
  gint y, x, i, g, max_y;
  gint grp_x;
  gint ch;

  g_return_val_if_fail (BT_IS_PATTERN_EDITOR (widget), FALSE);

  gtk_widget_get_allocation (widget, &allocation);

  style = gtk_widget_get_style_context (widget);
  gtk_render_background (style, cr, 0, 0, allocation.width, allocation.height);

  if (self->hadj) {
    self->ofs_x = (gint) gtk_adjustment_get_value (self->hadj);
    self->ofs_y = (gint) gtk_adjustment_get_value (self->vadj);
  }

  /* leave space for headers */
  x = self->rowhdr_width;
  y = ch = self->ch;
  /* calculate the first and last row in the dirty region */
  max_y = allocation.height + ch;       // one extra line
  max_y = ch + (gint) (ch * ((max_y - ch) / ch));

  GST_DEBUG ("Scroll: %d,%d, rows: %d", self->ofs_x, self->ofs_y,
      self->num_rows);

  /* draw group parameter columns */
  for (g = 0; g < self->num_groups; g++) {
    BtPatternEditorColumnGroup *cgrp = &self->groups[g];
    grp_x = x;
    for (i = 0; i < cgrp->num_columns; i++) {
      BtPatternEditorColumn *col = &cgrp->columns[i];
      ParamType *pt = &param_types[col->type];
      gint w = self->cw * (pt->chars + 1);
      gint xs = x - self->ofs_x, xe = xs + (w - 1);

      // check intersection
      if ((xs <= allocation.width) || (xe <= allocation.width)
          || (xe >= allocation.width)) {
        GST_DEBUG ("Draw Group/Column: %d,%d : x=%3d-%3d : y=%3d..%3d", g, i,
            xs, xe, y - self->ofs_y, max_y);
        bt_pattern_editor_draw_column (self, cr, x - self->ofs_x,
            y - self->ofs_y, col, g, i, max_y);
      } else {
        GST_DEBUG ("Skip Group/Column: %d,%d : x=%3d-%3d", g, i, xs, xe);
      }
      x += w;
    }
    x += self->cw;
    cgrp->width = x - grp_x;
  }

  /* draw left and top headers */
  bt_pattern_editor_draw_rownum (self, cr, 0, y - self->ofs_y, max_y);
  bt_pattern_editor_draw_colnames (self, cr, self->rowhdr_width - self->ofs_x,
      0, allocation.width);
  bt_pattern_editor_draw_rowname (self, cr, 0, 0);

  /* draw play-pos */
  if (self->play_pos >= 0.0) {
    gdouble h =
        (gdouble) (bt_pattern_editor_get_col_height (self) -
        self->colhdr_height);
    y = self->colhdr_height + (gint) (self->play_pos * h) - self->ofs_y;
    if (y > self->colhdr_height) {
      gdk_cairo_set_source_rgba (cr, &self->play_pos_color);
      cairo_set_line_width (cr, 2.0);
      cairo_move_to (cr, 0, y);
      cairo_line_to (cr, allocation.width, y);
      cairo_stroke (cr);
      //GST_INFO("Draw playline: %d,%d -> %d,%d",0, y);
    }
  }

  return FALSE;
}

static void
bt_pattern_editor_update_adjustments (BtPatternEditor * self)
{
  GtkAllocation allocation;

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return;

  gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);

  if (self->hadj) {
    gdouble lower = gtk_adjustment_get_lower (self->hadj);
    gdouble upper =
        MAX (allocation.width, bt_pattern_editor_get_row_width (self));
    gdouble step_increment = allocation.width * 0.1;
    gdouble page_increment = allocation.width * 0.9;
    gdouble page_size = allocation.width;
    gdouble value = gtk_adjustment_get_value (self->hadj);

    value = CLAMP (value, 0, upper - page_size);
    GST_INFO ("h: v=%lf, %lf..%lf / %lf", value, lower, upper, page_size);
    gtk_adjustment_configure (self->hadj, value, lower, upper, step_increment,
        page_increment, page_size);
  }
  if (self->vadj) {
    gdouble lower = gtk_adjustment_get_lower (self->vadj);
    gdouble upper =
        MAX (allocation.height, bt_pattern_editor_get_col_height (self));
    gdouble step_increment = allocation.height * 0.1;
    gdouble page_increment = allocation.height * 0.9;
    gdouble page_size = allocation.height;
    gdouble value = gtk_adjustment_get_value (self->vadj);

    value = CLAMP (value, 0, upper - page_size);
    GST_INFO ("v: v=%lf, %lf..%lf / %lf", value, lower, upper, page_size);
    gtk_adjustment_configure (self->vadj, value, lower, upper, step_increment,
        page_increment, page_size);
  }
}

static void
bt_pattern_editor_get_preferred_width (GtkWidget * widget, gint * minimal_width,
    gint * natural_width)
{
  /* calculate from pattern size */
  *minimal_width = *natural_width =
      bt_pattern_editor_get_row_width (BT_PATTERN_EDITOR (widget));
  GST_DEBUG ("%d", *minimal_width);
}

static void
bt_pattern_editor_get_preferred_height (GtkWidget * widget,
    gint * minimal_height, gint * natural_height)
{
  /* calculate from pattern size */
  *minimal_height = *natural_height =
      bt_pattern_editor_get_col_height (BT_PATTERN_EDITOR (widget));
  GST_DEBUG ("%d", *minimal_height);
}

static void
bt_pattern_editor_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);

  gtk_widget_set_allocation (widget, allocation);
  GST_INFO ("size_allocate: %d,%d %d,%d", allocation->x, allocation->y,
      allocation->width, allocation->height);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (self->window,
        allocation->x, allocation->y, allocation->width, allocation->height);

  bt_pattern_editor_update_adjustments (self);
}

static gboolean
bt_pattern_editor_key_press (GtkWidget * widget, GdkEventKey * event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);

  GST_INFO
      ("pattern_editor key : state 0x%x, keyval 0x%x, hw-code 0x%x, name %s",
      event->state, event->keyval, event->hardware_keycode,
      gdk_keyval_name (event->keyval));

  if (self->num_groups) {
    // map keypad numbers to ordinary numbers
    if (event->keyval >= GDK_KEY_KP_0 && event->keyval <= GDK_KEY_KP_9) {
      event->keyval -= (GDK_KEY_KP_0 - '0');
    }

    if (!(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) &&
        (event->keyval >= 32 && event->keyval < 127) &&
        self->callbacks->set_data_func) {
      BtPatternEditorColumn *col =
          &self->groups[self->group].columns[self->parameter];

      GST_INFO ("edit group %d, parameter %d, digit %d, type %d",
          self->group, self->parameter, self->digit, col->type);
      if (event->keyval == '.') {
        self->callbacks->set_data_func (self->pattern_data, col->user_data,
            self->row, self->group, self->parameter, self->digit, col->def);
        bt_pattern_editor_refresh_cell (self);
        advance (self);
      } else {
        static const gchar hexdigits[] = "0123456789abcdef";
        //static const char notenames[] = "zsxdcvgbhnjm\t\t\t\tq2w3er5t6y7u\t\t\t\ti9o0p";
        static const gchar notenames[] =
            "\x34\x27\x35\x28\x36\x37\x2a\x38\x2b\x39\x2c\x3a\x3a\x3a\x3a\x3a\x18\x0b\x19\x0c\x1a\x1b\x0e\x1c\x0f\x1d\x10\x1e\x1e\x1e\x1e\x1e\x1f\x12\x20\x13\x21";
        gfloat oldvalue =
            self->callbacks->get_data_func (self->pattern_data, col->user_data,
            self->row, self->group, self->parameter);
        const gchar *p;

        switch (col->type) {
          case PCT_FLOAT:
            // no editing yet
            break;
          case PCT_SWITCH:
            if (event->keyval == '0' || event->keyval == '1') {
              self->callbacks->set_data_func (self->pattern_data,
                  col->user_data, self->row, self->group, self->parameter,
                  self->digit, event->keyval - '0');
              advance (self);
            }
            break;
          case PCT_BYTE:
          case PCT_WORD:
            p = strchr (hexdigits, (char) event->keyval);
            if (p) {
              gint value = (gint) oldvalue;
              if (oldvalue == col->def)
                value = 0;
              gint shift =
                  4 * (((col->type == PCT_BYTE) ? 1 : 3) - self->digit);
              value = (value & ~(15 << shift)) | ((p - hexdigits) << shift);
              if (value < col->min)
                value = col->min;
              if (value > col->max)
                value = col->max;

              self->callbacks->set_data_func (self->pattern_data,
                  col->user_data, self->row, self->group, self->parameter,
                  self->digit, value);
              bt_pattern_editor_refresh_cell (self);
              advance (self);
            }
            break;
          case PCT_NOTE:
            if (self->digit == 0 && event->keyval == '1') {
              // note off
              self->callbacks->set_data_func (self->pattern_data,
                  col->user_data, self->row, self->group, self->parameter,
                  self->digit, 255);
              bt_pattern_editor_refresh_cell (self);
              advance (self);
              break;
            }
            if (self->digit == 0 && event->hardware_keycode <= 255) {
              // use event->hardware_keycode because of y<>z
              p = strchr (notenames, (char) event->hardware_keycode);
              if (p) {
                gint value =
                    GSTBT_NOTE_C_0 + (p - notenames) + 16 * self->octave;
                if (value < col->min)
                  value = col->min;
                if (value > col->max)
                  value = col->max;
                if ((value & 15) <= 12) {
                  self->callbacks->set_data_func (self->pattern_data,
                      col->user_data, self->row, self->group, self->parameter,
                      self->digit, value);
                  bt_pattern_editor_refresh_cell (self);
                  advance (self);
                }
              }
            }
            if (self->digit == 1) {
              if (isdigit (event->keyval)) {
                gint value = (gint) oldvalue;
                if (oldvalue == col->def)
                  value = 1;
                value = (value & 15) | ((event->keyval - '0') << 4);

                // note range = 1..12 and not 0..11
                if (value >= col->min && value <= col->max
                    && (value & 15) <= 12) {
                  self->callbacks->set_data_func (self->pattern_data,
                      col->user_data, self->row, self->group, self->parameter,
                      self->digit, value);
                  bt_pattern_editor_refresh_cell (self);
                  advance (self);
                }
              }
            }
            break;
        }
      }
    } else {
      gint kup = toupper (event->keyval);
      gint control = (event->state & GDK_CONTROL_MASK) != 0;
      gint shift = (event->state & GDK_SHIFT_MASK) != 0;
      gint modifier = event->state & gtk_accelerator_get_default_mod_mask ();

      GST_INFO ("cmd group %d, parameter %d, digit %d",
          self->group, self->parameter, self->digit);

      if (control && event->keyval >= '1' && event->keyval <= '9') {
        self->step = event->keyval - '0';
        return TRUE;
      } else if (control && kup >= 'A' && kup <= 'Z') {
        gboolean same = FALSE, handled = TRUE;

        switch (kup) {
          case 'B':
          case 'E':
            if (kup == 'B') {
              same = self->selection_enabled
                  && (self->selection_start == self->row)
                  && (self->selection_group == self->group)
                  && (self->selection_param == self->parameter);
              self->selection_enabled = TRUE;
              self->selection_start = self->row;
              if (self->selection_end < self->row)
                self->selection_end = self->row;
            } else {            /* Ctrl+E */
              same = self->selection_enabled
                  && (self->selection_end == self->row)
                  && (self->selection_group == self->group)
                  && (self->selection_param == self->parameter);
              self->selection_enabled = TRUE;
              self->selection_end = self->row;
              if (self->selection_start > self->row)
                self->selection_start = self->row;
            }

            /* if Ctrl+B(E) was pressed again, cycle selection mode */
            if (same) {
              if (self->selection_mode < (self->num_groups ==
                      1 ? PESM_GROUP : PESM_ALL))
                self->selection_mode++;
              else
                self->selection_mode = PESM_COLUMN;
            }
            self->selection_group = self->group;
            self->selection_param = self->parameter;
            break;
          case 'U':
            self->selection_enabled = FALSE;
            break;
          case 'L':
          case 'K':
          case 'A':
            self->selection_enabled = TRUE;
            self->selection_mode =
                kup == 'K' ? PESM_GROUP : (kup == 'L' ? PESM_COLUMN : PESM_ALL);
            self->selection_start = 0;
            self->selection_end = self->num_rows - 1;
            self->selection_group = self->group;
            self->selection_param = self->parameter;
            break;
          default:
            handled = FALSE;
            break;
        }
        if (handled) {
          // selection changed
          gtk_widget_queue_draw (GTK_WIDGET (self));
          return TRUE;
        }
      }
      switch (event->keyval) {
        case GDK_KEY_Up:
          if (!modifier) {
            if (self->row > 0) {
              // invalidate old pos
              bt_pattern_editor_refresh_cursor (self);
              self->row -= 1;
              g_object_notify ((gpointer) self, "cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll (self);
            }
            return TRUE;
          }
          break;
        case GDK_KEY_Down:
          if (!modifier) {
            if (self->row < self->num_rows - 1) {
              // invalidate old pos
              bt_pattern_editor_refresh_cursor (self);
              self->row += 1;
              g_object_notify ((gpointer) self, "cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll (self);
            }
            return TRUE;
          }
          break;
        case GDK_KEY_Page_Up:
          if (!modifier) {
            if (self->row > 0) {
              bt_pattern_editor_refresh_cursor (self);
              /* TODO(ensonic): should we tell pattern editor about the meassure
               * g_object_get(song_info,"bars",&bars,NULL); for 4/4 => 16
               */
              if (control)
                self->row = 0;
              else {
                gint p = (gint) self->row - 16;
                self->row = p < 0 ? 0 : p;
              }
              g_object_notify ((gpointer) self, "cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll (self);
            }
            return TRUE;
          }
          break;
        case GDK_KEY_Page_Down:
          if (!modifier) {
            if (self->row < self->num_rows - 1) {
              bt_pattern_editor_refresh_cursor (self);
              /* TODO(ensonic): see Page_Up */
              self->row = control ? self->num_rows - 1 : self->row + 16;
              if (self->row > self->num_rows - 1)
                self->row = self->num_rows - 1;
              g_object_notify ((gpointer) self, "cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll (self);
            }
            return TRUE;
          }
          break;
        case GDK_KEY_Home:
          bt_pattern_editor_refresh_cursor (self);
          if (control) {
            /* shouldn't this go to top left */
            self->digit = 0;
            self->parameter = 0;
            g_object_notify ((gpointer) self, "cursor-param");
          } else {
            /* go to begin of cell, group, pattern, top of pattern */
            if (self->digit > 0) {
              self->digit = 0;
            } else if (self->parameter > 0) {
              self->parameter = 0;
              g_object_notify ((gpointer) self, "cursor-param");
            } else if (self->group > 0) {
              self->group = 0;
              g_object_notify ((gpointer) self, "cursor-group");
            } else {
              self->row = 0;
              g_object_notify ((gpointer) self, "cursor-row");
            }
          }
          bt_pattern_editor_refresh_cursor_or_scroll (self);
          return TRUE;
        case GDK_KEY_End:
          bt_pattern_editor_refresh_cursor (self);
          if (control) {
            /* shouldn't this go to bottom right */
            if (self->groups[self->group].num_columns > 0)
              self->parameter = self->groups[self->group].num_columns - 1;
            g_object_notify ((gpointer) self, "cursor-param");
          } else {
            /* go to end of cell, group, pattern, bottom of pattern */
            BtPatternEditorColumn *pc = cur_column (self);
            if (self->digit < param_types[pc->type].columns - 1) {
              self->digit = param_types[pc->type].columns - 1;
            } else if (self->parameter <
                self->groups[self->group].num_columns - 1) {
              self->parameter = self->groups[self->group].num_columns - 1;
              g_object_notify ((gpointer) self, "cursor-param");
            } else if (self->group < self->num_groups - 1) {
              self->group = self->num_groups - 1;
              g_object_notify ((gpointer) self, "cursor-group");
            } else {
              self->row = self->num_rows - 1;
              g_object_notify ((gpointer) self, "cursor-row");
            }
          }
          bt_pattern_editor_refresh_cursor_or_scroll (self);
          return TRUE;
        case GDK_KEY_Left:
          if (!modifier) {
            bt_pattern_editor_refresh_cursor (self);
            if (self->digit > 0)
              self->digit--;
            else {
              BtPatternEditorColumn *pc;
              if (self->parameter > 0) {
                self->parameter--;
                g_object_notify ((gpointer) self, "cursor-param");
              } else if (self->group > 0) {
                self->group--;
                self->parameter = self->groups[self->group].num_columns - 1;
                /* only notify group, param will be read along anyway */
                g_object_notify ((gpointer) self, "cursor-group");
              } else
                return FALSE;
              pc = cur_column (self);
              self->digit = param_types[pc->type].columns - 1;
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          } else if (shift) {
            bt_pattern_editor_refresh_cursor (self);
            if (self->parameter > 0) {
              self->parameter--;
              self->digit = 0;
              g_object_notify ((gpointer) self, "cursor-param");
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
        case GDK_KEY_Right:
          if (!modifier) {
            BtPatternEditorColumn *pc = cur_column (self);
            bt_pattern_editor_refresh_cursor (self);
            if (self->digit < param_types[pc->type].columns - 1) {
              self->digit++;
            } else if (self->parameter <
                self->groups[self->group].num_columns - 1) {
              self->parameter++;
              self->digit = 0;
              g_object_notify ((gpointer) self, "cursor-param");
            } else if (self->group < self->num_groups - 1) {
              self->group++;
              self->parameter = 0;
              self->digit = 0;
              /* only notify group, param will be read along anyway */
              g_object_notify ((gpointer) self, "cursor-group");
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          } else if (shift) {
            bt_pattern_editor_refresh_cursor (self);
            if (self->parameter < self->groups[self->group].num_columns - 1) {
              self->parameter++;
              self->digit = 0;
              g_object_notify ((gpointer) self, "cursor-param");
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
        case GDK_KEY_Tab:
          if (!modifier) {
            bt_pattern_editor_refresh_cursor (self);
            if (self->group < self->num_groups - 1) {
              /* jump to same column when jumping from track to track, otherwise jump to first column of the group */
              self->group++;
              if (self->parameter >= self->groups[self->group].num_columns) {
                self->parameter = self->groups[self->group].num_columns - 1;
              }
              if (self->groups[self->group].columns[self->parameter].type !=
                  self->groups[self->group - 1].columns[self->parameter].type) {
                self->digit = 0;
              }
              g_object_notify ((gpointer) self, "cursor-group");
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
        case GDK_KEY_ISO_Left_Tab:
          if (shift) {
            bt_pattern_editor_refresh_cursor (self);
            if (self->group > 0) {
              self->group--;
              if (self->parameter >= self->groups[self->group].num_columns) {
                self->parameter = self->groups[self->group].num_columns - 1;
              }
              if (self->groups[self->group].columns[self->parameter].type !=
                  self->groups[self->group + 1].columns[self->parameter].type) {
                self->digit = 0;
              }
              g_object_notify ((gpointer) self, "cursor-group");
            } else              /* at leftmost group, reset cursor to first column */
              self->parameter = 0, self->digit = 0;
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
      }
    }
  }
  return FALSE;
}

static gboolean
bt_pattern_editor_button_press (GtkWidget * widget, GdkEventButton * event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (widget);
  gint row, group, parameter, digit;

  gtk_widget_grab_focus_savely (GTK_WIDGET (self));

  if (!bt_pattern_editor_position_to_coords (self, event->x, event->y, &row,
          &group, &parameter, &digit))
    return FALSE;

  bt_pattern_editor_refresh_cursor (self);
  if (row > -1) {
    self->row = row;
    g_object_notify ((gpointer) self, "cursor-row");
  }
  if (group > -1) {
    self->group = group;
    self->parameter = parameter;
    self->digit = digit;
    g_object_notify ((gpointer) self, "cursor-group");
    g_object_notify ((gpointer) self, "cursor-param");
    g_object_notify ((gpointer) self, "cursor-digit");
  }
  bt_pattern_editor_refresh_cursor_or_scroll (self);
  return TRUE;
}

static gboolean
bt_pattern_editor_button_release (GtkWidget * widget, GdkEventButton * event)
{
  return FALSE;
}

static void
bt_pattern_editor_set_hadjustment (BtPatternEditor * self, GtkAdjustment * adj)
{
  if (self->hadj == adj)
    return;

  if (self->hadj) {
    g_signal_handlers_disconnect_by_func (self->hadj, gtk_widget_queue_draw,
        self);
    g_object_unref (self->hadj);
  }
  if (!adj) {
    adj = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  }

  self->hadj = g_object_ref_sink (adj);
  g_signal_connect_swapped (self->hadj, "value-changed",
      G_CALLBACK (gtk_widget_queue_draw), self);

  bt_pattern_editor_update_adjustments (self);
}

static void
bt_pattern_editor_set_vadjustment (BtPatternEditor * self, GtkAdjustment * adj)
{
  if (self->vadj == adj)
    return;

  if (self->vadj) {
    g_signal_handlers_disconnect_by_func (self->vadj, gtk_widget_queue_draw,
        self);
    g_object_unref (self->vadj);
  }
  if (!adj) {
    adj = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  }

  self->vadj = g_object_ref_sink (adj);
  g_signal_connect_swapped (self->vadj, "value-changed",
      G_CALLBACK (gtk_widget_queue_draw), self);

  bt_pattern_editor_update_adjustments (self);
}

static void
bt_pattern_editor_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (object);

  switch (property_id) {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, self->hadj);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, self->vadj);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, self->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, self->vscroll_policy);
      break;
    case PROP_CURSOR_GROUP:
      g_value_set_uint (value, self->group);
      break;
    case PROP_CURSOR_PARAM:
      g_value_set_uint (value, self->parameter);
      break;
    case PROP_CURSOR_DIGIT:
      g_value_set_uint (value, self->digit);
      break;
    case PROP_CURSOR_ROW:
      g_value_set_uint (value, self->row);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_editor_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (object);

  switch (property_id) {
    case PROP_HADJUSTMENT:
      bt_pattern_editor_set_hadjustment (self, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      bt_pattern_editor_set_vadjustment (self, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      self->hscroll_policy = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;
    case PROP_VSCROLL_POLICY:
      self->vscroll_policy = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;
    case PROP_PLAY_POSITION:{
      gdouble old_pos = self->play_pos;
      self->play_pos = g_value_get_double (value);
      if (gtk_widget_get_realized (GTK_WIDGET (self)) &&
          (old_pos != self->play_pos)) {
        GtkAllocation allocation;
        gdouble h = (gdouble) bt_pattern_editor_get_col_height (self)
            - (gdouble) self->colhdr_height;
        gint w, x, y, yo;

        gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
        w = allocation.width;
        x = allocation.x;
        yo = allocation.y + self->colhdr_height - self->ofs_y;

        y = yo + (gint) (old_pos * h);
        gtk_widget_queue_draw_area (GTK_WIDGET (self), x, (y - 1), w, 2);
        GST_INFO ("Mark Area Dirty: %d,%d -> %d,%d", 0, y - 1, w, 2);
        y = yo + (gint) (self->play_pos * h);
        gtk_widget_queue_draw_area (GTK_WIDGET (self), x, (y - 1), w, 2);
        //GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",0, y-1, w, 2);
      }
      break;
    }
    case PROP_OCTAVE:
      self->octave = g_value_get_uint (value);
      break;
    case PROP_CURSOR_GROUP:{
      guint old = self->group;
      self->group = g_value_get_uint (value);
      if (self->group != old) {
        self->parameter = self->digit = 0;
        bt_pattern_editor_refresh_cursor_or_scroll (self);
      }
      break;
    }
    case PROP_CURSOR_PARAM:{
      guint old = self->parameter;
      self->parameter = g_value_get_uint (value);
      if (self->parameter != old) {
        self->digit = 0;
        bt_pattern_editor_refresh_cursor_or_scroll (self);
      }
      break;
    }
    case PROP_CURSOR_DIGIT:
      self->digit = g_value_get_uint (value);
      bt_pattern_editor_refresh_cursor_or_scroll (self);
      break;
    case PROP_CURSOR_ROW:
      self->row = g_value_get_uint (value);
      bt_pattern_editor_refresh_cursor_or_scroll (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);

      break;
  }
}

static void
bt_pattern_editor_dispose (GObject * object)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR (object);

  if (self->hadj) {
    g_object_unref (self->hadj);
    self->hadj = NULL;
  }
  if (self->vadj) {
    g_object_unref (self->vadj);
    self->vadj = NULL;
  }

  G_OBJECT_CLASS (bt_pattern_editor_parent_class)->dispose (object);
}

static void
bt_pattern_editor_class_init (BtPatternEditorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->set_property = bt_pattern_editor_set_property;
  gobject_class->get_property = bt_pattern_editor_get_property;
  gobject_class->dispose = bt_pattern_editor_dispose;

  widget_class->realize = bt_pattern_editor_realize;
  widget_class->unrealize = bt_pattern_editor_unrealize;
  widget_class->map = bt_pattern_editor_map;
  widget_class->unmap = bt_pattern_editor_unmap;
  widget_class->draw = bt_pattern_editor_draw;
  widget_class->style_updated = bt_pattern_editor_style_updated;
  widget_class->get_preferred_width = bt_pattern_editor_get_preferred_width;
  widget_class->get_preferred_height = bt_pattern_editor_get_preferred_height;
  widget_class->size_allocate = bt_pattern_editor_size_allocate;
  widget_class->key_press_event = bt_pattern_editor_key_press;
  widget_class->button_press_event = bt_pattern_editor_button_press;
  widget_class->button_release_event = bt_pattern_editor_button_release;

  /* GtkScrollable implementation */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,
      "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,
      "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY,
      "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY,
      "vscroll-policy");

  g_object_class_install_property (gobject_class, PROP_PLAY_POSITION,
      g_param_spec_double ("play-position",
          "play position prop.",
          "The current playing position as a fraction",
          -1.0, 1.0, -1.0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OCTAVE,
      g_param_spec_uint ("octave",
          "octave prop.",
          "The octave for note input",
          0, 12, 2, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CURSOR_GROUP,
      g_param_spec_uint ("cursor-group",
          "cursor group prop.",
          "The current group the cursor is in",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CURSOR_PARAM,
      g_param_spec_uint ("cursor-param",
          "cursor param prop.",
          "The current parameter the cursor is at",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CURSOR_DIGIT,
      g_param_spec_uint ("cursor-digit",
          "cursor digit prop.",
          "The current digit of the parameter the cursor is at",
          0, 3, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CURSOR_ROW,
      g_param_spec_uint ("cursor-row",
          "cursor row prop.",
          "The current cursor row",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
bt_pattern_editor_init (BtPatternEditor * self)
{
  self->row = self->parameter = self->digit = 0;
  self->group = 0;
  self->ofs_x = 0;
  self->ofs_y = 0;
  self->groups = NULL;
  self->num_groups = 0;
  self->num_rows = 0;
  self->octave = 4;
  self->step = 1;
  self->selection_mode = PESM_COLUMN;
  self->selection_start = 0;
  self->selection_end = 0;
  self->selection_group = 0;
  self->selection_param = 0;

  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
}

/**
 * bt_pattern_editor_set_pattern:
 * @self: the widget
 * @pattern_data: memory block of values (passed to the callbacks)
 * @num_rows: number of tick rows (y axis)
 * @num_groups: number of groups (x axis)
 * @groups: group parameters
 * @cb: value transformation callbacks
 *
 * Set pattern data to show in the widget.
 */
void
bt_pattern_editor_set_pattern (BtPatternEditor * self, gpointer pattern_data,
    guint num_rows, guint num_groups,
    BtPatternEditorColumnGroup * groups, BtPatternEditorCallbacks * cb)
{
  GtkWidget *widget = GTK_WIDGET (self);

  self->num_rows = num_rows;
  self->num_groups = num_groups;
  self->groups = groups;
  self->pattern_data = pattern_data;
  self->callbacks = cb;

  if (self->row >= self->num_rows)
    self->row = 0;
  if (self->group >= self->num_groups)
    self->group = 0;
  if (!self->groups || self->parameter >= self->groups[self->group].num_columns)
    self->parameter = 0;

  gtk_widget_queue_resize (widget);
}

/**
 * bt_pattern_editor_get_selection:
 * @self: the widget
 * @start: location for start tick
 * @end: location for end tick
 * @group: location for group
 * @param: location for parameter in group
 *
 * Get selection rectangle.
 *
 * Returns: %TRUE if there was a selection.
 */
gboolean
bt_pattern_editor_get_selection (BtPatternEditor * self,
    gint * start, gint * end, gint * group, gint * param)
{
  if (!self->selection_enabled)
    return FALSE;
  *start = self->selection_start;
  *end = self->selection_end;
  *group = self->selection_mode == PESM_ALL ? -1 : self->selection_group;
  *param = self->selection_mode != PESM_COLUMN ? -1 : self->selection_param;
  return TRUE;
}

/**
 * bt_pattern_editor_position_to_coords:
 * @self: the widget
 * @x: x position of the mouse
 * @y: y position of the mouse
 * @row: location for start tick
 * @group: location for group
 * @parameter: location for parameter in group
 * @digit: location for the digit in parameter
 *
 * Get data coordinates for the mouse position. All out variables must not be
 * %NULL.
 *
 * Returns: %TRUE if we selected a position.
 */
gboolean
bt_pattern_editor_position_to_coords (BtPatternEditor * self, gint x, gint y,
    gint * row, gint * group, gint * parameter, gint * digit)
{
  gboolean ret = FALSE;

  g_return_val_if_fail ((row && group && parameter && digit), FALSE);

  x = (self->ofs_x + x) - self->rowhdr_width;
  y = (self->ofs_y + y) - self->colhdr_height;

  *row = *group = *parameter = *digit = -1;
  if (y >= 0) {
    gint r = y / self->ch;
    if (r < self->num_rows) {
      *row = y / self->ch;
      ret = TRUE;
    }
  }
  if (x > 0) {
    gint g;
    for (g = 0; g < self->num_groups; g++) {
      BtPatternEditorColumnGroup *grp = &self->groups[g];
      if ((x < grp->width) && char_to_coords (x / self->cw, grp->columns,
              grp->num_columns, parameter, digit)) {
        *group = g;
        ret = TRUE;
        break;
      }
      x -= grp->width;
    }
  }
  return ret;
}
