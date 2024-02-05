/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btsequenceview
 * @short_description: the editor main sequence table view
 * @see_also: #BtMainPageSequence
 *
 * This widget derives from the #GtkTreeView to additionaly draw loop- and
 * play-position bars.
 */

#define BT_EDIT
#define BT_SEQUENCE_VIEW_C

#include <math.h>

#include "bt-edit.h"
#include "sequence-view.h"

// this only works for 4/4 meassure
//#define IS_SEQUENCE_POS_VISIBLE(pos,bars) ((pos&((bars)-1))==0)
#define IS_SEQUENCE_POS_VISIBLE(pos,bars) ((pos%bars)==0)
#define SEQUENCE_CELL_WIDTH 100
#define SEQUENCE_CELL_HEIGHT 28
#define SEQUENCE_CELL_XPAD 0
#define SEQUENCE_CELL_YPAD 0
#define POSITION_CELL_WIDTH 65
#define HEADER_SPACING 0

enum
{
  SEQUENCE_VIEW_PLAY_POSITION = 1,
  SEQUENCE_VIEW_LOOP_START,
  SEQUENCE_VIEW_LOOP_END,
  SEQUENCE_VIEW_BARS,
  SEQUENCE_VIEW_VISIBLE_ROWS,
  SEQUENCE_VIEW_CURRENT_MACHINE
};


struct _BtSequenceView
{
  GtkWidget parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* position of playing pointer from 0.0 ... 1.0 */
  gdouble play_pos;

  /* position of loop range from 0.0 ... 1.0 */
  gdouble loop_start, loop_end;

  /* number of visible rows, the height of one row, number of rows till song end */
  gulong visible_rows, row_height, song_end_rows;

  /* cache some ressources */
  GdkRGBA play_line_color, end_line_color, loop_line_color;

  /* pos unit selection menu */
  GtkDropDown *pos_menu;
  BtSequenceViewPosFormat pos_format;

  GtkDropDown *label_menu;
  
  /* the sequence table */
  GtkColumnView *sequence_pos_table;
  GtkColumnViewColumn *sequence_pos_table_pos;
  GtkBox *sequence_pos_table_header;
  GtkColumnView *sequence_table;
  GtkBox *sequence_table_header;

  gulong bars;
  GtkCustomFilter *row_filter;
  
  /* position-table header label widget */
  GtkWidget *pos_header;

  /* editor change log */
  BtChangeLog *change_log;
  
  BtSequenceGridModel *model;
  GMenu *context_menu;

  BtSongInfo *song_info;
  BtSetup *setup;
  BtSequence *sequence;
  BtMachine *current_machine;

  /* cursor */
  glong cursor_column;
  glong cursor_row;
  /* selection range */
  glong selection_start_column;
  glong selection_start_row;
  glong selection_end_column;
  glong selection_end_row;
  /* selection first cell */
  glong selection_column;
  glong selection_row;
};

//-- the class

G_DEFINE_TYPE (BtSequenceView, bt_sequence_view, GTK_TYPE_WIDGET);


static void sequence_set_play_pos(BtSequenceView * self, glong row);
static void sequence_set_loop_start(BtSequenceView * self, glong row);
static void sequence_set_loop_end(BtSequenceView * self, glong row);
static gchar *format_position(const BtSequenceView * view, gulong pos);
// Returns: (transfer full)
static GtkColumnViewColumn *sequence_table_make_machine_column(BtSequenceView* self, BtMachine *machine,
    guint track);
    
//-- enums

GType
bt_sequence_view_pos_format_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_SEQUENCE_VIEW_POS_FORMAT_TICKS,
          "BT_SEQUENCE_VIEW_POS_FORMAT_TICKS", "ticks"},
      {BT_SEQUENCE_VIEW_POS_FORMAT_TIME,
          "BT_SEQUENCE_VIEW_POS_FORMAT_TIME", "time"},
      {BT_SEQUENCE_VIEW_POS_FORMAT_BEATS,
          "BT_SEQUENCE_VIEW_POS_FORMAT_BEATS", "beats"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtSequenceViewPosFormat", values);
  }
  return type;
}

//-- event handler

static void
on_label_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  GST_INFO ("label_menu has changed : page=%p", user_data);

  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);

  BtListItemLong *item = gtk_drop_down_get_selected_item (self->label_menu);
  if (!item)
    return;
  
  GtkFilterListModel *filter_model =
    GTK_FILTER_LIST_MODEL (gtk_drop_down_get_model (self->label_menu));
  
  gtk_filter_changed (gtk_filter_list_model_get_filter (filter_model),
      GTK_FILTER_CHANGE_DIFFERENT);
}

#if 0 /// GTK4
static void
on_sequence_table_motion_notify_event (GtkEventControllerMotion* ctrl, gdouble x, gdouble y, gpointer user_data)
{

  GST_INFO ("motion notify");

  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (ctrl));
  
  // only activate in button_press ?
  if (state & GDK_BUTTON1_MASK) {
    if (gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)) ==
        (event->window)) {
      
      /// GTK4 guessing this was meant to allow clicking and 'dragging' the play pos?
      
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      // determine sequence position from mouse coordinates
      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x,
              event->y, &path, &column, NULL, NULL)) {
        if (widget == GTK_WIDGET (self->sequence_pos_table)) {
          gulong track, row;
          gulong modifier =
              (gulong) state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK);

          if (sequence_view_get_cursor_pos (GTK_TREE_VIEW (widget), path,
                  column, &track, &row) && !modifier) {
            sequence_set_play_pos (self, (glong) row);
          }
        } else {
          /// GTK4 is this still necessary with columnview?
          // handle selection
          glong cursor_column = self->cursor_column;
          glong cursor_row = self->cursor_row;

          if (self->selection_start_column == -1) {
            self->selection_column = self->cursor_column;
            self->selection_row = self->cursor_row;
          }
          gtk_tree_view_set_cursor (self->sequence_table, path, column,
              FALSE);
          gtk_widget_grab_focus_savely (GTK_WIDGET (self->
                  priv->sequence_table));
          // cursor updates are not yet processed
          on_sequence_table_cursor_changed_idle (self);
          GST_DEBUG ("cursor new/old: %3ld,%3ld -> %3ld,%3ld", cursor_column,
              cursor_row, self->cursor_column, self->cursor_row);
          if ((cursor_column != self->cursor_column)
              || (cursor_row != self->cursor_row)) {
            if (self->selection_start_column == -1) {
              self->selection_start_column =
                  MIN (cursor_column, self->selection_column);
              self->selection_start_row =
                  MIN (cursor_row, self->selection_row);
              self->selection_end_column =
                  MAX (cursor_column, self->selection_column);
              self->selection_end_row =
                  MAX (cursor_row, self->selection_row);
            } else {
              if (self->cursor_column < self->selection_column) {
                self->selection_start_column = self->cursor_column;
                self->selection_end_column = self->selection_column;
              } else {
                self->selection_start_column =
                    self->selection_column;
                self->selection_end_column = self->cursor_column;
              }
              if (self->cursor_row < self->selection_row) {
                self->selection_start_row = self->cursor_row;
                self->selection_end_row = self->selection_row;
              } else {
                self->selection_start_row = self->selection_row;
                self->selection_end_row = self->cursor_row;
              }
            }
            gtk_widget_queue_draw (GTK_WIDGET (self->sequence_table));
          }
        }
        res = TRUE;
      }
      if (path)
        gtk_tree_path_free (path);
    }
  }
}
#endif

static void
on_mute_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
  BtMachine *machine = BT_MACHINE (user_data);

  if (gtk_toggle_button_get_active (togglebutton)) {
    g_object_set (machine, "state", BT_MACHINE_STATE_MUTE, NULL);
  } else {
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  bt_machine_update_default_state_value (machine);
}

static void
on_solo_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
  BtMachine *machine = BT_MACHINE (user_data);

  if (gtk_toggle_button_get_active (togglebutton)) {
    g_object_set (machine, "state", BT_MACHINE_STATE_SOLO, NULL);
  } else {
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  bt_machine_update_default_state_value (machine);
}

static void
on_bypass_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
  BtMachine *machine = BT_MACHINE (user_data);

  if (gtk_toggle_button_get_active (togglebutton)) {
    g_object_set (machine, "state", BT_MACHINE_STATE_BYPASS, NULL);
  } else {
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  bt_machine_update_default_state_value (machine);
}

static void
on_machine_state_changed_mute_idle (GObject * machine, GParamSpec * arg,
    gpointer user_data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (user_data);
  BtMachineState state;

  g_object_get (machine, "state", &state, NULL);
  g_signal_handlers_block_matched (button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_mute_toggled,
      (gpointer) machine);
  gtk_toggle_button_set_active (button, (state == BT_MACHINE_STATE_MUTE));
  g_signal_handlers_unblock_matched (button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_mute_toggled,
      (gpointer) machine);
}

static void
on_machine_state_changed_mute (GObject * obj, GParamSpec * arg,
    gpointer user_data)
{
  bt_notify_idle_dispatch (obj, arg, user_data,
      on_machine_state_changed_mute_idle);
}

static void
on_machine_state_changed_solo_idle (GObject * machine, GParamSpec * arg,
    gpointer user_data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (user_data);
  BtMachineState state;

  g_object_get (machine, "state", &state, NULL);
  g_signal_handlers_block_matched (button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_solo_toggled,
      (gpointer) machine);
  gtk_toggle_button_set_active (button, (state == BT_MACHINE_STATE_SOLO));
  g_signal_handlers_unblock_matched (button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_solo_toggled,
      (gpointer) machine);
}

static void
on_machine_state_changed_solo (GObject * obj, GParamSpec * arg,
    gpointer user_data)
{
  bt_notify_idle_dispatch (obj, arg, user_data,
      on_machine_state_changed_solo_idle);
}

static void
on_machine_state_changed_bypass_idle (GObject * machine, GParamSpec * arg,
    gpointer user_data)
{
  GtkToggleButton *button = GTK_TOGGLE_BUTTON (user_data);
  BtMachineState state;

  g_object_get (machine, "state", &state, NULL);
  g_signal_handlers_block_matched (button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_bypass_toggled,
      (gpointer) machine);
  gtk_toggle_button_set_active (button, (state == BT_MACHINE_STATE_BYPASS));
  g_signal_handlers_unblock_matched (button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_bypass_toggled,
      (gpointer) machine);
}

static void
on_machine_state_changed_bypass (GObject * obj, GParamSpec * arg,
    gpointer user_data)
{
  bt_notify_idle_dispatch (obj, arg, user_data,
      on_machine_state_changed_bypass_idle);
}

static void
on_machine_state_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);

  bt_edit_application_set_song_unsaved (self->app);
}

static void
on_song_info_bars_changed (GObject* object, GParamSpec* spec, gpointer user_data) {
  BtSongInfo* info = BT_SONG_INFO (object);
  BtSequenceView* self = BT_SEQUENCE_VIEW (user_data);
  
  g_object_get (info, "bars", &self->bars, NULL);
  
  gtk_filter_changed (GTK_FILTER (self->row_filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void
disconnect_machine_signals (BtMachine * machine)
{
  // FIXME(ensonic): be careful when using the new sequence model
  // even though we can have multiple tracks per machine, we can disconnect them all, as we rebuild the treeview anyway
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0,
      0, NULL, on_machine_state_changed_mute, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0,
      0, NULL, on_machine_state_changed_solo, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0,
      0, NULL, on_machine_state_changed_bypass, NULL);
}

#if 0 /// GTK4 see what GTK already provides as builtins
// use key-press-event, as then we get key repeats
static gboolean
on_sequence_table_key_press_event (GtkEventControllerKey* controller, guint keyval,
    guint keycode, GdkModifierType state, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  gboolean res = FALSE;
  gulong row, track;

  GST_INFO
      ("sequence_table key : state 0x%x, keyval 0x%x, name %s",
      state, keyval, gdk_keyval_name (keyval));

  // determine timeline and timelinetrack from cursor pos
  if (sequence_view_get_current_pos (self, &row, &track)) {
    gulong length, tracks;
    gboolean change = FALSE;
    gulong modifier =
        (gulong) state & gtk_accelerator_get_default_mod_mask ();

    g_object_get (self->sequence, "length", &length, "tracks", &tracks,
        NULL);

    GST_DEBUG ("cursor pos : %lu/%lu, %lu/%lu", row, length, track, tracks);
    if (track > tracks)
      return FALSE;

    // look up pattern for key
    if (keyval == GDK_KEY_space || keyval == GDK_KEY_period) {
      // first column is label
      if ((track > 0) && (row < length)) {
        if ((res = change_pattern (self, NULL, row, track - 1))) {
          change = TRUE;
          res = TRUE;
        }
      }
    } else if (keyval == GDK_KEY_Return) {       /* GDK_KEY_KP_Enter */
      // first column is label
      if (track > 0) {
        switch_to_pattern_editor (self, row, track - 1);
        res = TRUE;
      }
    } else if (keyval == GDK_KEY_Menu) {
      GtkWidget* popup = gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu));
      gtk_widget_set_parent (popup, GTK_WIDGET (self));
      gtk_popover_popup (GTK_POPOVER (popup));
    } else if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down
        || keyval == GDK_KEY_Left || keyval == GDK_KEY_Right) {
      if (modifier == GDK_SHIFT_MASK) {
        gboolean select = FALSE;

        GST_INFO ("handling selection");

        // handle selection
        switch (keyval) {
          case GDK_KEY_Up:
            if ((self->cursor_row >= 0)) {
              self->cursor_row -= self->bars;
              LOG_SELECTION_AND_CURSOR ("up   ");
              if (self->selection_start_row == -1) {
                GST_INFO ("up   : new selection");
                self->selection_start_column = self->cursor_column;
                self->selection_end_column = self->cursor_column;
                self->selection_start_row = self->cursor_row;
                self->selection_end_row =
                    self->cursor_row + self->bars;
              } else {
                if (self->selection_start_row ==
                    (self->cursor_row + self->bars)) {
                  GST_INFO ("up   : expand selection");
                  self->selection_start_row -= self->bars;
                } else {
                  GST_INFO ("up   : shrink selection");
                  self->selection_end_row -= self->bars;
                }
              }
              LOG_SELECTION ("up   ");
              select = TRUE;
            }
            break;
          case GDK_KEY_Down:
            /* no check, we expand length */
            self->cursor_row += self->bars;
            LOG_SELECTION_AND_CURSOR ("down ");
            if (self->selection_end_row == -1) {
              GST_INFO ("down : new selection");
              self->selection_start_column = self->cursor_column;
              self->selection_end_column = self->cursor_column;
              self->selection_start_row =
                  self->cursor_row - self->bars;
              self->selection_end_row = self->cursor_row;
            } else {
              if (self->selection_end_row ==
                  (self->cursor_row - self->bars)) {
                GST_INFO ("down : expand selection");
                self->selection_end_row += self->bars;
              } else {
                GST_INFO ("down : shrink selection");
                self->selection_start_row += self->bars;
              }
            }
            LOG_SELECTION ("down ");
            select = TRUE;
            break;
          case GDK_KEY_Left:
            if (self->cursor_column >= 0) {
              self->cursor_column--;
              LOG_SELECTION_AND_CURSOR ("left ");
              if (self->selection_start_column == -1) {
                GST_INFO ("left : new selection");
                self->selection_start_column = self->cursor_column;
                self->selection_end_column =
                    self->cursor_column + 1;
                self->selection_start_row = self->cursor_row;
                self->selection_end_row = self->cursor_row;
              } else {
                if (self->selection_start_column ==
                    (self->cursor_column + 1)) {
                  GST_INFO ("left : expand selection");
                  self->selection_start_column--;
                } else {
                  GST_INFO ("left : shrink selection");
                  self->selection_end_column--;
                }
              }
              LOG_SELECTION ("left ");
              select = TRUE;
            }
            break;
          case GDK_KEY_Right:
            if (self->cursor_column < tracks) {
              self->cursor_column++;
              LOG_SELECTION_AND_CURSOR ("right");
              if (self->selection_end_column == -1) {
                GST_INFO ("right: new selection");
                self->selection_start_column =
                    self->cursor_column - 1;
                self->selection_end_column = self->cursor_column;
                self->selection_start_row = self->cursor_row;
                self->selection_end_row = self->cursor_row;
              } else {
                if (self->selection_end_column ==
                    (self->cursor_column - 1)) {
                  GST_INFO ("right: expand selection");
                  self->selection_end_column++;
                } else {
                  GST_INFO ("right: shrink selection");
                  self->selection_start_column++;
                }
              }
              LOG_SELECTION ("right");
              select = TRUE;
            }
            break;
        }
        if (select) {
          gtk_widget_queue_draw (GTK_WIDGET (self->sequence_table));
          res = TRUE;
        }
      } else {
        // remove selection
        if (self->selection_start_column != -1) {
          self->selection_start_column = self->selection_start_row =
              self->selection_end_column = self->selection_end_row =
              -1;
          gtk_widget_queue_draw (GTK_WIDGET (self->sequence_table));
        }
      }
    } else if (keyval == GDK_KEY_b) {
      if (modifier == GDK_CONTROL_MASK) {
        GST_INFO ("ctrl-b pressed, row %lu", row);
        sequence_set_loop_start (self, (glong) row);
        res = TRUE;
      }
    } else if (keyval == GDK_KEY_e) {
      if (modifier == GDK_CONTROL_MASK) {
        GST_INFO ("ctrl-e pressed, row %lu", row);
        sequence_set_loop_end (self, (glong) row);
        res = TRUE;
      }
    } else if (keyval == GDK_KEY_Insert) {
      if (modifier == 0) {
        GString *old_data = g_string_new (NULL), *new_data =
            g_string_new (NULL);
        glong col = (glong) track - 1;
        gulong sequence_length;

        GST_INFO ("insert pressed, row %lu, track %ld", row, col);
        g_object_get (self->sequence, "length", &sequence_length, NULL);
        sequence_range_copy (self, track, track, row, sequence_length - 1,
            old_data);
        bt_sequence_insert_rows (self->sequence, row, col,
            self->bars);
        sequence_range_copy (self, track, track, row, sequence_length - 1,
            new_data);
        sequence_range_log_undo_redo (self, track, track, row,
            sequence_length - 1, old_data->str, new_data->str);
        g_string_free (old_data, TRUE);
        g_string_free (new_data, TRUE);
        res = TRUE;
      } else if (modifier == GDK_SHIFT_MASK) {
        GString *old_data = g_string_new (NULL), *new_data =
            g_string_new (NULL);
        gulong sequence_length, number_of_tracks;
        gchar *undo_str, *redo_str;

        GST_INFO ("shift-insert pressed, row %lu", row);
        g_object_get (self->sequence, "length", &sequence_length,
            "tracks", &number_of_tracks, NULL);
        sequence_length += self->bars;

        sequence_range_copy (self, 0, number_of_tracks, row,
            sequence_length - 1, old_data);
        bt_sequence_insert_full_rows (self->sequence, row,
            self->bars);
        sequence_range_copy (self, 0, number_of_tracks, row,
            sequence_length - 1, new_data);

        bt_change_log_start_group (self->change_log);
        sequence_range_log_undo_redo (self, 0, number_of_tracks, row,
            sequence_length - 1, old_data->str, new_data->str);
        undo_str =
            g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
            sequence_length - self->bars);
        redo_str =
            g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
            sequence_length);
        bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
            undo_str, redo_str);
        bt_change_log_end_group (self->change_log);

        g_string_free (old_data, TRUE);
        g_string_free (new_data, TRUE);
        self->sequence_length += self->bars;
        // udpate the view
        sequence_calculate_visible_lines (self);
        sequence_update_model_length (self);
        res = TRUE;
      }
    } else if (keyval == GDK_KEY_Delete) {
      if (modifier == 0) {
        GString *old_data = g_string_new (NULL), *new_data =
            g_string_new (NULL);
        glong col = (glong) track - 1;
        gulong sequence_length;

        GST_INFO ("delete pressed, row %lu, track %ld", row, col);
        g_object_get (self->sequence, "length", &sequence_length, NULL);
        sequence_range_copy (self, track, track, row, sequence_length - 1,
            old_data);
        bt_sequence_delete_rows (self->sequence, row, col,
            self->bars);
        sequence_range_copy (self, track, track, row, sequence_length - 1,
            new_data);
        sequence_range_log_undo_redo (self, track, track, row,
            sequence_length - 1, old_data->str, new_data->str);
        g_string_free (old_data, TRUE);
        g_string_free (new_data, TRUE);
        res = TRUE;
      } else if (modifier == GDK_SHIFT_MASK) {
        GString *old_data = g_string_new (NULL), *new_data =
            g_string_new (NULL);
        gulong sequence_length, number_of_tracks;
        gchar *undo_str, *redo_str;

        GST_INFO ("shift-delete pressed, row %lu", row);
        g_object_get (self->sequence, "length", &sequence_length,
            "tracks", &number_of_tracks, NULL);

        sequence_range_copy (self, 0, number_of_tracks, row,
            sequence_length - 1, old_data);
        bt_sequence_delete_full_rows (self->sequence, row,
            self->bars);
        sequence_range_copy (self, 0, number_of_tracks, row,
            sequence_length - 1, new_data);

        bt_change_log_start_group (self->change_log);
        undo_str =
            g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
            sequence_length);
        redo_str =
            g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
            sequence_length - self->bars);
        bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
            undo_str, redo_str);
        sequence_range_log_undo_redo (self, 0, number_of_tracks, row,
            sequence_length - 1, old_data->str, new_data->str);
        bt_change_log_end_group (self->change_log);

        g_string_free (old_data, TRUE);
        g_string_free (new_data, TRUE);
        self->sequence_length -= self->bars;
        // update the view
        sequence_calculate_visible_lines (self);
        sequence_update_model_length (self);
        res = TRUE;
      }
    }

    if ((!res) && (keyval <= 'z') && ((modifier == 0)
            || (modifier == GDK_SHIFT_MASK))) {
      // first column is label
      if (track > 0) {
        gchar key = (gchar) (keyval & 0xff);
        BtCmdPattern *new_pattern;

        BtPatternListModel *store = BT_PATTERN_LIST_MODEL (
            gtk_single_selection_get_model (
               GTK_SINGLE_SELECTION (gtk_list_view_get_model (self->pattern_list))));
        
        if ((new_pattern = bt_pattern_list_model_get_pattern_by_key (store, key))) {
          gulong new_length = 0;

          bt_change_log_start_group (self->change_log);

          if (row >= length) {
            new_length = row + self->bars;
            g_object_set (self->sequence, "length", new_length, NULL);
            sequence_calculate_visible_lines (self);
            sequence_update_model_length (self);
          }

          if ((res = change_pattern (self, new_pattern, row, track - 1))) {
            change = TRUE;
            res = TRUE;
          }

          if (new_length) {
            gchar *undo_str, *redo_str;

            undo_str =
                g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
                length);
            redo_str =
                g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
                new_length);

            bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
                undo_str, redo_str);
          }

          bt_change_log_end_group (self->change_log);

          g_object_unref (new_pattern);
        } else {
          GST_WARNING_OBJECT (self->machine,
              "keyval '%c' not used by machine", key);
        }
      }
    }
    // update cursor pos in tree-view model
    if (change) {
      GtkMultiSelection *selection = GTK_MULTI_SELECTION (gtk_column_view_get_model (self->sequence_table));
      GListModel *model = gtk_multi_selection_get_model (selection);
      const guint extent = g_list_model_get_n_items (model);
      
      // move cursor down & set cell focus
      self->cursor_row = MIN (self->cursor_row + self->bars, extent - 1);

      gtk_selection_model_select_range (GTK_SELECTION_MODEL (selection), self->cursor_row, 1, TRUE);
    }
    //else if(!select) GST_INFO("  nothing assigned to this key");
  }
  return res;
}
#endif

static void
labelwidget_table_on_setup (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* label = gtk_label_new (NULL);

  gtk_list_item_set_child (GTK_LIST_ITEM (item), label);
}

static void
on_sequence_pos_table_button_press_event (GtkGestureClick* click, gint n_press, gdouble x, gdouble y,
    gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);
  GtkWidget *label = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (click));
  BtListItemLong *item = BT_LIST_ITEM_LONG (g_object_get_data (G_OBJECT (label), "bt-model-list-item"));
  
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (click));
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (click));

  if (button == GDK_BUTTON_PRIMARY) {
    gulong modifier =
        (gulong) state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK);

    switch (modifier) {
      case 0:
        sequence_set_play_pos (self, item->value);
        break;
      case GDK_CONTROL_MASK:
        sequence_set_loop_start (self, item->value);
        break;
      case GDK_CONTROL_MASK | GDK_SHIFT_MASK:
        sequence_set_loop_end (self, item->value);
        break;
    }
  } else if (button == GDK_BUTTON_SECONDARY) {
    GtkWidget* popup = gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu));
    gtk_widget_set_parent (popup, GTK_WIDGET (self));
    gtk_popover_popup (GTK_POPOVER (popup));
  }
}

static void sequence_pos_table_on_setup(GtkSignalListItemFactory *factory, GObject *object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* label = gtk_label_new (NULL);

  gtk_list_item_set_child (GTK_LIST_ITEM (item), label);

  GtkGesture* click = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (label), GTK_EVENT_CONTROLLER (click));
  
  g_signal_connect_object (click, "button-press-event",
      G_CALLBACK (on_sequence_pos_table_button_press_event), user_data, 0);
}

static void
sequence_pos_table_on_bind (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtListItemLong* item = BT_LIST_ITEM_LONG (gtk_list_item_get_item (listitem));
  BtSequenceView* self = BT_SEQUENCE_VIEW (user_data);

  GtkLabel* label = GTK_LABEL (gtk_list_item_get_child (listitem));
  gchar* text = format_position (self, item->value);
  gtk_label_set_text (label, text);
  g_free (text);

  g_object_ref (item);
  g_object_set_data_full (G_OBJECT (listitem), "bt-model-list-item", (gpointer) item, g_object_unref);
}

static void
machine_col_on_bind (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem *listitem = GTK_LIST_ITEM (object);
  BtListItemLong *item = BT_LIST_ITEM_LONG (gtk_list_item_get_item (listitem));
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);
  guint track = *(guint*) g_object_get_data (G_OBJECT (factory), "bt-track");

  GtkLabel *label = GTK_LABEL (gtk_list_item_get_child (listitem));
  BtCmdPattern *pattern = bt_sequence_get_pattern (self->sequence, item->value, track);
  gtk_label_set_text (label, bt_cmd_pattern_get_name (pattern));
  g_object_unref (pattern);

  g_object_ref (item);
  g_object_set_data_full (G_OBJECT (listitem), "bt-model-list-item", (gpointer) item, g_object_unref);
}

//-- tree filter func

static gboolean
step_visible_filter (gpointer item, gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);
  gulong pos;

  // determine row number and hide or show accordingly.
  // i.e. if step is 2, show every second row.
  pos = bt_sequence_grid_model_item_get_pos (self->model, BT_LIST_ITEM_LONG (item));

  if (IS_SEQUENCE_POS_VISIBLE (pos, self->bars))
    return TRUE;
  else
    return FALSE;
}

static gboolean
label_visible_filter (gpointer item, gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);

  // show only columns with labels
  return bt_sequence_grid_model_item_has_label (self->model, BT_LIST_ITEM_LONG (item));
}

static void
on_track_added (BtSequence * sequence, BtMachine * machine, gulong track,
    gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);

  GtkColumnViewColumn *col = sequence_table_make_machine_column (self, machine, track);
  gtk_column_view_insert_column(self->sequence_table, track, col);
  g_object_unref(col);
}

static void
on_track_removed (BtSequence * sequence, BtMachine * machine, gulong track,
    gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (user_data);

  GST_INFO ("machine removed %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));


  GListModel* columns = gtk_column_view_get_columns (self->sequence_table);
  gtk_column_view_remove_column (self->sequence_table, g_list_model_get_item (columns, track));
  g_object_unref (columns);

  GST_INFO ("machine removed %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

//-- helper methods

static gchar *
format_position (const BtSequenceView * view, gulong pos)
{
  static gchar pos_str[20];

  switch (view->pos_format) {
    case BT_SEQUENCE_VIEW_POS_FORMAT_TICKS:
      g_snprintf (pos_str, 5, "%lu", pos);
      break;
    case BT_SEQUENCE_VIEW_POS_FORMAT_TIME:{
      gulong msec, sec, min;
      bt_song_info_tick_to_m_s_ms (view->song_info, pos, &min, &sec,
          &msec);
      g_snprintf (pos_str, sizeof(pos_str), "%02lu:%02lu.%03lu", min, sec, msec);
    }
      break;
    case BT_SEQUENCE_VIEW_POS_FORMAT_BEATS:{
      gulong beat, tick, ticks_per_beat;
      gint tick_chars;

      g_object_get (view->song_info, "tpb", &ticks_per_beat, NULL);
      
      beat = pos / ticks_per_beat;
      tick = pos - (beat * ticks_per_beat);
      if (ticks_per_beat >= 100)
        tick_chars = 3;
      else if (ticks_per_beat >= 10)
        tick_chars = 2;
      else
        tick_chars = 1;
      g_snprintf (pos_str, sizeof(pos_str), "%lu.%0*lu", beat, tick_chars, tick);
    }
      break;
    default:
      *pos_str = '\0';
      GST_WARNING ("unimplemented time format %d", view->pos_format);
  }
  return pos_str;
}

static void
sequence_view_refresh_model (BtSequenceView * self)
{
  GST_INFO ("refresh sequence table");

  g_object_set (self->model, "pos-format", self->pos_format, NULL);

  // create a filtered model to realize step filtering
  GtkFilterListModel* filtermodel;
  self->row_filter = gtk_custom_filter_new (step_visible_filter, (gpointer) self, NULL);
  
  filtermodel = gtk_filter_list_model_new (G_LIST_MODEL (self->model), GTK_FILTER (self->row_filter));
  GtkMultiSelection* selection = gtk_multi_selection_new (G_LIST_MODEL (filtermodel));
  g_clear_object (&selection);
  
  // active models
  gtk_column_view_set_model (self->sequence_table, GTK_SELECTION_MODEL (selection));
  GtkNoSelection* no_selection = gtk_no_selection_new (G_LIST_MODEL (filtermodel));
  gtk_column_view_set_model (self->sequence_pos_table, GTK_SELECTION_MODEL (no_selection));
  g_clear_object (&filtermodel);
  g_clear_object (&no_selection);

  // create a filtered store for the labels menu
  GtkCustomFilter *label_filter = gtk_custom_filter_new (label_visible_filter, (gpointer) self, NULL);
  filtermodel = gtk_filter_list_model_new (G_LIST_MODEL (self->model), GTK_FILTER (label_filter));

  g_clear_object (&label_filter);
  
  // active models
  gtk_drop_down_set_model (self->label_menu, G_LIST_MODEL (filtermodel));
  g_object_unref (filtermodel);
}

static void
sequence_set_play_pos (BtSequenceView * self, glong row)
{
  BtSong *song;
  glong play_pos;

  g_object_get (self->app, "song", &song, NULL);
  g_object_get (song, "play-pos", &play_pos, NULL);
  if (row == -1) {
    g_object_get (self->sequence, "length", &row, NULL);
  }
  if (play_pos != row) {
    g_object_set (song, "play-pos", row, NULL);
  }
  g_object_unref (song);
}

static void
sequence_set_loop_start (BtSequenceView * self, glong row)
{
  gulong sequence_length;
  gdouble pos;
  glong loop_start, loop_end;
  glong old_loop_start, old_loop_end;
  gchar *undo_str, *redo_str;

  g_object_get (self->sequence, "length", &sequence_length, "loop-start",
      &old_loop_start, "loop-end", &loop_end, NULL);
  if (row == -1)
    row = sequence_length;
  // set and read back, as sequence might clamp the value
  g_object_set (self->sequence, "loop-start", row, NULL);
  g_object_get (self->sequence, "loop-start", &loop_start, NULL);

  if (loop_start != old_loop_start) {
    bt_change_log_start_group (self->change_log);

    undo_str =
        g_strdup_printf ("set_sequence_property \"loop-start\",\"%ld\"",
        old_loop_start);
    redo_str =
        g_strdup_printf ("set_sequence_property \"loop-start\",\"%ld\"",
        loop_start);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);

    pos = (gdouble) loop_start / (gdouble) sequence_length;
    g_object_set (self->sequence_table, "loop-start", pos, NULL);
    g_object_set (self->sequence_pos_table, "loop-start", pos, NULL);

    GST_INFO ("adjusted loop-end = %ld -> %ld", old_loop_start, loop_start);

    g_object_get (self->sequence, "loop-end", &old_loop_end, NULL);
    if ((old_loop_end != -1) && (old_loop_end <= old_loop_start)) {
      loop_end = loop_start + self->bars;
      g_object_set (self->sequence, "loop-end", loop_end, NULL);

      undo_str =
          g_strdup_printf ("set_sequence_property \"loop-end\",\"%ld\"",
          old_loop_end);
      redo_str =
          g_strdup_printf ("set_sequence_property \"loop-end\",\"%ld\"",
          loop_end);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);

      GST_INFO ("adjusted loop-end = %ld -> %ld", old_loop_end, loop_end);

      pos = (gdouble) loop_end / (gdouble) sequence_length;
      g_object_set (self->sequence_table, "loop-end", pos, NULL);
      g_object_set (self->sequence_pos_table, "loop-end", pos, NULL);
    }

    bt_change_log_end_group (self->change_log);
  }
}

static void
sequence_set_loop_end (BtSequenceView * self, glong row)
{
  gulong sequence_length;
  gdouble pos;
  glong loop_start, loop_end;
  glong old_loop_start, old_loop_end;
  gchar *undo_str, *redo_str;

  g_object_get (self->sequence, "length", &sequence_length, "loop-start",
      &loop_start, "loop-end", &old_loop_end, NULL);
  if (row == -1)
    row = sequence_length;
  // pos is beyond length or is on loop-end already -> adjust length
  if ((row > sequence_length) || (row == old_loop_end)) {
    GST_INFO ("adjusted length = %ld -> %ld", sequence_length, row);

    bt_change_log_start_group (self->change_log);

    undo_str =
        g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
        sequence_length);
    redo_str =
        g_strdup_printf ("set_sequence_property \"length\",\"%ld\"", row);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);

    // we shorten the song, backup data
    if (row < sequence_length) {
      /*GString *old_data = g_string_new (NULL);
      gulong number_of_tracks;

      g_object_get (self->sequence, "tracks", &number_of_tracks, NULL);
      sequence_range_copy (self, 0, number_of_tracks, row,
          sequence_length - 1, old_data);
      sequence_range_log_undo_redo (self, 0, number_of_tracks, row,
          sequence_length - 1, old_data->str, g_strdup (old_data->str));
          g_string_free (old_data, TRUE);*/
    }
    bt_change_log_end_group (self->change_log);

    sequence_length = row;
    g_object_set (self->sequence, "length", sequence_length, NULL);
    loop_end = old_loop_end;
  } else {
    // set and read back, as sequence might clamp the value
    g_object_set (self->sequence, "loop-end", row, NULL);
    g_object_get (self->sequence, "loop-end", &loop_end, NULL);

    if (loop_end != old_loop_end) {
      bt_change_log_start_group (self->change_log);

      GST_INFO ("adjusted loop-end = %ld -> %ld", old_loop_end, loop_end);

      undo_str =
          g_strdup_printf ("set_sequence_property \"loop-end\",\"%ld\"",
          old_loop_end);
      redo_str =
          g_strdup_printf ("set_sequence_property \"loop-end\",\"%ld\"",
          loop_end);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);

      g_object_get (self->sequence, "loop-start", &old_loop_start, NULL);
      if ((old_loop_start != -1) && (old_loop_start >= loop_end)) {
        loop_start = loop_end - self->bars;
        if (loop_start < 0)
          loop_start = 0;
        g_object_set (self->sequence, "loop-start", loop_start, NULL);

        undo_str =
            g_strdup_printf ("set_sequence_property \"loop-start\",\"%ld\"",
            old_loop_start);
        redo_str =
            g_strdup_printf ("set_sequence_property \"loop-start\",\"%ld\"",
            loop_start);
        bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
            undo_str, redo_str);

        GST_INFO ("and adjusted loop-start = %ld", loop_start);
      }

      bt_change_log_end_group (self->change_log);
    }
  }
  pos = (loop_end > -1) ? (gdouble) loop_end / (gdouble) sequence_length : 1.0;
  g_object_set (self->sequence_table, "loop-end", pos, NULL);
  g_object_set (self->sequence_pos_table, "loop-end", pos, NULL);

  pos =
      (loop_start >
      -1) ? (gdouble) loop_start / (gdouble) sequence_length : 0.0;
  g_object_set (self->sequence_table, "loop-start", pos, NULL);
  g_object_set (self->sequence_pos_table, "loop-start", pos, NULL);
}

// Returns: (transfer full)
static GtkColumnViewColumn *
sequence_table_make_machine_column (BtSequenceView* self, BtMachine *machine, guint track_idx)
{
  GtkListItemFactory *factory;

  factory = gtk_signal_list_item_factory_new();
  guint *data = g_malloc(sizeof(guint));
  *data = track_idx;
  g_object_set_data_full(G_OBJECT(factory), "bt-track", data, g_free);
  g_signal_connect_object(factory, "setup",
                          G_CALLBACK(labelwidget_table_on_setup),
                          (gpointer)self, 0);
  g_signal_connect_object(factory, "bind", G_CALLBACK(machine_col_on_bind),
                          (gpointer)self, 0);

  GtkColumnViewColumn *col =
      gtk_column_view_column_new(bt_machine_get_id(machine), factory);

  g_object_bind_property(machine, "id", col, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property(machine, "pretty-name", col, "tooltip-text",
                         G_BINDING_SYNC_CREATE);

  return col;
}

static void
sequence_table_refresh_columns (BtSequenceView * self,
    const BtSong * song)
{
  gulong j, track_ct;
  BtMachine *machine;
#if 0 /// GTK4
  GtkWidget *header;
  gint col_index;
  GtkTreeViewColumn *tree_col;
#endif
  GHashTable *machine_usage;

  // build dynamic sequence view
  GST_INFO ("refresh sequence view");

  g_object_get (self->sequence, "tracks", &track_ct, NULL);

  // TODO(ensonic): we'd like to update this instead of re-creating things
  GListModel* cols = gtk_column_view_get_columns (self->sequence_table);
  for (guint i = 0; ; ++i) {
    GtkColumnViewColumn *col = GTK_COLUMN_VIEW_COLUMN (g_list_model_get_object (cols, i));
    if (!col)
      break;
    
    gtk_column_view_remove_column (self->sequence_table, col);
  }
  g_clear_object (&cols);

  // add column for each machine
  machine_usage = g_hash_table_new (NULL, NULL);
  for (j = 0; j < track_ct; j++) {
    machine = bt_sequence_get_machine (self->sequence, j);
    GST_INFO ("refresh track %lu for machine %" G_OBJECT_REF_COUNT_FMT, j,
        G_OBJECT_LOG_REF_COUNT (machine));

    GtkColumnViewColumn *col = sequence_table_make_machine_column (self, machine, j);
    gtk_column_view_append_column (self->sequence_table, col);
    g_object_unref (col);
    
    // setup column header
    if (machine) {
#if 0 /// GTK4
      GtkWidget *label, *button, *box;
      GtkVUMeter *vumeter;
#endif
      GstElement *level;
      gchar *level_name = "output-post-level";

      GST_DEBUG ("  %3lu build column header", j);

      // enable level meters
      if (!BT_IS_SINK_MACHINE (machine)) {
        if (!bt_machine_enable_output_post_level (machine)) {
          GST_INFO ("enabling output level for machine failed");
        }
      } else {
        // its the sink, which already has it enabled
        level_name = "input-post-level";
      }
      g_object_get (machine, level_name, &level, NULL);

#if 0 /// GTK4
      // create header widget
      header = gtk_box_new (GTK_ORIENTATION_VERTICAL, HEADER_SPACING);

      label = gtk_entry_new ();
      gtk_widget_set_name (label, "BtSequenceHeaderLabel");
      // we need to set width-chars so that the natural size is calculated
      // instead of using the hard-coded 150 pixels, that still is not good
      // with gtk > 3.12 we also need to set "max-width-chars"
      // Despite the docs, even using "elipsize" does not affect the min alloc
      gint char_pixels = get_avg_pixels_per_char (label);
      gint num_chars = SEQUENCE_CELL_WIDTH / (char_pixels + 1);
      GST_DEBUG ("setting width to %d chars", num_chars);
      g_object_set (label, "has-frame", FALSE, "inner-border", 0, "width-chars",
          num_chars, NULL);
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (label),
              "max-width-chars")) {
        g_object_set (label, "max-width-chars", num_chars, NULL);
      }
      g_object_set_qdata ((GObject *) label, machine_for_track, machine);
      gtk_box_pack_start (GTK_BOX (header), label, TRUE, TRUE, 0);
      g_signal_connect (label, "activate", G_CALLBACK (on_machine_id_renamed),
          (gpointer) self);
#endif

#if 0 /// GTK4 use the ColumnView's header factory to create these widgets
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (header), box, TRUE, TRUE, 0);

      /* only do this for first track of a machine
       * - multiple level-meter views for same machine don't work
       * - MSB buttons would need to be synced
       */
      if (!g_hash_table_lookup (machine_usage, machine)) {
        BtMachineState state;

        g_object_get (machine, "state", &state, NULL);

        g_hash_table_insert (machine_usage, machine, machine);
        // add M/S/B butons and connect signal handlers
        button =
            make_mini_button ("M", "mute", (state == BT_MACHINE_STATE_MUTE));
        gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
        g_signal_connect_object (button, "toggled", G_CALLBACK (on_mute_toggled),
            (gpointer) machine, 0);
        g_signal_connect_object (button, "toggled",
            G_CALLBACK (on_machine_state_toggled), (gpointer) self, 0);
        g_signal_connect_object (machine, "notify::state",
            G_CALLBACK (on_machine_state_changed_mute), (gpointer) button, 0);

        if (BT_IS_SOURCE_MACHINE (machine)) {
          button =
              make_mini_button ("S", "solo", (state == BT_MACHINE_STATE_SOLO));
          gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
          g_signal_connect_object (button, "toggled", G_CALLBACK (on_solo_toggled),
              (gpointer) machine, 0);
          g_signal_connect_object (button, "toggled",
              G_CALLBACK (on_machine_state_toggled), (gpointer) self, 0);
          g_signal_connect_object (machine, "notify::state",
              G_CALLBACK (on_machine_state_changed_solo), (gpointer) button, 0);
        }

        if (BT_IS_PROCESSOR_MACHINE (machine)) {
          button =
              make_mini_button ("B", "bypass",
              (state == BT_MACHINE_STATE_BYPASS));
          gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
          g_signal_connect_object (button, "toggled", G_CALLBACK (on_bypass_toggled),
              (gpointer) machine, 0);
          g_signal_connect_object (button, "toggled",
              G_CALLBACK (on_machine_state_toggled), (gpointer) self, 0);
          g_signal_connect_object (machine, "notify::state",
              G_CALLBACK (on_machine_state_changed_bypass), (gpointer) button, 0);
        }
        vumeter = GTK_VUMETER (gtk_vumeter_new (GTK_ORIENTATION_HORIZONTAL));
        gtk_vumeter_set_min_max (vumeter, LOW_VUMETER_VAL, 0);
        // no falloff in widget, we have falloff in GstLevel
        //gtk_vumeter_set_peaks_falloff(vumeter, GTK_VUMETER_PEAKS_FALLOFF_MEDIUM);
        gtk_vumeter_set_scale (vumeter, GTK_VUMETER_SCALE_LINEAR);
        reset_level_meter (level, vumeter, NULL);
        gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (vumeter), TRUE, TRUE, 0);

        // add level meters to hashtable
        if (level) {
          g_hash_table_insert (self->level_to_vumeter, level, vumeter);
        }
      } else {
        // eat space
        gtk_box_pack_start (GTK_BOX (box), gtk_label_new (""), TRUE, TRUE, 0);
      }
#endif
    } else {
      // a missing machine
#if 0 /// GTK4
      header = gtk_label_new ("???");
#endif
      GST_WARNING ("can't get machine for column %lu", j);
    }
    g_object_try_unref (machine);
  }
  g_hash_table_destroy (machine_usage);

  GST_INFO ("finish sequence table");

#if 0 /// GTK4 still needed?
  // add a final column that eats remaining space
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "mode", GTK_CELL_RENDERER_MODE_INERT, NULL);
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);

  header = gtk_label_new ("");
  gtk_label_set_single_line_mode (GTK_LABEL (header), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (header), FALSE);
  gtk_widget_set_hexpand (header, TRUE);

  gtk_widget_show (header);
  gtk_box_pack_start (GTK_BOX (self->sequence_table_header), header, TRUE,
      TRUE, 0);
  if ((tree_col =
          gtk_tree_view_column_new_with_attributes ( /*title= */ NULL, renderer,
              NULL))) {
    g_object_set (tree_col, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
    col_index =
        gtk_tree_view_append_column (self->sequence_table, tree_col);
    GST_DEBUG ("    number of columns : %d", col_index);
  } else
    GST_WARNING ("can't create treeview column");
#endif
}

/*
 * sequence_calculate_visible_lines:
 * @self: the sequence subpage
 *
 * Recalculate the visible lines after length or bar-stepping changes. Also
 * updated the loop marker positions accordingly.
 */
static gboolean
sequence_view_get_cursor_pos (GtkColumnView * column_view, gulong * col, gulong * row)
{
  GtkSelectionModel* selection = gtk_column_view_get_model (column_view);
  GtkBitset* bitset = gtk_selection_model_get_selection (selection);
  guint max = gtk_bitset_get_maximum (bitset);
  g_object_unref (bitset);
  *col = 0;
  *row = max;
  return TRUE;
}


static void
on_pos_menu_changed (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (object);
  GtkDropDown *dropdown = GTK_DROP_DOWN (user_data);

  self->pos_format = gtk_drop_down_get_selected (dropdown);
  g_object_set (self->model, "pos-format", self->pos_format, NULL);
  bt_edit_application_set_song_unsaved (self->app);
}

//-- constructor methods

/**
 * bt_sequence_view_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSequenceView *
bt_sequence_view_new (BtSong* song)
{
  BtSequenceView *self;

  self = BT_SEQUENCE_VIEW (g_object_new (BT_TYPE_SEQUENCE_VIEW, NULL));

  bt_sequence_view_set_song (self, song);
  
  return self;
}

//-- methods

/**
 * Returns: (transfer none): The model underlying the view for the currently attached sequence.
 */
BtSequenceGridModel *
bt_sequence_view_get_model (BtSequenceView * self)
{
  return self->model;
}

void
bt_sequence_view_set_song (BtSequenceView* self, BtSong *song)
{
  self->model = bt_sequence_grid_model_new (self->sequence, self->song_info, self->bars);

  if (self->sequence) {
    g_signal_handlers_disconnect_by_func (self->song_info, G_CALLBACK (on_song_info_bars_changed), self);
    g_signal_handlers_disconnect_by_func (self->sequence, G_CALLBACK (on_track_added), self);
    g_signal_handlers_disconnect_by_func (self->sequence, G_CALLBACK (on_track_removed), self);
    g_object_unref (self->song_info);
  }

  g_object_get (song, "setup", &self->setup, "song-info", &self->song_info,
      "sequence", &self->sequence, NULL);

  sequence_view_refresh_model (self);

  // subscribe to changes in the rythm
  g_object_get (self->song_info, "bars", &self->bars, NULL);
  
  g_signal_connect_object (self->song_info, "notify::bars",
      G_CALLBACK (on_song_info_bars_changed), (gpointer) self, 0);

  // Remember that machines can have multiple tracks.
  g_signal_connect_object (self->sequence, "track-added",
      G_CALLBACK (on_track_added), (gpointer) self, 0);
  g_signal_connect_object (self->sequence, "track-removed",
      G_CALLBACK (on_track_removed), (gpointer) self, 0);
}

/*
 * sequence_view_get_current_pos:
 * @self: the sequence subpage
 * @time: pointer for time result
 * @track: pointer for track result
 *
 * Get the currently cursor position in the sequence table.
 * The result will be place in the respective pointers.
 * If one is %NULL, no value is returned for it.
 *
 * Returns: %TRUE if the cursor is at a valid track position
 */
gboolean
bt_sequence_view_get_current_pos (BtSequenceView * self, gulong * time,
    gulong * track)
{
  gboolean res = FALSE;

  GST_INFO ("get active sequence cell");

  res = sequence_view_get_cursor_pos (self->sequence_table, track, time);
  return res;
}

//-- wrapper

//-- class internals

static void
bt_sequence_view_realize (GtkWidget * widget)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (widget);
  GtkStyleContext *style;

  // first let the parent realize itslf
  GTK_WIDGET_CLASS (bt_sequence_view_parent_class)->realize (widget);

  // cache theme colors
  style = gtk_widget_get_style_context (widget);
  if (!gtk_style_context_lookup_color (style, "playline_color",
          &self->play_line_color)) {
    GST_WARNING ("Can't find 'playline_color' in css.");
  }
  if (!gtk_style_context_lookup_color (style, "endline_color",
          &self->end_line_color)) {
    GST_WARNING ("Can't find 'endline_color' in css.");
  }
  if (!gtk_style_context_lookup_color (style, "loopline_color",
          &self->loop_line_color)) {
    GST_WARNING ("Can't find 'loopline_color' in css.");
  }
}

static void
bt_sequence_view_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (widget);
  gdouble w, h, y;
  gdouble loop_pos_dash_list[] = { 4.0 };

  // let all child widgets draw first
  GtkWidget *child = gtk_widget_get_first_child (widget);
  while (child) {
    gtk_widget_snapshot_child (widget, child, snapshot);
    child = gtk_widget_get_next_sibling (child);
  }

  if (G_UNLIKELY (!self->row_height)) {
    // determine row height
    /// GTK4 revisit this. Style/CSS?
    GtkWidget *child = gtk_widget_get_first_child(widget);
    while (child) {
      if (GTK_IS_LABEL (child)) {
        self->row_height = gtk_widget_get_height(child);
        break;
      }        
    }
  }

  w = (gdouble) gtk_widget_get_width (widget);
  h = (gdouble) (self->visible_rows * self->row_height);

  graphene_rect_t rect = GRAPHENE_RECT_INIT(0, 0, w, h);
  cairo_t *c = gtk_snapshot_append_cairo (snapshot, &rect);
  
  // draw play-pos
  y = 0.5 + floor ((self->play_pos * h));
  if ((y >= 0) && (y < h)) {
    gdk_cairo_set_source_rgba (c, &self->play_line_color);
    cairo_set_line_width (c, 2.0);
    cairo_move_to (c, 0.0, y);
    cairo_line_to (c, w, y);
    cairo_stroke (c);
  }
  // draw song-end
  y = h - 1;
  if ((y >= 0) && (y < h)) {
    gdk_cairo_set_source_rgba (c, &self->end_line_color);
    cairo_set_line_width (c, 2.0);
    cairo_move_to (c, 0.0, y);
    cairo_line_to (c, w, y);
    cairo_stroke (c);
  }
  // draw loop-start/-end
  gdk_cairo_set_source_rgba (c, &self->loop_line_color);
  cairo_set_dash (c, loop_pos_dash_list, 1, 0.0);
  // draw these always from 0 as they are dashed and we can't adjust the start of the dash pattern
  y = (self->loop_start * h);
  if ((y >= 0) && (y < h)) {
    cairo_move_to (c, 0.0, y);
    cairo_line_to (c, w, y);
    cairo_stroke (c);
  }
  y = (self->loop_end * h) - 1;
  if ((y >= 0) && (y < h)) {
    cairo_move_to (c, 0.0, y);
    cairo_line_to (c, w, y);
    cairo_stroke (c);
  }

  cairo_destroy (c);
}

static void
bt_sequence_view_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (object);
  return_if_disposed_self ();

  switch (property_id) {
    case SEQUENCE_VIEW_CURRENT_MACHINE: {
      BtMachine *machine = bt_sequence_get_machine (self->sequence, self->cursor_column);
      g_value_set_object (value, machine);
      g_object_unref (machine);
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sequence_view_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (object);

  return_if_disposed_self ();
  switch (property_id) {
    case SEQUENCE_VIEW_PLAY_POSITION:{
      self->play_pos = g_value_get_double (value);
      if (gtk_widget_get_realized (GTK_WIDGET (self))) {
        gtk_widget_queue_draw (GTK_WIDGET (self));
      }
      break;
    }
    case SEQUENCE_VIEW_LOOP_START:{
      self->loop_start = g_value_get_double (value);
      if (gtk_widget_get_realized (GTK_WIDGET (self))) {
        gtk_widget_queue_draw (GTK_WIDGET (self));
      }
      break;
    }
    case SEQUENCE_VIEW_LOOP_END:{
      self->loop_end = g_value_get_double (value);
      if (gtk_widget_get_realized (GTK_WIDGET (self))) {
        gtk_widget_queue_draw (GTK_WIDGET (self));
      }
      break;
    }
    case SEQUENCE_VIEW_BARS:{
      self->bars = g_value_get_ulong (value);
      if (gtk_widget_get_realized (GTK_WIDGET (self))) {
        gtk_widget_queue_draw (GTK_WIDGET (self));
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sequence_view_dispose (GObject * object)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW (object);
  return_if_disposed_self ();

  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->app);

  g_object_unref (self->row_filter);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_SEQUENCE_VIEW);
  
  G_OBJECT_CLASS (bt_sequence_view_parent_class)->dispose (object);
}

static void
bt_sequence_view_init (BtSequenceView * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_sequence_view_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();

  // set a minimum size, otherwise the window can't be shrinked
  // (we need this because of GTK_POLICY_NEVER)
  /// GTK4: put this in UI file
  gtk_widget_set_size_request (GTK_WIDGET (self->sequence_pos_table),
      POSITION_CELL_WIDTH, 40);
  gtk_widget_set_vexpand (GTK_WIDGET (self->sequence_pos_table), TRUE);

  GtkEventController* motion;
  motion = gtk_event_controller_motion_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->sequence_pos_table), motion);

#if 0 /// GTK4  
  g_signal_connect (motion, "motion",
      G_CALLBACK (on_sequence_table_motion_notify_event), (gpointer) self);
#endif

  g_signal_connect (self->label_menu, "notify::selected", G_CALLBACK (on_label_menu_changed), (gpointer) self);

#if 0 /// GTK4
  GtkEventController* key = gtk_event_controller_key_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->sequence_table), key);
  g_signal_connect (key, "key-press-event",
      G_CALLBACK (on_sequence_table_key_press_event), (gpointer) self);
#endif

#if 0 /// GTK4 selection model signal?
  g_signal_connect_after (self->sequence_table, "cursor-changed",
      G_CALLBACK (on_sequence_table_cursor_changed), (gpointer) self);
#endif
  
#if 0 // GTK4 see if the "sequence pos" approach (signal per list widget) works first
  GtkGesture* click = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->sequence_table), GTK_EVENT_CONTROLLER (click));

  g_signal_connect (click, "button-press-event",
      G_CALLBACK (on_sequence_table_button_press_event), (gpointer) self);
#endif
  
  motion = gtk_event_controller_motion_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->sequence_table), motion);

#if 0 /// GTK4
  g_signal_connect (self->sequence_table, "motion",
      G_CALLBACK (on_sequence_table_motion_notify_event), (gpointer) self);
#endif

  gtk_widget_set_name (GTK_WIDGET (self->sequence_table),
      "sequence editor");

}

static void
bt_sequence_view_class_init (BtSequenceViewClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->get_property = bt_sequence_view_get_property;
  gobject_class->set_property = bt_sequence_view_set_property;
  gobject_class->dispose = bt_sequence_view_dispose;

  widget_class->realize = bt_sequence_view_realize;
  widget_class->snapshot = bt_sequence_view_snapshot;


  g_object_class_install_property (gobject_class, SEQUENCE_VIEW_PLAY_POSITION,
      g_param_spec_double ("play-position",
          "play position prop.",
          "The current playing position as a fraction",
          0.0, 1.0, 0.0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_VIEW_LOOP_START,
      g_param_spec_double ("loop-start",
          "loop start position prop.",
          "The start position of the loop range as a fraction",
          0.0, 1.0, 0.0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_VIEW_LOOP_END,
      g_param_spec_double ("loop-end",
          "loop end position prop.",
          "The end position of the loop range as a fraction",
          0.0, 1.0, 1.0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_VIEW_VISIBLE_ROWS,
      g_param_spec_ulong ("visible-rows",
          "visible rows prop.",
          "The number of currently visible sequence rows",
          0, G_MAXULONG, 0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_VIEW_BARS,
      g_param_spec_ulong ("bars",
          "bars prop.",
          "The duration in ticks of each cell in the sequence view",
          0, G_MAXULONG, 0, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (gobject_class, SEQUENCE_VIEW_BARS,
      g_param_spec_ulong ("current-machine",
          "Currently selected machine",
          "The machine whose sequences the cursor is currently over",
          0, G_MAXULONG, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  
  gtk_widget_class_set_template_from_resource(widget_class,
      "/org/buzztrax/ui/sequence-view.ui");

  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, label_menu);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, pos_header);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, pos_menu);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, sequence_pos_table);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, sequence_pos_table_pos);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, sequence_pos_table_header);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, sequence_table);
  gtk_widget_class_bind_template_child (widget_class, BtSequenceView, sequence_table_header);
  
  gtk_widget_class_bind_template_callback (widget_class, on_pos_menu_changed);
}
