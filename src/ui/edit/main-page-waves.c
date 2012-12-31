/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btmainpagewaves
 * @short_description: the editor wavetable page
 *
 * Manage a list of audio clips files. Provides an embedded file browser to load
 * files. A waveform viewer can show the selected clip.
 */
/* TODO(ensonic): need envelope editor and everything for it
 */
/* TODO(ensonic): gray wavetable entries for unused waves
 */
/* TODO(ensonic): move file-chooser widget to a notebook, to have extra tabs for
 * - loading from online sample sites
 * - recording (see lib/core/wave.c)
 */
/* TODO(ensonic): undo/redo (search for bt_edit_application_set_song_unsaved
 * - load/unload a wave
 * - change wave-table properties
 */
/* TODO(ensonic): focus on the wavetable tree view?
 * - unfortunately pressing space/enter activates rename
 * - being able to play notes with the keys (or some controller keyboard) would
 *   be sweet
 */
/* TODO(ensonic): we're using two extra pipeline for wave playback (loaded waves
 * and file-choser preview). This does not work if the choosen audio backend
 * cannot mix. Using the songs' sink is tricky as we would need to switch the
 * song to playing mode to play a wave.
 */
/* TODO(ensonic): allow to paste samples from other apps
 * - check if e.g. audacity places samples snippets in the clipboard
 */
#define BT_EDIT
#define BT_MAIN_PAGE_WAVES_C

#include "bt-edit.h"

struct _BtMainPageWavesPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the wavetable we are showing */
  BtWavetable *wavetable;
  /* the waveform graph widget */
  GtkWidget *waveform_viewer;

  /* the toolbar widgets */
  GtkWidget *list_toolbar, *browser_toolbar, *editor_toolbar;
  GtkWidget *wavetable_stop;
  GtkWidget *browser_play, *wavetable_play, *wavetable_clear;

  /* the list of wavetable entries */
  GtkTreeView *waves_list;
  /* and their parameters */
  GtkHScale *volume;
  GtkWidget *loop_mode;

  /* the list of wavelevels */
  GtkTreeView *wavelevels_list;
  //gulong 

  /* the sample chooser */
  GtkWidget *file_chooser;
  gboolean preview_in_chooser;

  /* playbin for filechooser preview */
  GstElement *playbin;
  /* seek events */
  GstEvent *play_seek_event;
  GstEvent *loop_seek_event[2];
  gint loop_seek_dir;

  /* elements for wavetable preview */
  GstElement *preview, *preview_src, *preview_sink;
  BtWave *play_wave;
  BtWavelevel *play_wavelevel;
  GstBtToneConversion *n2f;

  /* the query is used in update_playback_position */
  GstQuery *position_query;
  /* update handler id */
  guint preview_update_id;

  /* we need to hold the reference to not kill the notifies */
  BtSettings *settings;
};

//-- the class

G_DEFINE_TYPE (BtMainPageWaves, bt_main_page_waves, GTK_TYPE_VBOX);


//-- event handler helper

static void on_wave_list_model_row_changed (GtkTreeModel * tree_model,
    GtkTreePath * path, GtkTreeIter * iter, gpointer user_data);
static void on_waves_list_cursor_changed (GtkTreeView * treeview,
    gpointer user_data);
static void on_wavelevels_list_cursor_changed (GtkTreeView * treeview,
    gpointer user_data);
static void on_volume_changed (GtkRange * range, gpointer user_data);
static void on_loop_mode_changed (GtkComboBox * menu, gpointer user_data);

static BtWave *
waves_list_get_current (const BtMainPageWaves * self)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  BtWave *wave = NULL;

  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->waves_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    wave = bt_wave_list_model_get_object ((BtWaveListModel *) model, &iter);
  }
  return (wave);
}

static BtWavelevel *
wavelevels_list_get_current (const BtMainPageWaves * self, BtWave * wave)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  BtWavelevel *wavelevel = NULL;

  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->wavelevels_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    wavelevel = bt_wavelevel_list_model_get_object (
        (BtWavelevelListModel *) model, &iter);
  }
  return (wavelevel);
}

/*
 * waves_list_refresh:
 * @self: the waves page
 *
 * Build the list of waves from the songs wavetable
 */
static void
waves_list_refresh (const BtMainPageWaves * self)
{
  BtWaveListModel *store;
  GtkTreeIter tree_iter;
  GtkTreeSelection *selection;
  GtkTreePath *path = NULL;
  GtkTreeModel *old_store;
  gboolean have_selection = FALSE;

  GST_INFO ("refresh waves list: self=%p, wavetable=%p", self,
      self->priv->wavetable);

  store = bt_wave_list_model_new (self->priv->wavetable);

  // get old selection or get first item
  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->waves_list));
  if (gtk_tree_selection_get_selected (selection, &old_store, &tree_iter)) {
    path = gtk_tree_model_get_path (old_store, &tree_iter);
  }

  gtk_tree_view_set_model (self->priv->waves_list, GTK_TREE_MODEL (store));
  if (path) {
    have_selection =
        gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &tree_iter, path);
  } else {
    have_selection =
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &tree_iter);
    path = gtk_tree_path_new_from_indices (0, -1);
  }
  if (have_selection) {
    gtk_tree_selection_select_iter (selection, &tree_iter);
    gtk_tree_view_scroll_to_cell (self->priv->waves_list, path, NULL, TRUE, 0.5,
        0.5);
  }
  if (path) {
    gtk_tree_path_free (path);
  }

  g_signal_connect (store, "row-changed",
      G_CALLBACK (on_wave_list_model_row_changed), (gpointer) self);
  GST_INFO ("refresh waves list: tree-view-widget=%p", self->priv->waves_list);
  on_waves_list_cursor_changed (GTK_TREE_VIEW (self->priv->waves_list),
      (gpointer) self);

  g_object_unref (store);       // drop with treeview
}

/*
 * wavelevels_list_refresh:
 * @self: the waves page
 * @wave: the wave that
 *
 * Build the list of wavelevels for the given @wave
 */
static void
wavelevels_list_refresh (const BtMainPageWaves * self, const BtWave * wave)
{
  BtWavelevelListModel *store;
  GtkTreeSelection *selection;
  GtkTreeIter tree_iter;

  GST_INFO ("refresh wavelevels list: self=%p, wave=%p", self, wave);

  store = bt_wavelevel_list_model_new ((BtWave *) wave);
  gtk_tree_view_set_model (self->priv->wavelevels_list, GTK_TREE_MODEL (store));

  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->wavelevels_list));
  if (!gtk_tree_selection_get_selected (selection, NULL, NULL)) {
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &tree_iter))
      gtk_tree_selection_select_iter (selection, &tree_iter);
  }

  g_object_unref (store);       // drop with treeview
}

static BtWavelevel *
wavelevels_get_wavelevel_and_set_iter (BtMainPageWaves * self,
    GtkTreeIter * iter, GtkTreeModel ** store, gchar * path_string)
{
  BtWavelevel *wavelevel = NULL;
  BtWave *wave;

  g_assert (iter);
  g_assert (store);

  if ((wave = waves_list_get_current (self))) {
    if ((*store = gtk_tree_view_get_model (self->priv->wavelevels_list))) {
      if (gtk_tree_model_get_iter_from_string (*store, iter, path_string)) {
        wavelevel = bt_wavelevel_list_model_get_object (
            (BtWavelevelListModel *) * store, iter);
      }
    }
    g_object_unref (wave);
  }
  return (wavelevel);
}

static void
preview_stop (const BtMainPageWaves * self)
{
  if (self->priv->preview_update_id) {
    g_source_remove (self->priv->preview_update_id);
    self->priv->preview_update_id = 0;
    g_object_set (self->priv->waveform_viewer, "playback-cursor",
        G_GINT64_CONSTANT (-1), NULL);
  }
  /* if I set state directly to NULL, I don't get inbetween state-change messages */
  gst_element_set_state (self->priv->preview, GST_STATE_READY);
  self->priv->play_wave = NULL;
  self->priv->play_wavelevel = NULL;
}

static void
preview_update_seeks (const BtMainPageWaves * self)
{
  if (self->priv->play_wave) {
    GstEvent *old_play_event, *new_play_event;
    GstEvent *old_loop_event0, *new_loop_event0;
    GstEvent *old_loop_event1, *new_loop_event1;
    BtWaveLoopMode loop_mode;
    gulong loop_start, loop_end;
    gulong length, srate;
    GstBtNote root_note;
    gdouble prate;

    /* get parameters */
    g_object_get (self->priv->play_wave, "loop-mode", &loop_mode, NULL);
    g_object_get (self->priv->play_wavelevel,
        "root-note", &root_note,
        "length", &length,
        "loop-start", &loop_start, "loop-end", &loop_end, "rate", &srate, NULL);

    /* calculate pitch rate from root-note */
    prate =
        gstbt_tone_conversion_translate_from_number (self->priv->n2f,
        BT_WAVELEVEL_DEFAULT_ROOT_NOTE) /
        gstbt_tone_conversion_translate_from_number (self->priv->n2f,
        root_note);

    old_play_event = self->priv->play_seek_event;
    old_loop_event0 = self->priv->loop_seek_event[0];
    old_loop_event1 = self->priv->loop_seek_event[1];
    /* new events */
    if (loop_mode != BT_WAVE_LOOP_MODE_OFF) {
      GstClockTime play_beg, play_end;

      play_beg = gst_util_uint64_scale_int (GST_SECOND, loop_start, srate);
      play_end = gst_util_uint64_scale_int (GST_SECOND, loop_end, srate);

      new_play_event = gst_event_new_seek (prate, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
          GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT (0),
          GST_SEEK_TYPE_SET, play_end);
      if (loop_mode == BT_WAVE_LOOP_MODE_FORWARD) {
        GST_DEBUG ("prepare for forward loop play: %" GST_TIME_FORMAT " ... %"
            GST_TIME_FORMAT, GST_TIME_ARGS (play_beg),
            GST_TIME_ARGS (play_end));

        new_loop_event0 = gst_event_new_seek (prate, GST_FORMAT_TIME,
            GST_SEEK_FLAG_SEGMENT,
            GST_SEEK_TYPE_SET, play_beg, GST_SEEK_TYPE_SET, play_end);
      } else {
        GST_DEBUG ("prepare for pingpong loop play: %" GST_TIME_FORMAT " ... %"
            GST_TIME_FORMAT, GST_TIME_ARGS (play_beg),
            GST_TIME_ARGS (play_end));

        new_loop_event0 = gst_event_new_seek (-1.0 * prate, GST_FORMAT_TIME,
            GST_SEEK_FLAG_SEGMENT,
            GST_SEEK_TYPE_SET, play_beg, GST_SEEK_TYPE_SET, play_end);
      }
      new_loop_event1 = gst_event_new_seek (prate, GST_FORMAT_TIME,
          GST_SEEK_FLAG_SEGMENT,
          GST_SEEK_TYPE_SET, play_beg, GST_SEEK_TYPE_SET, play_end);
    } else {
      GstClockTime play_end =
          gst_util_uint64_scale_int (GST_SECOND, length, srate);

      GST_DEBUG ("prepare for no loop play: 0 ... %" GST_TIME_FORMAT,
          GST_TIME_ARGS (play_end));

      new_play_event = gst_event_new_seek (prate, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH,
          GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT (0),
          GST_SEEK_TYPE_SET, play_end);
      new_loop_event0 = gst_event_new_seek (prate, GST_FORMAT_TIME,
          GST_SEEK_FLAG_NONE,
          GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT (0),
          GST_SEEK_TYPE_SET, play_end);
      new_loop_event1 = NULL;
    }

    /* swap and replace */
    self->priv->play_seek_event = new_play_event;
    if (old_play_event)
      gst_event_unref (old_play_event);
    self->priv->loop_seek_event[0] = new_loop_event0;
    if (old_loop_event0)
      gst_event_unref (old_loop_event0);
    self->priv->loop_seek_event[1] = new_loop_event1;
    if (old_loop_event1)
      gst_event_unref (old_loop_event1);
  }
}

static GstElement *
make_audio_sink (const BtMainPageWaves * self)
{
  GstElement *sink = NULL;
  gchar *element_name, *device_name;

  if (bt_settings_determine_audiosink_name (self->priv->settings, &element_name,
          &device_name)) {
    sink = gst_element_factory_make (element_name, NULL);
    if (BT_IS_STRING (device_name)) {
      g_object_set (sink, "device", device_name, NULL);
    }
    g_free (element_name);
    g_free (device_name);
  }
  return sink;
}

static void
update_audio_sink (const BtMainPageWaves * self)
{
  GstState state;
  GstElement *sink;

  // check current state
  if (gst_element_get_state (GST_ELEMENT (self->priv->playbin), &state, NULL,
          0) == GST_STATE_CHANGE_SUCCESS) {
    if (state > GST_STATE_READY) {
      gst_element_set_state (self->priv->playbin, GST_STATE_READY);
    }
  }
  if (self->priv->preview
      && gst_element_get_state (GST_ELEMENT (self->priv->preview), &state, NULL,
          0) == GST_STATE_CHANGE_SUCCESS) {
    if (state > GST_STATE_READY) {
      preview_stop (self);
      gtk_widget_set_sensitive (self->priv->wavetable_stop, FALSE);
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->
              priv->wavetable_play), FALSE);
    }
  }
  // determine sink element
  if ((sink = make_audio_sink (self))) {
    g_object_set (self->priv->playbin, "audio-sink", sink, NULL);

    if (self->priv->preview) {
      GstPad *pad, *peer_pad;

      pad = gst_element_get_static_pad (self->priv->preview_sink, "sink");
      peer_pad = gst_pad_get_peer (pad);
      gst_pad_unlink (peer_pad, pad);
      gst_object_unref (pad);
      gst_element_set_state (self->priv->preview, GST_STATE_NULL);
      gst_bin_remove (GST_BIN (self->priv->preview), self->priv->preview_sink);
      self->priv->preview_sink = make_audio_sink (self);
      gst_bin_add (GST_BIN (self->priv->preview), self->priv->preview_sink);
      pad = gst_element_get_static_pad (self->priv->preview_sink, "sink");
      gst_pad_link (peer_pad, pad);
      gst_object_unref (pad);
    }
  }
}

//-- event handler

static void
refresh_after_wave_list_changes (BtMainPageWaves * self, BtWave * wave)
{
  wavelevels_list_refresh (self, wave);
  if (wave) {
    gdouble volume;
    BtWaveLoopMode loop_mode;

    // enable toolbar buttons
    gtk_widget_set_sensitive (self->priv->wavetable_play, TRUE);
    gtk_widget_set_sensitive (self->priv->wavetable_clear, TRUE);
    // enable and update properties
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->volume), TRUE);
    gtk_widget_set_sensitive (self->priv->loop_mode, TRUE);
    g_object_get (wave, "volume", &volume, "loop-mode", &loop_mode, NULL);

    g_signal_handlers_block_matched (self->priv->volume,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_volume_changed, (gpointer) self);
    gtk_range_set_value (GTK_RANGE (self->priv->volume), volume);
    g_signal_handlers_unblock_matched (self->priv->volume,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_volume_changed, (gpointer) self);

    g_signal_handlers_block_matched (self->priv->loop_mode,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_loop_mode_changed, (gpointer) self);
    gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->loop_mode), loop_mode);
    g_signal_handlers_unblock_matched (self->priv->loop_mode,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_loop_mode_changed, (gpointer) self);
  } else {
    // disable toolbar buttons
    gtk_widget_set_sensitive (self->priv->wavetable_play, FALSE);
    gtk_widget_set_sensitive (self->priv->wavetable_clear, FALSE);
    // disable properties
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->volume), FALSE);
    gtk_widget_set_sensitive (self->priv->loop_mode, FALSE);
  }
  on_wavelevels_list_cursor_changed (self->priv->wavelevels_list, self);
}

static void
on_wave_list_model_row_changed (GtkTreeModel * model, GtkTreePath * path,
    GtkTreeIter * iter, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  wave = bt_wave_list_model_get_object ((BtWaveListModel *) model, iter);
  refresh_after_wave_list_changes (self, wave);
  g_object_try_unref (wave);
}

static void
on_preview_state_changed (GstBus * bus, GstMessage * message,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->preview)) {
    GstState oldstate, newstate, pending;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    GST_INFO ("state change on the bin: %s -> %s",
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate));
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_NULL_TO_READY:
        if (!(gst_element_send_event (GST_ELEMENT (self->priv->preview),
                    gst_event_ref (self->priv->play_seek_event)))) {
          GST_WARNING ("bin failed to handle seek event");
        }
        //gst_element_set_state(self->priv->preview,GST_STATE_PLAYING);
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        gtk_widget_set_sensitive (self->priv->wavetable_stop, TRUE);
        break;
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        gtk_widget_set_sensitive (self->priv->wavetable_stop, FALSE);
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->
                priv->wavetable_play), FALSE);
        gst_element_set_state (self->priv->preview, GST_STATE_NULL);
        break;
      default:
        break;
    }
  }
}

static void
on_preview_eos (GstBus * bus, GstMessage * message, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->preview)) {
    GST_INFO ("received eos on the bin");
    preview_stop (self);
  }
}

static void
on_preview_segment_done (GstBus * bus, GstMessage * message, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->preview)) {
    GST_INFO ("received segment_done on the bin, looping %d",
        self->priv->loop_seek_dir);
    if (!(gst_element_send_event (GST_ELEMENT (self->priv->preview),
                gst_event_ref (self->priv->loop_seek_event[self->
                        priv->loop_seek_dir])))) {
      GST_WARNING ("element failed to handle continuing play seek event");
      /* if I set state directly to NULL, I don't get inbetween state-change messages */
      gst_element_set_state (self->priv->preview, GST_STATE_READY);
    }
    self->priv->loop_seek_dir = 1 - self->priv->loop_seek_dir;
  }
}

static void
on_preview_error (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GError *err = NULL;
  gchar *desc, *dbg = NULL;

  gst_message_parse_error (message, &err, &dbg);
  desc = gst_error_get_message (err->domain, err->code);
  GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "ERROR: %s (%s) (%s)",
      err->message, desc, (dbg ? dbg : "no debug"));
  g_error_free (err);
  g_free (dbg);
  g_free (desc);
  preview_stop (self);
}

static void
on_preview_warning (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GError *err = NULL;
  gchar *desc, *dbg = NULL;

  gst_message_parse_warning (message, &err, &dbg);
  desc = gst_error_get_message (err->domain, err->code);
  GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "WARNING: %s (%s) (%s)",
      err->message, desc, (dbg ? dbg : "no debug"));
  g_error_free (err);
  g_free (dbg);
  g_free (desc);
  preview_stop (self);
}

static void
on_wave_name_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeModel *store;

  if ((store = gtk_tree_view_get_model (self->priv->waves_list))) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_from_string (store, &iter, path_string)) {
      BtWave *wave;

      if ((wave = bt_wave_list_model_get_object ((BtWaveListModel *) store,
                  &iter))) {
        g_object_set (wave, "name", new_text, NULL);
        g_object_unref (wave);
        bt_edit_application_set_song_unsaved (self->priv->app);
      }
    }
  }
}

static void
on_wavelevel_root_note_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeIter iter;
  GtkTreeModel *store = NULL;
  BtWavelevel *wavelevel;

  if ((wavelevel =
          wavelevels_get_wavelevel_and_set_iter (self, &iter, &store,
              path_string))) {
    GstBtNote root_note = gstbt_tone_conversion_note_string_2_number (new_text);

    if (root_note) {
      g_object_set (wavelevel, "root-note", root_note, NULL);
      preview_update_seeks (self);
    }
    g_object_unref (wavelevel);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}

static void
on_wavelevel_rate_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeIter iter;
  GtkTreeModel *store = NULL;
  BtWavelevel *wavelevel;

  if ((wavelevel =
          wavelevels_get_wavelevel_and_set_iter (self, &iter, &store,
              path_string))) {
    gulong rate = atol (new_text);

    if (rate) {
      g_object_set (wavelevel, "rate", rate, NULL);
      preview_update_seeks (self);
    }
    g_object_unref (wavelevel);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}

static void
on_wavelevel_loop_start_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeIter iter;
  GtkTreeModel *store = NULL;
  BtWavelevel *wavelevel;

  if ((wavelevel =
          wavelevels_get_wavelevel_and_set_iter (self, &iter, &store,
              path_string))) {
    gulong loop_start = atol (new_text), loop_end;

    g_object_set (wavelevel, "loop-start", loop_start, NULL);
    g_object_get (wavelevel, "loop-start", &loop_start, "loop-end", &loop_end,
        NULL);
    g_object_set (self->priv->waveform_viewer, "loop-begin",
        (gint64) loop_start, "loop-end", (gint64) loop_end, NULL);
    preview_update_seeks (self);
    g_object_unref (wavelevel);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}

static void
on_wavelevel_loop_end_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeIter iter;
  GtkTreeModel *store = NULL;
  BtWavelevel *wavelevel;

  if ((wavelevel =
          wavelevels_get_wavelevel_and_set_iter (self, &iter, &store,
              path_string))) {
    gulong loop_start, loop_end = atol (new_text);

    g_object_set (wavelevel, "loop-end", loop_end, NULL);
    g_object_get (wavelevel, "loop-start", &loop_start, "loop-end", &loop_end,
        NULL);
    g_object_set (self->priv->waveform_viewer, "loop-begin",
        (gint64) loop_start, "loop-end", (gint64) loop_end, NULL);
    preview_update_seeks (self);
    g_object_unref (wavelevel);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}

static void
on_volume_changed (GtkRange * range, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  if ((wave = waves_list_get_current (self))) {
    gdouble volume = gtk_range_get_value (range);
    g_object_set (wave, "volume", volume, NULL);
    g_object_unref (wave);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}

static void
on_loop_mode_changed (GtkComboBox * menu, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  if ((wave = waves_list_get_current (self))) {
    BtWaveLoopMode loop_mode =
        gtk_combo_box_get_active (GTK_COMBO_BOX (self->priv->loop_mode));
    g_object_set (wave, "loop-mode", loop_mode, NULL);
    g_object_unref (wave);
    preview_update_seeks (self);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}

static void
on_waves_list_cursor_changed (GtkTreeView * treeview, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  GST_INFO ("waves list cursor changed");
  wave = waves_list_get_current (self);
  refresh_after_wave_list_changes (self, wave);
  g_object_try_unref (wave);
}

static void
on_wavelevels_list_cursor_changed (GtkTreeView * treeview, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;
  gboolean drawn = FALSE;

  GST_DEBUG ("wavelevels list cursor changed");
  if ((wave = waves_list_get_current (self))) {
    BtWavelevel *wavelevel;

    if ((wavelevel = wavelevels_list_get_current (self, wave))) {
      int16_t *data;
      guint channels;
      gulong length, loop_start, loop_end;

      g_object_get (wave, "channels", &channels, NULL);
      g_object_get (wavelevel, "length", &length, "loop-start", &loop_start,
          "loop-end", &loop_end, "data", &data, NULL);
      GST_INFO ("select wave-level: %p, %lu", data, length);

      bt_waveform_viewer_set_wave (BT_WAVEFORM_VIEWER (self->
              priv->waveform_viewer), data, channels, length);
      g_object_set (self->priv->waveform_viewer, "loop-begin",
          (int64_t) loop_start, "loop-end", (int64_t) loop_end, NULL);

      g_object_unref (wavelevel);
      drawn = TRUE;
    } else {
      GST_WARNING ("no current wavelevel");
    }
    g_object_unref (wave);
  } else {
    GST_INFO ("no current wave");
  }
  if (!drawn) {
    bt_waveform_viewer_set_wave (BT_WAVEFORM_VIEWER (self->
            priv->waveform_viewer), NULL, 0, 0);
  }
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtSong *song;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  // get song from app and then setup from song
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song)
    return;
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_try_unref (self->priv->wavetable);
  g_object_get (song, "wavetable", &self->priv->wavetable, NULL);
  waves_list_refresh (self);
  // release the references
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

static void
on_toolbar_style_changed (const BtSettings * settings, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkToolbarStyle style;
  gchar *toolbar_style;

  g_object_get ((gpointer) settings, "toolbar-style", &toolbar_style, NULL);
  if (!BT_IS_STRING (toolbar_style))
    return;

  GST_INFO ("!!!  toolbar style has changed '%s'", toolbar_style);
  style = gtk_toolbar_get_style_from_string (toolbar_style);
  gtk_toolbar_set_style (GTK_TOOLBAR (self->priv->list_toolbar), style);
  gtk_toolbar_set_style (GTK_TOOLBAR (self->priv->browser_toolbar), style);
  gtk_toolbar_set_style (GTK_TOOLBAR (self->priv->editor_toolbar), style);
  g_free (toolbar_style);
}

static void
on_default_sample_folder_changed (const BtSettings * settings, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gchar *sample_folder;

  g_object_get ((gpointer) settings, "sample-folder", &sample_folder, NULL);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (self->
          priv->file_chooser), sample_folder);
  g_free (sample_folder);
}

static void
on_browser_preview_sample (GtkFileChooser * chooser, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  // get current entry and play
  if (self->priv->preview_in_chooser) {
    gchar *uri = gtk_file_chooser_get_uri (chooser);
    GST_INFO ("current uri : %s", uri);
    if (uri) {
      // stop previous play
      gst_element_set_state (self->priv->playbin, GST_STATE_READY);

      // ... and play
      g_object_set (self->priv->playbin, "uri", uri, NULL);
      gst_element_set_state (self->priv->playbin, GST_STATE_PLAYING);

      g_free (uri);
    }
  } else {
    // stop previous play
    gst_element_set_state (self->priv->playbin, GST_STATE_READY);
  }
}

#if 0
/* TODO(ensonic): we'd like to have '<' and '>' to go to prev and next wave
 * entry. This would allow us to quickly load wave after wave using the keyboard
 */
static gboolean
on_browser_key_press_event (GtkWidget * widget, GdkEventKey * event,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gboolean res = FALSE;

  if (event->keyval == GDK_less) {
    GST_WARNING ("last wave");
    g_signal_emit_by_name (self->priv->waves_list, "move-cursor",
        GTK_SCROLL_STEP_BACKWARD, NULL);
    res = TRUE;
  } else if (event->keyval == GDK_greater) {
    GST_WARNING ("next wave");
    g_signal_emit_by_name (self->priv->waves_list, "move-cursor",
        GTK_SCROLL_STEP_FORWARD, NULL);
    res = TRUE;
  }
  if (res) {
    g_signal_emit_by_name (self->priv->waves_list, "select-cursor-row",
        TRUE, NULL);
  }
  return (res);
}
#endif

static void
on_browser_toolbar_play_clicked (GtkToolButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  self->priv->preview_in_chooser =
      gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button));

  on_browser_preview_sample ((GtkFileChooser *) self->priv->file_chooser,
      user_data);
}

static gboolean
on_preview_playback_update (gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gint64 pos_cur;

  // query playback position and update playcursor
  if ((gst_element_query (GST_ELEMENT (self->priv->preview_src),
              self->priv->position_query))) {
    //if((gst_element_query(GST_ELEMENT(self->priv->preview),self->priv->position_query))) {
    gst_query_parse_position (self->priv->position_query, NULL, &pos_cur);
    // update play-cursor in samples
    g_object_set (self->priv->waveform_viewer, "playback-cursor", pos_cur,
        NULL);
    /*GST_WARNING_OBJECT(self->priv->preview, "position query returned: %" G_GINT64_FORMAT,pos_cur); */
  } else {
    GST_WARNING_OBJECT (self->priv->preview, "position query failed");
  }
  return (TRUE);
}

static void
on_wavetable_toolbar_play_clicked (GtkToolButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button)))
    return;

  if ((wave = waves_list_get_current (self))) {
    BtWavelevel *wavelevel;

    // get current wavelevel
    if ((wavelevel = wavelevels_list_get_current (self, wave))) {
      GstCaps *caps;
      gulong play_length, play_rate;
      guint play_channels;
      gint16 *play_data;

      // create playback pipeline on demand
      if (!self->priv->preview) {
        GstElement *ares, *aconv;
        GstBus *bus;

        self->priv->preview = gst_element_factory_make ("pipeline", NULL);
        self->priv->preview_src =
            gst_element_factory_make ("memoryaudiosrc", NULL);
        ares = gst_element_factory_make ("audioresample", NULL);
        aconv = gst_element_factory_make ("audioconvert", NULL);
        if (!(self->priv->preview_sink = make_audio_sink (self))) {
          self->priv->preview_sink =
              gst_element_factory_make ("autoaudiosink", NULL);
        }
        gst_bin_add_many (GST_BIN (self->priv->preview),
            self->priv->preview_src, ares, aconv, self->priv->preview_sink,
            NULL);
        gst_element_link_many (self->priv->preview_src, ares, aconv,
            self->priv->preview_sink, NULL);

        bus = gst_element_get_bus (self->priv->preview);
        gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
        g_signal_connect (bus, "message::state-changed",
            G_CALLBACK (on_preview_state_changed), (gpointer) self);
        g_signal_connect (bus, "message::eos", G_CALLBACK (on_preview_eos),
            (gpointer) self);
        g_signal_connect (bus, "message::segment-done",
            G_CALLBACK (on_preview_segment_done), (gpointer) self);
        g_signal_connect (bus, "message::error", G_CALLBACK (on_preview_error),
            (gpointer) self);
        g_signal_connect (bus, "message::warning",
            G_CALLBACK (on_preview_warning), (gpointer) self);
        gst_object_unref (bus);

        self->priv->position_query =
            gst_query_new_position (GST_FORMAT_DEFAULT);
      }
      // get parameters
      g_object_get (wave, "channels", &play_channels, NULL);
      g_object_get (wavelevel,
          "length", &play_length, "rate", &play_rate, "data", &play_data, NULL);
      // build caps
      caps = gst_caps_new_simple ("audio/x-raw-int",
          "rate", G_TYPE_INT, play_rate,
          "channels", G_TYPE_INT, play_channels,
          "width", G_TYPE_INT, 16,
          "endianness", G_TYPE_INT, G_BYTE_ORDER,
          "signedness", G_TYPE_INT, TRUE, NULL);
      g_object_set (self->priv->preview_src,
          "caps", caps, "data", play_data, "length", play_length, NULL);
      gst_caps_unref (caps);

      self->priv->play_wave = wave;
      self->priv->play_wavelevel = wavelevel;

      // build seek events for looping
      preview_update_seeks (self);

      // update playback position 10 times a second
      self->priv->preview_update_id =
          g_timeout_add_full (G_PRIORITY_HIGH, 1000 / 10,
          on_preview_playback_update, (gpointer) self, NULL);

      // set playing
      gst_element_set_state (self->priv->preview, GST_STATE_PLAYING);
      g_object_unref (wavelevel);
    } else {
      GST_WARNING ("no current wavelevel");
    }
    g_object_unref (wave);
  } else {
    GST_WARNING ("no current wave");
  }
}

static void
on_wavetable_toolbar_stop_clicked (GtkToolButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  preview_stop (self);
}

static void
on_wavetable_toolbar_clear_clicked (GtkToolButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->waves_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    BtWave *wave;

    if ((wave = bt_wave_list_model_get_object ((BtWaveListModel *) model,
                &iter))) {
      bt_wavetable_remove_wave (self->priv->wavetable, wave);
      bt_edit_application_set_song_unsaved (self->priv->app);
      // release the references
      g_object_unref (wave);
    }
  }
}

static void
on_file_chooser_load_sample (GtkFileChooser * chooser, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->waves_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    BtSong *song;
    BtWave *wave;
    gchar *uri, *name, *tmp_name, *ext;
    gulong id;

    gtk_tree_model_get (model, &iter, BT_WAVE_LIST_MODEL_INDEX, &id, -1);

    if ((wave = bt_wave_list_model_get_object ((BtWaveListModel *) model,
                &iter))) {
      if (wave == self->priv->play_wave) {
        preview_stop (self);
      }
      g_object_unref (wave);
      wave = NULL;
    }
    // get current entry and load
    uri =
        gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (self->priv->file_chooser));

    g_object_get (self->priv->app, "song", &song, NULL);

    // trim off protocol, path and extension
    tmp_name = g_filename_from_uri (uri, NULL, NULL);
    name = g_path_get_basename (tmp_name);
    if ((ext = strrchr (name, '.')))
      *ext = '\0';
    g_free (tmp_name);
    wave = bt_wave_new (song, name, uri, id, 1.0, BT_WAVE_LOOP_MODE_OFF, 0);
    g_free (name);
    bt_edit_application_set_song_unsaved (self->priv->app);

    // release the references
    g_object_unref (wave);
    g_object_unref (song);
  }
}

static void
on_waves_list_row_activated (GtkTreeView * tree_view, GtkTreePath * path,
    GtkTreeViewColumn * column, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  //on_wavetable_toolbar_play_clicked(GTK_TOOL_BUTTON(self->priv->wavetable_play),user_data);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->
          priv->wavetable_play), TRUE);
}

static void
on_wavelevels_list_row_activated (GtkTreeView * tree_view, GtkTreePath * path,
    GtkTreeViewColumn * column, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  //on_wavetable_toolbar_play_clicked(GTK_TOOL_BUTTON(self->priv->wavetable_play),user_data);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->
          priv->wavetable_play), TRUE);
}

static void
on_audio_sink_changed (const BtSettings * const settings,
    GParamSpec * const arg, gconstpointer const user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  GST_INFO ("audio-sink has changed");
  update_audio_sink (self);
}

static void
on_system_audio_sink_changed (const BtSettings * const settings,
    GParamSpec * const arg, gconstpointer const user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gchar *element_name, *sink_name;

  GST_INFO ("system audio-sink has changed");

  // exchange the machine (only if the system-audiosink is in use)
  bt_settings_determine_audiosink_name (self->priv->settings, &element_name,
      NULL);
  g_object_get ((gpointer) settings, "system-audiosink-name", &sink_name, NULL);

  GST_INFO ("  -> '%s' (sytem_sink is '%s')", element_name, sink_name);
  if (!strcmp (element_name, sink_name)) {
    update_audio_sink (self);
  }
  g_free (sink_name);
  g_free (element_name);
}


//-- helper methods

static void
bt_main_page_waves_init_ui (const BtMainPageWaves * self,
    const BtMainPages * pages)
{
  GtkWidget *vpaned, *hpaned, *box, *box2, *table;
  GtkWidget *tool_item;
  GtkWidget *scrolled_window;
  GtkCellRenderer *renderer;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint i;

  GST_DEBUG ("!!!! self=%p", self);

  gtk_widget_set_name (GTK_WIDGET (self), "wave table view");

  // vpane
  vpaned = gtk_vpaned_new ();
  gtk_container_add (GTK_CONTAINER (self), vpaned);

  //   hpane
  hpaned = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (vpaned), GTK_WIDGET (hpaned), TRUE, FALSE);

  //     vbox (loaded sample list)
  box = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack1 (GTK_PANED (hpaned), GTK_WIDGET (box), FALSE, FALSE);
  //       toolbar
  self->priv->list_toolbar = gtk_toolbar_new ();
  gtk_widget_set_name (self->priv->list_toolbar, "sample list toolbar");

  // add buttons (play,stop,clear)
  self->priv->wavetable_play = tool_item =
      GTK_WIDGET (gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY));
  gtk_widget_set_name (tool_item, "Play");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (tool_item),
      _("Play current wave table entry as C-4"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->list_toolbar),
      GTK_TOOL_ITEM (tool_item), -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_wavetable_toolbar_play_clicked), (gpointer) self);
  gtk_widget_set_sensitive (tool_item, FALSE);

  self->priv->wavetable_stop = tool_item =
      GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_STOP));
  gtk_widget_set_name (tool_item, "Stop");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (tool_item),
      _("Stop playback of current wave table entry"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->list_toolbar),
      GTK_TOOL_ITEM (tool_item), -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_wavetable_toolbar_stop_clicked), (gpointer) self);
  gtk_widget_set_sensitive (tool_item, FALSE);

  self->priv->wavetable_clear = tool_item =
      GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_CLEAR));
  gtk_widget_set_name (tool_item, "Clear");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (tool_item),
      _("Clear current wave table entry"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->list_toolbar),
      GTK_TOOL_ITEM (tool_item), -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_wavetable_toolbar_clear_clicked), (gpointer) self);
  gtk_widget_set_sensitive (tool_item, FALSE);

  gtk_box_pack_start (GTK_BOX (box), self->priv->list_toolbar, FALSE, FALSE, 0);

  //       wave listview
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_ETCHED_IN);
  self->priv->waves_list = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_widget_set_name (GTK_WIDGET (self->priv->waves_list), "wave-list");
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (self->
          priv->waves_list), GTK_SELECTION_BROWSE);
  g_object_set (self->priv->waves_list, "enable-search", FALSE, "rules-hint",
      TRUE, NULL);
  g_signal_connect (self->priv->waves_list, "cursor-changed",
      G_CALLBACK (on_waves_list_cursor_changed), (gpointer) self);
  g_signal_connect (self->priv->waves_list, "row-activated",
      G_CALLBACK (on_waves_list_row_activated), (gpointer) self);

  renderer = gtk_cell_renderer_text_new ();
  // this seems to cause clipping on some theme+font combinations
  //gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "xalign", 1.0, "width-chars", 2, NULL);
  /* Ix: index in wavetable */
  gtk_tree_view_insert_column_with_attributes (self->priv->waves_list, -1,
      _("Ix"), renderer, "text", BT_WAVE_LIST_MODEL_HEX_ID, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
  g_signal_connect (renderer, "edited", G_CALLBACK (on_wave_name_edited),
      (gpointer) self);
  gtk_tree_view_insert_column_with_attributes (self->priv->waves_list, -1,
      _("Wave"), renderer, "text", BT_WAVE_LIST_MODEL_NAME, "editable",
      BT_WAVE_LIST_MODEL_HAS_WAVE, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (self->priv->waves_list));
  gtk_container_add (GTK_CONTAINER (box), scrolled_window);

  // loop-mode combo and volume slider
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_attach (GTK_TABLE (table), gtk_label_new (_("Volume")), 0, 1, 0, 1,
      GTK_FILL, GTK_SHRINK, 2, 1);
  self->priv->volume = GTK_HSCALE (gtk_hscale_new_with_range (0.0, 1.0, 0.01));
  gtk_scale_set_value_pos (GTK_SCALE (self->priv->volume), GTK_POS_RIGHT);
  g_signal_connect (self->priv->volume, "value-changed",
      G_CALLBACK (on_volume_changed), (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (self->priv->volume), 1, 2, 0,
      1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  gtk_table_attach (GTK_TABLE (table), gtk_label_new (_("Loop")), 0, 1, 1, 2,
      GTK_FILL, GTK_SHRINK, 2, 1);
  self->priv->loop_mode = gtk_combo_box_text_new ();
  // g_type_class_peek_static() returns NULL :/
  enum_class = g_type_class_ref (BT_TYPE_WAVE_LOOP_MODE);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->
              priv->loop_mode), enum_value->value_name);
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->loop_mode),
      BT_WAVE_LOOP_MODE_OFF);
  g_signal_connect (self->priv->loop_mode, "changed",
      G_CALLBACK (on_loop_mode_changed), (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), self->priv->loop_mode, 1, 2, 1, 2,
      GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);

  //     vbox (file browser)
  box = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack2 (GTK_PANED (hpaned), GTK_WIDGET (box), TRUE, FALSE);
  //       toolbar
  self->priv->browser_toolbar = gtk_toolbar_new ();
  gtk_widget_set_name (self->priv->browser_toolbar, "sample browser toolbar");

  // add buttons (play,load)
  self->priv->browser_play = tool_item =
      GTK_WIDGET (gtk_toggle_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY));
  gtk_widget_set_name (tool_item, "Play");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (tool_item),
      _("Play current sample"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->browser_toolbar),
      GTK_TOOL_ITEM (tool_item), -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_browser_toolbar_play_clicked), (gpointer) self);

  tool_item = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_OPEN));
  gtk_widget_set_name (tool_item, "Open");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (tool_item),
      _("Load current sample into selected wave table entry"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->browser_toolbar),
      GTK_TOOL_ITEM (tool_item), -1);
  //g_signal_connect(tool_item,"clicked",G_CALLBACK(on_browser_toolbar_open_clicked),(gpointer)self);

  gtk_box_pack_start (GTK_BOX (box), self->priv->browser_toolbar, FALSE, FALSE,
      0);

  //       file-chooser
  /* this causes warning on gtk 2.x
   * Gtk-CRITICAL **: gtk_file_system_path_is_local: assertion `path != NULL' failed
   * Gdk-CRITICAL **: gdk_drawable_get_size: assertion `GDK_IS_DRAWABLE (drawable)' failed
   */
  self->priv->file_chooser =
      gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
  g_signal_connect (self->priv->file_chooser, "file-activated",
      G_CALLBACK (on_file_chooser_load_sample), (gpointer) self);
  g_signal_connect (self->priv->file_chooser, "selection-changed",
      G_CALLBACK (on_browser_preview_sample), (gpointer) self);
  /*
     g_signal_connect (self->priv->file_chooser, "key-press-event",
     G_CALLBACK (on_browser_key_press_event), (gpointer) self);
   */
  gtk_box_pack_start (GTK_BOX (box), self->priv->file_chooser, TRUE, TRUE,
      BOX_BORDER);

  //   vbox (sample view)
  box = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack2 (GTK_PANED (vpaned), GTK_WIDGET (box), FALSE, FALSE);
  //     toolbar
  self->priv->editor_toolbar = gtk_toolbar_new ();
  gtk_widget_set_name (self->priv->editor_toolbar, "sample edit toolbar");

  gtk_box_pack_start (GTK_BOX (box), self->priv->editor_toolbar, FALSE, FALSE,
      0);

  //     hbox
  box2 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (box), box2);
  //       zone entries (multiple waves per sample (xm?)) -> (per entry: root key, length, rate, loope start, loop end
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_ETCHED_IN);
  self->priv->wavelevels_list = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_widget_set_name (GTK_WIDGET (self->priv->wavelevels_list),
      "wave-level-list");
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (self->
          priv->wavelevels_list), GTK_SELECTION_BROWSE);
  g_object_set (self->priv->wavelevels_list, "enable-search", FALSE,
      "rules-hint", TRUE, NULL);
  g_signal_connect (self->priv->wavelevels_list, "cursor-changed",
      G_CALLBACK (on_wavelevels_list_cursor_changed), (gpointer) self);
  g_signal_connect (self->priv->wavelevels_list, "row-activated",
      G_CALLBACK (on_wavelevels_list_row_activated), (gpointer) self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, "editable",
      TRUE, NULL);
  gtk_tree_view_insert_column_with_attributes (self->priv->wavelevels_list, -1,
      _("Root"), renderer, "text", BT_WAVELEVEL_LIST_MODEL_ROOT_NOTE, NULL);
  g_signal_connect (renderer, "edited",
      G_CALLBACK (on_wavelevel_root_note_edited), (gpointer) self);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_tree_view_insert_column_with_attributes (self->priv->wavelevels_list, -1,
      _("Length"), renderer, "text", BT_WAVELEVEL_LIST_MODEL_LENGTH, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, "editable",
      TRUE, NULL);
  gtk_tree_view_insert_column_with_attributes (self->priv->wavelevels_list, -1,
      _("Rate"), renderer, "text", BT_WAVELEVEL_LIST_MODEL_RATE, NULL);
  g_signal_connect (renderer, "edited", G_CALLBACK (on_wavelevel_rate_edited),
      (gpointer) self);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, "editable",
      TRUE, NULL);
  gtk_tree_view_insert_column_with_attributes (self->priv->wavelevels_list, -1,
      _("Loop start"), renderer, "text", BT_WAVELEVEL_LIST_MODEL_LOOP_START,
      NULL);
  g_signal_connect (renderer, "edited",
      G_CALLBACK (on_wavelevel_loop_start_edited), (gpointer) self);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, "editable",
      TRUE, NULL);
  gtk_tree_view_insert_column_with_attributes (self->priv->wavelevels_list, -1,
      _("Loop end"), renderer, "text", BT_WAVELEVEL_LIST_MODEL_LOOP_END, NULL);
  g_signal_connect (renderer, "edited",
      G_CALLBACK (on_wavelevel_loop_end_edited), (gpointer) self);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (self->priv->wavelevels_list));
  gtk_box_pack_start (GTK_BOX (box2), scrolled_window, FALSE, FALSE, 0);

  //       tabs for waveform/envelope? or envelope overlayed?
  //       sampleview
  self->priv->waveform_viewer = bt_waveform_viewer_new ();
  gtk_widget_set_size_request (self->priv->waveform_viewer, -1, 96);
  gtk_box_pack_start (GTK_BOX (box2), self->priv->waveform_viewer, TRUE, TRUE,
      0);

  // register event handlers
  g_signal_connect (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self);

  // let settings control toolbar style and listen to other settings changes
  on_toolbar_style_changed (self->priv->settings, NULL, (gpointer) self);
  on_default_sample_folder_changed (self->priv->settings, NULL,
      (gpointer) self);
  g_signal_connect (self->priv->settings, "notify::toolbar-style",
      G_CALLBACK (on_toolbar_style_changed), (gpointer) self);
  g_signal_connect (self->priv->settings, "notify::sample-folder",
      G_CALLBACK (on_default_sample_folder_changed), (gpointer) self);

  GST_DEBUG ("  done");
}

//-- constructor methods

/**
 * bt_main_page_waves_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainPageWaves *
bt_main_page_waves_new (const BtMainPages * pages)
{
  BtMainPageWaves *self;

  self = BT_MAIN_PAGE_WAVES (g_object_new (BT_TYPE_MAIN_PAGE_WAVES, NULL));
  self->priv->settings = bt_settings_make ();
  bt_main_page_waves_init_ui (self, pages);

  // create playbin
  self->priv->playbin = gst_element_factory_make ("playbin2", NULL);

  // watch settings changes
  g_signal_connect (self->priv->settings, "notify::audiosink",
      G_CALLBACK (on_audio_sink_changed), (gpointer) self);
  g_signal_connect (self->priv->settings, "notify::system-audiosink",
      G_CALLBACK (on_system_audio_sink_changed), (gpointer) self);
  update_audio_sink (self);

  return (self);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_main_page_waves_dispose (GObject * object)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_signal_handlers_disconnect_matched (self->priv->settings,
      G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (gpointer) self);
  g_object_unref (self->priv->settings);

  g_signal_handlers_disconnect_matched (self->priv->app, G_SIGNAL_MATCH_FUNC, 0,
      0, NULL, on_song_changed, NULL);

  g_object_unref (self->priv->n2f);
  g_object_try_unref (self->priv->wavetable);
  g_object_unref (self->priv->app);

  // shut down loader-preview playbin
  gst_element_set_state (self->priv->playbin, GST_STATE_NULL);
  gst_object_unref (self->priv->playbin);

  // shut down wavetable-preview playbin
  if (self->priv->preview) {
    GstBus *bus;

    preview_stop (self);
    gst_element_set_state (self->priv->preview, GST_STATE_NULL);
    bus = gst_element_get_bus (GST_ELEMENT (self->priv->preview));
    g_signal_handlers_disconnect_matched (bus, G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        NULL, (gpointer) self);
    gst_bus_remove_signal_watch (bus);
    gst_object_unref (bus);
    gst_object_unref (self->priv->preview);

    if (self->priv->play_seek_event)
      gst_event_unref (self->priv->play_seek_event);
    if (self->priv->loop_seek_event[0])
      gst_event_unref (self->priv->loop_seek_event[0]);
    if (self->priv->loop_seek_event[1])
      gst_event_unref (self->priv->loop_seek_event[1]);
    gst_query_unref (self->priv->position_query);
  }

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_waves_parent_class)->dispose (object);
}

static void
bt_main_page_waves_init (BtMainPageWaves * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MAIN_PAGE_WAVES,
      BtMainPageWavesPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
  self->priv->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
}

static void
bt_main_page_waves_class_init (BtMainPageWavesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtMainPageWavesPrivate));

  gobject_class->dispose = bt_main_page_waves_dispose;
}
