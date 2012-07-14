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
 * SECTION:btaudiosession
 * @short_description: bin to be used by #BtSinkMachine
 *
 * The audio-session provides a persistent audio-sink for some classes. This
 * e.g. ensures a persistent presence in qjackctrl if jackaudiosink is used.
 *
 * The top #BtApplication should create one and dispose it at the end of the
 * lifecycle. The audio-session is a singleton, parts in the code can just call
 * bt_audio_session_new() to get the instance.
 */
/* It turns out that what you see in qjackctrl are the ports and those are 
 * showing up from individual sinks and sources, the client is reused already.
 * So we need to keep the actual element alive!
 *
 * TODO(ensonic): try this for pulsesink too?
 * IDEA(ensonic): name it media-session?
 */

#define BT_CORE
#define BT_AUDIO_SESSION_C

#include "core_private.h"
#include "audio-session.h"

//-- property ids

enum
{
  AUDIO_SESSION_AUDIO_SINK = 1,
  AUDIO_SESSION_AUDIO_SINK_NAME,
  AUDIO_SESSION_AUDIO_LOCKED,
  AUDIO_SESSION_TRANSPORT_MODE,

};

struct _BtAudioSessionPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the active audio sink */
  gchar *audio_sink_name;
  GstElement *audio_sink;

  BtSettings *settings;
};

static BtAudioSession *singleton = NULL;

//-- the class

G_DEFINE_TYPE (BtAudioSession, bt_audio_session, G_TYPE_OBJECT);

//-- signal handlers

static void
update_tranport_mode (void)
{
  if (singleton->priv->audio_sink &&
      g_object_class_find_property (G_OBJECT_GET_CLASS (singleton->
              priv->audio_sink), "transport")
      ) {
    gboolean master, slave;

    g_object_get (singleton->priv->settings,
        "jack-transport-master", &master, "jack-transport-slave", &slave, NULL);
    g_object_set (singleton->priv->audio_sink, "transport",
        ((master << 0) | (slave << 1)), NULL);
  }
}

static void
on_jack_transport_mode_changed (const BtSettings * const settings,
    GParamSpec * const arg, gconstpointer const user_data)
{
  update_tranport_mode ();
}

//-- helper methods

static void
bt_audio_session_cleanup (void)
{
  if (singleton->priv->audio_sink) {
    GST_WARNING ("forgetting session audio sink %p, ref=%d",
        singleton->priv->audio_sink,
        G_OBJECT_REF_COUNT (singleton->priv->audio_sink));
    gst_element_set_state (singleton->priv->audio_sink, GST_STATE_NULL);
    gst_object_unref (singleton->priv->audio_sink);
    singleton->priv->audio_sink = NULL;
  }
}

static void
bt_audio_session_setup (void)
{
  gchar *plugin_name = singleton->priv->audio_sink_name;
  if (!strcmp (plugin_name, "jackaudiosink")) {
    GstElement *audio_sink = singleton->priv->audio_sink;

    if (!audio_sink) {
      GstElement *bin;
      GstElement *sink, *src;
      GstBus *bus;
      GstMessage *message;
      gboolean loop = TRUE;

      // create audio sink and drop floating ref
      audio_sink =
          gst_object_ref (gst_element_factory_make (plugin_name, NULL));
      gst_object_sink (audio_sink);
      GST_WARNING ("created session audio sink %p, ref=%d", audio_sink,
          G_OBJECT_REF_COUNT (audio_sink));

      // we need this hack to make the ports show up
      bin = gst_pipeline_new ("__kickstart__");
      bus = gst_element_get_bus (bin);
      src = gst_element_factory_make ("audiotestsrc", NULL);
      sink = gst_object_ref (audio_sink);
      gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
      gst_element_link (src, sink);
      gst_element_set_state (bin, GST_STATE_PAUSED);

      while (loop) {
        message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);
        switch (message->type) {
          case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
              GstState oldstate, newstate;

              gst_message_parse_state_changed (message, &oldstate, &newstate,
                  NULL);
              if (GST_STATE_TRANSITION (oldstate,
                      newstate) == GST_STATE_CHANGE_READY_TO_PAUSED)
                loop = FALSE;
            }
            break;
          case GST_MESSAGE_ERROR:
            loop = FALSE;
            break;
          default:
            break;
        }
        gst_message_unref (message);
      }
      gst_object_unref (bus);

      gst_element_set_state (bin, GST_STATE_READY);
      gst_element_set_locked_state (audio_sink, TRUE);
      gst_element_set_state (bin, GST_STATE_NULL);
      gst_object_unref (bin);

      singleton->priv->audio_sink = audio_sink;
      update_tranport_mode ();
    } else {
      GstObject *parent;
      // we know that we're not playing anymore, but due to ref-counting the
      // audio_sink might be still plugged, if so steal it!
      if ((parent = gst_object_get_parent (GST_OBJECT (audio_sink)))) {
        gst_bin_remove (GST_BIN (parent), audio_sink);
        gst_element_set_state (audio_sink, GST_STATE_READY);
        gst_element_set_locked_state (audio_sink, TRUE);
        gst_object_unref (parent);
        GST_WARNING ("stole session audio sink %p, ref=%d", audio_sink,
            G_OBJECT_REF_COUNT (audio_sink));
      } else {
        GST_WARNING ("reuse session audio sink %p, ref=%d", audio_sink,
            G_OBJECT_REF_COUNT (audio_sink));
      }
    }
  } else {
    bt_audio_session_cleanup ();
  }
}

//-- constructor methods

/**
 * bt_audio_session_new:
 *
 * Create a new audio-session or return the existing one. The audio session
 * keeps the audio setup alive across songs. An application can only have one
 * audio-session. This method can be called several times though.
 *
 * Returns: the audio-session, unref when done.
 */
BtAudioSession *
bt_audio_session_new (void)
{
  return (BT_AUDIO_SESSION (g_object_new (BT_TYPE_AUDIO_SESSION, NULL)));
}

//-- methods

//-- wrapper

//-- class internals

static GObject *
bt_audio_session_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  GObject *object;

  if (G_UNLIKELY (!singleton)) {
    object =
        G_OBJECT_CLASS (bt_audio_session_parent_class)->constructor (type,
        n_construct_params, construct_params);
    singleton = BT_AUDIO_SESSION (object);
    g_object_add_weak_pointer (object, (gpointer *) (gpointer) & singleton);
  } else {
    object = g_object_ref (singleton);
  }
  return object;
}

static void
bt_audio_session_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtAudioSession *const self = BT_AUDIO_SESSION (object);
  return_if_disposed ();
  switch (property_id) {
    case AUDIO_SESSION_AUDIO_SINK:
      g_value_set_object (value, self->priv->audio_sink);
      break;
    case AUDIO_SESSION_AUDIO_SINK_NAME:
      g_value_set_string (value, self->priv->audio_sink_name);
      break;
    case AUDIO_SESSION_AUDIO_LOCKED:
      g_value_set_boolean (value,
          (self->priv->audio_sink ? gst_element_is_locked_state (self->
                  priv->audio_sink) : FALSE));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_audio_session_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtAudioSession *const self = BT_AUDIO_SESSION (object);
  return_if_disposed ();

  switch (property_id) {
    case AUDIO_SESSION_AUDIO_SINK_NAME:
      g_free (self->priv->audio_sink_name);
      self->priv->audio_sink_name = g_value_dup_string (value);
      bt_audio_session_setup ();
      break;
    case AUDIO_SESSION_AUDIO_LOCKED:
      if (self->priv->audio_sink) {
        gboolean state = g_value_get_boolean (value);
        GST_WARNING_OBJECT (self, "%slock audio sink", (state ? "" : "un"));
        gst_element_set_locked_state (self->priv->audio_sink, state);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_audio_session_dispose (GObject * const object)
{
  const BtAudioSession *const self = BT_AUDIO_SESSION (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;
  GST_INFO ("!!!! self=%p", self);

  g_signal_handlers_disconnect_matched (self->priv->settings,
      G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (gpointer) self);
  g_object_unref (self->priv->settings);

  bt_audio_session_cleanup ();

  GST_INFO ("chaining up");
  G_OBJECT_CLASS (bt_audio_session_parent_class)->dispose (object);
  GST_INFO ("done");
}

static void
bt_audio_session_finalize (GObject * const object)
{
  const BtAudioSession *const self = BT_AUDIO_SESSION (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->audio_sink_name);

  G_OBJECT_CLASS (bt_audio_session_parent_class)->finalize (object);
}

static void
bt_audio_session_init (BtAudioSession * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_AUDIO_SESSION,
      BtAudioSessionPrivate);

  GST_INFO ("!!!! self=%p", self);

  // watch settings changes
  self->priv->settings = bt_settings_make ();
  g_signal_connect (self->priv->settings, "notify::jack-transport-master",
      G_CALLBACK (on_jack_transport_mode_changed), (gpointer) self);
  g_signal_connect (self->priv->settings, "notify::jack-transport-slave",
      G_CALLBACK (on_jack_transport_mode_changed), (gpointer) self);

  GST_INFO ("done");
}

static void
bt_audio_session_class_init (BtAudioSessionClass * klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtAudioSessionPrivate));

  gobject_class->constructor = bt_audio_session_constructor;
  gobject_class->set_property = bt_audio_session_set_property;
  gobject_class->get_property = bt_audio_session_get_property;
  gobject_class->dispose = bt_audio_session_dispose;
  gobject_class->finalize = bt_audio_session_finalize;

  g_object_class_install_property (gobject_class, AUDIO_SESSION_AUDIO_SINK, g_param_spec_object ("audio-sink", "audio-sink prop", "the audio-sink for the session", GST_TYPE_ELEMENT,   /* object type */
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, AUDIO_SESSION_AUDIO_SINK_NAME, g_param_spec_string ("audio-sink-name", "audio-sink-name prop", "The name of the audio sink factory", NULL,    /* default value */
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, AUDIO_SESSION_AUDIO_LOCKED,
      g_param_spec_boolean ("audio-locked",
          "audio-locked prop",
          "locked state for the audio-sink",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}
