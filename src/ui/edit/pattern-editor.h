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

#ifndef BT_PATTERN_EDITOR_H
#define BT_PATTERN_EDITOR_H

#include <gtk/gtk.h>
#include <pango/pango.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BtPatternEditor, bt_pattern_editor, BT, PATTERN_EDITOR, GtkWidget);

#define BT_TYPE_PATTERN_EDITOR          (bt_pattern_editor_get_type())

/**
 * BtPatternEditorColumnType:
 * @PCT_NOTE: musical notes
 * @PCT_SWITCH: on/off switches
 * @PCT_BYTE: 8bit values
 * @PCT_WORD: 16bit value
 * @PCT_FLOAT: single precission floating point values
 *
 * Column data types.
 */
enum _BtPatternEditorColumnType {
  PCT_NOTE=0,
  PCT_SWITCH,
  PCT_BYTE,
  PCT_WORD,
  PCT_FLOAT
};
typedef enum _BtPatternEditorColumnType BtPatternEditorColumnType;

/**
 * BtPatternEditorColumn:
 * @type: column value type
 * @def: default value
 * @min: minimum allowed value
 * @max: maximum allowed value
 * @user_data: extra data to attach
 *
 * A parameter column.
 */
struct _BtPatternEditorColumn {
  BtPatternEditorColumnType type;
  float def, min, max;
  gpointer user_data;
};
typedef struct _BtPatternEditorColumn BtPatternEditorColumn;

/**
 * BtPatternEditorColumnGroup:
 * @name: group name
 * @num_columns: number of columns
 * @columns: array of columns
 * @vg: extra data for main-page-patterns
 * @fmt: extra data for main-page-patterns
 *
 * A group of #BtPatternEditorColumns, such as a voice or all global parameters.
 */
struct _BtPatternEditorColumnGroup {
  // can be used for the headline above the group
  gchar *name;
  guint num_columns;
  BtPatternEditorColumn *columns;
  /* user_data for main-page-patterns */
  BtValueGroup *vg;
  gchar *fmt;
  /* < private > */
  /* in pixels for now, may change to chars some day when needed */
  guint width;
};
typedef struct _BtPatternEditorColumnGroup BtPatternEditorColumnGroup;

/**
 * BtPatternEditorCallbacks:
 * @get_data_func: called to fetch data from the model and convert for view
 * @set_data_func: called to convert from view format for storage in the model
 *
 * Data format conversion callbacks.
 */
struct _BtPatternEditorCallbacks {
  /* FIXME(ensonic): what about supplying
   * - BtPatternEditorColumn instead of BtPatternEditorColumn->user_data
   * - BtPatternEditorColumnGroup instead of track;
   */
  gfloat (*get_data_func)(gpointer pattern_data, gpointer column_data, guint row, guint group, guint param);
  void (*set_data_func)(gpointer pattern_data, gpointer column_data, guint row, guint group, guint param, guint digit, gfloat value);
};
typedef struct _BtPatternEditorCallbacks BtPatternEditorCallbacks;

/**
 * BtPatternEditorSelectionMode:
 * @PESM_COLUMN: a single columns
 * @PESM_GROUP: a whole group
 * @PESM_ALL: all columns (and groups)
 *
 * Seleting single columns, whole groups (e.g. voices) or the whole pattern.
 */
enum _BtPatternEditorSelectionMode {
  PESM_COLUMN,
  PESM_GROUP,
  PESM_ALL
};
typedef enum _BtPatternEditorSelectionMode BtPatternEditorSelectionMode;

/**
 * BtPatternEditor:
 *
 * Opaque editor instance data.
 */
struct _BtPatternEditor {
  GtkWidget parent;

  /*< private >*/
  /// GDK4 GdkWindow *window;
  /* cursor position */
  guint row;
  guint group;
  guint parameter;
  guint digit;
  /* cursor step */
  guint step;
  /* scroll location */
  gint ofs_x, ofs_y;
  /* pattern data */
  guint num_lines, num_groups, num_rows;
  BtPatternEditorColumnGroup *groups;
  BtPatternEditorCallbacks *callbacks;
  gpointer pattern_data;
  /* selection */
  gboolean selection_enabled;
  BtPatternEditorSelectionMode selection_mode;
  gint selection_start, selection_end, selection_group, selection_param;

  /* font metrics */
  PangoLayout *pl;
  guint cw, ch;
  guint rowhdr_width, colhdr_height;

  /* current octave number */
  guint octave;

  /* position of playing pointer from 0.0 ... 1.0 */
  gdouble play_pos;
  /* own colors */
  GdkRGBA play_pos_color, text_color, sel_color[2], cursor_color;
  GdkRGBA bg_shade_color[2], value_color[2];

  /* scroll adjustments */
  GtkAdjustment *hadj, *vadj;
  GtkScrollablePolicy hscroll_policy, vscroll_policy;
};

struct _BtPatternEditorClass {
  GtkWidgetClass parent_class;

  /* signal slots */
  void (*set_scroll_adjustments) (BtPatternEditor* self,
				  GtkAdjustment  * horizontal,
				  GtkAdjustment  * vertical);
};

/* note: does not copy the BtPatternEditorColumn * data (in this version) */
void bt_pattern_editor_set_pattern (BtPatternEditor *self,
    gpointer pattern_data, guint num_rows, guint num_groups,
    BtPatternEditorColumnGroup *groups, BtPatternEditorCallbacks *cb);

gboolean bt_pattern_editor_get_selection (BtPatternEditor *self,
    gint *start, gint *end, gint *group, gint *param);

gboolean bt_pattern_editor_position_to_coords (BtPatternEditor * self,
    gint x, gint y, gint * row, gint * group, gint * parameter, gint * digit);

GtkWidget *bt_pattern_editor_new (void);

GType bt_pattern_editor_get_type (void);

G_END_DECLS

#endif // BT_PATTERN_EDITOR_H

