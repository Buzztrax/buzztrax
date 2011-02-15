/* $Id$
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btpatterneditor
 * @short_description: the pattern editor widget
 * @see_also: #BtPattern, #BtMainPagePatterns
 *
 * Provides an editor widget for #BtPattern instances.
 */

/*
 * @todo:
 * - block operations
 *       o copy
 *       o paste
 *       o cut
 *       o clear
 *       o expand
 *       o shrink
 * - mouse handling (selections)
 *   bt_pattern_editor_button_press/release
 * - shift + cursor selection does not really work with the buzz keybindings
 * - if we want separator bars for headers, look at gtkhseparator.c
 * - drawing
 *   - having some 1 pixel padding left/right of groups would look better
 *     the group gap is one whole character anyway
 *
 * @idea: use gtk_widget_error_bell (widget); when hitting borders with cursor
 */

//#define USE_CAIRO
/* @todo: speedup drawing in bt_pattern_editor_expose()
 * - we use a lot of
 *   pango_layout_set_text (pl, str, str_len);
 *   gdk_draw_layout_with_colors (win, fg_gc, x, y, pl, color, NULL);
 *   - pango_layout_set_text() does a lot of checks (e.g. utf8 validation) and
 *     copies the text
 *   - gdk_draw_layout_with_colors() sets colors, x,y pos and call
 *     pango_renderer_draw_layout() and that is expensive
 *   - we should use cairo_show_text()
 */

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "pattern-editor.h"
#include "ui-resources-methods.h"
#include "tools.h"

#include "marshal.h"

enum {
  PATTERN_EDITOR_PLAY_POSITION=1,
  PATTERN_EDITOR_OCTAVE,
  PATTERN_EDITOR_CURSOR_GROUP,
  PATTERN_EDITOR_CURSOR_PARAM,
  PATTERN_EDITOR_CURSOR_ROW
};

//-- the class

G_DEFINE_TYPE (BtPatternEditor, bt_pattern_editor, GTK_TYPE_WIDGET);

struct ParamType
{
  int chars, columns;
  void (*to_string_func)(gchar *dest, gfloat value, gint def);
  void (*from_key_func)(gfloat *value_ptr, gint key, gint modifiers);
  guint column_pos[4];
};

//-- helper methods

static void
to_string_note (gchar *buf, gfloat value, gint def)
{
  static gchar note_names[] = "C-C#D-D#E-F-F#G-G#A-A#B-????????";
  gint note = ((gint)value)&255, octave;
  if (note == def || note == 0) {
    strcpy(buf, "...");
    return;
  }
  if (note == 255) {
    strcpy(buf, "off");
    return;
  }
  note--;
  octave = note >> 4;
  buf[0] = note_names[2 * (note & 15)];
  buf[1] = note_names[1 + 2 * (note & 15)];
  buf[2] = '0' + octave;
  buf[3] = '\0';
}

static void
to_string_trigger (gchar *buf, gfloat value, gint def)
{
  gint v = (gint)value;
  if (v == def)
    strcpy(buf, ".");
  else
    sprintf(buf, "%01X", v != 0 ? 1 : 0);
}

static void
to_string_byte (gchar *buf, gfloat value, gint def)
{
  gint v = (gint)(value+0.5);
  if (v == def)
    strcpy(buf, "..");
  else
    sprintf(buf, "%02X", v & 255);
}

static void
to_string_word (gchar *buf, gfloat value, gint def)
{
  gint v = (gint)(value+0.5);
  if (v == def)
    strcpy(buf, "....");
  else
    sprintf(buf, "%04X", v & 65535);
}

static void
to_string_float (gchar *buf, gfloat value, gint def)
{
  gint v = (gint)value;
  if (fabs(v - def) < 0.00001)
    strcpy(buf, "........");
  else
    sprintf(buf, "%-8f", value);

}

static struct ParamType param_types[] = {
  { 3, 2, to_string_note, NULL, {0, 2}},
  { 1, 1, to_string_trigger, NULL, {0}},
  { 2, 2, to_string_byte, NULL, {0, 1}},
  { 4, 4, to_string_word, NULL, {0, 1, 2, 3}},
  { 8, 1, to_string_float, NULL, {0}},
};


static gint
bt_pattern_editor_rownum_width (BtPatternEditor *self)
{
  return self->cw * 5;
}

static void
bt_pattern_editor_draw_rownum (BtPatternEditor *self,
    gint x, gint y, gint row, gint max_y)
{
  GtkWidget *widget = GTK_WIDGET(self);
#ifdef USE_CAIRO
  cairo_t *cr = gdk_cairo_create (widget->window);
#else
  GdkWindow *win = widget->window;
  GtkStyle *s = widget->style;
  PangoLayout *pl = self->pl;
  GdkGC *fg_gc = s->fg_gc[widget->state];
  GdkGC **bg_shade_gc = self->bg_shade_gc;
#endif
  gchar buf[16];
  gint cw = bt_pattern_editor_rownum_width(self);
  gint ch = self->ch;

  cw-=self->cw;
  while (y < max_y && row < self->num_rows) {
#ifdef USE_CAIRO
    gdouble *bg_shade_color=self->bg_shade_color[row&0x1];
    cairo_set_source_rgb(cr,bg_shade_color[0],bg_shade_color[1],bg_shade_color[2]);
    cairo_rectangle(cr,x,y,cw,ch);
    cairo_fill(cr);

    cairo_set_source_rgb(cr,self->text_color[0],self->text_color[1],self->text_color[2]);
    cairo_move_to(cr,x,y+ch);
    sprintf(buf, "%04X", row);
    cairo_show_text(cr, buf);
#else
    gdk_draw_rectangle (win, bg_shade_gc[row&0x1], TRUE, x, y, cw, ch);

    sprintf(buf, "%04X", row);
    pango_layout_set_text (pl, buf, 4);
    gdk_draw_layout_with_colors (win, fg_gc, x, y, pl, &s->text[GTK_STATE_NORMAL], NULL);
#endif
    y += ch;
    row++;
  }
}

static void
bt_pattern_editor_draw_colnames (BtPatternEditor *self,
    gint x, gint y)
{
  GtkWidget *widget = GTK_WIDGET(self);
#ifdef USE_CAIRO
  cairo_t *cr = gdk_cairo_create (widget->window);
#else
  GdkWindow *win = widget->window;
  GtkStyle *s = widget->style;
  PangoLayout *pl = self->pl;
  GdkGC *fg_gc = s->fg_gc[widget->state];
#endif
  gint g;
  gint ch = self->ch;

  // FIXME: this erases too much
#ifdef USE_CAIRO
  cairo_set_source_rgb(cr,self->bg_color[0],self->bg_color[1],self->bg_color[2]);
  cairo_rectangle(cr,0,0,widget->allocation.width, ch);
  cairo_fill(cr);
#else
  gdk_draw_rectangle (win, s->bg_gc[GTK_STATE_NORMAL],
      TRUE, 0, 0, widget->allocation.width,ch);
#endif

  for (g = 0; g < self->num_groups; g++) {
    BtPatternEditorColumnGroup *cgrp = &self->groups[g];

#ifdef USE_CAIRO
    cairo_set_source_rgb(cr,self->text_color[0],self->text_color[1],self->text_color[2]);
    cairo_move_to(cr,x,y+ch);
    cairo_show_text(cr, cgrp->name); // @todo: check if we need to truncate
#else
    pango_layout_set_text (pl, cgrp->name, ((cgrp->width/self->cw)-1));
    gdk_draw_layout_with_colors (win, fg_gc, x, y, pl, &s->text[GTK_STATE_NORMAL], NULL);
#endif
    x+=cgrp->width;
  }
}

static void
bt_pattern_editor_draw_rowname (BtPatternEditor *self,
    gint x, gint y)
{
  GtkWidget *widget = GTK_WIDGET(self);
#ifdef USE_CAIRO
  cairo_t *cr = gdk_cairo_create (widget->window);
#else
  GdkWindow *win = widget->window;
  GtkStyle *s = widget->style;
  PangoLayout *pl = self->pl;
#endif
  gint cw = bt_pattern_editor_rownum_width(self);
  gint ch = self->ch;

#ifdef USE_CAIRO
  cairo_set_source_rgb(cr,self->bg_color[0],self->bg_color[1],self->bg_color[2]);
  cairo_rectangle(cr,0,0,cw,ch);
  cairo_fill(cr);
#else
  gdk_draw_rectangle (win, s->bg_gc[GTK_STATE_NORMAL],
      TRUE, 0, 0, cw, ch);
#endif

  if (self->num_groups) {
#ifdef USE_CAIRO
    cairo_set_source_rgb(cr,self->text_color[0],self->text_color[1],self->text_color[2]);
    cairo_move_to(cr,x,y+ch);
    cairo_show_text(cr, "Tick");
#else
    pango_layout_set_text (pl, "Tick", 4);
    gdk_draw_layout_with_colors (win, s->fg_gc[widget->state], x, y, pl,
      &s->text[GTK_STATE_NORMAL], NULL);
#endif
  }
}

static gboolean inline
in_selection (BtPatternEditor *self,
    guint group, guint param, guint row)
{
  // check rows
  if (row < self->selection_start)
    return FALSE;
  if (row > self->selection_end)
    return FALSE;

  // check columns
  if (self->selection_mode == PESM_COLUMN)
    return (group == self->selection_group) && (param == self->selection_param);
  if (self->selection_mode == PESM_GROUP)
    return (group == self->selection_group);
  return TRUE; /* PESM_ALL */
}

/*
 * bt_pattern_editor_draw_column:
 * @x,@y: the top left corner for the column
 * @col,@group,@param: column data, group and param to draw
 * @row: the starting row
 * @max_y: max-y for clipping
 */
static void
bt_pattern_editor_draw_column (BtPatternEditor *self,
    gint x, gint y, BtPatternEditorColumn *col,
    guint group, guint param, guint row, gint max_y)
{
  GtkWidget *widget = GTK_WIDGET(self);
#ifdef USE_CAIRO
  cairo_t *cr = gdk_cairo_create (widget->window);
#else
  GdkWindow *win = widget->window;
  GtkStyle *s = widget->style;
  PangoLayout *pl = self->pl;
  GdkGC *bg_gc, *fg_gc = s->fg_gc[widget->state];
  GdkGC **bg_shade_gc = self->bg_shade_gc;
#endif
  struct ParamType *pt = &param_types[col->type];
  gchar buf[16];
  gint cw = self->cw, ch = self->ch;
  gint col_w = cw * (pt->chars + 1);
  gint col_w2 = col_w - (param == self->groups[group].num_columns - 1 ? cw : 0);
  gfloat (*get_data_func)(gpointer pattern_data, gpointer column_data, guint row, guint group, guint param) = self->callbacks->get_data_func;

  while (y < max_y && row < self->num_rows) {
    gint col_w3 = col_w2;

    /* draw background */
#ifdef USE_CAIRO
    gdouble *bg_shade_color=self->bg_shade_color[row&0x1];
    cairo_set_source_rgb(cr,bg_shade_color[0],bg_shade_color[1],bg_shade_color[2]);
#else
    bg_gc = bg_shade_gc[row&0x1];
#endif
    if (self->selection_enabled && in_selection(self, group, param, row)) {
      /* the last space should be selected if it's a within-group "glue"
         in multiple column selection, row colour otherwise */
      if (self->selection_mode == PESM_COLUMN) {
        /* draw row-coloured "continuation" after column, unless last column in
           a group */
        col_w3 = col_w - cw;
        if (param != self->groups[group].num_columns - 1) {
#ifdef USE_CAIRO
          cairo_rectangle(cr,x+col_w3,y,cw,ch);
          cairo_fill(cr);
#else
          gdk_draw_rectangle (win, bg_gc, TRUE, x + col_w3, y, cw, ch);
#endif
        }
      }
      /* draw selected column+continuation (unless last column, then don't draw
         continuation) */
#ifdef USE_CAIRO
      cairo_set_source_rgb(cr,self->sel_color[0],self->sel_color[1],self->sel_color[2]);
#else
      bg_gc = s->base_gc[GTK_STATE_SELECTED];
#endif
    }
#ifdef USE_CAIRO
    cairo_rectangle(cr,x,y,col_w3,ch);
    cairo_fill(cr);
#else
    gdk_draw_rectangle (win, bg_gc, TRUE, x, y, col_w3, ch);
#endif

#ifdef USE_CAIRO
    // draw cursor
    if (row == self->row && param == self->parameter && group == self->group) {
      gint cp = pt->column_pos[self->digit];
      cairo_set_source_rgb(cr,self->cursor_color[0],self->cursor_color[1],self->cursor_color[2]);
      cairo_rectangle(cr,x+cw*cp,y,cw,ch);
      cairo_fill(cr);
    }
#endif
    pt->to_string_func(buf, get_data_func(self->pattern_data, col->user_data, row, group, param), col->def);
#ifdef USE_CAIRO
    cairo_set_source_rgb(cr,self->text_color[0],self->text_color[1],self->text_color[2]);
    cairo_move_to(cr,x,y+ch);
    cairo_show_text(cr, buf);
#else
    pango_layout_set_text (pl, buf, pt->chars);
    gdk_draw_layout_with_colors (win, fg_gc, x, y, pl, &s->text[GTK_STATE_NORMAL], NULL);
#endif
#ifndef USE_CAIRO
    // draw cursor
    if (row == self->row && param == self->parameter && group == self->group) {
      gint cp = pt->column_pos[self->digit];
      pango_layout_set_text (pl, &buf[cp], 1);
      // we could also use text_aa[GTK_STATE_SELECTED] for the cursor
      gdk_draw_layout_with_colors (win, fg_gc, x + cw * cp, y, pl,
        &s->text[GTK_STATE_NORMAL], &s->text_aa[GTK_STATE_ACTIVE]);
    }
#endif
    y += ch;
    row++;
  }
}

static gint
bt_pattern_editor_get_row_width (BtPatternEditor *self)
{
  gint width = self->rowhdr_width, g;
  for (g = 0; g < self->num_groups; g++)
    width += self->groups[g].width;
  return width;
}

static gint
bt_pattern_editor_get_col_height (BtPatternEditor *self)
{
  return (self->colhdr_height + self->num_rows * self->ch);
}

static void
bt_pattern_editor_refresh_cell (BtPatternEditor *self)
{
  gint y = self->colhdr_height + (self->row * self->ch) - self->ofs_y;
  gint x = self->rowhdr_width - self->ofs_x;
  gint g, i, w;
  BtPatternEditorColumnGroup *cgrp;
  BtPatternEditorColumn *col;
  struct ParamType *pt;

  for (g = 0; g < self->group; g++) {
    cgrp = &self->groups[g];
    x += cgrp->width;
  }
  cgrp = &self->groups[self->group];
  for (i = 0; i < self->parameter; i++) {
    col=&cgrp->columns[i];
    pt = &param_types[col->type];
    x += self->cw * (pt->chars + 1);
  }
  col=&cgrp->columns[self->parameter];
  pt = &param_types[col->type];
  w = self->cw * (pt->chars + 1);

  //GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",x, y, w, self->ch);
  gtk_widget_queue_draw_area (GTK_WIDGET(self), x, y, w , self->ch);
}

static void
bt_pattern_editor_refresh_cursor (BtPatternEditor *self)
{
  gint y = self->colhdr_height + (self->row * self->ch) - self->ofs_y;
  gint x = self->rowhdr_width - self->ofs_x;
  gint g, i;
  BtPatternEditorColumnGroup *cgrp;
  BtPatternEditorColumn *col;
  struct ParamType *pt;

  if (!self->num_groups)
    return;

  for (g = 0; g < self->group; g++) {
    cgrp = &self->groups[g];
    x += cgrp->width;
  }
  cgrp = &self->groups[self->group];
  for (i = 0; i < self->parameter; i++) {
    col=&cgrp->columns[i];
    pt = &param_types[col->type];
    x += self->cw * (pt->chars + 1);
  }
  col=&cgrp->columns[self->parameter];
  pt = &param_types[col->type];
  x += self->cw * pt->column_pos[self->digit];

  //GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",x, y, self->cw, self->ch);
  gtk_widget_queue_draw_area (GTK_WIDGET(self), x, y, self->cw , self->ch);
}

static void
bt_pattern_editor_refresh_cursor_or_scroll (BtPatternEditor *self)
{
  gboolean draw=TRUE;

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
      gint w = self->cw * param_types[grp->columns[p].type].chars; /* column width */
      gint ww = GTK_WIDGET(self)->requisition.width - self->rowhdr_width; /* full width, less rowheader */
      gint pw = GTK_WIDGET(self)->allocation.width - self->rowhdr_width; /* visible (scrolled) width, less rowheader */
      /* rowheader is subtracted, because it is not taken into account in hscroll range */
      GST_INFO("xpos = %d cpos = %d w = %d width = %d / %d", xpos, cpos, w, ww, pw);
      if (xpos < cpos || xpos + w > cpos + pw) {
        /* if current parameter doesn't fit, try to center the cursor */
        xpos = xpos - pw / 2;
        if (xpos + pw > ww)
          xpos = ww - pw;
        if (xpos < 0)
          xpos = 0;
        gtk_adjustment_set_value(self->hadj, xpos);
        draw=FALSE;
      }
    }
  }

  {
    gint ypos = self->row * self->ch;
    gint cpos = self->ofs_y;
    gint h = self->ch;
    gint wh = GTK_WIDGET(self)->requisition.height - self->colhdr_height; /* full heigth, less colheader */
    gint ph = GTK_WIDGET(self)->allocation.height - self->colhdr_height; /* visible (scrolled) heigth, less colheader */
    /* colheader is subtracted, because it is not taken into account in vscroll range */
    GST_INFO("ypos = %d cpos = %d h = %d height = %d / %d", ypos, cpos, h, wh, ph);
    if (ypos < cpos || ypos + h > cpos + ph) {
      /* if current parameter doesn't fit, try to center the cursor */
      ypos = ypos - ph / 2;
      if (ypos + ph > wh)
        ypos = wh - ph;
      if (ypos < 0)
        ypos = 0;
      gtk_adjustment_set_value(self->vadj, ypos);
      draw=FALSE;
    }
  }
  if(draw) {
    // setting the adjustments already draws
    bt_pattern_editor_refresh_cursor(self);
  }
}

static void
advance (BtPatternEditor *self)
{
  if (self->row < self->num_rows - 1) {
    // invalidate old pos
    bt_pattern_editor_refresh_cursor(self);
    self->row += self->step;
    if (self->row > self->num_rows - 1)
      self->row = self->num_rows - 1;
    bt_pattern_editor_refresh_cursor_or_scroll(self);
  }
}

static BtPatternEditorColumn *
cur_column (BtPatternEditor *self)
{
  return &self->groups[self->group].columns[self->parameter];
}

static gboolean
char_to_coords(gint charpos, BtPatternEditorColumn *columns, gint num_cols,
  gint *parameter, gint *digit)
{
  gint i, j;
  for (i = 0; i < num_cols; i++) {
    struct ParamType *type = &param_types[columns[i].type];
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
  return GTK_WIDGET( g_object_new (BT_TYPE_PATTERN_EDITOR, NULL ));
}

//-- class internals

static void
bt_pattern_editor_realize (GtkWidget *widget)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);
  GdkWindowAttr attributes;
  gint attributes_mask;
#ifdef USE_CAIRO
  cairo_t *cr;
  cairo_font_extents_t extents;
  GdkColor *c;
#else
  PangoContext *pc;
  PangoFontDescription *pfd;
  PangoFontMetrics *pfm;
  GdkColor alt_row_color={0,};
#endif

  g_return_if_fail (BT_IS_PATTERN_EDITOR (widget));

  gtk_widget_set_realized(widget, TRUE);
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) |
      GDK_EXPOSURE_MASK | GDK_SCROLL_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK
      /*| GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK*/;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new ( gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_user_data (widget->window, widget);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  gtk_widget_set_can_focus(widget, TRUE);

  // setup graphic styles
#ifdef USE_CAIRO
  bt_ui_resources_get_rgb_color(BT_UI_RES_COLOR_PLAYLINE,&self->play_pos_color[0],&self->play_pos_color[1],&self->play_pos_color[2]);
  c=&widget->style->light[GTK_STATE_NORMAL];
  self->bg_shade_color[0][0]=((gdouble)c->red*0.9)/65535.0;
  self->bg_shade_color[0][1]=((gdouble)c->green*0.9)/65535.0;
  self->bg_shade_color[0][2]=((gdouble)c->blue*0.9)/65535.0;
  self->bg_shade_color[1][0]=(gdouble)c->red/65535.0;
  self->bg_shade_color[1][1]=(gdouble)c->green/65535.0;
  self->bg_shade_color[1][2]=(gdouble)c->blue/65535.0;
  c=&widget->style->text[GTK_STATE_NORMAL];
  self->text_color[0]=(gdouble)c->red/65535.0;
  self->text_color[1]=(gdouble)c->green/65535.0;
  self->text_color[2]=(gdouble)c->blue/65535.0;
  c=&widget->style->bg[GTK_STATE_NORMAL];
  self->bg_color[0]=(gdouble)c->red/65535.0;
  self->bg_color[1]=(gdouble)c->green/65535.0;
  self->bg_color[2]=(gdouble)c->blue/65535.0;
  c=&widget->style->base[GTK_STATE_SELECTED];
  self->sel_color[0]=(gdouble)c->red/65535.0;
  self->sel_color[1]=(gdouble)c->green/65535.0;
  self->sel_color[2]=(gdouble)c->blue/65535.0;
  c=&widget->style->text_aa[GTK_STATE_ACTIVE];
  self->cursor_color[0]=(gdouble)c->red/65535.0;
  self->cursor_color[1]=(gdouble)c->green/65535.0;
  self->cursor_color[2]=(gdouble)c->blue/65535.0;
#else
  self->play_pos_gc=gdk_gc_new(widget->window);
  gdk_gc_set_rgb_fg_color(self->play_pos_gc,bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_PLAYLINE));
  gdk_gc_set_line_attributes(self->play_pos_gc,2,GDK_LINE_SOLID,GDK_CAP_BUTT,GDK_JOIN_MITER);
  self->bg_shade_gc[0]=gdk_gc_new(widget->window);
// does not work yet, see bt-edit.gtkrc
//#if GTK_CHECK_VERSION(2, 10, 0)
//  gtk_style_lookup_color(widget->style,"alternative-row",&alt_row_color);
//#else
   alt_row_color.red=(guint16)(widget->style->light[GTK_STATE_NORMAL].red*0.9);
   alt_row_color.green=(guint16)(widget->style->light[GTK_STATE_NORMAL].green*0.9);
   alt_row_color.blue=(guint16)(widget->style->light[GTK_STATE_NORMAL].blue*0.9);
//#endif
  gdk_gc_set_rgb_fg_color(self->bg_shade_gc[0],&alt_row_color);
  self->bg_shade_gc[1]=widget->style->light_gc[GTK_STATE_NORMAL];
#endif

  /* copy size from default font and use default monospace font */
  GST_WARNING(" default font %p, size %d (is_absolute %d?), scl=%lf",
    widget->style->font_desc,
    pango_font_description_get_size(widget->style->font_desc),
    pango_font_description_get_size_is_absolute(widget->style->font_desc),
    (gdouble)PANGO_SCALE);
#ifdef USE_CAIRO
  cr = gdk_cairo_create (widget->window);
  cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, pango_font_description_get_size (widget->style->font_desc) / PANGO_SCALE);
  cairo_font_extents (cr, &extents);
  self->cw = 1+extents.max_x_advance;
  self->ch = 1+extents.ascent+extents.descent; // height
#else
  /* calculate font-metrics */
  pc = gtk_widget_get_pango_context (widget);
  pfd = pango_font_description_new();

  //pango_font_description_set_family_static (pfd, "Bitstream Vera Sans Mono");
  //pango_font_description_set_absolute_size (pfd, 12 * PANGO_SCALE);
  pango_font_description_set_family_static (pfd, "monospace");
  if(pango_font_description_get_size_is_absolute(widget->style->font_desc)) {
    pango_font_description_set_absolute_size (pfd,
      pango_font_description_get_size (widget->style->font_desc));
  }
  else {
    pango_font_description_set_size (pfd,
      pango_font_description_get_size (widget->style->font_desc));
  }
  pango_context_load_font (pc, pfd);

  pfm = pango_context_get_metrics (pc, pfd, NULL);
  self->cw = pango_font_metrics_get_approximate_digit_width (pfm) / PANGO_SCALE;
  self->ch = (pango_font_metrics_get_ascent (pfm) + pango_font_metrics_get_descent (pfm)) / PANGO_SCALE;
  pango_font_metrics_unref (pfm);

  self->pl = pango_layout_new (pc);
  pango_layout_set_font_description (self->pl, pfd);
  pango_font_description_free (pfd);
#endif

  GST_WARNING("char size: %d x %d", self->cw, self->ch);
}

static void bt_pattern_editor_unrealize(GtkWidget *widget)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);

#ifdef USE_CAIRO
#else
  g_object_unref (self->play_pos_gc);
  self->play_pos_gc=NULL;
  g_object_unref (self->bg_shade_gc[0]);
  self->bg_shade_gc[0]=NULL;
  g_object_unref (self->pl);
  self->pl = NULL;
#endif

  if(self->hadj) {
    g_object_unref (self->hadj);
    self->hadj=NULL;
  }
  if(self->vadj){
    g_object_unref (self->vadj);
    self->vadj=NULL;
  }
}

/* @todo:
 * - refactor layout and redrawing
 *   - calculate layout in bt_pattern_editor_size_allocate() and cache
 *   - only redraw in expose
 */
static gboolean
bt_pattern_editor_expose (GtkWidget *widget,
                     GdkEventExpose *event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);
  GdkRectangle rect = event->area;
  gint y, x, i, row, g, max_y;
  gint grp_x;
  gint rowhdr_x, ch;
#ifdef USE_CAIRO
  cairo_t *cr;
#endif

  g_return_val_if_fail (BT_IS_PATTERN_EDITOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;

  /* this is the dirty region */
  GST_INFO("Refresh Area: %d,%d -> %d,%d",rect.x, rect.y, rect.width, rect.height);

#ifdef USE_CAIRO
  cr = gdk_cairo_create (widget->window);
#endif

  if (self->hadj) {
    self->ofs_x = (gint)gtk_adjustment_get_value(self->hadj);
    self->ofs_y = (gint)gtk_adjustment_get_value(self->vadj);
  }

  /* calculate the first a last row in the dirty region */
  self->colhdr_height = ch = self->ch;
  x = 0;
  y = ch + (gint)(ch * floor((rect.y - ch) / ch));
  if (y < ch)
      y = ch;
  max_y = rect.y + rect.height + ch; // one extra line
  max_y = ch + (gint)(ch * ceil((max_y - ch) / ch));
  row = (y - ch) / ch;

  /* leave space for headers */
  rowhdr_x = x;
  self->rowhdr_width = bt_pattern_editor_rownum_width(self) + self->cw;
  x += self->rowhdr_width;

  GST_INFO("Scroll: %d,%d, row=%d",self->ofs_x, self->ofs_y, row);

  /* draw group parameter columns */
  for (g = 0; g < self->num_groups; g++) {
    BtPatternEditorColumnGroup *cgrp = &self->groups[g];
    grp_x = x;
    for (i = 0; i < cgrp->num_columns; i++) {
      BtPatternEditorColumn *col=&cgrp->columns[i];
      struct ParamType *pt = &param_types[col->type];
      gint w = self->cw * (pt->chars + 1);
      gint xs = x-self->ofs_x, xe = xs+(w-1);

      // check intersection
      if((xs>=rect.x && xs<=rect.x+rect.width) || (xe>=rect.x && xe<=rect.x+rect.width) || (xs<=rect.x && xe>=rect.x+rect.width)) {
        GST_DEBUG("Draw Group/Column: %d,%d : %3d-%3d",g,i,xs,xe);
        bt_pattern_editor_draw_column(self, x-self->ofs_x, y-self->ofs_y, col, g, i, row, max_y);
      }
      else {
        GST_DEBUG("Skip Group/Column: %d,%d : %3d-%3d",g,i,xs,xe);
      }
      x += w;
    }
    x += self->cw;
    cgrp->width = x - grp_x;
  }

  /* draw left and top headers */
  if(rect.x<self->rowhdr_width) {
    bt_pattern_editor_draw_rownum(self, rowhdr_x, y-self->ofs_y, row, max_y);
  }
  if(rect.y<self->ch) {
    bt_pattern_editor_draw_colnames(self, (rowhdr_x+self->rowhdr_width)-self->ofs_x, 0);
    bt_pattern_editor_draw_rowname(self, rowhdr_x, 0);
  }

  /* draw play-pos */
  if(self->play_pos>=0.0) {
    gdouble h=(gdouble)(bt_pattern_editor_get_col_height(self)-self->colhdr_height);
    y=self->colhdr_height+(gint)(self->play_pos*h) - self->ofs_y;
    // @todo: check rect.y, rect.height
#ifdef USE_CAIRO
    cairo_set_source_rgb(cr,self->play_pos_color[0],self->play_pos_color[1],self->play_pos_color[2]);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr,0,y);
    cairo_line_to(cr,widget->allocation.width,y);
    cairo_stroke(cr);
#else
    gdk_draw_line(widget->window,self->play_pos_gc, 0,y,widget->allocation.width,y);
#endif
    //GST_INFO("Draw playline: %d,%d -> %d,%d",0, y);
  }

  if (G_UNLIKELY(self->size_changed)) {
    // do this for resize the after the first redraw (see @todo above)
    self->size_changed=FALSE;
    gtk_widget_queue_resize_no_redraw (widget);
  }
  return FALSE;
}

static void
bt_pattern_editor_update_adjustments (BtPatternEditor *self)
{
  if(!gtk_widget_get_realized(GTK_WIDGET(self))) return;

  if (self->hadj) {
    self->hadj->upper = bt_pattern_editor_get_row_width(self);
    self->hadj->page_increment = GTK_WIDGET (self)->allocation.width;
    self->hadj->page_size = GTK_WIDGET (self)->allocation.width;
    self->hadj->step_increment = GTK_WIDGET (self)->allocation.width / 20.0;
    gtk_adjustment_changed(self->hadj);
    gtk_adjustment_set_value(self->hadj,
      MIN(self->hadj->value, self->hadj->upper - self->hadj->page_size));
  }
  if (self->vadj) {
    self->vadj->upper = bt_pattern_editor_get_col_height(self);
    self->vadj->page_increment = GTK_WIDGET (self)->allocation.height;
    self->vadj->page_size = GTK_WIDGET (self)->allocation.height;
    self->vadj->step_increment = GTK_WIDGET (self)->allocation.height / 20.0;
    gtk_adjustment_changed(self->vadj);
    gtk_adjustment_set_value(self->vadj,
      MIN(self->vadj->value, self->vadj->upper - self->vadj->page_size));
  }
}

static void
bt_pattern_editor_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);

  /* calculate from pattern size */
  requisition->width = bt_pattern_editor_get_row_width(self);
  requisition->height = bt_pattern_editor_get_col_height(self);
  GST_DEBUG("size_request: %d,%d",requisition->width, requisition->height);
}

static void
bt_pattern_editor_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  g_return_if_fail (allocation != NULL);

  GTK_WIDGET_CLASS(bt_pattern_editor_parent_class)->size_allocate(widget,allocation);

  widget->allocation = *allocation;
  GST_INFO("size_allocate: %d,%d %d,%d",allocation->x, allocation->y,
    allocation->width, allocation->height);

  bt_pattern_editor_update_adjustments (BT_PATTERN_EDITOR (widget));
}

static gboolean
bt_pattern_editor_key_press (GtkWidget *widget,
                        GdkEventKey *event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);

  if (self->num_groups) {
    if (!(event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK)) &&
      (event->keyval >= 32 && event->keyval < 127) &&
      self->callbacks->set_data_func)
    {
      BtPatternEditorColumn *col = &self->groups[self->group].columns[self->parameter];
      if (event->keyval == '.') {
        self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, self->digit, col->def);
        bt_pattern_editor_refresh_cell (self);
        advance(self);
      }
      else {
        static const gchar hexdigits[] = "0123456789abcdef";
        //static const char notenames[] = "zsxdcvgbhnjm\t\t\t\tq2w3er5t6y7u\t\t\t\ti9o0p";
        static const gchar notenames[] = "\x34\x27\x35\x28\x36\x37\x2a\x38\x2b\x39\x2c\x3a\x3a\x3a\x3a\x3a\x18\x0b\x19\x0c\x1a\x1b\x0e\x1c\x0f\x1d\x10\x1e\x1e\x1e\x1e\x1e\x1f\x12\x20\x13\x21";
        gfloat oldvalue = self->callbacks->get_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter);
        const gchar *p;
        switch(col->type) {
        case PCT_FLOAT:
          // no editing yet
          break;
        case PCT_SWITCH:
          if (event->keyval == '0' || event->keyval == '1') {
            self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, self->digit, event->keyval - '0');
            advance(self);
          }
          break;
        case PCT_BYTE:
        case PCT_WORD:
          p = strchr(hexdigits, (char)event->keyval);
          if (p) {
            gint value = (gint)oldvalue;
            if (oldvalue == col->def)
              value = 0;
            gint shift = 4*(((col->type == PCT_BYTE) ? 1 : 3) - self->digit);
            value = (value &~(15 << shift)) | ((p - hexdigits) << shift);
            if (value < col->min) value = col->min;
            if (value > col->max) value = col->max;

            self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, self->digit, value);
            bt_pattern_editor_refresh_cell (self);
            advance(self);
          }
          break;
        case PCT_NOTE:
          if (self->digit == 0 && event->keyval == '1') {
            // note off
            self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, self->digit, 255);
            bt_pattern_editor_refresh_cell (self);
            advance(self);
            break;
          }
          if (self->digit == 0 && event->hardware_keycode <= 255) {
            // FIXME: use event->hardware_keycode because of y<>z
            p = strchr(notenames, (char)event->hardware_keycode);
            if (p) {
              gint value = 1 + (p - notenames) + 16 * self->octave;
              if (value < col->min) value = col->min;
              if (value > col->max) value = col->max;

              if (value >= col->min && value <= col->max && (value & 15) <= 12) {
                self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, self->digit, value);
                bt_pattern_editor_refresh_cell (self);
                advance(self);
              }
            }
          }
          if (self->digit == 1) {
            if (isdigit(event->keyval)) {
              gint value = (gint)oldvalue;
              if (oldvalue == col->def)
                value = 1;
              value = (value & 15) | ((event->keyval - '0') << 4);

              // note range = 1..12 and not 0..11
              if (value >= col->min && value <= col->max && (value & 15) <= 12) {
                self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, self->digit, value);
                bt_pattern_editor_refresh_cell (self);
                advance(self);
              }
            }
          }
          break;
        }
      }
    }
    else {
      gint kup = toupper(event->keyval);
      gint control = (event->state & GDK_CONTROL_MASK) != 0;
      gint shift = (event->state & GDK_SHIFT_MASK) != 0;
      gint modifier = event->state & gtk_accelerator_get_default_mod_mask();

      if (control && event->keyval >= '1' && event->keyval <= '9') {
        self->step = event->keyval - '0';
        return TRUE;
      }
      else if (control && kup >= 'A' && kup <= 'Z') {
        gboolean same = FALSE, handled = TRUE;

        switch(kup) {
          case 'B':
          case 'E':
            if (kup == 'B') {
              same = self->selection_enabled && (self->selection_start == self->row)
                && (self->selection_group == self->group)
                && (self->selection_param == self->parameter);
              self->selection_enabled = TRUE;
              self->selection_start = self->row;
              if (self->selection_end < self->row)
                self->selection_end = self->row;
            } else { /* Ctrl+E */
              same = self->selection_enabled && (self->selection_end == self->row)
                && (self->selection_group == self->group)
                && (self->selection_param == self->parameter);
              self->selection_enabled = TRUE;
              self->selection_end = self->row;
              if (self->selection_start > self->row)
                self->selection_start = self->row;
            }

            /* if Ctrl+B(E) was pressed again, cycle selection mode */
            if (same) {
              if (self->selection_mode < (self->num_groups == 1 ? PESM_GROUP : PESM_ALL))
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
              kup == 'K' ? PESM_GROUP :
                (kup == 'L' ? PESM_COLUMN : PESM_ALL);
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
          gtk_widget_queue_draw (GTK_WIDGET(self));
          return TRUE;
        }
      }
      switch(event->keyval) {
        case GDK_Up:
          if (!modifier) {
            if (self->row > 0) {
              // invalidate old pos
              bt_pattern_editor_refresh_cursor(self);
              self->row -= 1;
              g_object_notify((gpointer)self,"cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll(self);
            }
            return TRUE;
          }
          break;
        case GDK_Down:
          if (!modifier) {
            if (self->row < self->num_rows - 1) {
              // invalidate old pos
              bt_pattern_editor_refresh_cursor(self);
              self->row += 1;
              g_object_notify((gpointer)self,"cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll(self);
            }
            return TRUE;
          }
          break;
        case GDK_Page_Up:
          if (!modifier) {
            if (self->row > 0) {
              bt_pattern_editor_refresh_cursor(self);
              /* @todo: should we tell pattern editor about the meassure
               * g_object_get(song_info,"bars",&bars,NULL); for 4/4 => 16
               */
              self->row = control ? 0 : (self->row - 16);
              if (self->row < 0)
                self->row = 0;
              g_object_notify((gpointer)self,"cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll(self);
            }
            return TRUE;
          }
          break;
        case GDK_Page_Down:
          if (!modifier) {
            if (self->row < self->num_rows - 1) {
              bt_pattern_editor_refresh_cursor(self);
              /* @todo: see Page_Up */
              self->row = control ? self->num_rows - 1 : self->row + 16;
              if (self->row > self->num_rows - 1)
                self->row = self->num_rows - 1;
              g_object_notify((gpointer)self,"cursor-row");
              bt_pattern_editor_refresh_cursor_or_scroll(self);
            }
            return TRUE;
          }
          break;
        case GDK_Home:
          bt_pattern_editor_refresh_cursor(self);
          if (control) {
            /* shouldn't this go to top left */
            self->digit = 0;
            self->parameter = 0;
            g_object_notify((gpointer)self,"cursor-param");
          } else {
            /* go to begin of cell, group, pattern, top of pattern */
            if (self->digit > 0) {
              self->digit = 0;
            } else if (self->parameter > 0) {
              self->parameter = 0;
              g_object_notify((gpointer)self,"cursor-param");
            } else if (self->group > 0) {
              self->group = 0;
              g_object_notify((gpointer)self,"cursor-group");
            } else {
              self->row = 0;
              g_object_notify((gpointer)self,"cursor-row");
            }
          }
          bt_pattern_editor_refresh_cursor_or_scroll(self);
          return TRUE;
        case GDK_End:
          bt_pattern_editor_refresh_cursor(self);
          if (control) {
            /* shouldn't this go to bottom right */
            if (self->groups[self->group].num_columns > 0)
              self->parameter = self->groups[self->group].num_columns - 1;
            g_object_notify((gpointer)self,"cursor-param");
          }
          else {
            /* go to end of cell, group, pattern, bottom of pattern */
            BtPatternEditorColumn *pc = cur_column (self);
            if (self->digit < param_types[pc->type].columns - 1) {
              self->digit = param_types[pc->type].columns - 1;
            } else if (self->parameter < self->groups[self->group].num_columns - 1) {
              self->parameter = self->groups[self->group].num_columns - 1;
              g_object_notify((gpointer)self,"cursor-param");
            } else if (self->group < self->num_groups - 1) {
              self->group = self->num_groups - 1;
              g_object_notify((gpointer)self,"cursor-group");
            } else {
              self->row = self->num_rows - 1;
              g_object_notify((gpointer)self,"cursor-row");
            }
          }
          bt_pattern_editor_refresh_cursor_or_scroll(self);
          return TRUE;
        case GDK_Left:
          if (!modifier) {
            bt_pattern_editor_refresh_cursor(self);
            if (self->digit > 0)
              self->digit--;
            else {
              BtPatternEditorColumn *pc;
              if (self->parameter > 0) {
                self->parameter--;
                g_object_notify((gpointer)self,"cursor-param");
              }
              else if (self->group > 0) {
                self->group--;
                self->parameter = self->groups[self->group].num_columns - 1;
                /* only notify group, param will be read along anyway */
                g_object_notify((gpointer)self,"cursor-group");
              }
              else
                return FALSE;
              pc = cur_column (self);
              self->digit = param_types[pc->type].columns - 1;
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
        case GDK_Right:
          if (!modifier) {
            BtPatternEditorColumn *pc = cur_column (self);
            bt_pattern_editor_refresh_cursor(self);
            if (self->digit < param_types[pc->type].columns - 1) {
              self->digit++;
            }
            else if (self->parameter < self->groups[self->group].num_columns - 1) {
              self->parameter++; self->digit = 0;
              g_object_notify((gpointer)self,"cursor-param");
            }
            else if (self->group < self->num_groups - 1) {
              self->group++; self->parameter = 0; self->digit = 0;
              /* only notify group, param will be read along anyway */
              g_object_notify((gpointer)self,"cursor-group");
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
        case GDK_Tab:
          if (!modifier) {
            bt_pattern_editor_refresh_cursor(self);
            if (self->group < self->num_groups - 1) {
              /* jump to same column when jumping from track to track, otherwise jump to first column of the group */
              if (self->groups[self->group].type != self->groups[self->group + 1].type) {
                self->parameter = 0; self->digit = 0;
              }
              self->group++;
              g_object_notify((gpointer)self,"cursor-group");
            }
            bt_pattern_editor_refresh_cursor_or_scroll (self);
            return TRUE;
          }
          break;
        case GDK_ISO_Left_Tab:
          if (shift) {
            bt_pattern_editor_refresh_cursor(self);
            if (self->group > 0) {
              self->group--;
              if (self->groups[self->group].type != self->groups[self->group + 1].type) {
                self->parameter = 0; self->digit = 0;
              }
              g_object_notify((gpointer)self,"cursor-group");
            }
            else /* at leftmost group, reset cursor to first column */
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
bt_pattern_editor_button_press (GtkWidget *widget,
                                GdkEventButton *event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);
  gint x = self->ofs_x + event->x;
  gint y = self->ofs_y + event->y;
  gint parameter, digit;
  gint g;

  gtk_widget_grab_focus_savely(GTK_WIDGET(self));

  if (x < self->rowhdr_width) {
    bt_pattern_editor_refresh_cursor(self);
    self->row = (y-self->colhdr_height) / self->ch;
    bt_pattern_editor_refresh_cursor_or_scroll(self);
    return TRUE;
  }
  x -= self->rowhdr_width;
  y -= self->colhdr_height;
  for (g = 0; g < self->num_groups; g++)
  {
    BtPatternEditorColumnGroup *grp = &self->groups[g];
    if (x < grp->width)
    {
      if (char_to_coords(x / self->cw, grp->columns, grp->num_columns, &parameter, &digit))
      {
        bt_pattern_editor_refresh_cursor(self);
        self->row = y / self->ch;
        self->group = g;
        self->parameter = parameter;
        self->digit = digit;
        g_object_notify((gpointer)self,"cursor-row");
        g_object_notify((gpointer)self,"cursor-group");
        bt_pattern_editor_refresh_cursor_or_scroll(self);
        return TRUE;
      }
      return FALSE;
    }
    x -= grp->width;
  }

  return FALSE;
}

static gboolean
bt_pattern_editor_button_release (GtkWidget *widget,
                                  GdkEventButton *event)
{
  return FALSE;
}

static void
bt_pattern_editor_set_scroll_adjustments (BtPatternEditor* self,
					  GtkAdjustment  * horizontal,
					  GtkAdjustment  * vertical)
{
	if (!horizontal) {
		horizontal = GTK_ADJUSTMENT (gtk_adjustment_new (0,0,0,0,0,0));
	}
	if (!vertical) {
		vertical = GTK_ADJUSTMENT (gtk_adjustment_new (0,0,0,0,0,0));
	}

	if (self->hadj) {
		g_signal_handlers_disconnect_by_func (self->hadj, gtk_widget_queue_draw, self);
		g_object_unref (self->hadj);
	}
	if (self->vadj) {
		g_signal_handlers_disconnect_by_func (self->vadj, gtk_widget_queue_draw, self);
		g_object_unref (self->vadj);
	}

	self->hadj = g_object_ref_sink (horizontal);
	g_signal_connect_swapped (self->hadj, "value-changed",
				  G_CALLBACK (gtk_widget_queue_draw), self);
	self->vadj = g_object_ref_sink (vertical);
	g_signal_connect_swapped (self->vadj, "value-changed",
				  G_CALLBACK (gtk_widget_queue_draw), self);

	bt_pattern_editor_update_adjustments (self);
}

static void
bt_pattern_editor_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(object);

  switch (property_id) {
    case PATTERN_EDITOR_CURSOR_GROUP: {
      g_value_set_uint(value, self->group);
    } break;
    case PATTERN_EDITOR_CURSOR_PARAM: {
      g_value_set_uint(value, self->parameter);
    } break;
    case PATTERN_EDITOR_CURSOR_ROW: {
      g_value_set_uint(value, self->row);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void
bt_pattern_editor_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(object);

  switch (property_id) {
    case PATTERN_EDITOR_PLAY_POSITION: {
      gdouble old_pos = self->play_pos;
      self->play_pos = g_value_get_double(value);
      if(gtk_widget_get_realized(GTK_WIDGET(self))) {
        gint y;
        gdouble h=(gdouble)(bt_pattern_editor_get_col_height(self)-self->colhdr_height);
        GtkAllocation allocation=GTK_WIDGET(self)->allocation;

        y=self->colhdr_height+(gint)(old_pos*h) - self->ofs_y;
        gtk_widget_queue_draw_area(GTK_WIDGET(self),0,y-1,allocation.width,2);
        GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",0, y-1, allocation.width, 2);
        y=self->colhdr_height+(gint)(self->play_pos*h) - self->ofs_y;
        gtk_widget_queue_draw_area(GTK_WIDGET(self),0,y-1,allocation.width,2);
        //GST_INFO("Mark Area Dirty: %d,%d -> %d,%d",0, y-1, allocation.width, 2);
      }
    } break;
    case PATTERN_EDITOR_OCTAVE: {
      self->octave = g_value_get_uint(value);
    } break;
    case PATTERN_EDITOR_CURSOR_GROUP: {
      guint old = self->group;
      self->group = g_value_get_uint(value);
      if (self->group != old)
        self->parameter=self->digit=0;
    } break;
    case PATTERN_EDITOR_CURSOR_PARAM: {
      guint old = self->parameter;
      self->parameter = g_value_get_uint(value);
      if (self->parameter != old)
        self->digit=0;
    } break;
    case PATTERN_EDITOR_CURSOR_ROW: {
      self->row = g_value_get_uint(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void
bt_pattern_editor_dispose(GObject *object) {
  BtPatternEditor *self = BT_PATTERN_EDITOR(object);

  if (self->hadj) {
    g_object_unref(self->hadj);
    self->hadj = NULL;
  }
  if (self->vadj) {
    g_object_unref(self->vadj);
    self->vadj = NULL;
  }

  G_OBJECT_CLASS(bt_pattern_editor_parent_class)->dispose(object);
}

static void
bt_pattern_editor_class_init(BtPatternEditorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  gobject_class->set_property = bt_pattern_editor_set_property;
  gobject_class->get_property = bt_pattern_editor_get_property;
  gobject_class->dispose      = bt_pattern_editor_dispose;

  widget_class->realize = bt_pattern_editor_realize;
  widget_class->unrealize = bt_pattern_editor_unrealize;
  widget_class->expose_event = bt_pattern_editor_expose;
  widget_class->size_request = bt_pattern_editor_size_request;
  widget_class->size_allocate = bt_pattern_editor_size_allocate;
  widget_class->key_press_event = bt_pattern_editor_key_press;
  widget_class->button_press_event = bt_pattern_editor_button_press;
  widget_class->button_release_event = bt_pattern_editor_button_release;

  widget_class->set_scroll_adjustments_signal = g_signal_new("set-scroll-adjustments",
							     BT_TYPE_PATTERN_EDITOR,
							     G_SIGNAL_RUN_LAST,
							     G_STRUCT_OFFSET (BtPatternEditorClass, set_scroll_adjustments),
							     NULL, NULL,
							     bt_marshal_VOID__OBJECT_OBJECT,
							     G_TYPE_NONE, 2,
							     GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

  klass->set_scroll_adjustments = bt_pattern_editor_set_scroll_adjustments;

  g_object_class_install_property(gobject_class,PATTERN_EDITOR_PLAY_POSITION,
                                  g_param_spec_double("play-position",
                                     "play position prop.",
                                     "The current playing position as a fraction",
                                     -1.0,
                                     1.0,
                                     -1.0,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_EDITOR_OCTAVE,
                                  g_param_spec_uint("octave",
                                     "octave prop.",
                                     "The octave for note input",
                                     0,
                                     12,
                                     2,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_EDITOR_CURSOR_GROUP,
                                  g_param_spec_uint("cursor-group",
                                     "cursor group prop.",
                                     "The current group the cursor is in",
                                     0,
                                     G_MAXUINT,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_EDITOR_CURSOR_PARAM,
                                  g_param_spec_uint("cursor-param",
                                     "cursor param prop.",
                                     "The current parameter the cursor is at",
                                     0,
                                     G_MAXUINT,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_EDITOR_CURSOR_ROW,
                                  g_param_spec_uint("cursor-row",
                                     "cursor row prop.",
                                     "The current cursor row",
                                     0,
                                     G_MAXUINT,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

}

static void
bt_pattern_editor_init(BtPatternEditor *self)
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
  self->size_changed = TRUE;
  self->selection_mode = PESM_COLUMN;
  self->selection_start = 0;
  self->selection_end = 0;
  self->selection_group = 0;
  self->selection_param = 0;
}

/**
 * bt_pattern_editor_set_pattern:
 * @self: the widget
 * @pattern_data: memory block of values
 * @num_rows: number of tick rows (y axis)
 * @num_groups: number of groups (x axis)
 * @groups: group parameters
 * @cb: value transformation callbacks
 *
 * Set pattern data to show in the widget.
 */
void
bt_pattern_editor_set_pattern(BtPatternEditor *self, gpointer pattern_data,
    guint num_rows, guint num_groups,
    BtPatternEditorColumnGroup *groups,
    BtPatternEditorCallbacks *cb)
{
  GtkWidget *widget = GTK_WIDGET(self);
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

  self->size_changed = TRUE;
  gtk_widget_queue_draw (widget);
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
gboolean bt_pattern_editor_get_selection(BtPatternEditor *self,
                                          gint *start, gint *end,
                                          gint *group, gint *param)
{
  if (!self->selection_enabled)
    return FALSE;
  *start = self->selection_start;
  *end = self->selection_end;
  *group = self->selection_mode == PESM_ALL ? -1 : self->selection_group;
  *param = self->selection_mode != PESM_COLUMN ? -1 : self->selection_param;
  return TRUE;
}

