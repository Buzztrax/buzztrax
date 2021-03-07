/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btplaybackcontrolleric
 * @short_description: interaction controller based playback controller
 *
 * Uses ic-devices and listens to configurable playback controls.
 */

#define BT_EDIT
#define BT_PLAYBACK_CONTROLLER_IC_C

#include "bt-edit.h"

struct _BtPlaybackControllerIcPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  BtIcDevice *device;
  GHashTable *commands;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtPlaybackControllerIc, bt_playback_controller_ic,
    G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtPlaybackControllerIc));

//-- signal handlers

static gboolean
get_key_state (const BtIcControl * control, GParamSpec * arg)
{
  gboolean key_pressed = FALSE;

  switch (arg->value_type) {
    case G_TYPE_BOOLEAN:
      g_object_get ((gpointer) control, arg->name, &key_pressed, NULL);
      break;
    case G_TYPE_LONG:{
      glong v;
      g_object_get ((gpointer) control, arg->name, &v, NULL);
      key_pressed = (v > 0);
      break;
    }
    default:
      GST_WARNING ("unhandled type \"%s\"", G_PARAM_SPEC_TYPE_NAME (arg));
      break;
  }
  return key_pressed;
}

static void
on_control_notify (const BtIcControl * control, GParamSpec * arg,
    gpointer user_data)
{
  BtPlaybackControllerIc *self = BT_PLAYBACK_CONTROLLER_IC (user_data);
  if (get_key_state (control, arg)) {
    BtSong *song;
    gchar *cmd;

    g_object_get (self->priv->app, "song", &song, NULL);
    if (!song)
      return;

    cmd = g_hash_table_lookup (self->priv->commands, control);
    if (cmd) {
      if (!strcmp (cmd, "play")) {
        bt_song_play (song);
      } else if (!strcmp (cmd, "stop")) {
        bt_song_stop (song);
      } else if (!strcmp (cmd, "rewind")) {
        glong play_pos;
        /* TODO(ensonic): implement, whats better?
         * - song::play-pos - skip ticks
         *   like when we're scrubbing on the sequence
         * - song::play-rate - trick mode  
         *   adjust the rate on press down and go back to 1.0 on release
         * 
         * - needs a timeout handler for initial timeout and for repeat while
         *   pressed 
         *   - nothing we can use from GtkSettings
         *   - dconf: org.gnome.settings-daemon.peripherals.keyboard has
         *     delay=500 ms, repeat=on/off, repeat-interval=30ms
         */
        /* TODO(ensonic): expose 'bars' as a readable property on sequence page
         * app->main-window->pages
         */
        g_object_get (song, "play-pos", &play_pos, NULL);
        play_pos = (play_pos > 16) ? (play_pos - 16) : 0;
        g_object_set (song, "play-pos", play_pos, NULL);
      } else if (!strcmp (cmd, "forward")) {
        glong play_pos;
        g_object_get (song, "play-pos", &play_pos, NULL);
        play_pos = (play_pos < LONG_MAX - 16) ? (play_pos + 16) : LONG_MAX;
        g_object_set (song, "play-pos", play_pos, NULL);
      } else {
        GST_WARNING ("unknown command: '%s'", cmd);
      }
    } else {
      GST_WARNING ("no command linked to control");
    }
    g_object_unref (song);
  }
}

//-- helper methods

static void
bt_playback_controller_ic_stop (BtPlaybackControllerIc * self)
{
  if (!self->priv->device)
    return;

  // stop device
  btic_device_stop (self->priv->device);
  g_clear_object (&self->priv->device);

  g_hash_table_destroy (self->priv->commands);
  self->priv->commands = NULL;
}

static void
bt_playback_controller_ic_start (BtPlaybackControllerIc * self)
{
  BtSettings *settings;
  GHashTable *ht;
  gchar *spec;
  const gchar *cfg_name;

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "ic-playback-spec", &spec, NULL);
  ht = bt_settings_parse_ic_playback_spec (spec);

  cfg_name = g_hash_table_lookup (ht, "!device");
  if (cfg_name && (self->priv->device =
          btic_registry_get_device_by_name (cfg_name))) {
    BtIcControl *control;
    GHashTableIter iter;
    gpointer key, value;

    self->priv->commands = g_hash_table_new_full (NULL, NULL, NULL, g_free);

    // start device + bind controllers
    btic_device_start (self->priv->device);
    g_hash_table_iter_init (&iter, ht);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      if (((gchar *) key)[0] == '!')
        continue;
      if ((control = btic_device_get_control_by_name (self->priv->device,
                  (gchar *) value))) {
        g_signal_connect_object (control, "notify::value",
            G_CALLBACK (on_control_notify), (gpointer) self, 0);
        g_hash_table_insert (self->priv->commands, control, g_strdup (key));
      }
    }
  }
  g_hash_table_destroy (ht);
  g_free (spec);
  g_object_unref (settings);
}

//-- event handler

static void
on_settings_active_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  BtPlaybackControllerIc *self = BT_PLAYBACK_CONTROLLER_IC (user_data);
  gboolean active;

  g_object_get (settings, "ic-playback-active", &active, NULL);
  if (active) {
    bt_playback_controller_ic_start (self);
  } else {
    bt_playback_controller_ic_stop (self);
  }
}

static void
on_settings_spec_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  BtPlaybackControllerIc *self = BT_PLAYBACK_CONTROLLER_IC (user_data);
  gboolean active;

  g_object_get (settings, "ic-playback-active", &active, NULL);
  if (active) {
    bt_playback_controller_ic_stop (self);
    bt_playback_controller_ic_start (self);
  }
}

static void
on_ic_registry_devices_changed (BtIcRegistry * const registry,
    GParamSpec * const arg, gconstpointer user_data)
{
  BtPlaybackControllerIc *self = BT_PLAYBACK_CONTROLLER_IC (user_data);
  BtSettings *settings;
  gboolean active;

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "ic-playback-active", &active, NULL);
  g_object_unref (settings);
  if (active) {
    bt_playback_controller_ic_stop (self);
    bt_playback_controller_ic_start (self);
  }
}

static void
settings_listen (BtPlaybackControllerIc * self)
{
  BtSettings *settings;
  BtIcRegistry *ic_registry;

  g_object_get (self->priv->app, "settings", &settings, "ic-registry",
      &ic_registry, NULL);
  g_signal_connect (settings, "notify::ic-playback-active",
      G_CALLBACK (on_settings_active_notify), (gpointer) self);
  g_signal_connect (settings, "notify::ic-playback-spec",
      G_CALLBACK (on_settings_spec_notify), (gpointer) self);
  g_signal_connect (ic_registry, "notify::devices",
      G_CALLBACK (on_ic_registry_devices_changed), (gpointer) self);
  on_settings_active_notify (settings, NULL, (gpointer) self);
  g_object_unref (settings);
  g_object_unref (ic_registry);
}

//-- constructor methods

/**
 * bt_playback_controller_ic_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtPlaybackControllerIc *
bt_playback_controller_ic_new (void)
{
  return
      BT_PLAYBACK_CONTROLLER_IC (g_object_new
      (BT_TYPE_PLAYBACK_CONTROLLER_IC, NULL));
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_playback_controller_ic_dispose (GObject * object)
{
  BtPlaybackControllerIc *self = BT_PLAYBACK_CONTROLLER_IC (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  bt_playback_controller_ic_stop (self);
  g_object_try_weak_unref (self->priv->app);

  G_OBJECT_CLASS (bt_playback_controller_ic_parent_class)->dispose (object);
}

static void
bt_playback_controller_ic_finalize (GObject * object)
{
  BtPlaybackControllerIc *self = BT_PLAYBACK_CONTROLLER_IC (object);

  GST_DEBUG ("!!!! self=%p", self);

  G_OBJECT_CLASS (bt_playback_controller_ic_parent_class)->finalize (object);
}

static void
bt_playback_controller_ic_init (BtPlaybackControllerIc * self)
{
  self->priv = bt_playback_controller_ic_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  /* this is created from the app, we need to avoid a ref-cycle */
  self->priv->app = bt_edit_application_new ();
  g_object_try_weak_ref (self->priv->app);
  g_object_unref (self->priv->app);

  // check settings
  settings_listen (self);
}

static void
bt_playback_controller_ic_class_init (BtPlaybackControllerIcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_playback_controller_ic_dispose;
  gobject_class->finalize = bt_playback_controller_ic_finalize;
}
