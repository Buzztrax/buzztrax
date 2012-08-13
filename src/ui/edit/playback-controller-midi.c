/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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
/**
 * SECTION:btplaybackcontrollermidi
 * @short_description: sockets based playback controller
 *
 * Activates midi devices and listens to playback controls.
 */
/* TODO(ensonic): baseclass + manager (see playback-controller-socket) */

#define BT_EDIT
#define BT_PLAYBACK_CONTROLLER_MIDI_C

#include "bt-edit.h"

struct _BtPlaybackControllerMidiPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
    G_POINTER_ALIAS (BtEditApplication *, app);

BtRegistry *registry};

//-- the class

G_DEFINE_TYPE (BtPlaybackControllerMidi, bt_playback_controller_midi,
    G_TYPE_OBJECT);

//-- helper methods


//-- event handler

//-- helper

static void
on_settings_master_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  BtPlaybackControllerMidi *self = BT_PLAYBACK_CONTROLLER_MIDI (user_data);
  gboolean active;

  g_object_get (settings, "raw-midi-master", &active, NULL);
}

static void
on_settings_slave_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  BtPlaybackControllerMidi *self = BT_PLAYBACK_CONTROLLER_MIDI (user_data);
  gboolean active;

  g_object_get (settings, "raw-midi-slave", &active, NULL);
}

/* we'll do that later, see settings-page-controller
static void on_settings_devices_notify(BtSettings * const settings, GParamSpec * const arg, gconstpointer user_data) {
  BtPlaybackControllerMidi *self=BT_PLAYBACK_CONTROLLER_MIDI(user_data);
  gchar *str;

  g_object_get(settings,"raw-midi-devices",&str,NULL);
  // split into a g_strv or GList
  // store list in self

  g_free(str);
}
*/

static void
settings_listen (BtPlaybackControllerMidi * self)
{
  BtSettings *settings;

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_signal_connect (settings, "notify::raw-midi-master",
      G_CALLBACK (on_settings_master_notify), (gpointer) self);
  g_signal_connect (settings, "notify::raw-midi-slave",
      G_CALLBACK (on_settings_slave_notify), (gpointer) self);
  //g_signal_connect(settings, "notify::raw-midi-devices", G_CALLBACK(on_settings_devices_notify), (gpointer)self);
  //on_settings_devices_notify(settings,NULL,(gpointer)self);
  on_settings_master_notify (settings, NULL, (gpointer) self);
  on_settings_slave_notify (settings, NULL, (gpointer) self);
  g_object_unref (settings);

}

static void
on_registry_devices_changed (BtIcRegistry * const registry,
    const GParamSpec * const arg, gconstpointer const user_data)
{
  BtPlaybackControllerMidi *self = BT_PLAYBACK_CONTROLLER_MIDI (user_data);
  GList *list;

  GST_INFO ("devices changed");

  g_object_get (registry, "devices", &list, NULL);
  // update list in self, start/stop devices if we're active
  g_list_free (list);
}

static void
devices_listen (BtPlaybackControllerMidi * self)
{
  BtIcRegistry *registry;

  g_object_get (self->priv->app, "ic-registry", &registry, NULL);
  g_signal_connect (registry, "notify::devices",
      G_CALLBACK (on_registry_devices_changed), (gpointer) self);
  on_registry_devices_changed (registry, NULL, (gpointer) self);
  g_object_unref (registry);
}

//-- constructor methods

/**
 * bt_playback_controller_midi_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtPlaybackControllerMidi *
bt_playback_controller_midi_new (void)
{
  return
      BT_PLAYBACK_CONTROLLER_MIDI (g_object_new
      (BT_TYPE_PLAYBACK_CONTROLLER_MIDI, NULL));
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_playback_controller_midi_dispose (GObject * object)
{
  BtPlaybackControllerMidi *self = BT_PLAYBACK_CONTROLLER_MIDI (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_try_weak_unref (self->priv->app);

  G_OBJECT_CLASS (bt_playback_controller_midi_parent_class)->dispose (object);
}

static void
bt_playback_controller_midi_finalize (GObject * object)
{
  BtPlaybackControllerMidi *self = BT_PLAYBACK_CONTROLLER_MIDI (object);

  GST_DEBUG ("!!!! self=%p", self);

  G_OBJECT_CLASS (bt_playback_controller_midi_parent_class)->finalize (object);
}

static void
bt_playback_controller_midi_init (BtPlaybackControllerMidi * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_PLAYBACK_CONTROLLER_MIDI,
      BtPlaybackControllerMidiPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  /* this is created from the app, we need to avoid a ref-cycle */
  self->priv->app = bt_edit_application_new ();
  g_object_try_weak_ref (self->priv->app);
  g_object_unref (self->priv->app);

  // check settings
  devices_listen (self);
  settings_listen (self);
}

static void
bt_playback_controller_midi_class_init (BtPlaybackControllerMidiClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtPlaybackControllerMidiPrivate));

  gobject_class->dispose = bt_playback_controller_midi_dispose;
  gobject_class->finalize = bt_playback_controller_midi_finalize;
}
