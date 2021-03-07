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
/**
 * SECTION:btplaybackcontrollersocket
 * @short_description: sockets based playback controller
 *
 * Allows the coherence upnp backend for buzztrax to remote control and query
 * buzztrax-edit.
 *
 * Functionality can be tested doing "netcat -n 127.0.0.1 7654". The commands
 * are implemented in client_cmd_parse_and_process<!-- -->(). The protocol uses
 * strings that are delimited by '|' chars.
 *
 * playlist - buzztrax sends a list of labels for the current song
 *   reply is: '|' delimited label names
 * play - play a playlist entry (or from start)
 *   play|&lt;playlist-entry-number&gt;
 * stop - stop playback
 * status - get player status
 *  reply is: event|&lt;play-state&gt;|&lt;playlist-label&gt;|&lt;play-pos&gt;|&lt;length&gt;|&lt;volume&gt;|&lt;mute-state&gt;|&lt;loop-mode&gt;
 * {set,get} - set or get a variable
 *  volume, mute, repeat
 */
/* TODO(ensonic): playbackcontroller baseclass
 * - this can provide command helpers
 * - this can provide an interface to query the capabilities
 */
/* TODO(ensonic): playbackcontroller manager
 * - instead of creating and starting each playback controller in
 *   edit-application, we could have a manager that listens to settings changes
 *   and creates/destroys them as needed
 */

#define BT_EDIT
#define BT_PLAYBACK_CONTROLLER_SOCKET_C

#include "bt-edit.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct _BtPlaybackControllerSocketPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* positions for each label */
  GList *playlist;
  gulong cur_pos;
  gboolean seek;

  /* data for the status */
  BtSequence *sequence;
  BtSongInfo *song_info;
  GstElement *gain;
  gboolean is_playing;
  gchar *length_str;
  gchar *cur_label;

  /* master */
  gint master_socket;
  guint master_source;
  GIOChannel *master_channel;
  /* client */
  gint client_socket;
  guint client_source;
  GIOChannel *client_channel;
};

#define DEFAULT_LABEL "start"

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtPlaybackControllerSocket, bt_playback_controller_socket,
    G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtPlaybackControllerSocket));

//-- helper methods

static void
client_connection_close (BtPlaybackControllerSocket * self)
{
  if (self->priv->client_channel) {
    GError *error = NULL;

    if (self->priv->client_source) {
      g_source_remove (self->priv->client_source);
      // the above already unrefs
      //g_io_channel_unref(self->priv->client_channel);
      self->priv->client_source = 0;
    }
    g_io_channel_shutdown (self->priv->client_channel, TRUE, &error);
    if (error) {
      GST_WARNING ("iochannel error while shutting down client: %s",
          error->message);
      g_error_free (error);
    }
    g_io_channel_unref (self->priv->client_channel);
    self->priv->client_channel = NULL;
  }
  if (self->priv->client_socket > -1) {
    close (self->priv->client_socket);
    self->priv->client_socket = -1;
  }
}

static gchar *
client_cmd_parse_and_process (BtPlaybackControllerSocket * self, gchar * cmd)
{
  gchar *reply = NULL;
  BtSong *song;

  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song)
    return NULL;

  if (!strcasecmp (cmd, "browse")) {
    BtSongInfo *song_info = self->priv->song_info;
    BtSequence *sequence = self->priv->sequence;
    gchar *str, *temp;
    gulong i, length;
    gboolean no_labels = TRUE;

    g_object_get (song_info, "name", &str, NULL);
    g_object_get (sequence, "length", &length, NULL);

    reply = g_strconcat ("playlist|", str, NULL);
    g_free (str);

    if (self->priv->playlist) {
      g_list_free (self->priv->playlist);
      self->priv->playlist = NULL;
    }
    // get sequence labels
    for (i = 0; i < length; i++) {
      str = bt_sequence_get_label (sequence, i);
      if (str) {
        temp = g_strconcat (reply, "|", str, NULL);
        g_free (str);
        g_free (reply);
        reply = temp;
        no_labels = FALSE;

        self->priv->playlist =
            g_list_append (self->priv->playlist, GINT_TO_POINTER (i));
      }
    }
    // if there are no labels, return the start
    if (no_labels) {
      temp = g_strconcat (reply, "|" DEFAULT_LABEL, NULL);
      g_free (reply);
      reply = temp;
    }

  } else if (!strncasecmp (cmd, "play|", 5)) {
    g_free (self->priv->cur_label);

    // always stop, so that seeking works
    bt_song_stop (song);

    // get playlst entry
    if (cmd[5] && self->priv->playlist) {
      // get position for ix-th label
      self->priv->cur_pos =
          GPOINTER_TO_INT (g_list_nth_data (self->priv->playlist,
              atoi (&cmd[5])));
      self->priv->cur_label =
          bt_sequence_get_label (self->priv->sequence, self->priv->cur_pos);
    } else {
      self->priv->cur_pos = 0;
      self->priv->cur_label = g_strdup (DEFAULT_LABEL);
    }

    GST_INFO ("starting to play");
    if (!bt_song_play (song)) {
      GST_WARNING ("failed to play");
    }
    // seek - we're seeking in on_song_is_playing_notify()
    //GST_INFO("seeking to pos=%d",self->priv->cur_pos);
    // this causes stack smashing, but only if we play afterwards
    // its also avoided by making the string buffer in main-statusbar.c (notify) +4 bytes
    //g_object_set(song,"play-pos",self->priv->cur_pos,NULL);
    self->priv->seek = TRUE;

  } else if (!strcasecmp (cmd, "stop")) {
    if (!bt_song_stop (song)) {
      GST_WARNING ("failed to stop");
    }
  } else if (!strcasecmp (cmd, "status")) {
    gchar *state[] = { "stopped", "playing" };
    gchar *mode[] = { "on", "off" };
    gulong pos, msec, sec, min;
    gdouble volume;
    gboolean loop;

    g_object_get (song, "play-pos", &pos, NULL);
    g_object_get (self->priv->sequence, "loop", &loop, NULL);

    // calculate playtime
    bt_song_info_tick_to_m_s_ms (self->priv->song_info, pos, &min, &sec, &msec);

    // get the current input_gain and adjust volume widget
    g_object_get (self->priv->gain, "volume", &volume, NULL);

    reply = g_strdup_printf ("event|%s|%s|0:%02lu:%02lu.%03lu|%s|%u|off|%s",
        state[self->priv->is_playing],
        self->priv->cur_label,
        min, sec, msec,
        self->priv->length_str, (guint) (100.0 * volume), mode[loop]);
    //reply=g_strdup("event|stopped|start|0:00:00.000|0:00:00.000|100|off|off");
  } else if (!strncasecmp (cmd, "set|", 4)) {
    gchar *subcmd = &cmd[4];

    if (!strncasecmp (subcmd, "volume|", 7)) {
      if (subcmd[7] && self->priv->gain) {
        gdouble volume;

        volume = ((gdouble) atoi (&subcmd[7])) / 100.0;
        g_object_set (self->priv->gain, "volume", volume, NULL);
      }
    } else if (!strncasecmp (subcmd, "mute|", 5)) {
      // we ignore this for now
    } else if (!strncasecmp (subcmd, "repeat|", 7)) {
      if (!strcasecmp (&subcmd[7], "on")) {
        g_object_set (self->priv->sequence, "loop", TRUE, NULL);
      } else if (!strcasecmp (&subcmd[7], "off")) {
        g_object_set (self->priv->sequence, "loop", FALSE, NULL);
      }
    } else {
      GST_WARNING ("unknown setting: %s", subcmd);
    }
  } else if (!strncasecmp (cmd, "get|", 4)) {
    gchar *subcmd = &cmd[4];

    if (!strcasecmp (subcmd, "volume")) {
      gdouble volume;

      g_object_get (self->priv->gain, "volume", &volume, NULL);
      reply = g_strdup_printf ("volume|%u", (guint) (100.0 * volume));
    } else if (!strcasecmp (subcmd, "mute")) {
      reply = g_strdup ("mute|off");
    } else if (!strcasecmp (subcmd, "repeat")) {
      gboolean loop;
      gchar *mode[] = { "on", "off" };

      g_object_get (self->priv->sequence, "loop", &loop, NULL);
      reply = g_strdup_printf ("repeat|%s", mode[loop]);
    } else {
      GST_WARNING ("unknown setting: %s", subcmd);
    }
  } else {
    GST_WARNING ("unknown command: %s", cmd);
  }

  g_object_unref (song);
  return reply;
}

static gchar *
client_read (BtPlaybackControllerSocket * self)
{
  gchar *str;
  gsize len, term;
  GError *error = NULL;

  g_io_channel_read_line (self->priv->client_channel, &str, &len, &term,
      &error);
  if (!error) {
    if (str && len > 0)
      str[term] = '\0';
    GST_INFO ("command received : %s", str);
  } else {
    GST_WARNING ("iochannel error while reading: %s", error->message);
    g_error_free (error);
    if (str) {
      g_free (str);
      str = NULL;
    }
  }
  return str;
}

static gboolean
client_write (BtPlaybackControllerSocket * self, gchar * str)
{
  gboolean res = FALSE;
  GError *error = NULL;
  gsize len;

  if (!self->priv->client_channel)
    return FALSE;

  GST_INFO ("sending reply : %s", str);

  g_io_channel_write_chars (self->priv->client_channel, str, -1, &len, &error);
  if (!error) {
    g_io_channel_write_chars (self->priv->client_channel, "\r\n", -1, &len,
        &error);
    if (!error) {
      g_io_channel_flush (self->priv->client_channel, &error);
      if (!error) {
        res = TRUE;
      } else {
        GST_WARNING ("iochannel error while flushing: %s", error->message);
        g_error_free (error);
      }
    } else {
      GST_WARNING ("iochannel error while writing: %s", error->message);
      g_error_free (error);
    }
  } else {
    GST_WARNING ("iochannel error while writing: %s", error->message);
    g_error_free (error);
  }
  return res;
}

//-- event handler

static gboolean
client_socket_io_handler (GIOChannel * channel, GIOCondition condition,
    gpointer user_data)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (user_data);
  gboolean res = TRUE;
  gchar *cmd, *reply;

  GST_INFO ("client io handler : %d", condition);
  if (condition & (G_IO_IN | G_IO_PRI)) {
    if ((cmd = client_read (self))) {
      if ((reply = client_cmd_parse_and_process (self, cmd))) {
        if (!client_write (self, reply)) {
          res = FALSE;
        }
        g_free (reply);
      }
      g_free (cmd);
    } else
      res = FALSE;
  }
  if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    res = FALSE;
  }
  if (!res) {
    GST_INFO ("closing client connection");
    self->priv->master_source = 0;
  }
  return res;
}

static gboolean
master_socket_io_handler (GIOChannel * channel, GIOCondition condition,
    gpointer user_data)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (user_data);
  struct sockaddr addr = { 0, };
  socklen_t addrlen = sizeof (struct sockaddr);

  GST_INFO ("master io handler : %d", condition);
  if (condition & (G_IO_IN | G_IO_PRI)) {
    if ((self->priv->client_socket =
            accept (self->priv->master_socket, &addr, &addrlen)) < 0) {
      GST_WARNING ("accept error: %s", g_strerror (errno));
    } else {
      self->priv->client_channel =
          g_io_channel_unix_new (self->priv->client_socket);
      //self->priv->client_source=g_io_add_watch(self->priv->client_channel,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,client_socket_io_handler,(gpointer)self);
      self->priv->client_source =
          g_io_add_watch_full (self->priv->client_channel, G_PRIORITY_LOW,
          G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
          client_socket_io_handler, (gpointer) self, NULL);
      GST_INFO ("playback controller client connected");
    }
  }
  if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    client_connection_close (self);
    GST_INFO ("playback controller client disconnected");
  }
  return TRUE;
}

static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (user_data);

  g_object_get ((gpointer) song, "is-playing", &self->priv->is_playing, NULL);

  if (self->priv->is_playing && self->priv->seek) {
    // do this only if play was invoked via playbackcontroller
    self->priv->seek = FALSE;
    GST_INFO ("seeking to pos=%lu", self->priv->cur_pos);
    g_object_set ((gpointer) song, "play-pos", self->priv->cur_pos, NULL);
  }
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (user_data);
  BtSong *song;
  BtSinkMachine *master;
  gulong msec, sec, min;

  GST_INFO ("song has changed : app=%p, toolbar=%p", app, user_data);

  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song)
    return;

  self->priv->cur_pos = 0;
  client_write (self, "flush");
  g_signal_connect (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), (gpointer) self);

  g_object_try_weak_unref (self->priv->sequence);
  g_object_try_weak_unref (self->priv->song_info);
  g_object_get (song, "sequence", &self->priv->sequence, "song-info",
      &self->priv->song_info, "master", &master, NULL);
  g_object_try_weak_ref (self->priv->sequence);
  g_object_try_weak_ref (self->priv->song_info);
  g_object_unref (self->priv->sequence);
  g_object_unref (self->priv->song_info);

  g_object_try_weak_unref (self->priv->gain);
  g_object_get (master, "input-gain", &self->priv->gain, NULL);
  g_object_try_weak_ref (self->priv->gain);
  g_object_unref (self->priv->gain);

  // calculate length
  g_free (self->priv->length_str);
  bt_song_info_tick_to_m_s_ms (self->priv->song_info,
      bt_sequence_get_loop_length (self->priv->sequence), &min, &sec, &msec);
  self->priv->length_str =
      g_strdup_printf ("0:%02lu:%02lu.%03lu", min, sec, msec);

  g_object_unref (master);
  g_object_unref (song);
}

//-- helper

static void
master_connection_close (BtPlaybackControllerSocket * self)
{
  if (self->priv->master_channel) {
    GError *error = NULL;

    if (self->priv->master_source) {
      g_source_remove (self->priv->master_source);
      // the above already unrefs
      //g_io_channel_unref(self->priv->master_channel);
      self->priv->master_source = 0;
    }
    g_io_channel_shutdown (self->priv->master_channel, TRUE, &error);
    if (error) {
      GST_WARNING ("iochannel error while shutting down master: %s",
          error->message);
      g_error_free (error);
    }
    g_io_channel_unref (self->priv->master_channel);
    self->priv->master_channel = NULL;
  }
  if (self->priv->master_socket > -1) {
    close (self->priv->master_socket);
    self->priv->master_socket = -1;
  }
  client_connection_close (self);
  GST_INFO ("playback controller terminated");
}

static void
master_connection_open (BtPlaybackControllerSocket * self)
{
  BtSettings *settings;
  gboolean active;
  guint port;
  static struct sockaddr_in serv_addr;

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "coherence-upnp-active", &active,
      "coherence-upnp-port", &port, NULL);
  g_object_unref (settings);

  if (!active)
    return;

  /* new socket: internet address family, stream socket */
  if ((self->priv->master_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    goto socket_error;
  }
  memset (&serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);       // my address
  serv_addr.sin_port = htons (port);
  if (bind (self->priv->master_socket, (struct sockaddr *) &serv_addr,
          sizeof (serv_addr)) < 0) {
    goto bind_error;
  }
  if (listen (self->priv->master_socket, 64) < 0) {
    goto listen_error;
  }
  self->priv->master_channel =
      g_io_channel_unix_new (self->priv->master_socket);
  //self->priv->master_source=g_io_add_watch(self->priv->master_channel,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,master_socket_io_handler,(gpointer)self);
  self->priv->master_source = g_io_add_watch_full (self->priv->master_channel,
      G_PRIORITY_LOW,
      G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
      master_socket_io_handler, (gpointer) self, NULL);

  GST_INFO ("playback controller running");
  return;

socket_error:
  GST_WARNING ("socket allocation failed: %s", g_strerror (errno));
  return;
bind_error:
  GST_WARNING ("binding the socket failed: %s", g_strerror (errno));
  return;
listen_error:
  GST_WARNING ("listen failed: %s", g_strerror (errno));
  return;
}

static void
on_port_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (user_data);
  gboolean active;

  g_object_get (settings, "coherence-upnp-active", &active, NULL);

  if (active) {
    master_connection_close (self);
    master_connection_open (self);
  }
}

static void
on_active_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (user_data);
  gboolean active;

  g_object_get (settings, "coherence-upnp-active", &active, NULL);

  if (active) {
    if (!self->priv->master_channel) {
      master_connection_open (self);
    }
  } else {
    master_connection_close (self);
  }
}

static void
settings_listen (BtPlaybackControllerSocket * self)
{
  BtSettings *settings;

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_signal_connect (settings, "notify::coherence-upnp-active",
      G_CALLBACK (on_active_notify), (gpointer) self);
  g_signal_connect (settings, "notify::coherence-upnp-port",
      G_CALLBACK (on_port_notify), (gpointer) self);
  on_active_notify (settings, NULL, (gpointer) self);
  g_object_unref (settings);

}

//-- constructor methods

/**
 * bt_playback_controller_socket_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtPlaybackControllerSocket *
bt_playback_controller_socket_new (void)
{
  return
      BT_PLAYBACK_CONTROLLER_SOCKET (g_object_new
      (BT_TYPE_PLAYBACK_CONTROLLER_SOCKET, NULL));
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_playback_controller_socket_dispose (GObject * object)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_try_weak_unref (self->priv->gain);
  g_object_try_weak_unref (self->priv->sequence);
  g_object_try_weak_unref (self->priv->song_info);
  g_object_try_weak_unref (self->priv->app);

  master_connection_close (self);

  G_OBJECT_CLASS (bt_playback_controller_socket_parent_class)->dispose (object);
}

static void
bt_playback_controller_socket_finalize (GObject * object)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->playlist) {
    g_list_free (self->priv->playlist);
    self->priv->playlist = NULL;
  }

  g_free (self->priv->length_str);
  g_free (self->priv->cur_label);

  G_OBJECT_CLASS (bt_playback_controller_socket_parent_class)->finalize
      (object);
}

static void
bt_playback_controller_socket_init (BtPlaybackControllerSocket * self)
{
  self->priv = bt_playback_controller_socket_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  /* this is created from the app, we need to avoid a ref-cycle */
  self->priv->app = bt_edit_application_new ();
  g_object_try_weak_ref (self->priv->app);
  g_object_unref (self->priv->app);

  // check settings
  //master_connection_open(self);
  settings_listen (self);
  // register event handlers
  g_signal_connect (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self);
}

static void
bt_playback_controller_socket_class_init (BtPlaybackControllerSocketClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_playback_controller_socket_dispose;
  gobject_class->finalize = bt_playback_controller_socket_finalize;
}
