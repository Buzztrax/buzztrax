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
 * SECTION:btmainpagesequence
 * @short_description: the editor main sequence page
 * @see_also: #BtSequence, #BtSequenceView
 *
 * Provides an editor for #BtSequence instances.
 */

/* TODO(ensonic): shortcuts
 * - Ctrl-<num> :  Stepping
 *   - set increment for cursor-down on edit
 * - Duplicate pattern: make a copy of pattern under cursor and go to pattern view
 * - New pattern: open new pattern dialog, insert pattern under cursor and go to
 *   pattern view
 */
/* TODO(ensonic): sequence view context menu
 * - open pattern properties
 * - copy current pattern
 * - allow to switch meters (off, level, scope, spectrum)
 */
/* TODO(ensonic): pattern list
 *   - go to next occurence when double clicking a pattern
 *   - show tick-length in pattern list (needs column in model)
 */
/* IDEA(ensonic): follow-playback
 * - make this a setting
 * - add a shortcut to jump to playback-pos,
 *   we have F6 for play from cursor, maybe add Shift+F6 for jump to play pos
 * - have a way to disable it externally (for song-recording)
 */
/* IDEA(ensonic): bold row,label for cursor row
 *   - makes it easier to follow position in wide sequences
 *     (same needed for pattern view)
 */
/* IDEA(ensonic): have a split horizontal command
 *   - we would share the hadjustment, but have separate vadjustments
 *   - the label-menu would require that we have a focused view
 */
/* @bugs
 * - hovering the mouse over the treeview causes redraws for the whole lines
 *   - cells are asked to do prelight, even if they wouldn't draw anything else
 *     http://www.gtk.org/plan/meetings/20041025.txt
 */
/* IDEA(ensonic): programmable keybindings
 * - we should define an enum for the key commands
 * - we should have a tables that maps keyval+state to one of the enums
 * - we should have a utility function to do the lookups
 * - we could have primary and secondary keys (keyval+state)
 * - each entry has a description (i18n)
 * - the whole group has a name (i18n)
 * - we can register the group to a binding manager
 * - the binding manager provides a ui to:
 *   - edit the bindings
 *   - save/load bindings to/from a named preset
 */

#include "src/ui/edit/main-page-sequence.h"
#define BT_EDIT
#define BT_MAIN_PAGE_SEQUENCE_C

#include "bt-edit.h"
#include "sequence-view.h"

#include <math.h>
#include "gtkvumeter.h"

enum
{
  MAIN_PAGE_SEQUENCE_CURSOR_ROW = 1,
  MAIN_PAGE_SEQUENCE_FOLLOW_PLAYBACK
};

struct _BtMainPageSequence
{
  GtkBox parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the main window */
  BtMainWindow *main_window;

  /* the sequence we are showing */
  BtSequence *sequence;
  /* the song-info for the timing */
  BtSongInfo *song_info;
  /* machine for current column */
  BtMachine *machine;

  BtSequenceView *sequence_view;
  
  /* bars selection menu */
  GtkDropDown *bars_menu;
  gulong bars;

  /* label selection menu */
  GtkDropDown *label_menu;

  /* the pattern list */
  GtkListView *pattern_list;

  /* local commands */
  /// GTK4 GtkAccelGroup *accel_group;

  GtkMenuButton *context_menu_button;
  
  /* sequence context_menu */
  GMenu *context_menu;
  GMenu *context_menu_add;

  /* colors */
  GdkRGBA cursor_bg;
  GdkRGBA selection_bg1, selection_bg2;
  GdkRGBA source_bg1, source_bg2;
  GdkRGBA processor_bg1, processor_bg2;
  GdkRGBA sink_bg1, sink_bg2;

  /* some internal states */
  glong tick_pos;

  GtkToggleButton *follow_playback_button;
  gboolean follow_playback;

  /* vumeter data */
  GHashTable *level_to_vumeter;
  GstClock *clock;

  /* signal handler id's */
  gulong pattern_removed_handler;

  /* playback state */
  gboolean is_playing;

  /* lock for multithreaded access */
  GMutex lock;

  /**
   * cached sequence properties
   *
   * This UI element modifies some sequence properties that should be made
   * persistent, such as the "bars" property.
   */
  GHashTable *sequence_properties;

  /* editor change log */
  BtChangeLog *change_log;
};

static GQuark bus_msg_level_quark = 0;
static GQuark vu_meter_skip_update = 0;
static GQuark machine_for_track = 0;


/// GTK4 static GdkAtom sequence_atom;

//-- the class

static void bt_main_page_sequence_change_logger_interface_init (gpointer const
    g_iface, gconstpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMainPageSequence, bt_main_page_sequence,
    GTK_TYPE_BOX,
    G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
        bt_main_page_sequence_change_logger_interface_init));

#define PATTERN_IX_CELL_WIDTH 30
#define PATTERN_NAME_CELL_WIDTH 70
#define PATTERN_LIST_WIDTH (13 + PATTERN_IX_CELL_WIDTH + PATTERN_NAME_CELL_WIDTH)

#define LOW_VUMETER_VAL -60.0

enum
{
  METHOD_SET_PATTERNS,
  METHOD_SET_LABELS,
  METHOD_SET_SEQUENCE_PROPERTY,
  METHOD_ADD_TRACK,
  METHOD_REM_TRACK,
  METHOD_MOVE_TRACK
};

static BtChangeLoggerMethods change_logger_methods[] = {
  BT_CHANGE_LOGGER_METHOD ("set_patterns", 13,
      "([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD ("set_labels", 11, "([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD ("set_sequence_property", 22,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("add_track", 10, "\"([a-zA-Z0-9 ]+)\",([0-9]+)$"),
  BT_CHANGE_LOGGER_METHOD ("rem_track", 10, "([0-9]+)$"),
  BT_CHANGE_LOGGER_METHOD ("move_track", 11, "([0-9]+),([0-9]+)$"),
  {NULL,}
};


static GQuark column_index_quark = 0;

static void on_pattern_removed (BtMachine * machine, BtPattern * pattern,
    gpointer user_data);

//-- main-window helper
static void
grab_main_window (BtMainPageSequence * self)
{
  GtkRoot *toplevel = gtk_widget_get_root (GTK_WIDGET (self));
  self->main_window = BT_MAIN_WINDOW (toplevel);
  GST_DEBUG ("top-level-window = %p", toplevel);
}

static gint
get_avg_pixels_per_char (GtkWidget * widget)
{
  PangoContext *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics *metrics = pango_context_get_metrics (context,
      pango_context_get_font_description (context),
      pango_context_get_language (context));
  gint char_width = pango_font_metrics_get_approximate_char_width (metrics);
  gint digit_width = pango_font_metrics_get_approximate_digit_width (metrics);

  pango_font_metrics_unref (metrics);

  gint width = MAX (char_width, digit_width);

  return (width + (PANGO_SCALE - 1)) / PANGO_SCALE;
}

//-- tree cell data functions
#if 0 /// GTK4
static void
label_cell_data_function (GtkTreeViewColumn * col, GtkCellRenderer * renderer,
    GtkTreeModel * model, GtkTreeIter * iter, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  gulong row;
  gboolean shade;
  GdkRGBA *bg_col = NULL;

  gtk_tree_model_get (model, iter,
      BT_SEQUENCE_GRID_MODEL_POS, &row,
      BT_SEQUENCE_GRID_MODEL_SHADE, &shade, -1);

  if ((0 == self->cursor_column) && (row == self->cursor_row)) {
    bg_col = &self->cursor_bg;
  } else if ((0 >= self->selection_start_column)
      && (0 <= self->selection_end_column)
      && (row >= self->selection_start_row)
      && (row <= self->selection_end_row)
      ) {
    bg_col = shade ? &self->selection_bg2 : &self->selection_bg1;
  }
  if (bg_col) {
    g_object_set (renderer,
        "background-rgba", bg_col, "background-set", TRUE, NULL);
  } else {
    g_object_set (renderer, "background-set", FALSE, NULL);
  }
}


static void
machine_cell_data_function (BtMainPageSequence * self, GtkTreeViewColumn * col,
    GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter,
    GdkRGBA * machine_bg1, GdkRGBA * machine_bg2)
{
  gulong row, column;
  gboolean shade;
  GdkRGBA *bg_col;
  gchar *str;

  column =
      1 + GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (col),
          column_index_quark));

  gtk_tree_model_get (model, iter,
      BT_SEQUENCE_GRID_MODEL_POS, &row,
      BT_SEQUENCE_GRID_MODEL_SHADE, &shade,
      BT_SEQUENCE_GRID_MODEL_LABEL + column, &str, -1);

  if ((column == self->cursor_column) && (row == self->cursor_row)) {
    bg_col = &self->cursor_bg;
  } else if ((column >= self->selection_start_column)
      && (column <= self->selection_end_column)
      && (row >= self->selection_start_row)
      && (row <= self->selection_end_row)) {
    bg_col = shade ? &self->selection_bg2 : &self->selection_bg1;
  } else {
    bg_col = shade ? machine_bg2 : machine_bg1;
  }
  g_object_set (renderer, "background-rgba", bg_col, "text", str, NULL);
  g_free (str);
}

static void
source_machine_cell_data_function (GtkTreeViewColumn * col,
    GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  machine_cell_data_function (self, col, renderer, model, iter,
      &self->source_bg1, &self->source_bg2);
}

static void
processor_machine_cell_data_function (GtkTreeViewColumn * col,
    GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  machine_cell_data_function (self, col, renderer, model, iter,
      &self->processor_bg1, &self->processor_bg2);
}

static void
sink_machine_cell_data_function (GtkTreeViewColumn * col,
    GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  machine_cell_data_function (self, col, renderer, model, iter,
      &self->sink_bg1, &self->sink_bg2);
}

#endif

//-- gtk helpers

static GtkWidget *
make_mini_button (const gchar * txt, const gchar * style, gboolean toggled)
{
  GtkWidget *button;

  button = gtk_toggle_button_new_with_label (txt);
  gtk_widget_set_can_focus (button, FALSE);
  if (toggled)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), toggled);

  gtk_widget_add_css_class (button, "mini");
  gtk_widget_add_css_class (button, style);

  return button;
}

//-- undo/redo helpers
static void
sequence_range_copy (BtMainPageSequence * self, glong track_beg,
    glong track_end, glong tick_beg, glong tick_end, GString * data)
{
  BtSequence *sequence = self->sequence;
  BtMachine *machine;
  BtCmdPattern *pattern;
  glong i, j, col;
  gchar *id, *str;
  gulong sequence_length;

  g_object_get (sequence, "len-patterns", &sequence_length, NULL);

  /* label-track */
  col = track_beg;
  if (col == 0) {
    g_string_append_c (data, ' ');
    for (j = tick_beg; j <= tick_end; j++) {
      if ((j < sequence_length) && (str = bt_sequence_get_label (sequence, j))) {
        g_string_append_c (data, ',');
        g_string_append (data, str);
        g_free (str);
      } else {
        // empty cell
        g_string_append (data, ", ");
      }
    }
    g_string_append_c (data, '\n');
    col++;
  }

  /* machine-tracks */
  for (i = col; i <= track_end; i++) {
    // store machine id
    machine = bt_sequence_get_machine (sequence, i - 1);
    g_object_get (machine, "id", &id, NULL);
    g_string_append (data, id);
    g_free (id);
    for (j = tick_beg; j <= tick_end; j++) {
      // store pattern id
      if ((j < sequence_length)
          && (pattern = bt_sequence_get_pattern (sequence, j, i - 1))) {
        g_object_get (pattern, "name", &id, NULL);
        g_string_append_c (data, ',');
        g_string_append (data, id);
        g_free (id);
        g_object_unref (pattern);
      } else {
        // empty cell
        g_string_append (data, ", ");
      }
    }
    g_string_append_c (data, '\n');
    g_object_unref (machine);
  }
}

static void
sequence_range_log_undo_redo (BtMainPageSequence * self, glong track_beg,
    glong track_end, glong tick_beg, glong tick_end, gchar * old_str,
    gchar * new_str)
{
  gchar *undo_str, *redo_str;
  gchar *p;
  glong i, col;

  bt_change_log_start_group (self->change_log);

  /* label-track */
  col = track_beg;
  if (col == 0) {
    p = strchr (old_str, '\n');
    *p = '\0';
    undo_str =
        g_strdup_printf ("set_labels %lu,%lu,%s", tick_beg, tick_end, old_str);
    old_str = &p[1];
    p = strchr (new_str, '\n');
    *p = '\0';
    redo_str =
        g_strdup_printf ("set_labels %lu,%lu,%s", tick_beg, tick_end, new_str);
    new_str = &p[1];
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    col++;
  }
  /* machine-tracks */
  for (i = col; i <= track_end; i++) {
    p = strchr (old_str, '\n');
    *p = '\0';
    undo_str =
        g_strdup_printf ("set_patterns %lu,%lu,%lu,%s", i - 1, tick_beg,
        tick_end, old_str);
    old_str = &p[1];
    p = strchr (new_str, '\n');
    *p = '\0';
    redo_str =
        g_strdup_printf ("set_patterns %lu,%lu,%lu,%s", i - 1, tick_beg,
        tick_end, new_str);
    new_str = &p[1];
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
  }
  bt_change_log_end_group (self->change_log);
}

//-- event handlers

static void
on_machine_id_renamed (GtkEntry * entry, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  const gchar *name = gtk_editable_get_text (GTK_EDITABLE (entry));
  BtMachine *cur_machine =
      g_object_get_qdata ((GObject *) entry, machine_for_track);
  gboolean unique = FALSE;

  // ensure uniqueness of the entered data
  if (*name) {
    BtSetup *setup;
    BtMachine *machine;

    bt_child_proxy_get (self->app, "song::setup", &setup, NULL);

    if ((machine = bt_setup_get_machine_by_id (setup, name))) {
      if (machine == cur_machine) {
        unique = TRUE;
      }
      g_object_unref (machine);
    } else {
      unique = TRUE;
    }
    g_object_unref (setup);
  }
  GST_INFO ("%s" "unique '%s'", (unique ? "" : "not "), name);
  if (unique) {
    g_object_set (cur_machine, "id", name, NULL);
  } else {
    gchar *id;
    g_object_get (cur_machine, "id", &id, NULL);
    gtk_editable_set_text (GTK_EDITABLE (entry), id);
    g_free (id);
  }
}

typedef struct
{
  BtMainPageSequence *self;
  GtkVUMeter *vumeter;
  gint peak, decay;
  // DEBUG
  //GstClockTime t0,t1,t2,t3;
  // DEBUG
} BtUpdateIdleData;

#define MAKE_UPDATE_IDLE_DATA(data,self,vumeter,peak,decay) G_STMT_START { \
  data=g_slice_new(BtUpdateIdleData); \
  data->self=self; \
  data->vumeter=vumeter; \
  data->peak=(gint)(peak+0.5); \
  data->decay=(gint)(decay+0.5); \
  g_mutex_lock(&self->lock); \
  g_object_add_weak_pointer((GObject *)self,(gpointer *)(&data->self)); \
  g_object_add_weak_pointer((GObject *)vumeter,(gpointer *)(&data->vumeter)); \
  g_mutex_unlock(&self->lock); \
} G_STMT_END

#define FREE_UPDATE_IDLE_DATA(data) G_STMT_START { \
  if(data->self) { \
    g_mutex_lock(&data->self->lock); \
    g_object_remove_weak_pointer((gpointer)data->self,(gpointer *)(&data->self)); \
    if(data->vumeter) g_object_remove_weak_pointer((gpointer)data->vumeter,(gpointer *)(&data->vumeter)); \
    g_mutex_unlock(&data->self->lock); \
  } \
  g_slice_free(BtUpdateIdleData,data); \
} G_STMT_END


static gboolean
on_delayed_idle_track_level_change (gpointer user_data)
{
  BtUpdateIdleData *data = (BtUpdateIdleData *) user_data;
  BtMainPageSequence *self = data->self;

  if (self && self->is_playing && data->vumeter) {
    //data->t3 = gst_util_get_timestamp ();
    //GST_WARNING ("wait.2 for %"GST_TIME_FORMAT", d %"GST_TIME_FORMAT", d %"GST_TIME_FORMAT,
    //  GST_TIME_ARGS (data->t0), GST_TIME_ARGS (data->t3 - data->t1), GST_TIME_ARGS (data->t3 - data->t2));
    //gtk_vumeter_set_levels (data->vumeter, data->decay, data->peak);
    gtk_vumeter_set_levels (data->vumeter, data->peak, data->decay);
  }
  FREE_UPDATE_IDLE_DATA (data);
  return FALSE;
}

static gboolean
on_delayed_track_level_change (GstClock * clock, GstClockTime time,
    GstClockID id, gpointer user_data)
{
  // the callback is called from a clock thread
  if (GST_CLOCK_TIME_IS_VALID (time)) {
    //BtUpdateIdleData *data = (BtUpdateIdleData *) user_data;
    //data->t2 = gst_util_get_timestamp ();
    //GST_WARNING ("wait.1 for %"GST_TIME_FORMAT", d %"GST_TIME_FORMAT,
    //  GST_TIME_ARGS (time), GST_TIME_ARGS (data->t2 - data->t1));
    g_idle_add_full (G_PRIORITY_HIGH, on_delayed_idle_track_level_change,
        user_data, NULL);
  } else {
    BtUpdateIdleData *data = (BtUpdateIdleData *) user_data;
    FREE_UPDATE_IDLE_DATA (data);
  }
  return TRUE;
}

static void
on_track_level_change (GstBus * bus, GstMessage * message, gpointer user_data)
{
  const GstStructure *s = gst_message_get_structure (message);
  const GQuark name_id = gst_structure_get_name_id (s);

  if (name_id == bus_msg_level_quark) {
    BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
    GstElement *level = GST_ELEMENT (GST_MESSAGE_SRC (message));
    GtkVUMeter *vumeter;

    // check if its our element (we can have multiple level meters)
    if ((vumeter = g_hash_table_lookup (self->level_to_vumeter, level))) {
      GstClockTime waittime = bt_gst_analyzer_get_waittime (level, s, TRUE);
      if (GST_CLOCK_TIME_IS_VALID (waittime)) {
        gdouble decay, peak;
        gint new_skip = 0, old_skip = 0;

        peak =
            bt_gst_level_message_get_aggregated_field (s, "peak",
            LOW_VUMETER_VAL);
        decay =
            bt_gst_level_message_get_aggregated_field (s, "decay",
            LOW_VUMETER_VAL);
        // check if we are silent or very loud
        if (decay <= LOW_VUMETER_VAL && peak <= LOW_VUMETER_VAL) {
          new_skip = 1;         // below min level
        } else if (decay >= 0.0 && peak >= 0.0) {
          new_skip = 2;         // beyond max level
        }
        // skip *updates* if we are still below LOW_VUMETER_VAL or beyond 0.0
        old_skip =
            GPOINTER_TO_INT (g_object_get_qdata ((GObject *) vumeter,
                vu_meter_skip_update));
        g_object_set_qdata ((GObject *) vumeter, vu_meter_skip_update,
            GINT_TO_POINTER (new_skip));
        if (!old_skip || !new_skip || old_skip != new_skip) {
          BtUpdateIdleData *data;
          GstClockID clock_id;
          GstClockReturn clk_ret;

          waittime += gst_element_get_base_time (level);
          clock_id = gst_clock_new_single_shot_id (self->clock, waittime);
          MAKE_UPDATE_IDLE_DATA (data, self, vumeter, peak, decay);
          //data->t0 = waittime;
          //data->t1 = gst_util_get_timestamp ();
          //GST_WARNING ("wait.0 for %"GST_TIME_FORMAT, GST_TIME_ARGS (waittime));
          if ((clk_ret = gst_clock_id_wait_async (clock_id,
                      on_delayed_track_level_change,
                      (gpointer) data, NULL)) != GST_CLOCK_OK) {
            GST_WARNING_OBJECT (vumeter, "clock wait failed: %d", clk_ret);
            FREE_UPDATE_IDLE_DATA (data);
          }
          gst_clock_id_unref (clock_id);
        }
        // just for counting
        //else GST_WARNING_OBJECT(level,"skipping level update");
      }
    }
  }
}

static void
on_sequence_label_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
#if 0 /// GTK4
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  GtkTreeModelFilter *filtered_store;
  GtkTreeModel *store;
  gulong pos;
  gchar *old_text;

  GST_INFO ("label edited: '%s': '%s'", path_string, new_text);

  if ((filtered_store =
          GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (self->
                  priv->sequence_table)))
      && (store = gtk_tree_model_filter_get_model (filtered_store))
      ) {
    GtkTreeIter iter, filter_iter;

    if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (filtered_store),
            &filter_iter, path_string)) {
      gboolean changed = FALSE;

      gtk_tree_model_filter_convert_iter_to_child_iter (filtered_store, &iter,
          &filter_iter);

      gtk_tree_model_get (store, &iter, BT_SEQUENCE_GRID_MODEL_POS, &pos,
          BT_SEQUENCE_GRID_MODEL_LABEL, &old_text, -1);
      GST_INFO ("old_text '%s'", old_text);

      if ((old_text == NULL) != (new_text == NULL)) {
        changed = TRUE;
        if (old_text && !*old_text)
          old_text = NULL;
        if (new_text && !*new_text)
          new_text = NULL;
      } else if (old_text && new_text && !strcmp (old_text, new_text))
        changed = TRUE;
      if (changed) {
        gchar *undo_str, *redo_str;
        gulong old_length, new_length = 0;

        GST_INFO ("label changed");
        g_object_get (self->sequence, "len-patterns", &old_length, NULL);

        // update the sequence

        bt_change_log_start_group (self->change_log);

        if (pos >= old_length) {
          new_length = pos + self->bars;
          g_object_set (self->sequence, "len-patterns", new_length, NULL);
          sequence_calculate_visible_lines (self);
          sequence_update_model_length (self);

          undo_str =
              g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
              old_length);
          redo_str =
              g_strdup_printf ("set_sequence_property \"length\",\"%ld\"",
              new_length);
          bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
              undo_str, redo_str);
        }
        bt_sequence_set_label (self->sequence, pos, new_text);

        undo_str =
            g_strdup_printf ("set_labels %lu,%lu, ,%s", pos, pos,
            (old_text ? old_text : " "));
        redo_str =
            g_strdup_printf ("set_labels %lu,%lu, ,%s", pos, pos,
            (new_text ? new_text : " "));
        bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
            undo_str, redo_str);
        bt_change_log_end_group (self->change_log);
      }
      g_free (old_text);
    }
  }
#endif
}

//-- event handler helper

static void
reset_level_meter (gpointer key, gpointer value, gpointer user_data)
{
  GtkVUMeter *vumeter = GTK_VUMETER (value);
  gtk_vumeter_set_levels (vumeter, LOW_VUMETER_VAL, LOW_VUMETER_VAL);
  g_object_set_qdata ((GObject *) vumeter, vu_meter_skip_update,
      GINT_TO_POINTER ((gint) FALSE));
}

/*
 * sequence_table_init:
 * @self: the sequence page
 *
 * inserts the Label columns.
 */
static void
sequence_table_init (BtMainPageSequence * self)
{
  gint col_index = 0;

  GST_INFO ("preparing sequence table");

#if 0 /// GTK 4 still need to destroy?
  GtkWidget *header, *vbox;
  GtkWidget *label;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  
  // do not destroy when flushing the header
  if ((vbox = gtk_widget_get_parent (GTK_WIDGET (self->label_menu)))) {
    GST_INFO ("holding label widget: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->label_menu));
    gtk_container_remove (GTK_CONTAINER (vbox),
        GTK_WIDGET (g_object_ref (self->label_menu)));
    //gtk_widget_unparent(GTK_WIDGET(g_object_ref(self->label_menu)));
    GST_INFO ("                    : %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->label_menu));
  }
  // empty header widget
  gtk_container_forall (GTK_CONTAINER (self->sequence_table_header),
      (GtkCallback) remove_container_widget,
      GTK_CONTAINER (self->sequence_table_header));
  
  // re-add static columns
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer,
      "mode", GTK_CELL_RENDERER_MODE_EDITABLE,
      "xalign", 1.0, "yalign", 0.5, "editable", TRUE, NULL);
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_signal_connect_object (renderer, "edited",
      G_CALLBACK (on_sequence_label_edited), (gpointer) self, 0);
  if ((tree_col =
          gtk_tree_view_column_new_with_attributes (_("Labels"), renderer,
              "text", BT_SEQUENCE_GRID_MODEL_LABEL, NULL))
      ) {
    g_object_set (tree_col,
        "sizing", GTK_TREE_VIEW_COLUMN_FIXED,
        "fixed-width", SEQUENCE_CELL_WIDTH, NULL);
    col_index =
        gtk_tree_view_append_column (self->sequence_table, tree_col);
    gtk_tree_view_column_set_cell_data_func (tree_col, renderer,
        label_cell_data_function, (gpointer) self, NULL);
  } else
    GST_WARNING ("can't create treeview column");
#endif

  if (self->level_to_vumeter)
    g_hash_table_destroy (self->level_to_vumeter);
  self->level_to_vumeter =
      g_hash_table_new_full (NULL, NULL, (GDestroyNotify) gst_object_unref,
      NULL);

  GST_DEBUG ("    number of columns : %d", col_index);
}

static void
machine_col_on_bind (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  BtListItemLong* item = BT_LIST_ITEM_LONG (object);
  BtMainPageSequence* self = BT_MAIN_PAGE_SEQUENCE (user_data);
  const guint time = item->value;
  const guint track_idx = *(guint*) g_object_get_data (G_OBJECT (factory), "bt-track");
  BtCmdPattern* pattern = bt_sequence_get_pattern (self->sequence, time, track_idx);

  gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (GTK_LIST_ITEM (item))), bt_cmd_pattern_get_name (pattern));
  g_object_unref (pattern);
}

static void
pattern_list_refresh (BtMainPageSequence * self)
{
  if (!self->machine) {
    // we're setting a NULL store for the label column which hides list
    gtk_list_view_set_model (self->pattern_list, NULL);
    return;
  }

  GST_INFO ("refresh pattern list for machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->machine));

  BtPatternListModel *store =
      bt_pattern_list_model_new (self->machine, self->sequence, FALSE);

  // sync machine in pattern page
  if (self->main_window) {
    BtMainPagePatterns *patterns_page;

    bt_child_proxy_get (self->main_window, "pages::patterns-page",
        &patterns_page, NULL);
    bt_main_page_patterns_show_machine (patterns_page, self->machine);
    g_object_unref (patterns_page);
  }

  GST_INFO ("refreshed pattern list for machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->machine));
  gtk_list_view_set_model (self->pattern_list, GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (store))));
  g_object_unref (store);       // drop with treeview
}


/*
 * update_after_current_machine_changed:
 * @self: the sequence page
 *
 * When the user has moved the cursor in the sequence view such that it's
 * pointing to a different machine to that previously selected, then update
 * the list of patterns so that it shows the patterns that belong to that
 * machine
 *
 * Also update the current selected machine in pattern view.
 */
static void
update_after_current_machine_changed (BtMainPageSequence * self)
{
  BtMachine *machine;

  GST_INFO ("change active track");

  g_object_get (self->sequence_view, "machine", &machine, NULL);
  
  if (machine == self->machine) {
    // nothing changed
    g_object_try_unref (machine);
    return;
  }

  GST_INFO ("changing machine %" G_OBJECT_REF_COUNT_FMT " to %"
      G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (self->machine),
      G_OBJECT_LOG_REF_COUNT (machine));

  if (self->machine) {
    GST_INFO ("unref old cur-machine %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->machine));
    g_signal_handler_disconnect (self->machine,
        self->pattern_removed_handler);
    // unref the old machine
    g_object_unref (self->machine);
    self->machine = NULL;
    self->pattern_removed_handler = 0;
  }
  if (machine) {
    GST_INFO ("ref new cur-machine: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    self->pattern_removed_handler =
        g_signal_connect (machine, "pattern-removed",
        G_CALLBACK (on_pattern_removed), (gpointer) self);
    // remember the new machine
    self->machine = machine;
  }
  pattern_list_refresh (self);
}

/*
 * machine_menu_refresh:
 * add all machines from setup to self->context_menu_add
 */
static void
machine_menu_refresh (BtMainPageSequence * self, const BtSetup * setup)
{
  BtMachine *machine;
  GList *node, *list;

  GST_INFO ("refreshing track menu");

  // (re)create a new menu
  g_menu_remove_all (self->context_menu_add);

  // fill machine menu
  g_object_get ((gpointer) setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);

    GMenuItem* item = g_menu_item_new (bt_machine_get_id (machine), NULL);
    g_menu_item_set_action_and_target (item, "machine.track.add", "s", bt_machine_get_id (machine));
    g_object_unref (item);
  }
  g_list_free (list);
}

/*
 * sequence_add_track:
 * @pos: the track position (-1 at the end)
 *
 * add a new track for the machine at the given position
 */
static void
sequence_add_track (BtMainPageSequence * self, BtMachine * machine,
    glong pos)
{
  BtSong *song;

  g_object_get (self->app, "song", &song, NULL);

  {
    bt_sequence_add_track (self->sequence, machine, pos);
    GST_INFO ("track added for machine %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
  }
  GST_INFO ("track update for machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  GST_INFO ("done for machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  g_object_unref (song);
}

static void
sequence_remove_track (BtMainPageSequence * self, gulong ix)
{
  BtMachine *machine;

  if ((machine = bt_sequence_get_machine (self->sequence, ix))) {
    // remove the track where the cursor is
    bt_sequence_remove_track_by_ix (self->sequence, ix);

    g_object_unref (machine);
  }
}

static gboolean
update_bars_menu (BtMainPageSequence * self, gulong bars)
{
  gchar str[21];
  gulong i, j;
  gint active = 2;
  gint selected = -1;
  gint added = 0;
  /* the useful stepping depends on the rythm
     beats=bars/tpb
     bars=16, beats=4, tpb=4 : 4/4 -> 1,8, 16,32,64
     bars=12, beats=3, tpb=4 : 3/4 -> 1,6, 12,24,48
     bars=18, beats=3, tpb=6 : 3/6 -> 1,9, 18,36,72
   */
  GtkStringList *store = gtk_string_list_new (NULL);

  if (bars != 1) {
    // single steps
    gtk_string_list_append (store, "1");
    if (self->bars == 1)
      selected = added;
    added++;
  } else {
    active--;
  }
  if (bars / 2 > 1) {
    // half bars
    snprintf (str, sizeof(str), "%lu", bars / 2);
    gtk_string_list_append (store, str);
    if (self->bars == (bars / 2))
      selected = added;
    added++;
  } else {
    active--;
  }
  // add bars and 3 times the double of bars
  for (j = 0, i = bars; j < 4; i *= 2, j++) {
    snprintf (str, sizeof(str), "%lu", i);
    gtk_string_list_append (store, str);
    if (self->bars == i)
      selected = added;
    added++;
  }
  if (selected > -1)
    active = selected;
  gtk_drop_down_set_model (self->bars_menu, G_LIST_MODEL (store));
  gtk_drop_down_set_selected (self->bars_menu, active);
  g_object_unref (store);       // drop with combobox

  return TRUE;
}

static void
switch_to_pattern_editor (BtMainPageSequence * self, gulong row, gulong track)
{
  BtMainPagePatterns *patterns_page;
  BtCmdPattern *pattern;
  gulong length;

  bt_child_proxy_get (self->main_window, "pages::patterns-page",
      &patterns_page, NULL);
  bt_child_proxy_set (self->main_window, "pages::page",
      BT_MAIN_PAGES_PATTERNS_PAGE, NULL);

  g_object_get (self->sequence, "length", &length, NULL);
  if ((row < length) &&
      (pattern = bt_sequence_get_pattern (self->sequence, row, track))) {
    if (BT_IS_PATTERN (pattern)) {
      GST_INFO ("show pattern");
      bt_main_page_patterns_show_pattern (patterns_page, (BtPattern *) pattern);
    }
    g_object_unref (pattern);
  } else {
    GST_INFO ("show machine");
    bt_main_page_patterns_show_machine (patterns_page, self->machine);
  }
  g_object_unref (patterns_page);
}

//-- event handler

static void
on_track_add_activated (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);

  // get the machine by the menuitems name
  //id=(gchar *)gtk_widget_get_name(GTK_WIDGET(menu_item));
  const gchar *id;
  BtSetup *setup;
  BtMachine *machine;

  // get song from app and then setup from song
  bt_child_proxy_get (self->app, "song::setup", &setup, NULL);

  g_variant_get (parameter, "&s", &id);

  GST_INFO ("adding track for machine \"%s\"", id);
  if ((machine = bt_setup_get_machine_by_id (setup, id))) {
    gchar *undo_str, *redo_str;
    gulong ix;

    g_object_get (self->sequence, "tracks", &ix, NULL);

    sequence_add_track (self, machine, -1);

    /* handle undo/redo */
    undo_str = g_strdup_printf ("rem_track %lu", ix);
    redo_str = g_strdup_printf ("add_track \"%s\",%lu", id, ix);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);

    g_object_unref (machine);
  }
  g_object_unref (setup);
}

static void
on_track_remove_activated (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);
  gulong number_of_tracks;
  guint64 column;
  
  g_variant_get (parameter, "t", &column);

  // change number of tracks
  g_object_get (self->sequence, "tracks", &number_of_tracks, NULL);
  if (number_of_tracks > 0) {
    {

      /* handle undo/redo */
      BtMachine * machine = bt_sequence_get_machine (self->sequence, column);
      g_assert (machine);
      
      const gchar* mid = bt_machine_get_id (machine);
      g_clear_object (&machine);
      
      gulong sequence_length;
      g_object_get (self->sequence, "length", &sequence_length, NULL);

      bt_change_log_start_group (self->change_log);

      GString *old_data = g_string_new (NULL);
      gchar *undo_str = g_strdup_printf ("add_track \"%s\",%lu", mid, column);
      gchar *redo_str = g_strdup_printf ("rem_track %lu", column);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self), undo_str, redo_str);

      sequence_range_copy (self, column + 1, column + 1, 0, sequence_length - 1,
          old_data);
      sequence_range_log_undo_redo (self, column + 1, column + 1, 0,
          sequence_length - 1, old_data->str, g_strdup (old_data->str));
      
      g_string_free (old_data, TRUE);

      bt_change_log_end_group (self->change_log);

      sequence_remove_track (self, column);
    }

    update_after_current_machine_changed (self);
  }
}

static void
on_track_move_left_activated (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);
  gulong track;
  g_variant_get (parameter, "t", &track);

  GST_INFO ("move track %ld to left", track);

  if (track > 0) {
    if (bt_sequence_move_track_left (self->sequence, track)) {
      BtSong *song;
      gchar *undo_str, *redo_str;

      // get song from app
      g_object_get (self->app, "song", &song, NULL);

      /* handle undo/redo */
      undo_str = g_strdup_printf ("move_track %lu,%lu", track - 1, track);
      redo_str = g_strdup_printf ("move_track %lu,%lu", track, track - 1);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);

      g_object_unref (song);
    }
  }
}

static void
on_track_move_right_activated (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);
  gulong track;
  g_variant_get (parameter, "t", &track);

  GST_INFO ("move track %ld to right", track);

  gulong number_of_tracks;
  g_object_get (self->sequence, "tracks", &number_of_tracks, NULL);

  if (track < number_of_tracks) {
    if (bt_sequence_move_track_right (self->sequence, track)) {
      BtSong *song;
      gchar *undo_str, *redo_str;

      // get song from app
      g_object_get (self->app, "song", &song, NULL);

      /* handle undo/redo */
      undo_str = g_strdup_printf ("move_track %lu,%lu", track + 1, track);
      redo_str = g_strdup_printf ("move_track %lu,%lu", track, track + 1);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);

      g_object_unref (song);
    }
  }
}

// Returns: (transfer full)
static BtMachine*
get_machine_by_id (BtMainPageSequence *self, const gchar* id) {
  BtSong* song;
  g_object_get (self->app, "song", &song, NULL);
  if (!song) // dbeswick: when could this happen?
    return NULL;
  
  BtSetup* setup;
  g_object_get (song, "setup", &setup, NULL);

  g_assert (setup);

  BtMachine* machine = bt_setup_get_machine_by_id (setup, id);
  
  g_object_unref (song);
  g_object_unref (setup);

  return machine;
}

static void
on_context_menu_machine_properties_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);
  
  const gchar* machine_id;
  g_variant_get (parameter, "&s", &machine_id);
  
  BtMachine *machine = get_machine_by_id (self, machine_id);
  
  bt_machine_show_properties_dialog (machine);

  g_object_unref (machine);
}

static void
on_context_menu_machine_preferences_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);
  
  const gchar* machine_id;
  g_variant_get (parameter, "&s", &machine_id);
  
  BtMachine *machine = get_machine_by_id (self, machine_id);
  
  bt_machine_show_properties_dialog (machine);

  g_object_unref (machine);
}

static void
on_sequence_view_current_machine_notify (GObject* object, GParamSpec *arg, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  
  update_after_current_machine_changed (self);
}

static void
on_song_play_pos_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  // calculate fractional pos and set into sequence-viewer
#if 0 /// GTK4 bt_sequence_view_reveal_play_pos... or add an additional follow-playback var to sequence_view
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);

  gdouble play_pos;
  gulong sequence_length, pos;
  
  g_object_get ((gpointer) song, "play-pos", &pos, NULL);
  g_object_get (self->sequence, "length", &sequence_length, NULL);
  play_pos = (gdouble) pos / (gdouble) sequence_length;
  if (play_pos <= 1.0) {
    g_object_set (self->sequence_table, "play-position", play_pos, NULL);
    g_object_set (self->sequence_pos_table, "play-position", play_pos,
        NULL);
  }
  //GST_DEBUG("sequence tick received : %d",pos);

  if (self->follow_playback) {
    sequence_sync_to_play_pos (self, pos);
  }
#endif
}

static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);

  g_object_get ((gpointer) song, "is-playing", &self->is_playing, NULL);
  // stop all level meters
  if (!self->is_playing) {
    g_hash_table_foreach (self->level_to_vumeter, reset_level_meter,
        NULL);
  }
}

static void
on_bars_menu_changed (GObject* object, GParamSpec* pspec, gpointer user_data)
{
  GtkDropDown *dropdown = GTK_DROP_DOWN (object);
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);

  GST_INFO ("bars_menu has changed : page=%p", user_data);

  GtkStringObject *str = GTK_STRING_OBJECT (gtk_drop_down_get_selected_item (dropdown));
  gulong bars = atoi (gtk_string_object_get_string (str));
  
  g_object_set (self->sequence_view, "bars", bars, NULL);
  g_hash_table_insert (self->sequence_properties, g_strdup ("bars"),
                       g_strdup (bt_str_format_ulong (self->bars)));
}

static void
on_follow_playback (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (widget);

  self->follow_playback = !self->follow_playback;
  g_object_notify ((GObject *) self, "follow-playback");
}

static void
updated_adjustment (GtkWidget * range, GdkScrollDirection direction,
    gdouble delta)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (range));
  gdouble new_value, value, lower, upper, page_size;

  g_object_get (adj, "value", &value, "upper", &upper, "lower", &lower,
      "page-size", &page_size, NULL);

  if (delta == 0.0) {
    delta = pow (page_size, 2.0 / 3.0);
    if (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_LEFT)
      delta = -delta;
    if (gtk_range_get_inverted ((GtkRange *) range))
      delta = -delta;
  } else {
    delta *= pow (page_size, 2.0 / 3.0);
  }

  new_value = CLAMP (value + delta, lower, upper - page_size);
  gtk_adjustment_set_value (adj, new_value);
}

#if 0 /// GTK4
static gboolean
on_scroll_event (GtkWidget * widget, GdkEventScroll * event, gpointer user_data)
{
  GtkScrolledWindow *sw = GTK_SCROLLED_WINDOW (widget);
  GtkWidget *range;
  GdkScrollDirection direction = event->direction;
  gboolean handled = FALSE;

  if (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_DOWN) {
    if ((range = gtk_scrolled_window_get_vscrollbar (sw))) {
      updated_adjustment (range, direction, 0.0);
      handled = TRUE;
    }
  } else if (direction == GDK_SCROLL_LEFT || direction == GDK_SCROLL_RIGHT) {
    if ((range = gtk_scrolled_window_get_hscrollbar (sw))) {
      updated_adjustment (range, direction, 0.0);
      handled = TRUE;
    }
  } else {
    gdouble dx, dy;

    if (gdk_event_get_scroll_deltas ((GdkEvent *) event, &dx, &dy)) {
      if (dx != 0.0) {
        if ((range = gtk_scrolled_window_get_hscrollbar (sw))) {
          updated_adjustment (range, direction, dx);
          handled = TRUE;
        }
      }
      if (dy != 0.0) {
        if ((range = gtk_scrolled_window_get_vscrollbar (sw))) {
          updated_adjustment (range, direction, dy);
          handled = TRUE;
        }
      }
    }
  }
  return handled;
}
#endif

/// GTK4 still needed?
// BUG(726795): the next two signal handlers are a hack to hide scrollbars
// on sync window
static void
on_scrollbar_visibility_changed (GObject * sb, GParamSpec * property,
    gpointer user_data)
{
  gboolean visible;

  g_object_get (sb, "visible", &visible, NULL);
  if (visible) {
    g_object_set (sb, "visible", FALSE, NULL);
  }
}

#if 0 /// GTK4
static void
on_scrolled_sync_window_realize (GtkWidget * widget, gpointer user_data)
{
  GtkWidget *sb;
  GtkOrientation orientation = GPOINTER_TO_INT (user_data);

  if (orientation == GTK_ORIENTATION_VERTICAL) {
    sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  } else {
    sb = gtk_scrolled_window_get_hscrollbar (GTK_SCROLLED_WINDOW (widget));
  }
  if (sb) {
    g_object_set (sb, "visible", FALSE, "no-show-all", TRUE, NULL);
    g_signal_connect (sb, "notify::visible",
        G_CALLBACK (on_scrollbar_visibility_changed), NULL);
  }
}
#endif

#if 0 /// GTK4
static gboolean
on_sequence_table_cursor_changed_idle (gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gulong cursor_column, cursor_row;
  GList *columns;

  g_return_val_if_fail (user_data, FALSE);

  GST_INFO ("sequence_table cursor has changed : self=%p", user_data);

  gtk_tree_view_get_cursor (self->sequence_table, &path, &column);
  if (column && path) {
    if (sequence_view_get_cursor_pos (self->sequence_table, &cursor_column, &cursor_row)) {
      gulong last_line, last_bar, last_column;

      columns = gtk_tree_view_get_columns (self->sequence_table);
      last_column = g_list_length (columns) - 2;
      g_list_free (columns);

      GST_INFO ("new row = %3lu <-> old row = %3ld", cursor_row,
          self->cursor_row);
      self->cursor_row = cursor_row;

      if (cursor_column > last_column) {
        cursor_column = last_column;
      }

      GST_INFO ("new col = %3lu <-> old col = %3ld", cursor_column,
          self->cursor_column);
      if (cursor_column != self->cursor_column) {
        self->cursor_column = cursor_column;
      }
      GST_INFO ("cursor has changed: %3ld,%3ld", self->cursor_column,
          self->cursor_row);

      // calculate the last visible row from step-filter and scroll-filter
      last_line = self->sequence_length - 1;
      last_bar = last_line - (last_line % self->bars);

      // do we need to extend sequence?
      if (cursor_row >= last_bar) {
        self->sequence_length += self->bars;
        sequence_update_model_length (self);

        gtk_tree_view_set_cursor (self->sequence_table, path, column,
            FALSE);
        gtk_widget_grab_focus_savely (GTK_WIDGET (self->sequence_table));
      }
      gtk_tree_view_scroll_to_cell (self->sequence_table, path, column,
          FALSE, 1.0, 0.0);
      gtk_widget_queue_draw (GTK_WIDGET (self->sequence_table));
    }
  } else {
    GST_INFO ("No cursor pos, column=%p, path=%p", column, path);
  }
  if (path)
    gtk_tree_path_free (path);

  return FALSE;
}

static void
on_sequence_table_cursor_changed (GtkTreeView * treeview, gpointer user_data)
{
  g_return_if_fail (user_data);
  BtMainPageSequence* main_page_sequence = BT_MAIN_PAGE_SEQUENCE (user_data);

  GST_INFO ("delay processing ...");
  /* delay the action */
  bt_g_object_idle_add ((GObject *) main_page_sequence, G_PRIORITY_HIGH_IDLE,
      on_sequence_table_cursor_changed_idle);
}
#endif

static gboolean
change_pattern (BtMainPageSequence * self, BtCmdPattern * new_pattern,
    gulong row, gulong track)
{
  gboolean res = FALSE;
  BtMachine *machine;

  if ((machine = bt_sequence_get_machine (self->sequence, track))) {
    BtCmdPattern *old_pattern =
        bt_sequence_get_pattern (self->sequence, row, track);
    gchar *undo_str, *redo_str;
    gchar *mid, *old_pid = NULL, *new_pid = NULL;

    g_object_get (machine, "id", &mid, NULL);
    bt_sequence_set_pattern (self->sequence, row, track, new_pattern);
    if (old_pattern) {
      g_object_get (old_pattern, "name", &old_pid, NULL);
      g_object_unref (old_pattern);
    }
    if (new_pattern) {
      g_object_get (new_pattern, "name", &new_pid, NULL);
    }
    undo_str =
        g_strdup_printf ("set_patterns %lu,%lu,%lu,%s,%s", track, row, row, mid,
        (old_pid ? old_pid : " "));
    redo_str =
        g_strdup_printf ("set_patterns %lu,%lu,%lu,%s,%s", track, row, row, mid,
        (new_pid ? new_pid : " "));
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    g_free (mid);
    g_free (new_pid);
    g_free (old_pid);
    g_object_unref (machine);
    res = TRUE;
  }
  return res;
}

#ifdef USE_DEBUG
#define LOG_SELECTION_AND_CURSOR(dir) \
GST_INFO (dir": %3ld,%3ld -> %3ld,%3ld @ %3ld,%3ld", \
    self->selection_start_column, self->selection_start_row,\
    self->selection_end_column, self->selection_end_row,\
    self->cursor_column, self->cursor_row)
#define LOG_SELECTION(dir) \
GST_INFO (dir": %3ld,%3ld -> %3ld,%3ld", \
    self->selection_start_column, self->selection_start_row,\
    self->selection_end_column, self->selection_end_row)
#else
#define LOG_SELECTION_AND_CURSOR(dir)
#define LOG_SELECTION(dir)
#endif

static gboolean
on_sequence_header_button_press_event (GtkGestureClick* click, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  gboolean res = FALSE;
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (click));

  GST_WARNING ("sequence_header button_press : button 0x%x", button);
  if (button == GDK_BUTTON_SECONDARY) {
    GtkWidget* popup = gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu));
    gtk_widget_set_parent (popup, GTK_WIDGET (self));
    gtk_popover_popup (GTK_POPOVER (popup));
    res = TRUE;
  }
  return res;
}

// This function is triggered by both the sequence view and the sequence pos
// view.
#if 0 // GTK4 see if the "sequence pos" approach (signal per list widget) works first
static gboolean
on_sequence_table_button_press_event (GtkGestureClick* click, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  gboolean res = FALSE;
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (click));
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (click));

  GST_INFO ("sequence_table button_press : button 0x%x", button);
  if (button == GDK_BUTTON_PRIMARY) {
      gulong modifier =
          (gulong) state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK);
      // determine sequence position from mouse coordinates
      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x,
              event->y, &path, &column, NULL, NULL)) {
        gulong track, row;

        if (sequence_view_get_cursor_pos (GTK_TREE_VIEW (widget), path, column,
                &track, &row)) {
          GST_INFO ("  left click to column %lu, row %lu", track, row);
          if (widget == GTK_WIDGET (self->sequence_pos_table)) {
            switch (modifier) {
              case 0:
                sequence_set_play_pos (self, (glong) row);
                break;
              case GDK_CONTROL_MASK:
                sequence_set_loop_start (self, (glong) row);
                break;
              case GDK_CONTROL_MASK | GDK_SHIFT_MASK:
                sequence_set_loop_end (self, (glong) row);
                break;
            }
          } else {
            // set cell focus
            gtk_tree_view_set_cursor (self->sequence_table, path, column,
                FALSE);
            gtk_widget_grab_focus_savely (GTK_WIDGET (self->
                    priv->sequence_table));
            // reset selection
            self->selection_start_column =
                self->selection_start_row =
                self->selection_end_column =
                self->selection_end_row = -1;
            if (event->type == GDK_2BUTTON_PRESS) {
              switch_to_pattern_editor (self, row, track - 1);
            }
          }
          res = TRUE;
        }
      } else {
        GST_INFO ("clicked outside data area - #1");
        switch (modifier) {
          case 0:
            sequence_set_play_pos (self, -1);
            break;
          case GDK_CONTROL_MASK:
            sequence_set_loop_start (self, -1);
            break;
          case GDK_CONTROL_MASK | GDK_SHIFT_MASK:
            sequence_set_loop_end (self, -1);
            break;
        }
        res = TRUE;
      }
      if (path)
        gtk_tree_path_free (path);
    }
  } else if (event->button == GDK_BUTTON_SECONDARY) {
    gtk_menu_popup (self->context_menu, NULL, NULL, NULL, NULL,
        GDK_BUTTON_SECONDARY, gtk_get_current_event_time ());
    res = TRUE;
  }
  return res;
}
#endif

static void
on_pattern_removed (BtMachine * machine, BtPattern * pattern,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  BtSequence *sequence = self->sequence;

  GST_INFO ("pattern has been removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (pattern));

  if (bt_sequence_is_pattern_used (sequence, pattern)) {
    glong tick;
    glong track = 0;
    gchar *undo_str, *redo_str;
    gchar *mid, *pid;

    g_object_get (machine, "id", &mid, NULL);
    g_object_get (pattern, "name", &pid, NULL);

    GST_WARNING ("pattern %s is used in sequence, doing undo/redo", pid);
    /* save the cells that use the pattern */
    bt_change_log_start_group (self->change_log);
    while ((track =
            bt_sequence_get_track_by_machine (sequence, machine, track)) > -1) {
      tick = 0;
      while ((tick =
              bt_sequence_get_tick_by_pattern (sequence, track,
                  (BtCmdPattern *) pattern, tick)) > -1) {
        undo_str =
            g_strdup_printf ("set_patterns %lu,%lu,%lu,%s,%s", track, tick,
            tick, mid, pid);
        redo_str =
            g_strdup_printf ("set_patterns %lu,%lu,%lu,%s,%s", track, tick,
            tick, mid, " ");
        bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
            undo_str, redo_str);
        bt_sequence_set_pattern_quick (sequence, tick, track, NULL);
        tick++;
      }
      track++;
    }
    bt_change_log_end_group (self->change_log);
    g_free (mid);
    g_free (pid);
  }
}

static void
on_song_info_bars_changed (const BtSongInfo * song_info, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  gulong bars;

  g_object_get ((gpointer) song_info, "bars", &bars, NULL);
  // this also recolors the sequence
  self->bars = 0;
  update_bars_menu (self, bars);
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  BtSong *song;
  BtSetup *setup;
  GstBin *bin;
  GstBus *bus;
  glong bars;
  gulong sequence_length;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  g_object_try_unref (self->sequence);
  g_object_try_unref (self->song_info);

  // get song from app and then setup from song
  g_object_get (self->app, "song", &song, NULL);
  if (!song) {
    self->sequence = NULL;
    self->song_info = NULL;
    self->sequence_properties = NULL;
    return;
  }
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_get (song, "setup", &setup, "song-info", &self->song_info,
      "sequence", &self->sequence, "bin", &bin, NULL);
  g_object_get (self->sequence, "len-patterns", &sequence_length, "properties",
      &self->sequence_properties, NULL);

  // reset vu-meter hash (rebuilt below)
  if (self->level_to_vumeter)
    g_hash_table_destroy (self->level_to_vumeter);
  self->level_to_vumeter =
      g_hash_table_new_full (NULL, NULL, (GDestroyNotify) gst_object_unref,
      NULL);

  // update page
  // update sequence and pattern list
  machine_menu_refresh (self, setup);
  // update toolbar
  g_object_get (self->song_info, "bars", &bars, NULL);
  update_bars_menu (self, bars);

  // vumeters
  bus = gst_element_get_bus (GST_ELEMENT (bin));
  bt_g_signal_connect_object (bus, "sync-message::element",
      G_CALLBACK (on_track_level_change), (gpointer) self, 0);
  gst_object_unref (bus);
  if (self->clock)
    gst_object_unref (self->clock);
  self->clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

  // subscribe to play-pos changes of song->sequence
  g_signal_connect_object (song, "notify::play-pos",
      G_CALLBACK (on_song_play_pos_notify), (gpointer) self, 0);
  g_signal_connect_object (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), (gpointer) self, 0);
  //-- release the references
  gst_object_unref (bin);
  g_object_unref (setup);
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

//-- helper methods

static void
bt_main_page_sequence_init_ui (BtMainPageSequence * self)
{
  GST_DEBUG ("!!!! self=%p", self);

  gtk_widget_set_name (GTK_WIDGET (self), "sequence view");

  g_signal_connect (self->bars_menu, "notify::selected",
      G_CALLBACK (on_bars_menu_changed), (gpointer) self);

  // popup menu button
  GtkBuilder *builder = gtk_builder_new_from_resource(
      "/org/buzztrax/ui/main-page-sequence-context-menu.ui");
  self->context_menu = G_MENU (gtk_builder_get_object (builder, "menu")); 
  g_object_ref (self->context_menu);
  self->context_menu_add = G_MENU (gtk_builder_get_object (builder, "add_track")); 
  g_object_ref (self->context_menu_add);
  g_object_unref (builder);

  gtk_menu_button_set_menu_model (self->context_menu_button, G_MENU_MODEL (self->context_menu));

  // --
  // TODO(ensonic): cut, copy, paste


#if 0 /// GTK4
  g_signal_connect (scrolled_vsync_window, "scroll-event",
      G_CALLBACK (on_scroll_event), NULL);
  g_signal_connect (scrolled_vsync_window, "realize",
      G_CALLBACK (on_scrolled_sync_window_realize),
      GINT_TO_POINTER (GTK_ORIENTATION_VERTICAL));
  
  g_signal_connect (scrolled_hsync_window, "scroll-event",
      G_CALLBACK (on_scroll_event), NULL);
  g_signal_connect (scrolled_hsync_window, "realize",
      G_CALLBACK (on_scrolled_sync_window_realize),
      GINT_TO_POINTER (GTK_ORIENTATION_HORIZONTAL));
#endif
  //GST_DEBUG("pos_view=%p, data_view=%p", self->sequence_pos_table,self->sequence_table);


  pattern_list_refresh (self);  // this is needed for the initial model creation

  // register event handlers
  g_signal_connect_object (self->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self, 0);

  // let settings control toolbar style
#if 0 /// GTK4
  g_object_get (self->app, "settings", &settings, NULL);
  g_object_bind_property_full (settings, "toolbar-style", toolbar,
      "toolbar-style", G_BINDING_SYNC_CREATE, bt_toolbar_style_changed, NULL,
      NULL, NULL);
#endif
  
  GST_DEBUG ("  done");
}

//-- constructor methods

//-- cut/copy/paste

#if 0 /// GTK4
static void
sequence_clipboard_get_func (GtkClipboard * clipboard,
    GtkSelectionData * selection_data, guint info, gpointer data)
{
  GST_INFO ("get clipboard data, info=%d, data=%p", info, data);
  GST_INFO ("sending : [%s]", (gchar *) data);
  // FIXME(ensonic): do we need to format differently depending on info?
  if (gtk_selection_data_get_target (selection_data) == sequence_atom) {
    gtk_selection_data_set (selection_data, sequence_atom, 8, (guchar *) data,
        strlen (data));
  } else {
    // allow pasting into a test editor for debugging
    // its only active if we register the formats in _copy_selection() below
    gtk_selection_data_set_text (selection_data, data, -1);
  }
}

static void
sequence_clipboard_clear_func (GtkClipboard * clipboard, gpointer data)
{
  GST_INFO ("freeing clipboard data, data=%p", data);
  g_free (data);
}
#endif

/**
 * bt_main_page_sequence_delete_selection:
 * @self: the sequence subpage
 *
 * Delete (clear) the selected area.
 */
void
bt_main_page_sequence_delete_selection (BtMainPageSequence * self)
{
#if 0 /// GTK4
  GtkTreeModel *store;
  glong selection_start_column, selection_start_row;
  glong selection_end_column, selection_end_row;

  if (self->selection_start_column == -1) {
    selection_start_column = selection_end_column = self->cursor_column;
    selection_start_row = selection_end_row = self->cursor_row;
  } else {
    selection_start_column = self->selection_start_column;
    selection_start_row = self->selection_start_row;
    selection_end_column = self->selection_end_column;
    selection_end_row = self->selection_end_row;
  }

  GST_INFO ("delete sequence region: %3ld,%3ld -> %3ld,%3ld",
      selection_start_column, selection_start_row, selection_end_column,
      selection_end_row);

  if ((store = sequence_model_get_store (self))) {
    GtkTreePath *path;

    if ((path = gtk_tree_path_new_from_indices (selection_start_row, -1))) {
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter (store, &iter, path)) {
        glong i, j;

        for (i = selection_start_row; i <= selection_end_row; i++) {
          for (j = selection_start_column - 1; j < selection_end_column; j++) {
            GST_DEBUG ("  delete sequence cell: %3ld,%3ld", j, i);
            bt_sequence_set_pattern_quick (self->sequence, i, j, NULL);
          }
          if (!gtk_tree_model_iter_next (store, &iter)) {
            if (j < self->selection_end_column) {
              GST_WARNING ("  can't get next tree-iter");
            }
            break;
          }
        }
      } else {
        GST_WARNING ("  can't get tree-iter for row %ld", selection_start_row);
      }
      gtk_tree_path_free (path);
    } else {
      GST_WARNING ("  can't get tree-path for row %ld", selection_start_row);
    }
  } else {
    GST_WARNING ("  can't get tree-model");
  }
  // reset selection
  self->selection_start_column = self->selection_start_row =
      self->selection_end_column = self->selection_end_row = -1;
#endif
}

/**
 * bt_main_page_sequence_cut_selection:
 * @self: the sequence subpage
 *
 * Cut selected area.
 * <note>not yet working</note>
 */
void
bt_main_page_sequence_cut_selection (BtMainPageSequence * self)
{
  bt_main_page_sequence_copy_selection (self);
  bt_main_page_sequence_delete_selection (self);
}

/**
 * bt_main_page_sequence_copy_selection:
 * @self: the sequence subpage
 *
 * Copy selected area.
 * <note>not yet working</note>
 */
void
bt_main_page_sequence_copy_selection (BtMainPageSequence * self)
{
#if 0 /// GTK4
  if (self->selection_start_row != -1
      && self->selection_start_column != -1) {
    //GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
    //GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->sequence_table),GDK_SELECTION_SECONDARY);
    GtkClipboard *cb =
        gtk_widget_get_clipboard (GTK_WIDGET (self->sequence_table),
        GDK_SELECTION_CLIPBOARD);
    GtkTargetEntry *targets;
    gint n_targets;
    GString *data = g_string_new (NULL);

    GST_INFO ("copying : %ld,%ld - %ld,%ld", self->selection_start_column,
        self->selection_start_row, self->selection_end_column,
        self->selection_end_row);

    targets = gtk_target_table_make (sequence_atom, &n_targets);

    /* the number of ticks */
    g_string_append_printf (data, "%ld\n",
        (self->selection_end_row + 1) - self->selection_start_row);

    sequence_range_copy (self,
        self->selection_start_column, self->selection_end_column,
        self->selection_start_row, self->selection_end_row, data);

    GST_INFO ("copying : [%s]", data->str);

    /* put to clipboard */
    if (gtk_clipboard_set_with_data (cb, targets, n_targets,
            sequence_clipboard_get_func, sequence_clipboard_clear_func,
            g_string_free (data, FALSE))
        ) {
      gtk_clipboard_set_can_store (cb, NULL, 0);
    } else {
      GST_INFO ("copy failed");
    }

    gtk_target_table_free (targets, n_targets);
    GST_INFO ("copy done");
  }
#endif
}

static gboolean
sequence_deserialize_pattern_track (BtMainPageSequence * self,
    GtkTreeModel * store, GtkTreePath * path, gchar ** fields, gulong track,
    gulong row)
{
  gboolean res = TRUE;
#if 0 /// GTK4
  GtkTreeIter iter;
  BtMachine *machine;

  GST_INFO ("get machine for col %lu", track);
  machine = bt_sequence_get_machine (self->sequence, track);
  if (machine) {
    gchar *id, *str;

    g_object_get (machine, "id", &id, NULL);
    if (!strcmp (id, fields[0])) {
      if (gtk_tree_model_get_iter (store, &iter, path)) {
        BtSequence *sequence = self->sequence;
        BtCmdPattern *pattern;
        gint j = 1;
        gulong sequence_length;

        g_object_get (sequence, "len-patterns", &sequence_length, NULL);

        while ((row < sequence_length) && fields[j] && *fields[j] && res) {
          
          // Note: fields[j] may be a pattern name, " " for empty cell,
          // "   break" for a pattern break...
          if (strcmp (fields[j], " ") != 0) {
            pattern = bt_machine_get_pattern_by_name (machine, fields[j]);
            if (!pattern) {
              GST_WARNING ("machine %p on track %lu, has no pattern with id %s",
                  machine, track, fields[j]);
              str = NULL;
            } else {
              g_object_get (pattern, "name", &str, NULL);
            }
          } else {
            pattern = NULL;
            str = NULL;
          }
          bt_sequence_set_pattern_quick (sequence, row, track, pattern);
          GST_DEBUG ("inserted %s @ %lu,%lu", str, row, track);
          g_object_try_unref (pattern);
          g_free (str);
          if (!gtk_tree_model_iter_next (store, &iter)) {
            GST_WARNING ("  can't get next tree-iter");
            res = FALSE;
          }
          j++;
          row++;
        }
      } else {
        GST_WARNING ("  can't get tree-iter for row %ld",
            self->cursor_row);
        res = FALSE;
      }
    } else {
      GST_INFO ("machines don't match in %s <-> %s", fields[0], id);
      res = FALSE;
    }

    g_free (id);
    g_object_unref (machine);
  } else {
    GST_INFO ("no machine for track");
    res = FALSE;
  }
#endif
  return res;
}

static gboolean
sequence_deserialize_label_track (BtMainPageSequence * self,
    GtkTreeModel * store, GtkTreePath * path, gchar ** fields, gulong row)
{
  gboolean res = TRUE;
#if 0 /// GTK4
  GtkTreeIter iter;
  gchar *str;

  GST_INFO ("paste labels");
  if (gtk_tree_model_get_iter (store, &iter, path)) {
    BtSequence *sequence = self->sequence;
    gint j = 1;
    gulong sequence_length;

    g_object_get (sequence, "len-patterns", &sequence_length, NULL);

    while ((row < sequence_length) && fields[j] && *fields[j] && res) {
      
      if (*fields[j] != ' ') {
        str = fields[j];
      } else {
        str = NULL;
      }
      bt_sequence_set_label (sequence, row, str);
      GST_DEBUG ("inserted %s @ %lu", str, row);
      if (!gtk_tree_model_iter_next (store, &iter)) {
        GST_WARNING ("  can't get next tree-iter");
        res = FALSE;
      }
      j++;
      row++;
    }
  } else {
    GST_WARNING ("  can't get tree-iter for row %ld", self->cursor_row);
    res = FALSE;
  }

#endif
  return res;
}


#if 0 /// GTK4
static void
sequence_clipboard_received_func (GtkClipboard * clipboard,
    GtkSelectionData * selection_data, gpointer user_data)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (user_data);
  gchar **lines;
  guint ticks;
  gchar *data;

  GST_INFO ("receiving clipboard data");

  data = (gchar *) gtk_selection_data_get_data (selection_data);
  GST_INFO ("pasting : [%s]", data);

  if (!data)
    return;

  lines = g_strsplit_set (data, "\n", 0);
  if (lines[0]) {
    GtkTreeModel *store;
    gint i = 1;
    gint beg, end;
    gulong sequence_length;
    gchar **fields;
    gboolean res = TRUE;

    g_object_get (self->sequence, "len-patterns", &sequence_length, NULL);

    ticks = atol (lines[0]);
    sequence_length--;
    // paste from self->cursor_row to MIN(self->cursor_row+ticks,sequence_length)
    beg = self->cursor_row;
    end = beg + ticks;

    if (end > sequence_length)
      g_object_set (self->priv->sequence, "len-patterns", end, NULL);
    
    GST_INFO ("pasting from row %d to %d", beg, end);

    if ((store = sequence_model_get_store (self))) {
      GtkTreePath *path;

      if ((path = gtk_tree_path_new_from_indices (self->cursor_row, -1))) {
        // process each line (= pattern column)
        while (lines[i] && *lines[i]
            && (self->cursor_row + (i - 1) <= end) && res) {

          // result should be <machine_id>,<pattern_name | " " | "   break">,...
          fields = g_strsplit_set (lines[i], ",", 0);

          if ((self->cursor_column + (i - 2)) >= 0) {
            res =
                sequence_deserialize_pattern_track (self, store, path, fields,
                (self->cursor_column + i - 2), self->cursor_row);
          } else if (*fields[0] == ' ') {
            res =
                sequence_deserialize_label_track (self, store, path, fields,
                self->cursor_row);
          }
          g_strfreev (fields);
          i++;
        }
        gtk_tree_path_free (path);
      } else {
        GST_WARNING ("  can't get tree-path");
      }
    } else {
      GST_WARNING ("  can't get tree-model");
    }
  }
  g_strfreev (lines);
  
  sequence_calculate_visible_lines (self);
  sequence_update_model_length (self);
}
#endif

/**
 * bt_main_page_sequence_paste_selection:
 * @self: the sequence subpage
 *
 * Paste at the top of the selected area.
 */
void
bt_main_page_sequence_paste_selection (BtMainPageSequence * self)
{
#if 0 /// GTK4
  //GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
  //GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->sequence_table),GDK_SELECTION_SECONDARY);
  GtkClipboard *cb =
      gtk_widget_get_clipboard (GTK_WIDGET (self->sequence_table),
      GDK_SELECTION_CLIPBOARD);

  gtk_clipboard_request_contents (cb, sequence_atom,
      sequence_clipboard_received_func, (gpointer) self);
#endif
}

//-- change logger interface

static gboolean
bt_main_page_sequence_change_logger_change (BtChangeLogger * owner,
    const gchar * data)
{
  /// GTK4 BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (owner);
  GMatchInfo *match_info;
  gboolean res = FALSE;
  /// GTK4 gchar *s;

  GST_INFO ("undo/redo: [%s]", data);
  // parse data and apply action
  switch (bt_change_logger_match_method (change_logger_methods, data,
          &match_info)) {
#if 0 /// GTK4
    case METHOD_SET_PATTERNS:{
      GtkTreeModel *store;
      gchar *str;
      gulong track, s_row, e_row;

      // track, beg, end, patterns
      s = g_match_info_fetch (match_info, 1);
      track = atol (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 2);
      s_row = atol (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 3);
      e_row = atol (s);
      g_free (s);
      str = g_match_info_fetch (match_info, 4);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%lu|%lu|%lu|%s]", track, s_row, e_row, str);

      if ((store = sequence_model_get_store (self))) {
        GtkTreePath *path;
        if ((path = gtk_tree_path_new_from_indices (s_row, -1))) {
          gchar **fields = g_strsplit_set (str, ",", 0);

          res =
              sequence_deserialize_pattern_track (self, store, path, fields,
              track, s_row);
          g_strfreev (fields);
          gtk_tree_path_free (path);
        }
      }
      if (res) {
        // move cursor to s_row (+self->bars?)
        self->cursor_row = s_row;
      }
      g_free (str);
      break;
    }
    case METHOD_SET_LABELS:{
      GtkTreeModel *store;
      gchar *str;
      gulong s_row, e_row;

      // track, beg, end, patterns
      s = g_match_info_fetch (match_info, 1);
      s_row = atol (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 2);
      e_row = atol (s);
      g_free (s);
      str = g_match_info_fetch (match_info, 3);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%lu|%lu|%s]", s_row, e_row, str);

      if ((store = sequence_model_get_store (self))) {
        GtkTreePath *path;
        if ((path = gtk_tree_path_new_from_indices (s_row, -1))) {
          gchar **fields = g_strsplit_set (str, ",", 0);

          res =
              sequence_deserialize_label_track (self, store, path, fields,
              s_row);
          g_strfreev (fields);
          gtk_tree_path_free (path);
        }
      }
      if (res) {
        // move cursor to s_row (+self->bars?)
        self->cursor_row = s_row;
      }
      g_free (str);
      break;
    }
    case METHOD_SET_SEQUENCE_PROPERTY:{
      gchar *key, *val;

      key = g_match_info_fetch (match_info, 1);
      val = g_match_info_fetch (match_info, 2);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s]", key, val);
      // length
      // loop-start/end
      res = TRUE;
      if (!strcmp (key, "loop-start")) {
        g_object_set (self->sequence, "loop-start", atol (val), NULL);
        sequence_calculate_visible_lines (self);
      } else if (!strcmp (key, "loop-end")) {
        g_object_set (self->sequence, "loop-end", atol (val), NULL);
        sequence_calculate_visible_lines (self);
      } else if (!strcmp (key, "length")) {
        g_object_set (self->sequence, "length", atol (val), NULL);
        sequence_calculate_visible_lines (self);
        sequence_update_model_length (self);
      } else {
        GST_WARNING ("unhandled property '%s'", key);
        res = FALSE;
      }
      g_free (key);
      g_free (val);
      break;
    }
    case METHOD_ADD_TRACK:{
      gchar *mid;
      gulong ix;
      BtSetup *setup;
      BtMachine *machine;

      mid = g_match_info_fetch (match_info, 1);
      s = g_match_info_fetch (match_info, 2);
      ix = atol (s);
      g_free (s);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%lu]", mid, ix);

      // get song from app and then setup from song
      bt_child_proxy_get (self->app, "song::setup", &setup, NULL);

      if ((machine = bt_setup_get_machine_by_id (setup, mid))) {
        sequence_add_track (self, machine, ix);
        g_object_unref (machine);

        update_after_track_changed (self);
        res = TRUE;
      }
      g_object_unref (setup);
      g_free (mid);
      break;
    }
    case METHOD_REM_TRACK:{
      gulong ix;

      s = g_match_info_fetch (match_info, 1);
      ix = atol (s);
      g_free (s);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%lu]", ix);
      sequence_remove_track (self, ix);

      update_after_track_changed (self);
      res = TRUE;
      break;
    }
    case METHOD_MOVE_TRACK:{
      gulong ix_cur, ix_new;

      s = g_match_info_fetch (match_info, 1);
      ix_cur = atol (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 2);
      ix_new = atol (s);
      g_free (s);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%lu|%lu]", ix_cur, ix_new);
      // we only move right/left by one right now
      // TODO(ensonic): but maybe better change that sequence API, then one function
      // would be all we need
      if (ix_cur > ix_new) {
        if (bt_sequence_move_track_left (self->sequence, ix_cur)) {
          self->cursor_column--;
          res = TRUE;
        }
      } else {
        if (bt_sequence_move_track_right (self->sequence, ix_cur)) {
          self->cursor_column++;
          res = TRUE;
        }
      }
      if (res) {
        BtSong *song;

        g_object_get (self->app, "song", &song, NULL);
        // reinit the view
        sequence_table_refresh_columns (self, song);
        g_object_unref (song);
      }
      break;
    }
#endif
    default:
      GST_WARNING ("unhandled undo/redo method: [%s]", data);
  }

  return res;
}

static void
bt_main_page_sequence_change_logger_interface_init (gpointer const g_iface,
    gconstpointer const iface_data)
{
  BtChangeLoggerInterface *const iface = g_iface;

  iface->change = bt_main_page_sequence_change_logger_change;
}

//-- class internals

static void
bt_main_page_sequence_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MAIN_PAGE_SEQUENCE_FOLLOW_PLAYBACK:{
      gboolean active = g_value_get_boolean (value);
      if (active != self->follow_playback) {
        self->follow_playback = active;
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_page_sequence_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MAIN_PAGE_SEQUENCE_FOLLOW_PLAYBACK:
      g_value_set_boolean (value, self->follow_playback);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_page_sequence_dispose (GObject * object)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (object);

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->song_info);
  g_object_unref (self->sequence);
  self->main_window = NULL;

  g_object_unref (self->change_log);
  g_object_unref (self->app);

  if (self->machine) {
    GST_INFO ("unref old cur-machine: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->machine));
    if (self->pattern_removed_handler)
      g_signal_handler_disconnect (self->machine,
          self->pattern_removed_handler);
    g_object_unref (self->machine);
  }

  g_object_unref (self->context_menu);

  /// GTK4 g_object_try_unref (self->accel_group);

  if (self->clock)
    gst_object_unref (self->clock);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MAIN_PAGE_SEQUENCE);
  
  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_sequence_parent_class)->dispose (object);
}

static void
bt_main_page_sequence_finalize (GObject * object)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_mutex_clear (&self->lock);
  g_hash_table_destroy (self->level_to_vumeter);

  G_OBJECT_CLASS (bt_main_page_sequence_parent_class)->finalize (object);
}

static void
bt_main_page_sequence_init (BtMainPageSequence * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_main_page_sequence_get_instance_private(self);
  GST_DEBUG("!!!! self=%p", self);
  self->app = bt_edit_application_new();

  g_signal_connect_object (self->sequence_view, "notify::current-machine",
      G_CALLBACK (on_sequence_view_current_machine_notify), (gpointer) self, 0);
  
  self->bars = 16;
  // self->cursor_column=0;
  // self->cursor_row=0;

  self->follow_playback = TRUE;

  g_mutex_init(&self->lock);

  // the undo/redo changelogger
  self->change_log = bt_change_log_new();
  bt_change_log_register(self->change_log, BT_CHANGE_LOGGER(self));

  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  bt_main_page_sequence_init_ui (self);
}

static void bt_main_page_sequence_class_init(BtMainPageSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

#if 0 /// GTK4
  sequence_atom =
      gdk_atom_intern_static_string ("application/buzztrax::sequence");
#endif

  column_index_quark =
      g_quark_from_static_string("BtMainPageSequence::column-index");
  bus_msg_level_quark = g_quark_from_static_string("level");
  vu_meter_skip_update =
      g_quark_from_static_string("BtMainPageSequence::skip-update");
  machine_for_track =
      g_quark_from_static_string("BtMainPageSequence::machine_for_track");

  gobject_class->get_property = bt_main_page_sequence_get_property;
  gobject_class->set_property = bt_main_page_sequence_set_property;
  gobject_class->dispose = bt_main_page_sequence_dispose;
  gobject_class->finalize = bt_main_page_sequence_finalize;

  g_object_class_install_property(
      gobject_class, MAIN_PAGE_SEQUENCE_FOLLOW_PLAYBACK,
      g_param_spec_boolean("follow-playback", "follow-playback prop",
                           "scroll the sequence in sync with the playback",
                           TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  gtk_widget_class_set_template_from_resource(widget_class,
      "/org/buzztrax/ui/main-page-sequence.ui");

  gtk_widget_class_bind_template_child (widget_class, BtMainPageSequence, bars_menu);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageSequence, context_menu_button);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageSequence, pattern_list);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageSequence, sequence_view);

  // See res/ui/main_page_sequence_context_menu.ui  
  gtk_widget_class_install_property_action (widget_class, "follow-playback", "follow-playback");
  gtk_widget_class_install_action (widget_class, "track.add", "s", on_track_add_activated);
  gtk_widget_class_install_action (widget_class, "track.remove", "t", on_track_remove_activated);
  gtk_widget_class_install_action (widget_class, "track.move-left", "t", on_track_move_left_activated);
  gtk_widget_class_install_action (widget_class, "track.move-right", "t", on_track_move_right_activated);
  gtk_widget_class_install_action (widget_class, "machine.properties.show", "s", on_track_move_right_activated);
  gtk_widget_class_install_action (widget_class, "machine.preferences.show", "s", on_track_move_right_activated);
}
