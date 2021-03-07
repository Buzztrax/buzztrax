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
 * SECTION:btmainstatusbar
 * @short_description: class for the editor main statusbar
 *
 * The statusbar shows some contextual help, as well as things like playback
 * status.
 */

#define BT_EDIT
#define BT_MAIN_STATUSBAR_C

#include "bt-edit.h"
#include <glib/gprintf.h>

enum
{
  MAIN_STATUSBAR_STATUS = 1
};

//#define USE_MAIN_LOOP_IDLE_TRACKER 1


struct _BtMainStatusbarPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* main status bar */
  GtkStatusbar *status;
  /* identifier of the status message group */
  gint status_context_id;

  /* time-elapsed (total) status bar */
  GtkLabel *elapsed;
  /* time-current (current play pos) status bar */
  GtkLabel *current;
  /* time-loop status bar */
  GtkLabel *loop;

  /* cpu load */
  GtkProgressBar *cpu_load;
  guint cpu_load_handler_id;

#ifdef USE_MAIN_LOOP_IDLE_TRACKER
  /* main-loop monitor */
  guint main_loop_idle_handler_id;
  GstClockTime ml_tlast, ml_tavg;
  guint64 ml_ct;
#endif

  /* total playtime */
  gulong total_ticks;
  gulong last_pos, play_start;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMainStatusbar, bt_main_statusbar, GTK_TYPE_BOX, 
    G_ADD_PRIVATE(BtMainStatusbar));


//-- helper

static void
bt_main_statusbar_update_length (const BtMainStatusbar * self,
    const BtSequence * sequence, const BtSongInfo * song_info)
{
  gchar str[2 + 2 + 3 + 3];
  gulong msec, sec, min;

  // get new song length
  bt_song_info_tick_to_m_s_ms (song_info,
      bt_sequence_get_loop_length (sequence), &min, &sec, &msec);
  g_snprintf (str, sizeof(str), "%02lu:%02lu.%03lu", min, sec, msec);
  // update statusbar fields
  gtk_label_set_text (self->priv->loop, str);
}

//-- event handler

static void
on_song_play_pos_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  BtSequence *sequence;
  BtSongInfo *song_info;
  gchar str[32];
  gulong pos, msec, sec, min;

  GST_DEBUG ("tick update");

  g_object_get ((gpointer) song, "sequence", &sequence, "song-info", &song_info,
      "play-pos", &pos, NULL);
  if (!sequence || !song_info)
    goto Error;

  //GST_INFO("sequence tick received : %d",pos);
  // update current statusbar
  bt_song_info_tick_to_m_s_ms (song_info, pos, &min, &sec, &msec);
  // format
  g_snprintf (str, sizeof(str), "%02lu:%02lu.%03lu", min, sec, msec);
  // update statusbar fields
  gtk_label_set_text (self->priv->current, str);

  // update elapsed statusbar
  if (pos < self->priv->last_pos) {
    self->priv->total_ticks += bt_sequence_get_loop_length (sequence);
    GST_INFO ("wrapped around total_ticks=%lu", self->priv->total_ticks);
  }
  pos -= self->priv->play_start;
  bt_song_info_tick_to_m_s_ms (song_info, pos + self->priv->total_ticks,
      &min, &sec, &msec);
  // format
  g_snprintf (str, sizeof(str), "%02lu:%02lu.%03lu", min, sec, msec);
  // update statusbar fields
  gtk_label_set_text (self->priv->elapsed, str);

  self->priv->last_pos = pos;
Error:
  g_object_try_unref (sequence);
  g_object_try_unref (song_info);
}

static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  gboolean is_playing;
  gulong play_start;

  g_object_get ((gpointer) song, "is-playing", &is_playing, "play-pos",
      &play_start, NULL);
  self->priv->total_ticks = 0;
  if (!is_playing) {
    GST_INFO ("play_start=%lu", play_start);
    // update statusbar fields
    self->priv->last_pos = play_start;
    self->priv->play_start = play_start;
    on_song_play_pos_notify (song, NULL, user_data);
  } else {
    self->priv->last_pos = 0;
    self->priv->play_start = 0;
  }
}

static void
on_song_info_rhythm_notify (const BtSongInfo * song_info, GParamSpec * arg,
    gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  BtSequence *sequence;

  bt_child_proxy_get (self->priv->app, "song::sequence", &sequence, NULL);
  bt_main_statusbar_update_length (self, sequence, song_info);
  g_object_unref (sequence);
}

static void
on_sequence_loop_time_notify (const BtSequence * sequence, GParamSpec * arg,
    gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  BtSongInfo *song_info;

  bt_child_proxy_get (self->priv->app, "song::song-info", &song_info, NULL);
  bt_main_statusbar_update_length (self, sequence, song_info);
  g_object_unref (song_info);
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  BtSong *song;
  BtSongInfo *song_info;
  BtSequence *sequence;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  // get song from app
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song)
    return;

  g_object_get (song, "sequence", &sequence, "song-info", &song_info, NULL);
  bt_main_statusbar_update_length (self, sequence, song_info);
  // subscribe to property changes in song
  g_signal_connect_object (song, "notify::play-pos",
      G_CALLBACK (on_song_play_pos_notify), (gpointer) self, 0);
  g_signal_connect_object (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), (gpointer) self, 0);
  // subscribe to property changes in song_info
  g_signal_connect_object (song_info, "notify::bpm",
      G_CALLBACK (on_song_info_rhythm_notify), (gpointer) self, 0);
  g_signal_connect_object (song_info, "notify::tpb",
      G_CALLBACK (on_song_info_rhythm_notify), (gpointer) self, 0);
  // subscribe to property changes in sequence
  g_signal_connect_object (sequence, "notify::length",
      G_CALLBACK (on_sequence_loop_time_notify), (gpointer) self, 0);
  g_signal_connect_object (sequence, "notify::loop",
      G_CALLBACK (on_sequence_loop_time_notify), (gpointer) self, 0);
  g_signal_connect_object (sequence, "notify::loop-start",
      G_CALLBACK (on_sequence_loop_time_notify), (gpointer) self, 0);
  g_signal_connect_object (sequence, "notify::loop-end",
      G_CALLBACK (on_sequence_loop_time_notify), (gpointer) self, 0);
  // release the references
  g_object_unref (song_info);
  g_object_unref (sequence);
  g_object_unref (song);
}

static gboolean
on_cpu_load_update (gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  guint cpu_load = bt_cpu_load_get_current ();
#ifdef USE_MAIN_LOOP_IDLE_TRACKER
  guint ml_lag = GST_TIME_AS_MSECONDS (self->priv->ml_tavg);
  gchar str[strlen ("CPU: 000 %, ML: 00000 ms") + 3];

  g_snprintf (str, sizeof(str), "CPU: %d %%, ML: %d ms", cpu_load, ml_lag);
#else
  gchar str[strlen ("CPU: 000 %") + 3];

  g_snprintf (str, sizeof(str), "CPU: %d %%", cpu_load);
#endif
  gtk_progress_bar_set_fraction (self->priv->cpu_load,
      (gdouble) cpu_load / 100.0);
  gtk_progress_bar_set_text (self->priv->cpu_load, str);
  return TRUE;
}

#ifdef USE_MAIN_LOOP_IDLE_TRACKER
static gboolean
on_main_loop_idle (gpointer user_data)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (user_data);
  GstClockTime tnow, tdiff;

  tnow = gst_util_get_timestamp ();
  if (self->priv->ml_ct != 0) {
    tdiff = self->priv->ml_tlast - tnow;
    // moving cumulative average
    self->priv->ml_tavg =
        self->priv->ml_tavg + ((tdiff -
            self->priv->ml_tavg) / self->priv->ml_ct);
    self->priv->ml_ct++;
  }
  self->priv->ml_tlast = tnow;

  return TRUE;
}
#endif

//-- helper methods

static void
bt_main_statusbar_init_ui (const BtMainStatusbar * self)
{
  GtkWidget *ev_box;
  gchar str[] = "00:00.000";

  gtk_widget_set_name (GTK_WIDGET (self), "status-bar");

  // context sensitive help statusbar
  self->priv->status = GTK_STATUSBAR (gtk_statusbar_new ());
  self->priv->status_context_id =
      gtk_statusbar_get_context_id (self->priv->status, "default");

  gtk_statusbar_push (self->priv->status,
      self->priv->status_context_id, BT_MAIN_STATUSBAR_DEFAULT);
  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->priv->status), TRUE,
      TRUE, 1);

  // cpu load
  // TODO(ensonic): add some settings to show/hide (view menu?)
  self->priv->cpu_load = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  // see https://bugzilla.gnome.org/show_bug.cgi?id=746688
  // the progress bar does not fill since gtk 3.13
  g_object_set (self->priv->cpu_load, "tooltip-text", _("CPU load"), "valign",
      GTK_ALIGN_CENTER, NULL);

  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->priv->cpu_load),
      FALSE, FALSE, 1);
  self->priv->cpu_load_handler_id =
      g_timeout_add_seconds (1, on_cpu_load_update, (gpointer) self);
#ifdef USE_MAIN_LOOP_IDLE_TRACKER
  self->priv->main_loop_idle_handler_id =
      g_idle_add_full (G_PRIORITY_LOW, on_main_loop_idle, (gpointer) self,
      NULL);
#endif

  // timer status-bars
  ev_box = gtk_event_box_new ();
  g_object_set (ev_box, "visible-window", FALSE, NULL);
  gtk_widget_set_tooltip_text (ev_box, _("Playback time"));
  self->priv->elapsed = GTK_LABEL (gtk_label_new (str));
  g_object_set (self->priv->elapsed, "xalign", 1.0, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (self->priv->elapsed), 100, -1);
  gtk_container_add (GTK_CONTAINER (ev_box), GTK_WIDGET (self->priv->elapsed));
  gtk_box_pack_start (GTK_BOX (self), ev_box, FALSE, FALSE, 1);

  ev_box = gtk_event_box_new ();
  g_object_set (ev_box, "visible-window", FALSE, NULL);
  gtk_widget_set_tooltip_text (ev_box, _("Playback position"));
  self->priv->current = GTK_LABEL (gtk_label_new (str));
  g_object_set (self->priv->current, "xalign", 1.0, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (self->priv->current), 100, -1);
  gtk_container_add (GTK_CONTAINER (ev_box), GTK_WIDGET (self->priv->current));
  gtk_box_pack_start (GTK_BOX (self), ev_box, FALSE, FALSE, 1);

  ev_box = gtk_event_box_new ();
  g_object_set (ev_box, "visible-window", FALSE, NULL);
  gtk_widget_set_tooltip_text (ev_box, _("Playback length"));
  self->priv->loop = GTK_LABEL (gtk_label_new (str));
  g_object_set (self->priv->loop, "xalign", 1.0, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (self->priv->loop), 100, -1);
  gtk_container_add (GTK_CONTAINER (ev_box), GTK_WIDGET (self->priv->loop));
  gtk_box_pack_start (GTK_BOX (self), ev_box, FALSE, FALSE, 1);

  // register event handlers
  g_signal_connect (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self);
}

//-- constructor methods

/**
 * bt_main_statusbar_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainStatusbar *
bt_main_statusbar_new (void)
{
  BtMainStatusbar *self;

  self = BT_MAIN_STATUSBAR (g_object_new (BT_TYPE_MAIN_STATUSBAR, "orientation",
          GTK_ORIENTATION_HORIZONTAL, NULL));
  bt_main_statusbar_init_ui (self);
  return self;
}

//-- methods


//-- class internals

static void
bt_main_statusbar_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (object);
  return_if_disposed ();
  switch (property_id) {
    case MAIN_STATUSBAR_STATUS:{
      gchar *str = g_value_dup_string (value);
      gtk_statusbar_pop (self->priv->status, self->priv->status_context_id);
      if (str) {
        gtk_statusbar_push (self->priv->status, self->priv->status_context_id,
            str);
        g_free (str);
      } else
        gtk_statusbar_push (self->priv->status, self->priv->status_context_id,
            BT_MAIN_STATUSBAR_DEFAULT);
      // FIXME(ensonic): this was done to ensure status-bar updates when the
      // loader sets status while loading
      //while (gtk_events_pending ()) gtk_main_iteration ();
      // FIXME(ensonic): this does not help either, but on the other hand
      // loading is fast
      // gtk_widget_queue_draw (GTK_WIDGET (self->priv->status));
      // see https://github.com/Buzztrax/buzztrax/issues/52
      //GST_DEBUG("set the status-text for main_statusbar");
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_statusbar_dispose (GObject * object)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->cpu_load_handler_id)
    g_source_remove (self->priv->cpu_load_handler_id);
#ifdef USE_MAIN_LOOP_IDLE_TRACKER
  if (self->priv->main_loop_idle_handler_id)
    g_source_remove (self->priv->main_loop_idle_handler_id);
#endif

  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_main_statusbar_parent_class)->dispose (object);
}

static void
bt_main_statusbar_init (BtMainStatusbar * self)
{
  self->priv = bt_main_statusbar_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_main_statusbar_class_init (BtMainStatusbarClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_main_statusbar_set_property;
  gobject_class->dispose = bt_main_statusbar_dispose;

  g_object_class_install_property (gobject_class, MAIN_STATUSBAR_STATUS,
      g_param_spec_string ("status", "status prop", "main status text",
          BT_MAIN_STATUSBAR_DEFAULT,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}
