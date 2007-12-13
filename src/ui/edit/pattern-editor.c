/* $Id: pattern-editor.c,v 1.2 2007-12-05 21:34:51 kfoltman Exp $
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

/*
 * @todo:
 * - no-value handling
 * - gobject properties
 *       o current octave
 *       o playback position
 * - block operations
 *       o copy
 *       o paste
 *       o cut
 *       o clear
 *       o interpolate
 *       o expand
 *       o shrink 
 * - cursor step (different than Buzz - Buzz did it in a bad way)
 * - mouse handling
 * - implement GtkWidgetClass::set_scroll_adjustments_signal
 *   see gtk/gtkviewport.{c,h}
 *     o left: ticks
 *     o top: groups (input, global, voice 1, voice 2)
 * - use raw-key codes for note-input (see FIXME below and
 *   main-page-pattern.c:on_pattern_table_key_release_event()
 * - a signal for column changed (group, parameter)
 *   to be able to show the parameter details in the status bar
 */

#include <ctype.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "pattern-editor.h"

static void
bt_pattern_editor_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (BT_IS_PATTERN_EDITOR (widget));
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED | GTK_CAN_FOCUS);
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
  
  //widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);
  widget->window = gdk_window_new ( gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_user_data (widget->window, widget);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

struct ParamType
{
  int chars, columns;
  void (*to_string_func)(gchar *dest, float value, int def);
  void (*from_key_func)(float *value_ptr, int key, int modifiers); 
  int column_pos[4];
};

static void
to_string_note (char *buf, float value, int def)
{
  static gchar note_names[] = "C-C#D-D#E-F-F#G-G#A-A#B-????????";
  int note = ((int)value)&255, octave;
  if (note == def || note == 0) {
    strcpy(buf, "...");
    return;
  }
  if (note == 254) {
    strcpy(buf, "-r-");
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
to_string_trigger (char *buf, float value, int def)
{
  int v = (int)value;
  if (v == def)
    strcpy(buf, ".");
  else
    sprintf(buf, "%01X", v != 0 ? 1 : 0);
}

static void
to_string_byte (char *buf, float value, int def)
{
  int v = (int)value;
  if (v == def)
    strcpy(buf, "..");
  else
    sprintf(buf, "%02X", v & 255);
}

static void
to_string_word (char *buf, float value, int def)
{
  int v = (int)value;
  if (v == def)
    strcpy(buf, "....");
  else
    sprintf(buf, "%04X", v & 65535);
}

static struct ParamType param_types[] = {
  { 3, 2, to_string_note, NULL, {0, 2}},
  { 1, 1, to_string_trigger, NULL, {0}},
  { 2, 2, to_string_byte, NULL, {0, 1}},
  { 4, 4, to_string_word, NULL, {0, 1, 2, 3}},
};



static int
bt_pattern_editor_draw_rownum (BtPatternEditor *self,
                          int x,
                          int y,
                          int row,
                          PangoLayout *pl)
{
  char buf[16];
  int cw = self->cw, ch = self->ch;
  GtkWidget *widget = GTK_WIDGET(self);
  int col_w = cw * 5;

  while (y < widget->allocation.height && row < self->num_rows) {
    gdk_draw_rectangle (widget->window,
      (row&0x1) ? widget->style->bg_gc[widget->state] : widget->style->light_gc[widget->state],
      TRUE, x, y, col_w, ch);
    
    sprintf(buf, "%04X", row);
    pango_layout_set_text (pl, buf, 4);
    gdk_draw_layout_with_colors (widget->window, widget->style->fg_gc[widget->state], x, y, pl,
      &widget->style->text[GTK_STATE_NORMAL], NULL);
    y += ch;
    row++;
  }
  return col_w;
}

static int
bt_pattern_editor_draw_column (BtPatternEditor *self,
                          int x,
                          int y,
                          PatternColumn *col,
                          int group,
                          int param,
                          int row,
                          int max_y,
                          PangoLayout *pl)
{
  GtkWidget *widget = GTK_WIDGET(self);
  struct ParamType *pt = &param_types[col->type];
  char buf[16];
  int cw = self->cw, ch = self->ch;
  int col_w = cw * (pt->chars + 1);
  /*
  GdkGC *gcs[]={
    widget->style->fg_gc[widget->state],
    widget->style->bg_gc[widget->state],
    widget->style->light_gc[widget->state],
    widget->style->dark_gc[widget->state],
    widget->style->mid_gc[widget->state],
    widget->style->text_gc[widget->state],
    widget->style->base_gc[widget->state],
    widget->style->text_aa_gc[widget->state],
  };
  */  

  while (y < max_y && row < self->num_rows) {
    gdk_draw_rectangle (widget->window,
      /* gcs[row&0x7] */
      (row&0x1) ? widget->style->bg_gc[widget->state] : widget->style->light_gc[widget->state],
      TRUE, x, y, col_w, ch);
    
    pt->to_string_func(buf, self->callbacks->get_data_func(self->pattern_data, col->user_data, row, group, param), col->def);
    pango_layout_set_text (pl, buf, pt->chars);
    gdk_draw_layout_with_colors (widget->window, widget->style->fg_gc[widget->state], x, y, pl,
      &widget->style->text[GTK_STATE_NORMAL], NULL);
    if (row == self->row && param == self->parameter && group == self->group) {
      int cp = pt->column_pos[self->digit];
      pango_layout_set_text (pl, buf + cp, 1);
      gdk_draw_layout_with_colors (widget->window, widget->style->fg_gc[widget->state], x + cw * cp, y, pl,
        &widget->style->bg[GTK_STATE_NORMAL], &widget->style->text[GTK_STATE_NORMAL]);
    }
    y += ch;
    row++;
  }
  return col_w;
}
                          

static gboolean
bt_pattern_editor_expose (GtkWidget *widget,
                     GdkEventExpose *event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);
  PangoContext *pc;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  PangoFontMetrics *pfm;
  GdkRectangle rect = event->area;
  int y, x, i, row, g, max_y;
  int start;

  g_return_val_if_fail (BT_IS_PATTERN_EDITOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (event->count > 0)
    return FALSE;

  
  //printf("Area: %d,%d -> %d,%d\n",rect.x, rect.y, rect.width, rect.height);

  gdk_window_clear_area (widget->window, 0, 0, widget->allocation.width, widget->allocation.height);
  //gdk_draw_line (widget->window, widget->style->fg_gc[widget->state], 0, 0, widget->allocation.width, widget->allocation.height);

  /* calculate font-metrics */  
  pc = gtk_widget_get_pango_context (widget);
  pfd = pango_font_description_new();
  pango_font_description_set_family_static (pfd, "Bitstream Vera Sans Mono");
  pango_font_description_set_absolute_size (pfd, 12 * PANGO_SCALE);
  pango_context_load_font (pc, pfd);

  pfm = pango_context_get_metrics (pc, pfd, NULL);
  self->cw = pango_font_metrics_get_approximate_digit_width (pfm) / PANGO_SCALE;
  self->ch = (pango_font_metrics_get_ascent (pfm) + pango_font_metrics_get_descent (pfm)) / PANGO_SCALE;
  pango_font_metrics_unref (pfm);

  pl = pango_layout_new (pc);
  pango_layout_set_font_description (pl, pfd);

  x = -self->ofs_x;
  y = -self->ofs_y;
  /* calculate which row to start from */
  row = (rect.y + self->ofs_y) / self->ch;
  y += row * self->ch;
  max_y = rect.y + rect.height; /* max y */
  
  /* draw row-number column */
  start = x;
  x += bt_pattern_editor_draw_rownum(self, x, y, row, pl) + self->cw;
  self->rowhdr_width = x - start;

  /* draw group parameter columns */
  for (g = 0; g < self->num_groups; g++) {
    PatternColumnGroup *cgrp = &self->groups[g];
    start = x;
    for (i = 0; i < cgrp->num_columns; i++) {
      x += bt_pattern_editor_draw_column(self, x, y, &cgrp->columns[i], g, i, row, max_y, pl);
    }
    x += self->cw;
    cgrp->width = x - start;
  }
  
  g_object_unref (pl);
  pango_font_description_free (pfd);

  if (G_UNLIKELY(self->size_changed)) {  
    // do this for the after the first redraw
    self->size_changed=FALSE;
    gtk_widget_queue_resize_no_redraw (widget);
  }  
  return FALSE;
}

static int
bt_pattern_editor_get_row_width (BtPatternEditor *self)
{
  int width = 0, g;
  for (g = 0; g < self->num_groups; g++)
    width += self->groups[g].width;
  return width;
}

static int
bt_pattern_editor_get_col_height (BtPatternEditor *self)
{
  return (self->num_rows * self->ch);
}


static void
bt_pattern_editor_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);

  /* calculate from pattern size */
  requisition->width = bt_pattern_editor_get_row_width(self);
  requisition->height = bt_pattern_editor_get_col_height(self);
  //printf("Size: %d,%d\n",requisition->width, requisition->height);
}

static void
bt_pattern_editor_size_allocate (GtkWidget *widget,
                            GtkAllocation *allocation)
{
  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
  {
    gdk_window_move_resize (widget->window, allocation->x, allocation->y, allocation->width, allocation->height);
  }
}

static void
bt_pattern_editor_refresh_cursor (BtPatternEditor *self)
{
  int y = self->row * self->ch - self->ofs_y;
  gtk_widget_queue_draw_area (&self->parent, 0, y, self->parent.allocation.width, self->ch);
}

static void
bt_pattern_editor_refresh_cursor_or_scroll (BtPatternEditor *self)
{
  int first_row = self->ofs_y / self->ch;
  int last_row = (self->ofs_y + self->parent.allocation.height) / self->ch - 1;
  if (self->row < first_row) {
    self->ofs_y = self->row * self->ch;
    gtk_widget_queue_draw (&self->parent);
    return;
  }
  if (self->row > last_row) {
    int visible_rows = self->parent.allocation.height / self->ch;
    first_row = self->row - visible_rows + 1;
    if (first_row < 0)
      first_row = 0;
    self->ofs_y = first_row * self->ch;
    gtk_widget_queue_draw (&self->parent);
    return;
  }
  bt_pattern_editor_refresh_cursor(self);
}

static void
advance (BtPatternEditor *self)
{
  if (self->row < self->num_rows - 1) {
    bt_pattern_editor_refresh_cursor(self);
    self->row++;
  }
  bt_pattern_editor_refresh_cursor_or_scroll(self);
}

static PatternColumn *
cur_column (BtPatternEditor *self)
{
  return &self->groups[self->group].columns[self->parameter];
}

static gboolean
bt_pattern_editor_key_press (GtkWidget *widget,
                        GdkEventKey *event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);
  if (self->num_groups && 
    (event->keyval >= 32 && event->keyval < 127) &&
    self->callbacks->set_data_func)
  {
    PatternColumn *col = &self->groups[self->group].columns[self->parameter];
    if (event->keyval == '.') {
      self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, col->def);
      advance(self);
    }
    else {
      static const char hexdigits[] = "0123456789abcdef";
      static const char notenames[] = "zsxdcvgbhnjm\t\t\t\tq2w3er5t6y7u\t\t\t\ti9o0p";
      float oldvalue = self->callbacks->get_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter);
      const char *p;
      switch(col->type) {
      case PCT_SWITCH:
        if (event->keyval == '0' || event->keyval == '1') {
          self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, event->keyval - '0');
          advance(self);
        }
        break;
      case PCT_BYTE:
      case PCT_WORD:
        p = strchr(hexdigits, (char)event->keyval);
        if (p) {
          int value = (int)oldvalue;
          if (oldvalue == col->def)
            value = 0;
          int shift = 4*(((col->type == PCT_BYTE) ? 1 : 3) - self->digit);
          value = (value &~(15 << shift)) | ((p - hexdigits) << shift);
          if (value < col->min) value = col->min;
          if (value > col->max) value = col->max;
          
          self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, value);
          advance(self);
        }
        break;
      case PCT_NOTE:
        if (self->digit == 0) {
          // FIXME: use event->hardware_keycode because of y<>z
          p = strchr(notenames, (char)event->keyval);
          if (p) {
            int value = 1 + (p - notenames) + 16 * self->octave;
            if (value < col->min) value = col->min;
            if (value > col->max) value = col->max;
            
            self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, value);
            advance(self);
          }
        }
        if (self->digit == 1) {
          if (isdigit(event->keyval)) {
            int value = (int)oldvalue;
            if (oldvalue == col->def)
              value = 0;
            value = (value & 15) | ((event->keyval - '0') << 4);
            if (value < col->min) value = col->min;
            if (value > col->max) value = col->max;
            
            self->callbacks->set_data_func(self->pattern_data, col->user_data, self->row, self->group, self->parameter, value);
            advance(self);
          }
        }
        break;
      }
    }
  }
  if (self->num_groups) {
    switch(event->keyval)
    {
    case GDK_Up:
      if (self->row > 0) {
        bt_pattern_editor_refresh_cursor(self);
        self->row--;
        bt_pattern_editor_refresh_cursor_or_scroll(self);
      }
      return TRUE;
    case GDK_Down:
      if (self->row < self->num_rows - 1) {
        bt_pattern_editor_refresh_cursor(self);
        self->row++;
        bt_pattern_editor_refresh_cursor_or_scroll(self);
      }
      return TRUE;
    case GDK_Left:
      if (self->digit > 0)
        self->digit--;
      else {
        PatternColumn *pc;
        if (self->parameter > 0) {
          self->parameter--;
        }
        else if (self->group > 0) { 
          self->group--;
          self->parameter = self->groups[self->group].num_columns - 1;
        }
        else
          return FALSE;
        pc = cur_column (self);
        self->digit = param_types[pc->type].columns - 1;
      }
      bt_pattern_editor_refresh_cursor (self);
      return TRUE;
    case GDK_Right:
      {
        PatternColumn *pc = cur_column (self);
        if (self->digit < param_types[pc->type].columns - 1)
          self->digit++;
        else if (self->parameter < self->groups[self->group].num_columns - 1)
          self->parameter++, self->digit = 0;
        else if (self->group < self->num_groups - 1)
          self->group++, self->parameter = 0, self->digit = 0;
      }
      bt_pattern_editor_refresh_cursor (self);
      return TRUE;  
    case GDK_Tab:
      {
        if (self->group < self->num_groups - 1) {
          /* jump to same column when jumping from track to track, otherwise jump to first column of the group */
          if (self->groups[self->group].type != self->groups[self->group + 1].type)
            self->parameter = 0, self->digit = 0;
          self->group++;
        }
        bt_pattern_editor_refresh_cursor (self);
        return TRUE;
      }
    case GDK_ISO_Left_Tab:
      {
        if (self->group > 0) {
          self->group--;
          if (self->groups[self->group].type != self->groups[self->group + 1].type)
            self->parameter = 0, self->digit = 0;
        }
        else /* at leftmost group, reset cursor to first column */
          self->parameter = 0, self->digit = 0;
        bt_pattern_editor_refresh_cursor (self);
        return TRUE;
      }
    }
  }
  return FALSE;
}

static gboolean
char_to_coords(int charpos,
               PatternColumn *columns, 
               int num_cols,
               int *parameter,
               int *digit)
{
  int i, j;
  for (i = 0; i < num_cols; i++)
  {
    struct ParamType *type = &param_types[columns[i].type];
    if (charpos < type->chars)
    {
      for (j = 0; j < type->columns; j++)
      {
        if (type->column_pos[j] == charpos) {
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

static gboolean
bt_pattern_editor_button_press (GtkWidget *widget,
                                GdkEventButton *event)
{
  BtPatternEditor *self = BT_PATTERN_EDITOR(widget);
  int x = self->ofs_x + event->x;
  int y = self->ofs_y + event->y;
  int parameter, digit;
  int g;
  if (x < self->rowhdr_width) {
    bt_pattern_editor_refresh_cursor(self);
    self->row = y / self->ch;
    bt_pattern_editor_refresh_cursor_or_scroll(self);
    return TRUE;
  }
  x -= self->rowhdr_width;
  for (g = 0; g < self->num_groups; g++)
  {
    PatternColumnGroup *grp = &self->groups[g];
    if (x < grp->width)
    {
      if (char_to_coords(x / self->cw, grp->columns, grp->num_columns, &parameter, &digit))
      {
        bt_pattern_editor_refresh_cursor(self);
        self->row = y / self->ch;
        self->group = g;
        self->parameter = parameter;
        self->digit = digit;
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
bt_pattern_editor_class_init (BtPatternEditorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  
  widget_class->realize = bt_pattern_editor_realize;
  widget_class->expose_event = bt_pattern_editor_expose;
  widget_class->size_request = bt_pattern_editor_size_request;
  widget_class->size_allocate = bt_pattern_editor_size_allocate;
  widget_class->key_press_event = bt_pattern_editor_key_press;
  widget_class->button_press_event = bt_pattern_editor_button_press;
  widget_class->button_release_event = bt_pattern_editor_button_release;
}

static void
bt_pattern_editor_init (BtPatternEditor *self)
{
  self->row = self->parameter = self->digit = 0;
  self->group = 0;
  self->ofs_x = 0, self->ofs_y = 0;
  self->groups = NULL;
  self->num_groups = 0;
  self->num_rows = 64;
  self->octave = 2;
  self->size_changed = TRUE;
}

/**
 * bt_pattern_editor_set_pattern:
 */
void
bt_pattern_editor_set_pattern (BtPatternEditor *self,
                          gpointer pattern_data,
                          int num_rows,
                          int num_groups,
                          PatternColumnGroup *groups,
                          BtPatternEditorCallbacks *cb
                          )
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


GType
bt_pattern_editor_get_type (void)
{
  static GType type = 0;
  if (!type) {
    static const GTypeInfo type_info = {
      sizeof(BtPatternEditorClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc)bt_pattern_editor_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(BtPatternEditor),
      0,    /* n_preallocs */
      (GInstanceInitFunc)bt_pattern_editor_init
    };
    
    type = g_type_register_static(GTK_TYPE_WIDGET,
                                  "BtPatternEditor",
                                  &type_info,
                                  0);
  }
  return type;
}

GtkWidget *
bt_pattern_editor_new()
{
  return GTK_WIDGET( g_object_new (BT_TYPE_PATTERN_EDITOR, NULL ));
}
