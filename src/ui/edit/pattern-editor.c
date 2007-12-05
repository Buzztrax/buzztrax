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

/**** TODO
    * API for setting current octave 
    * block operations
          o copy
          o paste
          o cut
          o clear
          o interpolate
          o expand
          o shrink 
    * cursor step (different than Buzz - Buzz did it in a bad way)
    * integrate with the rest of the code
    * mouse handling
    ********************************/

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
      GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK | 
      GDK_BUTTON_RELEASE_MASK /*| GDK_POINTER_MOTION_MASK |
      GDK_POINTER_MOTION_HINT_MASK*/;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_user_data (widget->window, widget);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}

struct ParamType
{
  int chars, columns;
  void (*to_string_func)(gchar *dest, float value, int def_value);
  void (*from_key_func)(float *value_ptr, int key, int modifiers); 
  int column_pos[4];
};

static void
to_string_note (char *buf, float value, int def_value)
{
  static gchar note_names[] = "C-C#D-D#E-F-F#G-G#A-A#B-????????";
  int note = ((int)value)&255, octave;
  if (note == def_value) {
    strcpy(buf, "...");
    return;
  }
  if (note == 254) {
    strcpy(buf, "-r-");
    return;
  }
  octave = note >> 4;
  buf[0] = note_names[2 * (note & 15)];
  buf[1] = note_names[1 + 2 * (note & 15)];
  buf[2] = '0' + octave;
  buf[3] = '\0';
}

static void
to_string_trigger (char *buf, float value, int def_value)
{
  int v = (int)value;
  if (v == def_value)
    strcpy(buf, ".");
  else
    sprintf(buf, "%01X", v != 0 ? 1 : 0);
}

static void
to_string_byte (char *buf, float value, int def_value)
{
  int v = (int)value;
  if (v == def_value)
    strcpy(buf, "..");
  else
    sprintf(buf, "%02X", v & 255);
}

static void
to_string_word (char *buf, float value, int def_value)
{
  int v = (int)value;
  if (v == def_value)
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
bt_pattern_editor_draw_rownum (BtPatternEditor *view,
                          int x,
                          int y,
                          int row,
                          PangoLayout *pl)
{
  char buf[16];
  int cw = view->cw, ch = view->ch;
  GtkWidget *widget = &view->parent;

  while (y < widget->allocation.height && row < view->num_rows) {
    sprintf(buf, "%04X", row);
    pango_layout_set_text (pl, buf, 4);
    gdk_draw_layout_with_colors (widget->window, widget->style->fg_gc[widget->state], x, y, pl, &widget->style->fg[1], &widget->style->bg[1]);
    y += ch;
    row++;
  }
  return cw * 5;
}

static int
bt_pattern_editor_draw_column (BtPatternEditor *view,
                          int x,
                          int y,
                          PatternColumn *col,
                          int track,
                          int param,
                          int row,
                          int max_y,
                          PangoLayout *pl)
{
  char buf[16];
  int cw = view->cw, ch = view->ch;
  GtkWidget *widget = &view->parent;
  struct ParamType *pt = &param_types[col->type];

  while (y < max_y && row < view->num_rows) {
    pt->to_string_func(buf, view->callbacks->get_data_func(view->pattern_data, col->user_data, row, track, param), col->def_value);
    pango_layout_set_text (pl, buf, pt->chars);
    gdk_draw_layout_with_colors (widget->window, widget->style->fg_gc[widget->state], x, y, pl, &widget->style->fg[1], &widget->style->bg[1]);
    if (row == view->row && param == view->parameter && track == view->track) {
      int cp = pt->column_pos[view->digit];
      pango_layout_set_text (pl, buf + cp, 1);
      gdk_draw_layout_with_colors (widget->window, widget->style->fg_gc[widget->state], x + cw * cp, y, pl, &widget->style->bg[1], &widget->style->fg[1]);
    }
    y += ch;
    row++;
  }
  return cw * (pt->chars + 1);
}
                          

static gboolean
bt_pattern_editor_expose (GtkWidget *widget,
                     GdkEventExpose *event)
{
  PangoContext *pc;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  PangoFontMetrics *pfm;
  BtPatternEditor *view = (BtPatternEditor *)widget;
  GdkRectangle rect = event->area;
  int cx = widget->allocation.width, cy = widget->allocation.height;
  int y, x, i, row, t, max_y;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BT_IS_PATTERN_EDITOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  gdk_window_clear_area (widget->window, 0, 0, cx, cy);
  /* gdk_draw_line (widget->window, widget->style->fg_gc[widget->state], 0, 0, cx, cy); */
  pc = gtk_widget_get_pango_context (widget);
  pfd = pango_font_description_new();
  pango_font_description_set_family_static (pfd, "Bitstream Vera Sans Mono");
  pango_font_description_set_absolute_size (pfd, 12 * PANGO_SCALE);
  pango_context_load_font (pc, pfd);
  pfm = pango_context_get_metrics (pc, pfd, NULL);
  pl = pango_layout_new (pc);
  pango_layout_set_font_description (pl, pfd);
  view->cw = pango_font_metrics_get_approximate_digit_width (pfm) / PANGO_SCALE;
  view->ch = (pango_font_metrics_get_ascent (pfm) + pango_font_metrics_get_descent (pfm)) / PANGO_SCALE;
  x = -view->ofs_x;
  y = - view->ofs_y;
  /* calculate which row to start from */
  row = (rect.y + view->ofs_y) / view->ch;
  y += row * view->ch;
  max_y = rect.y + rect.height; /* max y */
  int start = x;
  x += bt_pattern_editor_draw_rownum(view, x, y, row, pl) + view->cw;
  view->rowhdr_width = x - start;
  if (view->num_globals) {
    int start = x;
    for (i = 0; i < view->num_globals; i++) {
      x += bt_pattern_editor_draw_column(view, x, y, &view->globals[i], -1, i, row, max_y, pl);
    }
    x += view->cw;
    view->global_width = x - start;
  }
  for (t = 0; t < view->num_tracks; t++) {
    int start = x;
    for (i = 0; i < view->num_locals; i++) {
      x += bt_pattern_editor_draw_column(view, x, y, &view->locals[i], t, i, row, max_y, pl);
    }
    x += view->cw;
    view->local_width = x - start;
  }
  g_object_unref (pl);
  pango_font_metrics_unref (pfm);
  pango_font_description_free (pfd);
  return FALSE;
}

static void
bt_pattern_editor_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
  requisition->width = 768;
  requisition->height = 256;
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
bt_pattern_editor_refresh_cursor (BtPatternEditor *view)
{
  int y = view->row * view->ch - view->ofs_y;
  gtk_widget_queue_draw_area (&view->parent, 0, y, view->parent.allocation.width, view->ch);
}

static void
bt_pattern_editor_refresh_cursor_or_scroll (BtPatternEditor *view)
{
  int first_row = view->ofs_y / view->ch;
  int last_row = (view->ofs_y + view->parent.allocation.height) / view->ch - 1;
  if (view->row < first_row) {
    view->ofs_y = view->row * view->ch;
    gtk_widget_queue_draw (&view->parent);
    return;
  }
  if (view->row > last_row) {
    int visible_rows = view->parent.allocation.height / view->ch;
    first_row = view->row - visible_rows + 1;
    if (first_row < 0)
      first_row = 0;
    view->ofs_y = first_row * view->ch;
    gtk_widget_queue_draw (&view->parent);
    return;
  }
  bt_pattern_editor_refresh_cursor(view);
}

static void
advance (BtPatternEditor *view)
{
  if (view->row < view->num_rows - 1) {
    bt_pattern_editor_refresh_cursor(view);
    view->row++;
  }
  bt_pattern_editor_refresh_cursor_or_scroll(view);
}

static gboolean
bt_pattern_editor_key_press (GtkWidget *widget,
                        GdkEventKey *event)
{
  BtPatternEditor *view = (BtPatternEditor *)widget;
  if ((view->num_globals || view->num_locals) && 
    (event->keyval >= 32 && event->keyval < 127) &&
    view->callbacks->set_data_func)
  {
    PatternColumn *col = &(view->track == -1 ? view->globals : view->locals)[view->parameter];
    if (event->keyval == '.') {
      view->callbacks->set_data_func(view->pattern_data, view->row, view->track, view->parameter, (float)col->def_value);
      advance(view);
    }
    else {
      static const char hexdigits[] = "0123456789abcdef";
      static const char notenames[] = "zsxdcvgbhnjm\t\t\t\tq2w3er5t6y7u\t\t\t\ti9o0p";
      float oldvalue = view->callbacks->get_data_func(view->pattern_data, col->user_data, view->row, view->track, view->parameter);
      const char *p;
      switch(col->type) {
      case PCT_TRIGGER:
        if (event->keyval == '0' || event->keyval == '1') {
          view->callbacks->set_data_func(view->pattern_data, view->row, view->track, view->parameter, event->keyval - '0');
          advance(view);
        }
        break;
      case PCT_BYTE:
      case PCT_WORD:
        p = strchr(hexdigits, (char)event->keyval);
        if (p) {
          int value = (int)oldvalue;
          if (oldvalue == col->def_value)
            value = 0;
          int shift = 4*(((col->type == PCT_BYTE) ? 1 : 3) - view->digit);
          value = (value &~(15 << shift)) | ((p - hexdigits) << shift);
          if (value < col->min) value = col->min;
          if (value > col->max) value = col->max;
          
          view->callbacks->set_data_func(view->pattern_data, view->row, view->track, view->parameter, value);
          advance(view);
        }
        break;
      case PCT_NOTE:
        if (view->digit == 0) {
          // FIXME: use event->hardware_keycode because of y<>z
          p = strchr(notenames, (char)event->keyval);
          if (p) {
            int value = (p - notenames) + 16 * view->octave;
            if (value < col->min) value = col->min;
            if (value > col->max) value = col->max;
            
            view->callbacks->set_data_func(view->pattern_data, view->row, view->track, view->parameter, value);
            advance(view);
          }
        }
        if (view->digit == 1) {
          if (isdigit(event->keyval)) {
            int value = (int)oldvalue;
            if (oldvalue == col->def_value)
              value = 0;
            value = (value & 15) | ((event->keyval - '0') << 4);
            if (value < col->min) value = col->min;
            if (value > col->max) value = col->max;
            
            view->callbacks->set_data_func(view->pattern_data, view->row, view->track, view->parameter, value);
            advance(view);
          }
        }
        break;
      }
    }
  }
  switch(event->keyval)
  {
  case GDK_Up:
    if (view->row > 0) {
      bt_pattern_editor_refresh_cursor(view);
      view->row--;
      bt_pattern_editor_refresh_cursor_or_scroll(view);
    }
    return TRUE;
  case GDK_Down:
    if (view->row < view->num_rows - 1) {
      bt_pattern_editor_refresh_cursor(view);
      view->row++;
      bt_pattern_editor_refresh_cursor_or_scroll(view);
    }
    return TRUE;
  case GDK_Left:
    if (view->digit > 0)
      view->digit--;
    else {
      int lowest = view->num_globals ? -1 : 0;
      PatternColumn *pc;
      if (view->parameter > 0) {
        view->parameter--;
      }
      else if (view->track > lowest) { 
        view->track--;
        view->parameter = view->track == -1 ? view->num_globals - 1 : view->num_locals - 1;
      }
      else
        return FALSE;
      pc = view->track == -1 ? &view->globals[view->parameter] : &view->locals[view->parameter];
      view->digit = param_types[pc->type].columns - 1;
    }
    bt_pattern_editor_refresh_cursor (view);
    return TRUE;
  case GDK_Right:
    {
      int maxp = view->track == -1 ? view->num_globals - 1 : view->num_locals - 1;
      PatternColumn *pc = view->track == -1 ? &view->globals[view->parameter] : &view->locals[view->parameter];
      int highest = view->num_locals ? view->num_tracks - 1 : -1;
      if (view->digit < param_types[pc->type].columns - 1)
        view->digit++;
      else if (view->parameter < maxp)
        view->parameter++, view->digit = 0;
      else if (view->track < highest)
        view->track++, view->parameter = 0, view->digit = 0;
    }
    bt_pattern_editor_refresh_cursor (view);
    return TRUE;  
  case GDK_Tab:
    {
      int highest = view->num_locals ? view->num_tracks - 1 : -1;
      if (view->track < highest) {
        if (view->track == -1)
          view->parameter = 0, view->digit = 0;
        view->track++;
      }
      bt_pattern_editor_refresh_cursor (view);
      return TRUE;
    }
  case GDK_ISO_Left_Tab:
    {
      int lowest = view->num_globals ? -1 : 0;
      if (view->track > lowest)
        view->track--;
      if (view->track == -1)
        view->parameter = 0, view->digit = 0;
      bt_pattern_editor_refresh_cursor (view);
      return TRUE;
    }
  }
  return FALSE;
}

int bt_pattern_editor_get_row_width (BtPatternEditor *view)
{
  return view->rowhdr_width + view->local_width + view->global_width * view->num_tracks;
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
  BtPatternEditor *view = (BtPatternEditor *)widget;
  int x = view->ofs_x + event->x;
  int y = view->ofs_y + event->y;
  int parameter, digit;
  if (x < view->rowhdr_width) {
    bt_pattern_editor_refresh_cursor(view);
    view->row = y / view->ch;
    bt_pattern_editor_refresh_cursor_or_scroll(view);
    return TRUE;
  }
  x -= view->rowhdr_width;
  if (x < view->global_width) {
    if (char_to_coords(x / view->cw, view->globals, view->num_globals, &parameter, &digit))
    {
      bt_pattern_editor_refresh_cursor(view);
      view->row = y / view->ch;
      view->track = -1;
      view->parameter = parameter;
      view->digit = digit;
      bt_pattern_editor_refresh_cursor_or_scroll(view);
      return TRUE;
    }
  }
  x -= view->global_width;
  if (x >= view->local_width * view->num_tracks)
    return FALSE;
  if (char_to_coords((x % view->local_width) / view->cw, view->locals, view->num_locals, &parameter, &digit))
  {
    bt_pattern_editor_refresh_cursor(view);
    view->row = y / view->ch;
    view->track = x / view->local_width;
    view->parameter = parameter;
    view->digit = digit;
    bt_pattern_editor_refresh_cursor_or_scroll(view);
    return TRUE;
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
  klass->parent_class.realize = bt_pattern_editor_realize;
  klass->parent_class.expose_event = bt_pattern_editor_expose;
  klass->parent_class.size_request = bt_pattern_editor_size_request;
  klass->parent_class.size_allocate = bt_pattern_editor_size_allocate;
  klass->parent_class.key_press_event = bt_pattern_editor_key_press;
  klass->parent_class.button_press_event = bt_pattern_editor_button_press;
  klass->parent_class.button_release_event = bt_pattern_editor_button_release;
}

static void
bt_pattern_editor_init (BtPatternEditor *view)
{
  view->row = view->parameter = view->digit = 0;
  view->track = -1;
  view->ofs_x = 0, view->ofs_y = 0;
  view->num_globals = 0;
  view->num_locals = 0;
  view->num_tracks = 0;
  view->num_rows = 64;
  view->globals = NULL;
  view->locals = NULL;
  view->octave = 2;
}

void
bt_pattern_editor_set_pattern (BtPatternEditor *view,
                          gpointer pattern_data,
                          int num_rows,
                          int num_tracks,
                          int num_globals,
                          int num_locals,
                          PatternColumn *globals,
                          PatternColumn *locals,
                          BtPatternEditorCallbacks *cb
                          )
{
  int maxval;
  view->num_rows = num_rows;
  view->num_tracks = num_tracks;
  view->num_globals = num_globals;
  view->num_locals = num_locals;
  view->globals = globals;
  view->locals = locals;
  view->pattern_data = pattern_data;
  view->callbacks = cb;

  if (view->row >= view->num_rows)
    view->row = 0;
  if (view->track >= view->num_tracks || !view->num_locals)
    view->track = -1;
  maxval = view->track == -1 ? view->num_globals : view->num_locals;
  if (view->parameter >= maxval)
    view->parameter = 0;
  gtk_widget_queue_draw (&view->parent);
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

#if 0
float global_vals[64][2];
float track_vals[4][64][3];

static float
example_get_data_at (gpointer pattern_data,
                     gpointer column_data,
                     int row,
                     int track,
                     int param)
{
  if (track == -1)
    return global_vals[row][param];
  else
    return track_vals[track][row][param];
}

static void
example_set_data_at (gpointer pattern_data,
                     int row,
                     int track,
                     int param,
                     float value)
{
  if (track == -1)
    global_vals[row][param] = value;
  else
    track_vals[track][row][param] = value;
}

GtkWidget *window;
BtPatternEditor *pview;

static void 
destroy( GtkWidget *widget, 
              gpointer data )
{
  gtk_main_quit();
}

int main(int argc, char *argv[])
{
  static PatternColumn globals[] = { { PCT_BYTE, 256, 0, 255, NULL}, { PCT_WORD, 65535, 0, 32768, NULL } };
  static PatternColumn locals[] = { { PCT_NOTE, 255, 0, 127, NULL}, { PCT_BYTE, 255, 0, 125, NULL}, { PCT_TRIGGER, 255, 0, 1, NULL} };
  static BtPatternEditorCallbacks callbacks = { example_get_data_at, example_set_data_at, NULL };
  int i, j;
  
  for (i=0; i<64; i++) {
    global_vals[i][0] = i;
    global_vals[i][1] = i*257;
    for (j=0; j<4; j++) {
      track_vals[j][i][0] = i;
      track_vals[j][i][1] = i;
      track_vals[j][i][2] = (i&3) ? 255 : ((i>>2)&1);
    }
  }

  gtk_init(&argc, &argv);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

  pview = BT_PATTERN_EDITOR(bt_pattern_editor_new());
  bt_pattern_editor_set_pattern(pview, NULL, 64, 3, 2, 3, globals, locals, &callbacks);

  gtk_container_add (GTK_CONTAINER(window), GTK_WIDGET(custom));
  gtk_widget_show_all (window);
  gtk_widget_grab_focus (GTK_WIDGET(custom));
  gtk_main ();
  return 0;
}
#endif
