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
  AUDIO_SESSION_AUDIO_SINK_DEVICE,
  AUDIO_SESSION_AUDIO_LOCKED,
  AUDIO_SESSION_TRANSPORT_MODE,

};

struct _BtAudioSessionPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the active audio sink */
  gchar *audio_sink_name, *audio_sink_device;
  GstElement *audio_sink;

  BtSettings *settings;
};

static BtAudioSession *singleton = NULL;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtAudioSession, bt_audio_session, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtAudioSession));

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
    GST_WARNING ("forgetting session audio sink %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (singleton->priv->audio_sink));
    gst_element_set_state (singleton->priv->audio_sink, GST_STATE_NULL);
    gst_object_unref (singleton->priv->audio_sink);
    singleton->priv->audio_sink = NULL;
  }
}

static gboolean
bt_audio_session_sink_is_server (void)
{
  gchar *element_name = singleton->priv->audio_sink_name;
  // ->audio_sink_device is not used to detect a session sinks right now
  /* TODO(ensonic): improve heuristic, both pulsesink and jackaudiosink:
   * - have a property 'server' with the server name
   * - habe a property 'client-name' to name the connected client
   */
  return !strcmp (element_name, "jackaudiosink");
}

static void
bt_audio_session_setup (void)
{
  if (bt_audio_session_sink_is_server ()) {
    GstElement *audio_sink = singleton->priv->audio_sink;

    if (!audio_sink) {
      GstElement *bin;
      GstElement *sink, *src;
      gchar *element_name = singleton->priv->audio_sink_name;

      // create audio sink and drop floating ref
      audio_sink = gst_element_factory_make (element_name, NULL);
      gst_object_ref_sink (audio_sink);
      GST_WARNING ("created session audio sink %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (audio_sink));

      // we need this hack to make the ports show up
      bin = gst_pipeline_new ("__kickstart__");
      src = gst_element_factory_make ("audiotestsrc", NULL);
      sink = gst_object_ref (audio_sink);
      gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
      if (!gst_element_link (src, sink)) {
        GST_WARNING_OBJECT (bin, "can't link elements: src=%s, sink=%s",
            GST_OBJECT_NAME (src), GST_OBJECT_NAME (sink));
        gst_object_unref (audio_sink);
      } else {
        GstBus *bus;
        GstMessage *msg;
        gboolean loop = TRUE;

        gst_element_set_state (bin, GST_STATE_PAUSED);

        bus = gst_element_get_bus (bin);
        while (loop) {
          msg = gst_bus_poll (bus,
              GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR |
              GST_MESSAGE_WARNING, GST_CLOCK_TIME_NONE);
          if (!msg)
            continue;

          switch (msg->type) {
            case GST_MESSAGE_STATE_CHANGED:
              if (GST_MESSAGE_SRC (msg) == GST_OBJECT (bin)) {
                GstState oldstate, newstate;

                gst_message_parse_state_changed (msg, &oldstate, &newstate,
                    NULL);
                if (GST_STATE_TRANSITION (oldstate,
                        newstate) == GST_STATE_CHANGE_READY_TO_PAUSED)
                  loop = FALSE;
              }
              break;
            case GST_MESSAGE_ERROR:
              BT_GST_LOG_MESSAGE_ERROR (msg, NULL, NULL);
              loop = FALSE;
              break;
            case GST_MESSAGE_WARNING:
              BT_GST_LOG_MESSAGE_WARNING (msg, NULL, NULL);
              break;
            default:
              break;
          }
          gst_message_unref (msg);
        }
        gst_object_unref (bus);

        gst_element_set_state (bin, GST_STATE_READY);
        gst_element_set_locked_state (audio_sink, TRUE);
        gst_element_set_state (bin, GST_STATE_NULL);
        singleton->priv->audio_sink = audio_sink;
        update_tranport_mode ();
      }
      gst_object_unref (bin);
    } else {
      GstObject *parent;
      // we know that we're not playing anymore, but due to ref-counting the
      // audio_sink might be still plugged, if so steal it!
      if ((parent = gst_object_get_parent (GST_OBJECT (audio_sink)))) {
        gst_bin_remove (GST_BIN (parent), audio_sink);
        gst_element_set_state (audio_sink, GST_STATE_READY);
        gst_element_set_locked_state (audio_sink, TRUE);
        gst_object_unref (parent);
        GST_WARNING ("stole session audio sink %" G_OBJECT_REF_COUNT_FMT,
            G_OBJECT_LOG_REF_COUNT (audio_sink));
      } else {
        GST_WARNING ("reuse session audio sink %" G_OBJECT_REF_COUNT_FMT,
            G_OBJECT_LOG_REF_COUNT (audio_sink));
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
  return BT_AUDIO_SESSION (g_object_new (BT_TYPE_AUDIO_SESSION, NULL));
}

//-- methods

/**
 * bt_audio_session_get_sink_for:
 * @element_name: the element name of the sink
 * @device_name: optional device name (not implemented)
 *
 * If the element is a sound server return the a persistent session sink for it.
 *
 * Since: 0.11
 * Returns: the #GstElement or %NULL, unref when done.
 */
GstElement *
bt_audio_session_get_sink_for (const gchar * element_name,
    const gchar * device_name)
{
  g_return_val_if_fail (element_name, NULL);

  if (G_LIKELY (singleton)) {
    g_object_set (singleton, "audio-sink-device", device_name,
        "audio-sink-name", element_name, NULL);
    if (singleton->priv->audio_sink) {
      return g_object_ref (singleton->priv->audio_sink);
    }
  }
  return NULL;
}

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
    object = g_object_ref ((gpointer) singleton);
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
    case AUDIO_SESSION_AUDIO_SINK_DEVICE:
      g_value_set_string (value, self->priv->audio_sink_device);
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
    case AUDIO_SESSION_AUDIO_SINK_DEVICE:
      g_free (self->priv->audio_sink_device);
      self->priv->audio_sink_device = g_value_dup_string (value);
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
  g_free (self->priv->audio_sink_device);

  G_OBJECT_CLASS (bt_audio_session_parent_class)->finalize (object);
}

static void
bt_audio_session_init (BtAudioSession * self)
{
  self->priv = bt_audio_session_get_instance_private(self);

  GST_INFO ("!!!! self=%p", self);

  // watch settings changes
  self->priv->settings = bt_settings_make ();
  g_signal_connect_object (self->priv->settings,
      "notify::jack-transport-master",
      G_CALLBACK (on_jack_transport_mode_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->settings, "notify::jack-transport-slave",
      G_CALLBACK (on_jack_transport_mode_changed), (gpointer) self, 0);

  GST_INFO ("done");
}

static void
bt_audio_session_class_init (BtAudioSessionClass * klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = bt_audio_session_constructor;
  gobject_class->set_property = bt_audio_session_set_property;
  gobject_class->get_property = bt_audio_session_get_property;
  gobject_class->dispose = bt_audio_session_dispose;
  gobject_class->finalize = bt_audio_session_finalize;

  g_object_class_install_property (gobject_class, AUDIO_SESSION_AUDIO_SINK,
      g_param_spec_object ("audio-sink", "audio-sink prop",
          "the audio-sink for the session", GST_TYPE_ELEMENT,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, AUDIO_SESSION_AUDIO_SINK_NAME,
      g_param_spec_string ("audio-sink-name", "audio-sink-name prop",
          "The name of the audio sink factory", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      AUDIO_SESSION_AUDIO_SINK_DEVICE,
      g_param_spec_string ("audio-sink-device", "audio-sink-device prop",
          "The name of the audio sink device", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, AUDIO_SESSION_AUDIO_LOCKED,
      g_param_spec_boolean ("audio-locked",
          "audio-locked prop",
          "locked state for the audio-sink",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}
