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
 * SECTION:btmaintoolbar
 * @short_description: class for the editor main toolbar
 *
 * Contains typical applications buttons for file i/o and playback, plus volume
 * meters and volume control.
 */

/* TODO(ensonic): should we separate the toolbars?
 * - common - load, save, ...
 * - playback - play, stop, loop, seek, ...
 * - volume - gain, levels
 */
#define BT_EDIT
#define BT_MAIN_TOOLBAR_C

#include <math.h>

#include "bt-edit.h"
#include "gtkvumeter.h"

/* lets keep multichannel audio for later :) */
//#define MAX_VUMETER 4
#define MAX_VUMETER 2
#define LOW_VUMETER_VAL -90.0

struct _BtMainToolbarPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the level meters */
  GtkVUMeter *vumeter[MAX_VUMETER];
  GstElement *level;
  GstClock *clock;
  gint num_channels;

  /* the volume gain */
  GtkScale *volume;
  GstElement *gain;
  BtMachine *master;

  /* action buttons */
  GtkWidget *save_button;
  GtkWidget *play_button;
  GtkWidget *stop_button;
  GtkWidget *loop_button;

  /* signal handler ids */
  guint playback_update_id;
  guint playback_rate_id;

  /* playback state */
  gboolean is_playing;
  gboolean has_error;
  gdouble playback_rate;

  /* lock for multithreaded access */
  GMutex lock;
};

static GQuark bus_msg_level_quark = 0;

static void on_toolbar_play_clicked (GtkButton * button, gpointer user_data);
static void reset_playback_rate (BtMainToolbar * self);
static void on_song_volume_changed (GstElement * gain, GParamSpec * arg,
    gpointer user_data);

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMainToolbar, bt_main_toolbar, GTK_TYPE_TOOLBAR, 
    G_ADD_PRIVATE(BtMainToolbar));


//-- event handler

static gboolean
on_song_playback_update (gpointer user_data)
{
  return bt_song_update_playback_position (BT_SONG (user_data));
}

static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

  g_object_get ((gpointer) song, "is-playing", &self->priv->is_playing, NULL);
  if (!self->priv->is_playing) {
    gint i;

    GST_INFO ("song stop event occurred: %p", g_thread_self ());
    // stop update timer and reset trick playback
    if (self->priv->playback_update_id) {
      g_source_remove (self->priv->playback_update_id);
      self->priv->playback_update_id = 0;
    }
    bt_song_update_playback_position (song);
    reset_playback_rate (self);
    // disable stop button
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->stop_button), FALSE);
    // switch off play button
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->priv->
            play_button), FALSE);
    // enable play button
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->play_button), TRUE);
    // reset level meters
    for (i = 0; i < MAX_VUMETER; i++) {
      gtk_vumeter_set_levels (self->priv->vumeter[i], LOW_VUMETER_VAL,
          LOW_VUMETER_VAL);
    }

    self->priv->has_error = FALSE;

    GST_INFO ("song stop event handled");
  } else {
    // update playback position 10 times a second
    self->priv->playback_update_id =
        g_timeout_add_full (G_PRIORITY_HIGH, 1000 / 10, on_song_playback_update,
        (gpointer) song, NULL);
    bt_song_update_playback_position (song);

    // if we started playback remotely activate playbutton
    if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (self->priv->
                play_button))) {
      g_signal_handlers_block_matched (self->priv->play_button,
          G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
          on_toolbar_play_clicked, (gpointer) self);
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->priv->
              play_button), TRUE);
      g_signal_handlers_unblock_matched (self->priv->play_button,
          G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
          on_toolbar_play_clicked, (gpointer) self);
    }
    // disable play button
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->play_button), FALSE);
    // enable stop button
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->stop_button), TRUE);
  }
}

static void
on_toolbar_new_clicked (GtkButton * button, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  BtMainWindow *main_window;

  GST_INFO ("toolbar new event occurred");
  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  bt_main_window_new_song (main_window);
  g_object_unref (main_window);
}

static void
on_toolbar_open_clicked (GtkButton * button, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  BtMainWindow *main_window;

  GST_INFO ("toolbar open event occurred");
  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  bt_main_window_open_song (main_window);
  g_object_unref (main_window);
}

static void
on_toolbar_save_clicked (GtkButton * button, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  BtMainWindow *main_window;

  GST_INFO ("toolbar open event occurred");
  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  bt_main_window_save_song (main_window);
  g_object_unref (main_window);
}

static void
on_toolbar_play_clicked (GtkButton * button, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button))) {
    BtSong *song;

    GST_INFO ("toolbar play activated");

    // get song from app and start playback
    g_object_get (self->priv->app, "song", &song, NULL);
    if (!bt_song_play (song)) {
      // switch off play button
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (button),
          FALSE);
    }
    // release the reference
    g_object_unref (song);
  }
}

static void
on_toolbar_stop_clicked (GtkButton * button, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  BtSong *song;

  GST_INFO ("toolbar stop event occurred");
  // get song from app
  g_object_get (self->priv->app, "song", &song, NULL);
  bt_song_stop (song);

  GST_INFO ("  song stopped");
  // release the reference
  g_object_unref (song);
}

static void
on_toolbar_loop_toggled (GtkButton * button, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gboolean loop =
      gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button));

  GST_INFO ("toolbar loop toggle event occurred, new-state=%d", loop);
  bt_child_proxy_set (self->priv->app, "song::sequence::loop", loop, NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
set_new_playback_rate (BtMainToolbar * self, gdouble playback_rate)
{
  self->priv->playback_rate = playback_rate;
  bt_child_proxy_set (self->priv->app, "song::play-rate",
      self->priv->playback_rate, NULL);
}

static void
reset_playback_rate (BtMainToolbar * self)
{
  set_new_playback_rate (self, 1.0);

  if (self->priv->playback_rate_id) {
    g_source_remove (self->priv->playback_rate_id);
    self->priv->playback_rate_id = 0;
  }
}

#define SEEK_FACTOR 1.2
#define SEEK_TIMEOUT 2
#define SEEK_MAX_RATE 4.0

static gboolean
on_song_playback_rate_rewind (gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gdouble playback_rate = self->priv->playback_rate * SEEK_FACTOR;

  GST_DEBUG (" << speedup");

  if (G_UNLIKELY (playback_rate > 0.0)) {
    // we switched from forrward to backward
    return FALSE;
  }

  if (playback_rate > -SEEK_MAX_RATE) {
    set_new_playback_rate (self, playback_rate);
    return TRUE;
  } else {
    self->priv->playback_rate_id = 0;
    return FALSE;
  }
}

static gboolean
on_toolbar_rewind_pressed (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  GST_DEBUG (" << pressed");

  set_new_playback_rate (self, -1.0);
  self->priv->playback_rate_id =
      g_timeout_add_seconds (SEEK_TIMEOUT, on_song_playback_rate_rewind,
      (gpointer) self);

  GST_DEBUG (" << <<");

  return FALSE;
}

static gboolean
on_toolbar_rewind_released (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  GST_DEBUG (" << released");
  reset_playback_rate (self);

  return FALSE;
}

static gboolean
on_song_playback_rate_forward (gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gdouble playback_rate = self->priv->playback_rate * SEEK_FACTOR;

  GST_DEBUG (" >> speedup");

  if (G_UNLIKELY (playback_rate < 0.0)) {
    // we switched from forrward to backward
    return FALSE;
  }

  if (playback_rate < SEEK_MAX_RATE) {
    set_new_playback_rate (self, playback_rate);
    return TRUE;
  } else {
    self->priv->playback_rate_id = 0;
    return FALSE;
  }
}

static gboolean
on_toolbar_forward_pressed (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  GST_DEBUG (" >> pressed");

  set_new_playback_rate (self, SEEK_FACTOR);

  self->priv->playback_rate_id =
      g_timeout_add_seconds (SEEK_TIMEOUT, on_song_playback_rate_forward,
      (gpointer) self);

  GST_DEBUG (" >> >>");
  return FALSE;
}

static gboolean
on_toolbar_forward_released (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  GST_DEBUG (" >> released");
  reset_playback_rate (self);

  return FALSE;
}

static void
on_song_error (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtMainToolbar *const self = BT_MAIN_TOOLBAR (user_data);

  if (!self->priv->has_error) {
    BtSong *song;
    BtMainWindow *main_window;
    gchar *msg, *desc;

    BT_GST_LOG_MESSAGE_ERROR (message, &msg, &desc);

    // get song from app
    g_object_get (self->priv->app, "song", &song, "main-window", &main_window,
        NULL);
    // debug the state
    GST_INFO ("stopping");
    bt_song_write_to_lowlevel_dot_file (song);
    bt_song_stop (song);

    bt_dialog_message (main_window, _("Error"), msg, desc);

    // release the reference
    g_object_unref (song);
    g_object_unref (main_window);
    g_free (msg);
    g_free (desc);
  } else {
    BT_GST_LOG_MESSAGE_ERROR (message, NULL, NULL);
  }

  self->priv->has_error = TRUE;
}

static void
on_song_warning (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtMainToolbar *const self = BT_MAIN_TOOLBAR (user_data);

  if (!self->priv->has_error) {
    BtMainWindow *main_window;
    gchar *msg, *desc;

    BT_GST_LOG_MESSAGE_WARNING (message, &msg, &desc);

    g_object_get (self->priv->app, "main-window", &main_window, NULL);
    bt_dialog_message (main_window, _("Warning"), msg, desc);
    g_object_unref (main_window);
  } else {
    BT_GST_LOG_MESSAGE_WARNING (message, NULL, NULL);
  }
}

#define g_value_array_get_ix(va,ix) (va->values + ix)

static gboolean
on_delayed_idle_song_level_change (gpointer user_data)
{
  gconstpointer *const params = (gconstpointer *) user_data;
  BtMainToolbar *self = (BtMainToolbar *) params[0];
  GstMessage *message = (GstMessage *) params[1];

  if (self) {
    const GstStructure *structure = gst_message_get_structure (message);
    const GValue *values;
    GValueArray *decay_arr, *peak_arr;
    gdouble decay, peak;
    guint i, size;

    g_mutex_lock (&self->priv->lock);
    g_object_remove_weak_pointer ((gpointer) self, (gpointer *) & params[0]);
    g_mutex_unlock (&self->priv->lock);

    if (!self->priv->is_playing)
      goto done;

    values = (GValue *) gst_structure_get_value (structure, "peak");
    peak_arr = (GValueArray *) g_value_get_boxed (values);
    values = (GValue *) gst_structure_get_value (structure, "decay");
    decay_arr = (GValueArray *) g_value_get_boxed (values);
    size = decay_arr->n_values;
    for (i = 0; i < size; i++) {
      decay = g_value_get_double (g_value_array_get_ix (decay_arr, i));
      peak = g_value_get_double (g_value_array_get_ix (peak_arr, i));
      if (isinf (decay) || isnan (decay))
        decay = LOW_VUMETER_VAL;
      if (isinf (peak) || isnan (peak))
        peak = LOW_VUMETER_VAL;
      //GST_INFO("level.%d  %.3f %.3f", i, peak, decay);
      //gtk_vumeter_set_levels (self->priv->vumeter[i], (gint)decay, (gint)peak);
      gtk_vumeter_set_levels (self->priv->vumeter[i], (gint) peak,
          (gint) decay);
    }
  }
done:
  gst_message_unref (message);
  g_slice_free1 (2 * sizeof (gconstpointer), params);
  return FALSE;
}

static gboolean
on_delayed_song_level_change (GstClock * clock, GstClockTime time,
    GstClockID id, gpointer user_data)
{
  // the callback is called from a clock thread
  if (GST_CLOCK_TIME_IS_VALID (time))
    g_idle_add_full (G_PRIORITY_HIGH, on_delayed_idle_song_level_change,
        user_data, NULL);
  else {
    gconstpointer *const params = (gconstpointer *) user_data;
    GstMessage *message = (GstMessage *) params[1];
    gst_message_unref (message);
    g_slice_free1 (2 * sizeof (gconstpointer), user_data);
  }
  return TRUE;
}


static void
on_song_level_change (GstBus * bus, GstMessage * message, gpointer user_data)
{
  const GstStructure *s = gst_message_get_structure (message);
  const GQuark name_id = gst_structure_get_name_id (s);

  if (name_id == bus_msg_level_quark) {
    BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
    GstElement *level = GST_ELEMENT (GST_MESSAGE_SRC (message));

    // check if its our element (we can have multiple level meters)
    if (level == self->priv->level) {
      GstClockTime waittime = bt_gst_analyzer_get_waittime (level, s, TRUE);
      if (GST_CLOCK_TIME_IS_VALID (waittime)) {
        gpointer *data = (gpointer *) g_slice_alloc (2 * sizeof (gpointer));
        GstClockID clock_id;
        GstClockReturn clk_ret;

        //GST_WARNING("target %"GST_TIME_FORMAT" %"GST_TIME_FORMAT,
        //  GST_TIME_ARGS(endtime),GST_TIME_ARGS(waittime));

        data[0] = (gpointer) self;
        data[1] = (gpointer) gst_message_ref (message);
        g_mutex_lock (&self->priv->lock);
        g_object_add_weak_pointer ((gpointer) self, &data[0]);
        g_mutex_unlock (&self->priv->lock);
        waittime += gst_element_get_base_time (level);
        clock_id = gst_clock_new_single_shot_id (self->priv->clock, waittime);
        if ((clk_ret =
                gst_clock_id_wait_async (clock_id, on_delayed_song_level_change,
                    (gpointer) data, NULL)) != GST_CLOCK_OK) {
          GST_WARNING_OBJECT (level, "clock wait failed: %d", clk_ret);
          gst_message_unref (message);
          g_object_remove_weak_pointer ((gpointer) self, &data[0]);
          g_slice_free1 (2 * sizeof (gpointer), data);
        }
        gst_clock_id_unref (clock_id);
      }
    }
  }
}

static gboolean
update_level_meters (gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gint i;

  for (i = 0; i < self->priv->num_channels; i++) {
    gtk_widget_show (GTK_WIDGET (self->priv->vumeter[i]));
  }
  for (i = self->priv->num_channels; i < MAX_VUMETER; i++) {
    gtk_widget_hide (GTK_WIDGET (self->priv->vumeter[i]));
  }
  return FALSE;
}


static gdouble
volume_slider2real (gdouble lin)
{
  if (lin <= 0)
    return 0.0;
  return pow (10000.0, lin - 1.0);
}

static gdouble
volume_real2slider (gdouble logv)
{
  if (logv <= (1.0 / 10000.0))
    return 0.0;
  return log (logv) / log (10000.0) + 1.0;
}

static void
on_song_volume_slider_change (GtkRange * range, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gdouble nvalue, ovalue;

  g_assert (self->priv->gain);
  g_assert (self->priv->volume);

  // get value from HScale and change volume
  nvalue =
      volume_slider2real (gtk_range_get_value (GTK_RANGE (self->priv->volume)));
  g_object_get (self->priv->gain, "master-volume", &ovalue, NULL);
  if (fabs (nvalue - ovalue) > 0.000001) {
    GST_DEBUG ("volume-slider has changed : %f->%f", ovalue, nvalue);
    g_signal_handlers_block_matched (self->priv->volume,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_song_volume_changed, (gpointer) self);
    g_object_set (self->priv->gain, "master-volume", nvalue, NULL);
    g_signal_handlers_unblock_matched (self->priv->volume,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_song_volume_changed, (gpointer) self);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
  /*else {
     GST_WARNING("IGN volume-slider has changed : %f->%f",ovalue,nvalue);
     } */
}

static gboolean
on_song_volume_slider_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->type == GDK_BUTTON_PRESS) {
    if (event->button == GDK_BUTTON_PRIMARY) {
      BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
      GstObject *machine;

      g_object_get (self->priv->master, "machine", &machine, NULL);
      gst_object_set_control_binding_disabled (machine, "master-volume", TRUE);
      g_object_unref (machine);
    }
  }
  return FALSE;
}

static gboolean
on_song_volume_slider_release_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_BUTTON_RELEASE) {
    BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
    GstObject *machine;
    GstControlBinding *cb;

    g_object_get (self->priv->master, "machine", &machine, NULL);
    if ((cb = gst_object_get_control_binding (machine, "master-volume"))) {
      BtParameterGroup *pg =
          bt_machine_get_global_param_group (self->priv->master);
      glong param = bt_parameter_group_get_param_index (pg, "master-volume");
      if (G_LIKELY (param != -1)) {
        // update the default value at ts=0
        bt_parameter_group_set_param_default (pg, param);
      }

      /* TODO(ensonic): it should actualy postpone the enable to the next
       * timestamp.
       * for that in pattern-cs it would need to peek at the control-point-list,
       * or somehow schedule this for the next sync call
       */
      /* re-enable cb on button_release
       * see gst_object_set_control_binding_disabled() on button_press
       */
      gst_control_binding_set_disabled (cb, FALSE);
      gst_object_unref (cb);
    }
    g_object_unref (machine);
  }
  return FALSE;
}

static void
on_song_volume_changed (GstElement * gain, GParamSpec * arg, gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gdouble nvalue, ovalue;

  g_assert (self->priv->gain);
  g_assert (self->priv->volume);

  // get value from Element and change HScale
  g_object_get (self->priv->gain, "master-volume", &nvalue, NULL);
  nvalue = volume_real2slider (nvalue);
  ovalue = gtk_range_get_value (GTK_RANGE (self->priv->volume));
  if (fabs (nvalue - ovalue) > 0.000001) {
    GST_DEBUG ("volume-slider notify : %f", nvalue);

    g_signal_handlers_block_matched (self->priv->gain,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_song_volume_slider_change, (gpointer) self);
    gtk_range_set_value (GTK_RANGE (self->priv->volume), nvalue);
    g_signal_handlers_unblock_matched (self->priv->gain,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_song_volume_slider_change, (gpointer) self);
  }
  /*else {
     GST_WARNING("IGN volume-slider notify : %f",nvalue);
     } */
}


static void
on_channels_negotiated (GstPad * pad, GParamSpec * arg, gpointer user_data)
{
  GstCaps *caps;

  if ((caps = (GstCaps *) gst_pad_get_current_caps (pad))) {
    BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);

    if (GST_CAPS_IS_SIMPLE (caps)) {
      gint old_channels = self->priv->num_channels;
      gst_structure_get_int (gst_caps_get_structure (caps, 0), "channels",
          &self->priv->num_channels);
      if (self->priv->num_channels != old_channels) {
        GST_INFO ("input level src has %d output channels",
            self->priv->num_channels);
        // need to call this via g_idle_add as it triggers the redraw
        bt_g_object_idle_add ((GObject *) self, G_PRIORITY_DEFAULT_IDLE,
            update_level_meters);
      }
    } else {
      GST_WARNING_OBJECT (pad, "expecting simple caps");
    }
    gst_caps_unref (caps);
  }
}

static void
on_song_unsaved_changed (const GObject * object, GParamSpec * arg,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gboolean unsaved = bt_edit_application_is_song_unsaved (self->priv->app);

  gtk_widget_set_sensitive (self->priv->save_button, unsaved);
}

static void
on_sequence_loop_notify (const BtSequence * sequence, GParamSpec * arg,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  gboolean loop;

  g_object_get ((gpointer) sequence, "loop", &loop, NULL);
  g_signal_handlers_block_matched (self->priv->loop_button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_toolbar_loop_toggled, (gpointer) self);
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (self->priv->
          loop_button), loop);
  g_signal_handlers_unblock_matched (self->priv->loop_button,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_toolbar_loop_toggled, (gpointer) self);
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (user_data);
  BtSong *song;
  BtSequence *sequence;
  GstBin *bin;
  //gboolean loop;

  GST_INFO ("song has changed : app=%p, toolbar=%p", app, user_data);

  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song)
    return;
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  // get the audio_sink (song->master is a bt_sink_machine) if there is one already
  g_object_try_weak_unref (self->priv->master);
  g_object_get (song, "master", &self->priv->master, "sequence", &sequence,
      "bin", &bin, NULL);

  if (self->priv->master) {
    GstPad *pad;
    GstBus *bus;

    GST_INFO ("connect to input-level: master=%" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->master));
    g_object_try_weak_ref (self->priv->master);

    // get the input_level and input_gain properties from audio_sink
    g_object_try_weak_unref (self->priv->gain);
    g_object_try_weak_unref (self->priv->level);
    g_object_get (self->priv->master, "input-post-level", &self->priv->level,
        "machine", &self->priv->gain, NULL);
    g_object_try_weak_ref (self->priv->gain);
    g_object_try_weak_ref (self->priv->level);

    // connect bus signals
    bus = gst_element_get_bus (GST_ELEMENT (bin));
    bt_g_signal_connect_object (bus, "message::error",
        G_CALLBACK (on_song_error), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::warning",
        G_CALLBACK (on_song_warning), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "sync-message::element",
        G_CALLBACK (on_song_level_change), (gpointer) self, 0);
    gst_object_unref (bus);

    if (self->priv->clock)
      gst_object_unref (self->priv->clock);
    self->priv->clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

    // get the pad from the input-level and listen there for channel negotiation
    g_assert (GST_IS_ELEMENT (self->priv->level));
    if ((pad = gst_element_get_static_pad (self->priv->level, "src"))) {
      g_signal_connect_object (pad, "notify::caps",
          G_CALLBACK (on_channels_negotiated), (gpointer) self, 0);
      gst_object_unref (pad);
    }

    g_assert (GST_IS_ELEMENT (self->priv->gain));
    // get the current input_gain and adjust volume widget
    on_song_volume_changed (self->priv->gain, NULL, (gpointer) self);

    // connect slider changed and volume changed events
    g_signal_connect (self->priv->volume, "value_changed",
        G_CALLBACK (on_song_volume_slider_change), (gpointer) self);
    g_signal_connect (self->priv->volume, "button-press-event",
        G_CALLBACK (on_song_volume_slider_press_event), (gpointer) self);
    g_signal_connect (self->priv->volume, "button-release-event",
        G_CALLBACK (on_song_volume_slider_release_event), (gpointer) self);
    g_signal_connect_object (self->priv->gain, "notify::master-volume",
        G_CALLBACK (on_song_volume_changed), (gpointer) self, 0);

    gst_object_unref (self->priv->gain);
    gst_object_unref (self->priv->level);
    g_object_unref (self->priv->master);
  } else {
    GST_WARNING ("failed to get the master element of the song");
  }
  g_signal_connect_object (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), (gpointer) self, 0);
  on_sequence_loop_notify (sequence, NULL, (gpointer) self);
  g_signal_connect_object (sequence, "notify::loop",
      G_CALLBACK (on_sequence_loop_notify), (gpointer) self, 0);
  //-- release the references
  gst_object_unref (bin);
  g_object_unref (sequence);
  g_object_unref (song);
}

//-- helper methods

static void
bt_main_toolbar_init_ui (const BtMainToolbar * self)
{
  BtSettings *settings;
  GtkToolItem *tool_item;
  GtkWidget *box, *child;
  gulong i;
  BtChangeLog *change_log;

  gtk_widget_set_name (GTK_WIDGET (self), "main toolbar");

  //-- file controls

  tool_item = gtk_tool_button_new_from_icon_name ("document-new", _("_New"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Prepare a new empty song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_new_clicked),
      (gpointer) self);

  tool_item = gtk_tool_button_new_from_icon_name ("document-open", _("_Open"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Load a new song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_open_clicked),
      (gpointer) self);

  tool_item = gtk_tool_button_new_from_icon_name ("document-save", _("_Save"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Save this song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_save_clicked),
      (gpointer) self);
  self->priv->save_button = GTK_WIDGET (tool_item);

  gtk_toolbar_insert (GTK_TOOLBAR (self), gtk_separator_tool_item_new (), -1);

  //-- media controls

  tool_item = gtk_tool_button_new_from_icon_name ("media-seek-backward",
      _("R_ewind"));
  gtk_tool_item_set_tooltip_text (tool_item,
      _("Rewind playback position of this song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  child = gtk_bin_get_child (GTK_BIN (tool_item));
  gtk_widget_add_events (child,
      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (child, "button-press-event",
      G_CALLBACK (on_toolbar_rewind_pressed), (gpointer) self);
  g_signal_connect (child, "button-release-event",
      G_CALLBACK (on_toolbar_rewind_released), (gpointer) self);

  tool_item =
      gtk_toggle_tool_button_new_from_icon_name ("media-playback-start",
      _("_Play"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Play this song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_play_clicked),
      (gpointer) self);
  self->priv->play_button = GTK_WIDGET (tool_item);

  tool_item = gtk_tool_button_new_from_icon_name ("media-seek-forward",
      _("_Forward"));
  gtk_tool_item_set_tooltip_text (tool_item,
      _("Forward playback position of this song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  child = gtk_bin_get_child (GTK_BIN (tool_item));
  gtk_widget_add_events (child,
      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect (child, "button-press-event",
      G_CALLBACK (on_toolbar_forward_pressed), (gpointer) self);
  g_signal_connect (child, "button-release-event",
      G_CALLBACK (on_toolbar_forward_released), (gpointer) self);

  tool_item =
      gtk_tool_button_new_from_icon_name ("media-playback-stop", _("_Stop"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Stop playback of this song"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_stop_clicked),
      (gpointer) self);
  self->priv->stop_button = GTK_WIDGET (tool_item);
  gtk_widget_set_sensitive (self->priv->stop_button, FALSE);

  tool_item = gtk_toggle_tool_button_new_from_icon_name ("view-refresh",
      _("Loop"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Toggle looping of playback"));
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);
  g_signal_connect (tool_item, "toggled", G_CALLBACK (on_toolbar_loop_toggled),
      (gpointer) self);
  self->priv->loop_button = GTK_WIDGET (tool_item);

  gtk_toolbar_insert (GTK_TOOLBAR (self), gtk_separator_tool_item_new (), -1);

  //-- volume level and control

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 0);
  // add gtk_vumeter widgets and update from level_callback
  for (i = 0; i < MAX_VUMETER; i++) {
    self->priv->vumeter[i] =
        GTK_VUMETER (gtk_vumeter_new (GTK_ORIENTATION_HORIZONTAL));
    // @idea have distinct tooltips with channel names
    gtk_widget_set_tooltip_text (GTK_WIDGET (self->priv->vumeter[i]),
        _("playback volume"));
    gtk_vumeter_set_min_max (self->priv->vumeter[i], LOW_VUMETER_VAL, 0);
    gtk_vumeter_set_levels (self->priv->vumeter[i], LOW_VUMETER_VAL,
        LOW_VUMETER_VAL);
    // no falloff in widget, we have falloff in GstLevel
    //gtk_vumeter_set_peaks_falloff(self->priv->vumeter[i], GTK_VUMETER_PEAKS_FALLOFF_MEDIUM);
    gtk_vumeter_set_scale (self->priv->vumeter[i], GTK_VUMETER_SCALE_LINEAR);
    gtk_widget_set_no_show_all (GTK_WIDGET (self->priv->vumeter[i]), TRUE);
    if (i < self->priv->num_channels) {
      gtk_widget_show (GTK_WIDGET (self->priv->vumeter[i]));
    } else {
      gtk_widget_hide (GTK_WIDGET (self->priv->vumeter[i]));
    }
    gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (self->priv->vumeter[i]),
        TRUE, TRUE, 0);
  }

  // add gain-control
  self->priv->volume =
      GTK_SCALE (gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
          /*min= */ 0.0, /*max= */ 1.0, /*step= */ 0.01));
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->priv->volume),
      _("Change playback volume"));
  gtk_scale_set_draw_value (self->priv->volume, FALSE);
  //gtk_range_set_update_policy(GTK_RANGE(self->priv->volume),GTK_UPDATE_DELAYED);
  gtk_widget_set_size_request (GTK_WIDGET (box), 250, -1);
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (self->priv->volume), TRUE,
      TRUE, 0);
  gtk_widget_show_all (GTK_WIDGET (box));

  tool_item = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (tool_item), box);
  gtk_toolbar_insert (GTK_TOOLBAR (self), tool_item, -1);

  // register event handlers
  g_signal_connect (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self);
  g_signal_connect (self->priv->app, "notify::unsaved",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);

  change_log = bt_change_log_new ();
  g_signal_connect (change_log, "notify::can-undo",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);
  g_object_unref (change_log);

  // let settings control toolbar style
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_bind_property_full (settings, "toolbar-style", (GObject *) self,
      "toolbar-style", G_BINDING_SYNC_CREATE, bt_toolbar_style_changed, NULL,
      NULL, NULL);
  g_object_unref (settings);
}

//-- constructor methods

/**
 * bt_main_toolbar_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainToolbar *
bt_main_toolbar_new (void)
{
  BtMainToolbar *self;

  self = BT_MAIN_TOOLBAR (g_object_new (BT_TYPE_MAIN_TOOLBAR, NULL));
  bt_main_toolbar_init_ui (self);
  return self;
}

//-- methods


//-- class internals

static void
bt_main_toolbar_dispose (GObject * object)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_weak_unref (self->priv->master);
  g_object_try_weak_unref (self->priv->gain);
  g_object_try_weak_unref (self->priv->level);

  if (self->priv->clock)
    gst_object_unref (self->priv->clock);

  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_main_toolbar_parent_class)->dispose (object);
}

static void
bt_main_toolbar_finalize (GObject * object)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_mutex_clear (&self->priv->lock);

  G_OBJECT_CLASS (bt_main_toolbar_parent_class)->finalize (object);
}

static void
bt_main_toolbar_init (BtMainToolbar * self)
{
  self->priv = bt_main_toolbar_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
  g_mutex_init (&self->priv->lock);
  self->priv->playback_rate = 1.0;
  self->priv->num_channels = 2;
}

static void
bt_main_toolbar_class_init (BtMainToolbarClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  bus_msg_level_quark = g_quark_from_static_string ("level");

  gobject_class->dispose = bt_main_toolbar_dispose;
  gobject_class->finalize = bt_main_toolbar_finalize;

}
