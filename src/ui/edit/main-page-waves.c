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
/* TODO(ensonic): once we have selections in waveform-viewer, add tools:
 * - selection to new wave: copy selection to next free wave
 *   - do a lazy copy to safe extra memory?
 * - zoom to selection
 *   - nned toolbar for the commands (e.v. to the right to save space
 */
/* IDEA(ensonic): allow to paste samples from other apps
 * - audacity does not place samples snippets in the clipboard
 */
#define BT_EDIT
#define BT_MAIN_PAGE_WAVES_C

#include "bt-edit.h"
#include <gst/audio/audio.h>
#include "gst/toneconversion.h"

struct _BtMainPageWaves
{
  GtkBox parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the wavetable we are showing */
  BtWavetable *wavetable;
  /* the waveform graph widget */
  GtkWidget *waveform_viewer;

  /* the toolbar widgets */
  GtkWidget *list_toolbar, *browser_toolbar;
  GtkWidget *wavetable_stop;
  GtkWidget *browser_play, *wavetable_play, *wavetable_clear;

  /* the list of wavetable entries */
  GtkColumnView *waves_list;
  /* and their parameters */
  GtkWidget *volume;
  GtkDropDown *loop_mode;

  /* the list of wavelevels */
  GtkColumnView *wavelevels_list;
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

G_DEFINE_TYPE (BtMainPageWaves, bt_main_page_waves, GTK_TYPE_BOX);


//-- event handler helper

static void on_waves_list_selection_changed (GObject *object, GParamSpec *spec, gpointer user_data) ;
static void on_wavelevels_list_cursor_changed (GObject *object, GParamSpec *spec, gpointer user_data);
static void on_volume_changed (GtkRange * range, gpointer user_data);
static void on_loop_mode_changed (GtkComboBox * menu, gpointer user_data);

static void
set_loop_mode_drop_down (BtMainPageWaves *self, BtWaveLoopMode loop_mode_enum)
{
    AdwEnumListModel *enum_model =
      ADW_ENUM_LIST_MODEL (
        gtk_single_selection_get_model (GTK_SINGLE_SELECTION (gtk_drop_down_get_model (self->loop_mode))));
    
    gtk_drop_down_set_selected(
        self->loop_mode,
        adw_enum_list_model_find_position(enum_model, loop_mode_enum));
}

// (transfer none)
static BtWave *
waves_list_get_current (const BtMainPageWaves * self)
{
  GObject* item = gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (gtk_column_view_get_model (self->waves_list)));
  
  return item ? BT_WAVE (item) : NULL;
}

static gulong
waves_list_get_selected_idx (const BtMainPageWaves * self)
{
  return gtk_single_selection_get_selected (
    GTK_SINGLE_SELECTION (gtk_column_view_get_model (self->waves_list)));
}

static gulong
waves_list_get_count (const BtMainPageWaves * self)
{
  return g_list_model_get_n_items (
    gtk_single_selection_get_model (
      GTK_SINGLE_SELECTION (gtk_column_view_get_model (self->waves_list))));
}

static BtWavelevel *
wavelevels_list_get_current (const BtMainPageWaves * self, BtWave * wave)
{
  GObject* item = gtk_single_selection_get_selected_item (
    GTK_SINGLE_SELECTION (gtk_column_view_get_model (self->wavelevels_list)));
  
  return item ? BT_WAVELEVEL (item) : NULL;
}

static void
preview_stop (BtMainPageWaves * self)
{
  if (self->preview_update_id) {
    g_source_remove (self->preview_update_id);
    self->preview_update_id = 0;
    g_object_set (self->waveform_viewer, "playback-cursor",
        G_GINT64_CONSTANT (-1), NULL);
  }
  /* if I set state directly to NULL, I don't get inbetween state-change messages */
  gst_element_set_state (self->preview, GST_STATE_READY);
  self->play_wave = NULL;
  self->play_wavelevel = NULL;

  /* untoggle play button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->wavetable_play), FALSE);
}

static void
preview_update_seeks (BtMainPageWaves * self)
{
  if (self->play_wave) {
    GstEvent *old_play_event, *new_play_event;
    GstEvent *old_loop_event0, *new_loop_event0;
    GstEvent *old_loop_event1, *new_loop_event1;
    BtWaveLoopMode loop_mode;
    gulong loop_start, loop_end;
    gulong length, srate;
    GstBtNote root_note;
    gdouble prate;

    /* get parameters */
    g_object_get (self->play_wave, "loop-mode", &loop_mode, NULL);
    g_object_get (self->play_wavelevel,
        "root-note", &root_note,
        "length", &length,
        "loop-start", &loop_start, "loop-end", &loop_end, "rate", &srate, NULL);

    /* calculate pitch rate from root-note */
    prate =
        gstbt_tone_conversion_translate_from_number (self->n2f,
        BT_WAVELEVEL_DEFAULT_ROOT_NOTE) /
        gstbt_tone_conversion_translate_from_number (self->n2f,
        root_note);

    old_play_event = self->play_seek_event;
    old_loop_event0 = self->loop_seek_event[0];
    old_loop_event1 = self->loop_seek_event[1];
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
    self->play_seek_event = new_play_event;
    if (old_play_event)
      gst_event_unref (old_play_event);
    self->loop_seek_event[0] = new_loop_event0;
    if (old_loop_event0)
      gst_event_unref (old_loop_event0);
    self->loop_seek_event[1] = new_loop_event1;
    if (old_loop_event1)
      gst_event_unref (old_loop_event1);
  }
}

static GstElement *
make_audio_sink (const BtMainPageWaves * self)
{
  GstElement *sink = NULL;
  gchar *element_name, *device_name;

  if (bt_settings_determine_audiosink_name (self->settings, &element_name,
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
update_audio_sink (BtMainPageWaves * self)
{
  GstState state;
  GstElement *sink;

  // check current state
  if (self->playbin
      && gst_element_get_state (GST_ELEMENT (self->playbin), &state, NULL,
          0) == GST_STATE_CHANGE_SUCCESS) {
    if (state > GST_STATE_READY) {
      gst_element_set_state (self->playbin, GST_STATE_READY);
    }
  }
  if (self->preview
      && gst_element_get_state (GST_ELEMENT (self->preview), &state, NULL,
          0) == GST_STATE_CHANGE_SUCCESS) {
    if (state > GST_STATE_READY) {
      preview_stop (self);
      gtk_widget_set_sensitive (self->wavetable_stop, FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->wavetable_play), FALSE);
    }
  }
  // determine sink element
  if ((sink = make_audio_sink (self))) {
    if (self->playbin) {
      g_object_set (self->playbin, "audio-sink", sink, NULL);
    }
    if (self->preview) {
      GstPad *pad, *peer_pad;
      GstPadLinkReturn plr;

      pad = gst_element_get_static_pad (self->preview_sink, "sink");
      peer_pad = gst_pad_get_peer (pad);
      gst_pad_unlink (peer_pad, pad);
      gst_object_unref (pad);
      gst_element_set_state (self->preview, GST_STATE_NULL);
      gst_bin_remove (GST_BIN (self->preview), self->preview_sink);
      self->preview_sink = make_audio_sink (self);
      gst_bin_add (GST_BIN (self->preview), self->preview_sink);
      pad = gst_element_get_static_pad (self->preview_sink, "sink");
      if (GST_PAD_LINK_FAILED (plr = gst_pad_link (peer_pad, pad))) {
        GST_WARNING_OBJECT (self, "can't link %s:%s with %s:%s: %d",
            GST_DEBUG_PAD_NAME (peer_pad), GST_DEBUG_PAD_NAME (pad), plr);
        GST_WARNING_OBJECT (self, "%s",
            bt_gst_debug_pad_link_return (plr, peer_pad, pad));
      }
      gst_object_unref (pad);
    }
  }
}

//-- event handler

static void
refresh_after_wave_list_changes (BtMainPageWaves * self, BtWave * wave)
{
  if (wave) {
    gdouble volume;
    BtWaveLoopMode loop_mode;

    // enable toolbar buttons
    gtk_widget_set_sensitive (self->wavetable_play, TRUE);
    gtk_widget_set_sensitive (self->wavetable_clear, TRUE);
    // enable and update properties
    gtk_widget_set_sensitive (self->volume, TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->loop_mode), TRUE);
    g_object_get (wave, "volume", &volume, "loop-mode", &loop_mode, NULL);

    g_signal_handlers_block_matched (self->volume,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_volume_changed, (gpointer) self);
    gtk_range_set_value (GTK_RANGE (self->volume), volume);
    g_signal_handlers_unblock_matched (self->volume,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_volume_changed, (gpointer) self);

    g_signal_handlers_block_matched (self->loop_mode,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_loop_mode_changed, (gpointer) self);

    set_loop_mode_drop_down (self, loop_mode);

    g_signal_handlers_unblock_matched (self->loop_mode,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_loop_mode_changed, (gpointer) self);
  } else {
    // disable toolbar buttons
    gtk_widget_set_sensitive (self->wavetable_play, FALSE);
    gtk_widget_set_sensitive (self->wavetable_clear, FALSE);
    // disable properties
    gtk_widget_set_sensitive (self->volume, FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->loop_mode), FALSE);
  }
  on_wavelevels_list_cursor_changed (G_OBJECT (self), NULL, NULL);
}

static void
on_preview_state_changed (GstBus * bus, GstMessage * message,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->preview)) {
    GstState oldstate, newstate, pending;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    GST_INFO ("state change on the bin: %s -> %s",
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate));
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
        //case GST_STATE_CHANGE_NULL_TO_READY:
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        if (!(gst_element_send_event (GST_ELEMENT (self->preview),
                    gst_event_ref (self->play_seek_event)))) {
          GST_WARNING ("bin failed to handle seek event");
        }
        gst_element_set_state (self->preview, GST_STATE_PLAYING);
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        gtk_widget_set_sensitive (self->wavetable_stop, TRUE);
        break;
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        gtk_widget_set_sensitive (self->wavetable_stop, FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->wavetable_play), FALSE);
        gst_element_set_state (self->preview, GST_STATE_NULL);
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

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->preview)) {
    GST_INFO ("received eos on the bin");
    preview_stop (self);
  }
}

static void
on_preview_segment_done (GstBus * bus, GstMessage * message, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->preview)) {
    GST_INFO ("received segment_done on the bin, looping %d",
        self->loop_seek_dir);
    if (!(gst_element_send_event (GST_ELEMENT (self->preview),
                gst_event_ref (self->loop_seek_event[self->loop_seek_dir])))) {
      GST_WARNING ("element failed to handle continuing play seek event");
      /* if I set state directly to NULL, I don't get inbetween state-change messages */
      gst_element_set_state (self->preview, GST_STATE_READY);
    }
    self->loop_seek_dir = 1 - self->loop_seek_dir;
  }
}

static void
on_preview_error (const GstBus * const bus, GstMessage * message,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  BT_GST_LOG_MESSAGE_ERROR (message, NULL, NULL);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (self->preview),
      GST_DEBUG_GRAPH_SHOW_ALL, PACKAGE_NAME "-waves");

  preview_stop (self);
}

static void
on_preview_warning (const GstBus * const bus, GstMessage * message,
    gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  BT_GST_LOG_MESSAGE_WARNING (message, NULL, NULL);

  preview_stop (self);
}

static void
on_wave_name_edited (GtkEditable* editable, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave = BT_WAVE (g_object_get_data (G_OBJECT (editable), "bt-wave"));
  
  g_object_set (wave, "name", gtk_editable_get_text (editable), NULL);
  bt_edit_application_set_song_unsaved (self->app);
}

static void
on_wavelevel_root_note_edited (GtkEditable* editable, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWavelevel *wavelevel = BT_WAVELEVEL (g_object_get_data (G_OBJECT (editable), "bt-wavelevel"));

  GstBtNote root_note = gstbt_tone_conversion_note_string_2_number (gtk_editable_get_text (editable));

  if (root_note) {
    g_object_set (wavelevel, "root-note", root_note, NULL);
    preview_update_seeks (self);
  }
  bt_edit_application_set_song_unsaved (self->app);
}

static void
on_wavelevel_rate_edited (GtkEditable* editable, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWavelevel *wavelevel = BT_WAVELEVEL (g_object_get_data (G_OBJECT (editable), "bt-wavelevel"));

  gulong rate = atol (gtk_editable_get_text (editable));

  if (rate) {
    g_object_set (wavelevel, "rate", rate, NULL);
    preview_update_seeks (self);
  }
  bt_edit_application_set_song_unsaved (self->app);
}

static void
on_wavelevel_loop_start_edited (GtkEditable* editable, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWavelevel *wavelevel = BT_WAVELEVEL (g_object_get_data (G_OBJECT (editable), "bt-wavelevel"));

  gulong loop_start = atol (gtk_editable_get_text (editable)), loop_end;

  g_object_set (wavelevel, "loop-start", loop_start, NULL);
  g_object_get (wavelevel, "loop-start", &loop_start, "loop-end", &loop_end,
      NULL);
  g_object_set (self->waveform_viewer, "loop-start",
      (gint64) loop_start, "loop-end", (gint64) loop_end, NULL);
  preview_update_seeks (self);
  bt_edit_application_set_song_unsaved (self->app);
}

static void
on_wavelevel_loop_end_edited (GtkEditable* editable, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWavelevel *wavelevel = BT_WAVELEVEL (g_object_get_data (G_OBJECT (editable), "bt-wavelevel"));

  gulong loop_start, loop_end = atol (gtk_editable_get_text (editable));

  g_object_set (wavelevel, "loop-end", loop_end, NULL);
  g_object_get (wavelevel, "loop-start", &loop_start, "loop-end", &loop_end,
      NULL);
  g_object_set (self->waveform_viewer, "loop-start",
      (gint64) loop_start, "loop-end", (gint64) loop_end, NULL);
  preview_update_seeks (self);
  bt_edit_application_set_song_unsaved (self->app);
}

static void
on_volume_changed (GtkRange * range, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  if ((wave = waves_list_get_current (self))) {
    gdouble volume = gtk_range_get_value (range);
    g_object_set (wave, "volume", volume, NULL);
    bt_edit_application_set_song_unsaved (self->app);
  }
}

static void
on_loop_mode_changed (GtkComboBox * menu, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  if ((wave = waves_list_get_current (self))) {
    BtWaveLoopMode loop_mode = gtk_drop_down_get_selected (self->loop_mode);
    gint64 loop_start = -1, loop_end = -1;

    g_object_set (wave, "loop-mode", loop_mode, NULL);
    if (loop_mode != BT_WAVE_LOOP_MODE_OFF) {
      BtWavelevel *wavelevel;

      if ((wavelevel = wavelevels_list_get_current (self, wave))) {
        gulong ls, le;
        g_object_get (wavelevel, "loop-start", &ls, "loop-end", &le, NULL);
        loop_start = ls;
        loop_end = le;
        g_object_unref (wavelevel);
      }
    }
    g_object_set (self->waveform_viewer, "loop-start", loop_start,
        "loop-end", loop_end, NULL);
    preview_update_seeks (self);
    bt_edit_application_set_song_unsaved (self->app);
  }
}

static void
on_waves_list_selection_changed (GObject *object, GParamSpec *spec, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  return_if_disposed_self ();

  GST_INFO ("waves list selection changed");
  wave = waves_list_get_current (self);
  if (wave)
    refresh_after_wave_list_changes (self, wave);
}

static void
on_wavelevels_list_cursor_changed (GObject *object, GParamSpec *spec, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;
  gboolean drawn = FALSE;

  return_if_disposed_self ();

  GST_DEBUG ("wavelevels list cursor changed");
  if ((wave = waves_list_get_current (self))) {
    BtWavelevel *wavelevel;

    if ((wavelevel = wavelevels_list_get_current (self, wave))) {
      int16_t *data;
      guint channels;
      gulong length;
      gint64 loop_start = -1, loop_end = -1;
      BtWaveLoopMode loop_mode;

      g_object_get (wave, "channels", &channels, "loop-mode", &loop_mode, NULL);
      g_object_get (wavelevel, "length", &length, "data", &data, NULL);
      GST_INFO ("select wave-level: %p, %lu", data, length);

      if (loop_mode != BT_WAVE_LOOP_MODE_OFF) {
        gulong ls, le;
        g_object_get (wavelevel, "loop-start", &ls, "loop-end", &le, NULL);
        loop_start = ls;
        loop_end = le;
      }

      bt_waveform_viewer_set_wave (BT_WAVEFORM_VIEWER (self->waveform_viewer), data, channels, length);
      g_object_set (self->waveform_viewer, "loop-start", loop_start,
          "loop-end", loop_end, NULL);

      g_object_unref (wavelevel);
      drawn = TRUE;
    } else {
      GST_WARNING ("no current wavelevel");
    }
  } else {
    GST_INFO ("no current wave");
  }
  if (!drawn) {
    bt_waveform_viewer_set_wave (BT_WAVEFORM_VIEWER (self->waveform_viewer), NULL, 0, 0);
  }
}

static void
on_waveform_viewer_loop_start_changed (GtkWidget * waveform_viewer,
    GParamSpec * arg, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;
  BtWavelevel *wavelevel;

  if ((wave = waves_list_get_current (self))) {
    if ((wavelevel = wavelevels_list_get_current (self, wave))) {
      gint64 loop_start;

      g_object_get (waveform_viewer, "loop-start", &loop_start, NULL);
      if (loop_start != -1) {
        g_object_set (wavelevel, "loop-start", (gulong) loop_start, NULL);

        preview_update_seeks (self);
        bt_edit_application_set_song_unsaved (self->app);
      }
      g_object_unref (wavelevel);
    }
  }
}

static void
on_waveform_viewer_loop_end_changed (GtkWidget * waveform_viewer,
    GParamSpec * arg, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;
  BtWavelevel *wavelevel;

  if ((wave = waves_list_get_current (self))) {
    if ((wavelevel = wavelevels_list_get_current (self, wave))) {
      gint64 loop_end;

      g_object_get (waveform_viewer, "loop-end", &loop_end, NULL);
      if (loop_end != -1) {
        g_object_set (wavelevel, "loop-end", (gulong) loop_end, NULL);

        preview_update_seeks (self);
        bt_edit_application_set_song_unsaved (self->app);
      }
      g_object_unref (wavelevel);
    }
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
  g_object_get (self->app, "song", &song, NULL);
  if (!song)
    return;
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_try_unref (self->wavetable);
  g_object_get (song, "wavetable", &self->wavetable, NULL);
  // release the references
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

static void
on_default_sample_folder_changed (const BtSettings * settings, GParamSpec * arg,
    gpointer user_data)
{
#if 0 /// GTK4
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gchar *sample_folder;

  g_object_get ((gpointer) settings, "sample-folder", &sample_folder, NULL);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (self->file_chooser), sample_folder);
  g_free (sample_folder);
#endif
}

static void
on_browser_preview_sample (GtkFileChooser * chooser, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  if (!self->playbin)
    return;

  // get current entry and play
  if (self->preview_in_chooser) {
    gchar *uri = NULL; /// GTK4 gtk_file_chooser_get_uri (chooser);
    GST_INFO ("current uri : %s", uri);
    if (uri) {
      // stop previous play
      /* FIXME(ensonic): this sometimes hangs
         "failed to link pad uridecodebin0:src_0 to combiner, reason -2:was linked"
         gst_element_set_state (self->playbin, GST_STATE_READY);
         if (gst_element_get_state (self->playbin, NULL, NULL,
         GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS) {
         GST_WARNING_OBJECT (self->playbin, "not yet paused");
         }
       */
      gst_element_set_state (self->playbin, GST_STATE_NULL);

      // ... and play
      g_object_set (self->playbin, "uri", uri, NULL);
      gst_element_set_state (self->playbin, GST_STATE_PLAYING);

      g_free (uri);
    }
  } else {
    // stop previous play
    gst_element_set_state (self->playbin, GST_STATE_READY);
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

  if (event->keyval == GDK_KEY_less) {
    GST_WARNING ("last wave");
    g_signal_emit_by_name (self->waves_list, "move-cursor",
        GTK_SCROLL_STEP_BACKWARD, NULL);
    res = TRUE;
  } else if (event->keyval == GDK_KEY_greater) {
    GST_WARNING ("next wave");
    g_signal_emit_by_name (self->waves_list, "move-cursor",
        GTK_SCROLL_STEP_FORWARD, NULL);
    res = TRUE;
  }
  if (res) {
    g_signal_emit_by_name (self->waves_list, "select-cursor-row",
        TRUE, NULL);
  }
  return res;
}
#endif

static void
on_browser_toolbar_play_clicked (GtkToggleButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  self->preview_in_chooser =
      gtk_toggle_button_get_active (button);

  on_browser_preview_sample ((GtkFileChooser *) self->file_chooser,
      user_data);
}

static gboolean
on_preview_playback_update (gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gint64 pos_cur;

  // query playback position and update playcursor
  // we could also query from the sink or the pipeline, on the source we can define
  // the granularity by the chuck size we deliver the audio
  if ((gst_element_query (GST_ELEMENT (self->preview_src),
              self->position_query))) {
    gst_query_parse_position (self->position_query, NULL, &pos_cur);
    // update play-cursor in samples
    g_object_set (self->waveform_viewer, "playback-cursor", pos_cur,
        NULL);
    /*
       GST_WARNING_OBJECT (self->preview, "position query returned: %"
       G_GINT64_FORMAT, pos_cur);
     */
  } else {
    GST_WARNING_OBJECT (self->preview, "position query failed");
  }
  return TRUE;
}

static void
on_wavetable_toolbar_play_clicked (GtkToggleButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return;

  if ((wave = waves_list_get_current (self))) {
    BtWavelevel *wavelevel;

    // get current wavelevel
    if ((wavelevel = wavelevels_list_get_current (self, wave))) {
      GstCaps *caps;
      GstStateChangeReturn scr;
      gulong play_length, play_rate;
      guint play_channels;
      gint16 *play_data;

      // create playback pipeline on demand
      if (!self->preview) {
        GstElement *ares, *aconv;
        GstBus *bus;

        self->preview = gst_element_factory_make ("pipeline", NULL);
        self->preview_src =
            gst_element_factory_make ("memoryaudiosrc", NULL);
        ares = gst_element_factory_make ("audioresample", NULL);
        aconv = gst_element_factory_make ("audioconvert", NULL);
        if (!(self->preview_sink = make_audio_sink (self))) {
          GST_WARNING ("fallback to autoaudiosink");
          self->preview_sink =
              gst_element_factory_make ("autoaudiosink", NULL);
        }
        gst_bin_add_many (GST_BIN (self->preview),
            self->preview_src, ares, aconv, self->preview_sink,
            NULL);
        if (!gst_element_link_many (self->preview_src, ares, aconv,
                self->preview_sink, NULL)) {
          GST_WARNING ("failed to link playback elements");
        }

        bus = gst_element_get_bus (self->preview);
        gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
        g_signal_connect_object (bus, "message::state-changed",
            G_CALLBACK (on_preview_state_changed), (gpointer) self, 0);
        g_signal_connect_object (bus, "message::eos",
            G_CALLBACK (on_preview_eos), (gpointer) self, 0);
        g_signal_connect_object (bus, "message::segment-done",
            G_CALLBACK (on_preview_segment_done), (gpointer) self, 0);
        g_signal_connect_object (bus, "message::error",
            G_CALLBACK (on_preview_error), (gpointer) self, 0);
        g_signal_connect_object (bus, "message::warning",
            G_CALLBACK (on_preview_warning), (gpointer) self, 0);
        gst_object_unref (bus);

        self->position_query =
            gst_query_new_position (GST_FORMAT_DEFAULT);
      }
      // get parameters
      g_object_get (wave, "channels", &play_channels, NULL);
      g_object_get (wavelevel,
          "length", &play_length, "rate", &play_rate, "data", &play_data, NULL);
      // build caps
      caps = gst_caps_new_simple ("audio/x-raw",
          "format", G_TYPE_STRING, GST_AUDIO_NE (S16),
          "layout", G_TYPE_STRING, "interleaved",
          "rate", G_TYPE_INT, play_rate,
          "channels", G_TYPE_INT, play_channels, NULL);
      g_object_set (self->preview_src,
          "caps", caps, "data", play_data, "length", play_length, NULL);
      gst_caps_unref (caps);

      self->play_wave = wave;
      self->play_wavelevel = wavelevel;

      // build seek events for looping
      preview_update_seeks (self);

      // update playback position 10 times a second
      self->preview_update_id =
          g_timeout_add_full (G_PRIORITY_HIGH, 1000 / 10,
          on_preview_playback_update, (gpointer) self, NULL);

      // set playing
      scr = gst_element_set_state (self->preview, GST_STATE_PAUSED);
      GST_INFO ("start playback: %s",
          gst_element_state_change_return_get_name (scr));
      g_object_unref (wavelevel);
    } else {
      GST_WARNING ("no current wavelevel");
    }
  } else {
    GST_WARNING ("no current wave");
  }
}

static void
on_wavetable_toolbar_stop_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  preview_stop (self);
}

static void
on_wavetable_toolbar_clear_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  BtWave *wave = waves_list_get_current (self);

  if (wave) {
    bt_wavetable_remove_wave (self->wavetable, wave);
    bt_edit_application_set_song_unsaved (self->app);
  }
}

static void
on_file_chooser_load_sample (GtkFileChooser * chooser, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gulong id = waves_list_get_selected_idx (self);
  if (id == GTK_INVALID_LIST_POSITION)
    id = waves_list_get_count (self);

  BtWave *wave = waves_list_get_current (self);

  if (wave) {
    if (wave == self->play_wave) {
      preview_stop (self);
    }
    wave = NULL;
  }
  
  // get current entry and load
  GFile *file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (self->file_chooser));

  if (file) {
    BtSong *song;
    g_object_get(self->app, "song", &song, NULL);

    // trim off protocol, path and extension
    gchar* name = g_file_get_basename (file);
    gchar* ext;
    
    if ((ext = strrchr(name, '.')))
      *ext = '\0';
    
    g_free(name);
    gchar* uri = g_file_get_uri (file);
    wave = bt_wave_new (song, name, uri, id, 1.0, BT_WAVE_LOOP_MODE_OFF, 0);
    g_free (name);
    bt_edit_application_set_song_unsaved (self->app);

    // release the references
    g_object_unref (file);
    g_object_try_unref (wave);
    g_object_unref (song);
  }
}

static void
on_waves_list_row_activated (GtkColumnView * tree_view, guint position, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->wavetable_play), TRUE);
}

static void
on_wavelevels_list_row_activated (GtkColumnView * tree_view, guint position, gpointer user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->wavetable_play), TRUE);
}

static void
on_audio_sink_changed (const BtSettings * const settings,
    GParamSpec * const arg, gpointer const user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);

  GST_INFO ("audio-sink has changed");
  update_audio_sink (self);
}

static void
on_system_audio_sink_changed (const BtSettings * const settings,
    GParamSpec * const arg, gpointer const user_data)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (user_data);
  gchar *element_name, *sink_name;

  GST_INFO ("system audio-sink has changed");

  // exchange the machine (only if the system-audiosink is in use)
  bt_settings_determine_audiosink_name (self->settings, &element_name,
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
label_list_item_on_setup (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* widget = gtk_label_new (NULL);
  
  gtk_list_item_set_child (GTK_LIST_ITEM (item), widget);
}

static void
waves_list_on_bind_idx (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);

  GtkLabel* label = GTK_LABEL (gtk_list_item_get_child (listitem));
  
  gchar idx[16];
  g_snprintf (idx, sizeof (idx), "%x", gtk_list_item_get_position (listitem));
  
  gtk_label_set_text (label, idx);
}

static void
waves_list_on_setup_wave (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* widget = gtk_entry_new ();
  
  gtk_list_item_set_child (GTK_LIST_ITEM (item), widget);

  g_signal_connect_object (widget, "changed", G_CALLBACK (on_wave_name_edited), user_data, 0);
}

static void
waves_list_on_bind_wave (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtWave* item = BT_WAVE (gtk_list_item_get_item (listitem));

  GtkLabel* label = GTK_LABEL (gtk_list_item_get_child (GTK_LIST_ITEM (item)));
  gtk_label_set_text (label, bt_wave_get_name (item));
  
  g_object_ref (item);
  g_object_set_data_full (G_OBJECT (label), "bt-wave", item, g_object_unref);
}

// Run common code for any Wavelevel-based list item with an editable control
static void
wavelevels_list_bind_editable (GtkEditable* widget, BtWavelevel* wavelevel)
{
  g_object_ref (wavelevel);
  g_object_set_data_full (G_OBJECT (widget), "bt-wavelevel", wavelevel, g_object_unref);
}

static void
wavelevels_list_on_setup_root (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* widget = gtk_entry_new ();
  
  gtk_list_item_set_child (GTK_LIST_ITEM (item), widget);

  g_signal_connect_object (widget, "changed", G_CALLBACK (on_wavelevel_root_note_edited), user_data, 0);
}

static void
wavelevels_list_on_bind_root (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtWavelevel* wavelevel = BT_WAVELEVEL (gtk_list_item_get_item (listitem));
  GtkEditable* editable = GTK_EDITABLE (gtk_list_item_get_child (listitem));
  
  GstBtNote root_note;
  g_object_get (G_OBJECT (wavelevel), "root-note", &root_note, NULL);
  gtk_editable_set_text (editable, gstbt_tone_conversion_note_number_2_string (root_note));

  wavelevels_list_bind_editable (editable, wavelevel);
}

static void
wavelevels_list_on_bind_length (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtWavelevel* wavelevel = BT_WAVELEVEL (gtk_list_item_get_item (listitem));
  GtkLabel* label = GTK_LABEL (gtk_list_item_get_child (listitem));

  gulong length;
  g_object_get (G_OBJECT (wavelevel), "length", &length, NULL);
  
  gchar val[16];
  g_snprintf (val, sizeof (val), "%ld", length);
  gtk_label_set_text (label, val);
}

static void
wavelevels_list_on_setup_rate (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* widget = gtk_entry_new ();
  
  gtk_list_item_set_child (GTK_LIST_ITEM (item), widget);

  g_signal_connect_object (widget, "changed", G_CALLBACK (on_wavelevel_rate_edited), user_data, 0);
}

static void
wavelevels_list_on_bind_rate (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtWavelevel* wavelevel = BT_WAVELEVEL (gtk_list_item_get_item (listitem));
  GtkEditable* editable = GTK_EDITABLE (gtk_list_item_get_child (listitem));
  
  gulong rate;
  g_object_get (G_OBJECT (wavelevel), "rate", &rate, NULL);

  gchar val[16];
  g_snprintf (val, sizeof (val), "%ld", rate);
  gtk_editable_set_text (editable, val);
  
  wavelevels_list_bind_editable (editable, wavelevel);
}

static void
wavelevels_list_on_setup_loop_start (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* widget = gtk_entry_new ();
  
  gtk_list_item_set_child (GTK_LIST_ITEM (item), widget);

  g_signal_connect_object (widget, "changed", G_CALLBACK (on_wavelevel_loop_start_edited), user_data, 0);
}

static void
wavelevels_list_on_bind_loop_start (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtWavelevel* wavelevel = BT_WAVELEVEL (gtk_list_item_get_item (listitem));
  GtkEditable* editable = GTK_EDITABLE (gtk_list_item_get_child (listitem));
  
  gulong val;
  g_object_get (G_OBJECT (wavelevel), "loop-start", &val, NULL);

  gchar valstr[16];
  g_snprintf (valstr, sizeof (valstr), "%ld", val);
  gtk_editable_set_text (editable, valstr);

  wavelevels_list_bind_editable (editable, wavelevel);
}

static void
wavelevels_list_on_setup_loop_end (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* item = GTK_LIST_ITEM (object);
  GtkWidget* widget = gtk_entry_new ();
  
  gtk_list_item_set_child (GTK_LIST_ITEM (item), widget);

  g_signal_connect_object (widget, "changed", G_CALLBACK (on_wavelevel_loop_end_edited), user_data, 0);
}

static void
wavelevels_list_on_bind_loop_end (GtkSignalListItemFactory* factory, GObject* object, gpointer user_data)
{
  GtkListItem* listitem = GTK_LIST_ITEM (object);
  BtWavelevel* wavelevel = BT_WAVELEVEL (gtk_list_item_get_item (listitem));
  GtkEditable* editable = GTK_EDITABLE (gtk_list_item_get_child (listitem));
  
  gulong val;
  g_object_get (G_OBJECT (wavelevel), "loop-end", &val, NULL);
  
  gchar valstr[16];
  g_snprintf (valstr, sizeof (valstr), "%ld", val);
  gtk_editable_set_text (editable, valstr);

  wavelevels_list_bind_editable (editable, wavelevel);
}

static void
bt_main_page_waves_init_ui (BtMainPageWaves * self,
    const BtMainPages * pages)
{
  GST_DEBUG ("!!!! self=%p", self);

  //       wave listview
  GtkSingleSelection *selection = gtk_single_selection_new (G_LIST_MODEL (self->wavetable));
  gtk_column_view_set_model (self->waves_list, GTK_SELECTION_MODEL (selection));

  GtkListItemFactory *factory;
  GtkColumnViewColumn *col;

  // Ix
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->waves_list)), 0);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (label_list_item_on_setup), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (waves_list_on_bind_idx), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);

  // Wave
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->waves_list)), 1);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (waves_list_on_setup_wave), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (waves_list_on_bind_wave), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);
  
  // loop-mode combo and volume slider
  AdwEnumListModel *enum_model = adw_enum_list_model_new (BT_TYPE_WAVE_LOOP_MODE);
  gtk_drop_down_set_model (self->loop_mode, G_LIST_MODEL (enum_model));
  gtk_drop_down_set_selected (self->loop_mode, adw_enum_list_model_find_position (enum_model, BT_WAVE_LOOP_MODE_OFF));
  g_object_unref (enum_model);
  enum_model = NULL;
  
#if 0 /// GTK4 signals missing?
  g_signal_connect (self->file_chooser, "file-activated",
      G_CALLBACK (on_file_chooser_load_sample), (gpointer) self);
  g_signal_connect (self->file_chooser, "selection-changed",
      G_CALLBACK (on_browser_preview_sample), (gpointer) self);
#endif

  //       zone entries (multiple waves per sample (xm?)) -> (per entry: root key, length, rate, loope start, loop end
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->wavelevels_list)), 0);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (wavelevels_list_on_setup_root), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (wavelevels_list_on_bind_root), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);
  
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->wavelevels_list)), 1);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (label_list_item_on_setup), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (wavelevels_list_on_bind_length), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);
  
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->wavelevels_list)), 2);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (wavelevels_list_on_setup_rate), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (wavelevels_list_on_bind_rate), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);
  
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->wavelevels_list)), 3);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (wavelevels_list_on_setup_loop_start), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (wavelevels_list_on_bind_loop_start), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);
  
  col = g_list_model_get_item (G_LIST_MODEL (gtk_column_view_get_columns (self->wavelevels_list)), 4);
  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect_object (factory, "setup", G_CALLBACK (wavelevels_list_on_setup_loop_end), (gpointer) self, 0);
  g_signal_connect_object (factory, "bind", G_CALLBACK (wavelevels_list_on_bind_loop_end), (gpointer) self, 0);
  gtk_column_view_column_set_factory (col, factory);
  
  // register event handlers
  g_signal_connect_object (self->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self, 0);

  // let settings control toolbar style and listen to other settings changes
  on_default_sample_folder_changed (self->settings, NULL,
      (gpointer) self);
  g_signal_connect_object (self->settings, "notify::sample-folder",
      G_CALLBACK (on_default_sample_folder_changed), (gpointer) self, 0);
#if 0 /// GTK4 can do with css?
  g_object_bind_property_full (self->settings, "toolbar-style",
      self->list_toolbar, "toolbar-style", G_BINDING_SYNC_CREATE,
      bt_toolbar_style_changed, NULL, NULL, NULL);
  g_object_bind_property_full (self->settings, "toolbar-style",
      self->browser_toolbar, "toolbar-style", G_BINDING_SYNC_CREATE,
      bt_toolbar_style_changed, NULL, NULL, NULL);
#endif
  g_signal_connect_object (self->settings, "notify::audiosink",
      G_CALLBACK (on_audio_sink_changed), (gpointer) self, 0);
  g_signal_connect_object (self->settings, "notify::system-audiosink",
      G_CALLBACK (on_system_audio_sink_changed), (gpointer) self, 0);
  update_audio_sink (self);

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
  bt_main_page_waves_init_ui (self, pages);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_main_page_waves_dispose (GObject * object)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES (object);

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->settings);
  g_object_unref (self->n2f);
  g_object_try_unref (self->wavetable);
  g_object_unref (self->app);

  // shut down loader-preview playbin
  if (self->playbin) {
    gst_element_set_state (self->playbin, GST_STATE_NULL);
    gst_object_unref (self->playbin);
  }
  // shut down wavetable-preview playbin
  if (self->preview) {
    GstBus *bus;

    preview_stop (self);
    gst_element_set_state (self->preview, GST_STATE_NULL);
    bus = gst_element_get_bus (GST_ELEMENT (self->preview));
    gst_bus_remove_signal_watch (bus);
    gst_object_unref (bus);
    gst_object_unref (self->preview);

    if (self->play_seek_event)
      gst_event_unref (self->play_seek_event);
    if (self->loop_seek_event[0])
      gst_event_unref (self->loop_seek_event[0]);
    if (self->loop_seek_event[1])
      gst_event_unref (self->loop_seek_event[1]);
    gst_query_unref (self->position_query);
  }

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MAIN_PAGE_WAVES);
  
  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_waves_parent_class)->dispose (object);
}

static void
bt_main_page_waves_init (BtMainPageWaves * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_main_page_waves_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
  self->settings = bt_settings_make ();
  self->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);

  self->playbin = gst_element_factory_make ("playbin", NULL);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
      GTK_ORIENTATION_VERTICAL);
}

static void
bt_main_page_waves_class_init (BtMainPageWavesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_main_page_waves_dispose;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/main-page-waves.ui");

  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, browser_play);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, loop_mode);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, volume);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, waveform_viewer);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, wavetable_play);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, wavelevels_list);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageWaves, waves_list);
  
  gtk_widget_class_bind_template_callback (widget_class, on_browser_toolbar_play_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_loop_mode_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_volume_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_waveform_viewer_loop_end_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_waveform_viewer_loop_start_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_wavelevels_list_cursor_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_wavelevels_list_row_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_waves_list_row_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_waves_list_selection_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_wavetable_toolbar_clear_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_wavetable_toolbar_play_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_wavetable_toolbar_stop_clicked);
}
