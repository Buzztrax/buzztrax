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
  PCT_WORD,
  PCT_FLOAT
};

typedef struct _PatternColumn
{
  enum PatternColumnType type;
  float def, min, max;
  gpointer user_data;
} PatternColumn;

typedef struct _PatternColumnGroup
{
  // just an id to tell groups apart (wire=0, global=1, voice=2)
  int type;
  // can be used for the headline above the group
  char *name;
  int num_columns;
  PatternColumn *columns;
  gpointer user_data;  
  int width; /* in pixels for now, may change to chars some day when needed */
} PatternColumnGroup;

typedef struct _BtPatternEditorCallbacks
{
  /* FIXME: what about supplying
   * - PatternColumn instead of PatternColumn->user_data
   * - PatternColumnGroup instead of track;
   */
  float (*get_data_func)(gpointer pattern_data, gpointer column_data, int row, int group, int param);
  void (*set_data_func)(gpointer pattern_data, gpointer column_data, int row, int group, int param, float value);
  void (*notify_cursor_func)(gpointer pattern_data, int row, int group, int param, int digit);
} BtPatternEditorCallbacks;

typedef struct _BtPatternEditorPrivate BtPatternEditorPrivate;

enum BtPatternEditorSelectionMode
{
  PESM_COLUMN,
  PESM_GROUP,
  PESM_ALL
};

typedef struct _BtPatternEditor
{
  GtkWidget parent;

  /*< private >*/
  /* cursor position */
  int row;
  int group;
  int parameter;
  int digit;
  /* cursor step */
  int step;
  /* scroll location */
  int ofs_x, ofs_y;
  /* pattern data */
  int num_lines, num_groups, num_rows;
  PatternColumnGroup *groups;
  BtPatternEditorCallbacks *callbacks;
  gpointer pattern_data;
  /* selection */
  gboolean selection_enabled;
  enum BtPatternEditorSelectionMode selection_mode;
  int selection_start, selection_end, selection_group, selection_param;
  
  /* font metrics */
  PangoLayout *pl;
  int cw, ch;
  int rowhdr_width, colhdr_height;

  gboolean size_changed;
  
  /* current octave number */
  guint octave;

  /* position of playing pointer from 0.0 ... 1.0 */
  gdouble play_pos;
  /* own colors */
  GdkGC *play_pos_gc, *shade_gc;
  
  /* scroll adjustments */
  GtkAdjustment *hadj,*vadj;

} BtPatternEditor;

typedef struct _BtPatternEditorClass
{
  GtkWidgetClass parent_class;

  /* signal slots */
  void (*set_scroll_adjustments) (BtPatternEditor* self,
				  GtkAdjustment  * horizontal,
				  GtkAdjustment  * vertical);
} BtPatternEditorClass;

/* note: does not copy the PatternColumn * data (in this version) */
extern void
bt_pattern_editor_set_pattern (BtPatternEditor *view,
                          gpointer pattern_data,
                          int num_rows,
                          int num_groups,
                          PatternColumnGroup *groups,
                          BtPatternEditorCallbacks *cb);

GtkWidget *bt_pattern_editor_new();

GType bt_pattern_editor_get_type (void);

gboolean bt_pattern_editor_get_selection (BtPatternEditor *self,
                                          int *start, int *end, 
                                          int *group, int *param);

G_END_DECLS

#endif // BT_PATTERN_EDITOR_H

