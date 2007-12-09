/* $Id: pattern-editor.h,v 1.2 2007-12-05 21:34:51 kfoltman Exp $
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

#ifndef BT_PATTERN_EDITOR_H
#define BT_PATTERN_EDITOR_H

#include <gtk/gtk.h>
#include <pango/pango.h>

G_BEGIN_DECLS

#define BT_TYPE_PATTERN_EDITOR          (bt_pattern_editor_get_type())
#define BT_PATTERN_EDITOR(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN_EDITOR, BtPatternEditor))
#define BT_IS_PATTERN_EDITOR(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN_EDITOR))
#define BT_PATTERN_EDITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  BT_TYPE_PATTERN_EDITOR, BtPatternEditorClass))
#define BT_IS_PATTERN_EDITOR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BT_TYPE_PATTERN_EDITOR))


enum PatternColumnType
{
  PCT_NOTE,
  PCT_SWITCH,
  PCT_BYTE,
  PCT_WORD
};

typedef struct _PatternColumn
{
  enum PatternColumnType type;
  float def, min, max;
  gpointer user_data;
} PatternColumn;

typedef struct _BtPatternEditorCallbacks
{
  float (*get_data_func)(gpointer pattern_data, PatternColumn *column_data, int row, int track, int param);
  void (*set_data_func)(gpointer pattern_data, PatternColumn *column_data, int row, int track, int param, float value);
  void (*notify_cursor_func)(gpointer pattern_data, int row, int track, int param, int digit);
} BtPatternEditorCallbacks;

typedef struct _BtPatternEditor
{
  GtkWidget parent;
  /* cursor position */
  int row;
  int track;
  int parameter;
  int digit;
  /* scroll location */
  int ofs_x, ofs_y;
  /* pattern data */
  int num_lines, num_tracks, num_globals, num_locals, num_rows;
  PatternColumn *globals;
  PatternColumn *locals;
  BtPatternEditorCallbacks *callbacks;
  gpointer pattern_data;
  
  /* font metrics */
  int cw, ch;
  /* pixel widths of global section, a single track and the row-number column*/
  int global_width, local_width, rowhdr_width;
  
  /* current octave number */
  int octave;
} BtPatternEditor;

typedef struct _BtPatternEditorClass
{
  GtkWidgetClass parent_class;  
} BtPatternEditorClass;

/* note: does not copy the PatternColumn * data (in this version) */
extern void
bt_pattern_editor_set_pattern (BtPatternEditor *view,
                          gpointer pattern_data,
                          int num_rows,
                          int num_tracks,
                          int num_globals,
                          int num_locals,
                          PatternColumn *globals,
                          PatternColumn *locals,
                          BtPatternEditorCallbacks *cb);

GtkWidget *bt_pattern_editor_new();

GType bt_pattern_editor_get_type (void);

G_END_DECLS

#endif // BT_PATTERN_EDITOR_H

