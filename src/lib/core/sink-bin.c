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
 * SECTION:btsinkbin
 * @short_description: bin to be used by #BtSinkMachine
 *
 * The sink-bin provides switchable play and record facilities.
 * It also provides controllable master-volume.
 *
 * In play and record modes it plugs a chain of elements. In combined play and
 * record mode it uses a tee and plugs both pipelines.
 */

/* TODO(ensonic): add properties for bpm, master volume and musical key,
 * - bpm
 *   - tempo-iface is implemented, but is hidden from the ui
 *     (the iface properties are not controllable)
 *   - we could have separate properties and forward the changes
 * - key
 *   - this would need to propagate to all sources that support tuning
 */
/* TODO(ensonic): for upnp it would be nice to stream on-demand */
/* TODO(ensonic): add a metronome
 * - a clock properties to sink-bin:
 *   - two guint properties "beat", "tick"
 *   - a float/double property: "beat.tick":
 *     - 3.0, 3.25, 3.50, 3.75, 4.0
 *     - 3.0, 3.1,  3.2,  3.3,  4.0
 * - use a pad-probe like master_volume_sync_handler (consider reusing) to sync them
 *   - this is followed by volume ! level ! capsfilter ! audioresample ! tee ! audiosink
 *   - this might be good to compensate the processing latency for control-out
 *   - we could also plug a fakesink/appsink to the tee
 * - we need to calculate beat,tick from the buffer ts
 *   subtick = ts / chunk
 *   tick = ts / (chunk * subticks_per_tick)
 *   beat = (gint) tick / ticks_per_beat
 * - the gtk-ui can have two on-screen leds
 * - sink-bin could use the keyboard leds to indicate them (with #ifdef LINUX)
 *   - need root permissions :/
 * - we can also use this to implement control-out machines
 */
/* TODO(ensonic): work around some gstreamer limitations here
 * - our pipeline has no natural duration
 * - there is no api to set the duration on sources
 * - we 'set' the duration by seeking with a stop position, we also sync
 *   basesrc->segment.duration to sequence->length in BtMachine
 * - some muxers reject seeks, in which case we play endlessly when recording
 *   - we need a pad-probe in the recording case to watch the timestamp on
 *     passing buffers and send an eos when ts > duration
 *   - we also need a way to figure the duration though, there is no duration
 *     event and we won't get a new-segment as the seek was rejected by the
 *     muxer
 * - some recording elements send duration queries, which no one answers
 *   - we could answer duration queries on the bin
 * - use a silent (GAP) source connected to the master
 *   - unfortunately we need to make all sources report the duration
 */
/* TODO(ensonic): improve encoding profiles
 * - we'd like to extend profiles
 *   - ideally gstreamer encoding profiles are stored somewhere and we just read
 *     them from an ini file or so
 *   - we could move the profiles to a BtAudioEncodingProfiles class
 *     - this could have the API do add, remove and probe profiles
 *     - test could easily e.g. add fake profiles then
 */

#define BT_CORE
#define BT_SINK_BIN_C

#include "core_private.h"
#include "sink-bin.h"
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiobasesink.h>
#include <gst/base/gstbasesink.h>
#include <gst/pbutils/encoding-profile.h>
#include <gst/pbutils/missing-plugins.h>
#include "gst/tempo.h"

/* define this to get diagnostics of the sink data flow */
//#define BT_MONITOR_SINK_DATA_FLOW
/* define this to verify continuous timestamps */
//#define BT_MONITOR_TIMESTAMPS

//-- property ids

enum
{
  SINK_BIN_MODE = 1,
  SINK_BIN_RECORD_FORMAT,
  SINK_BIN_RECORD_FILE_NAME,
  SINK_BIN_INPUT_GAIN,
  SINK_BIN_MASTER_VOLUME,
  SINK_BIN_ANALYZERS,
};

typedef enum
{
  RECORD_FORMAT_STATE_MISSES_ELEMENTS = -1,
  RECORD_FORMAT_STATE_NOT_CHECKED = 0,
  RECORD_FORMAT_STATE_WORKING = 1,
} BtSinkBinRecordFormatState;

typedef struct
{
  gchar *name;
  gchar *desc;
  gchar *container_caps;
  gchar *audio_caps;
} BtSinkBinRecordFormatInfo;

static const BtSinkBinRecordFormatInfo formats[] = {
  {"Ogg Vorbis record format", "Ogg Vorbis", "audio/ogg", "audio/x-vorbis"},
  {"MP3 record format", "MP3 Audio", "application/x-id3",
      "audio/mpeg, mpegversion=1, layer=3"},
  {"WAV record format", "WAV Audio", "audio/x-wav",
      "audio/x-raw, format=(string)S16LE"},
  {"Ogg Flac record format", "Ogg Flac", "audio/ogg", "audio/x-flac"},
  {"Raw format", "Raw", NULL, NULL},
  {"M4A record format", "M4A Audio", "video/quicktime",
      "audio/mpeg, mpegversion=(int)4"},
  {"Flac record format", "Flac", NULL, "audio/x-flac"},
  {"Ogg Opus record format", "Ogg Opus", "audio/ogg", "audio/x-opus"}
};

BtSinkBinRecordFormatState format_states[] = {
  RECORD_FORMAT_STATE_NOT_CHECKED,
  RECORD_FORMAT_STATE_NOT_CHECKED,
  RECORD_FORMAT_STATE_NOT_CHECKED,
  RECORD_FORMAT_STATE_NOT_CHECKED,
  RECORD_FORMAT_STATE_WORKING,  /* RAW */
  RECORD_FORMAT_STATE_NOT_CHECKED,
  RECORD_FORMAT_STATE_NOT_CHECKED,
  RECORD_FORMAT_STATE_NOT_CHECKED
};

struct _BtSinkBinPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* change configuration at next convenient point? */
  gboolean pending_update;

  /* mode of operation */
  BtSinkBinMode mode;
  BtSinkBinRecordFormat record_format;
  guint sample_rate, channels;

  gchar *record_file_name;

  /* ghost-pads of the bin */
  GstPad *sink, *src;

  /* we need to hold the reference to not kill the notifies */
  BtSettings *settings;

  /* player sink or NULL */
  GstElement *audio_sink;

  /* tee */
  GstElement *tee;

  /* master volume */
  GstElement *gain;
  gulong mv_handler_id;
  gdouble volume;

  /* sink format */
  GstElement *caps_filter;

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;

  /* master analyzers */
  GList *analyzers;
};

//-- prototypes

static void bt_sink_bin_configure_latency (const BtSinkBin * const self);
static void on_audio_sink_child_added (GstBin * bin, GstElement * element,
    gpointer user_data);

//-- the class

G_DEFINE_TYPE (BtSinkBin, bt_sink_bin, GST_TYPE_BIN);

//-- enums

GType
bt_sink_bin_mode_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_SINK_BIN_MODE_PLAY, "BT_SINK_BIN_MODE_PLAY", "play"},
      {BT_SINK_BIN_MODE_RECORD, "BT_SINK_BIN_MODE_RECORD", "record"},
      {BT_SINK_BIN_MODE_PLAY_AND_RECORD, "BT_SINK_BIN_MODE_PLAY_AND_RECORD",
          "play-and-record"},
      {BT_SINK_BIN_MODE_PASS_THRU, "BT_SINK_BIN_MODE_PASS_THRU", "pass-thru"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtSinkBinMode", values);
  }
  return type;
}

GType
bt_sink_bin_record_format_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, ".vorbis.ogg", "ogg-vorbis"},
      {BT_SINK_BIN_RECORD_FORMAT_MP3, ".mp3", "mp3"},
      {BT_SINK_BIN_RECORD_FORMAT_WAV, ".wav", "wav"},
      {BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC, ".flac.ogg", "ogg-flac"},
      {BT_SINK_BIN_RECORD_FORMAT_RAW, ".raw", "raw"},
      {BT_SINK_BIN_RECORD_FORMAT_MP4_AAC, ".m4a", "mp4-aac"},
      {BT_SINK_BIN_RECORD_FORMAT_FLAC, ".flac", "flac"},
      {BT_SINK_BIN_RECORD_FORMAT_OGG_OPUS, ".opus.ogg", "ogg-opus"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtSinkBinRecordFormat", values);
  }
  return type;
}

//-- profile method

static GstEncodingProfile *
bt_sink_bin_create_recording_profile (const BtSinkBinRecordFormatInfo * info)
{
  GstEncodingContainerProfile *c_profile = NULL;
  GstEncodingAudioProfile *a_profile = NULL;
  GstCaps *caps;

  GST_INFO ("create profile for \"%s\", \"%s\", \"%s\"", info->name,
      info->container_caps, info->audio_caps);

  if ((!info->container_caps) && (!info->audio_caps)) {
    GST_INFO ("no container nor audio-caps \"%s\"", info->name);
    return NULL;
  }
  // generate a container profile if requested
  if (info->container_caps) {
    if (!(caps = gst_caps_from_string (info->container_caps))) {
      GST_WARNING ("can't parse caps \"%s\" for \"%s\"", info->container_caps,
          info->name);
      return NULL;
    }
    if (!(c_profile =
            gst_encoding_container_profile_new (info->name, info->desc, caps,
                NULL))) {
      GST_WARNING ("can't create container profile for caps \"%s\" for \"%s\"",
          info->container_caps, info->name);
    }
    gst_caps_unref (caps);
  }
  // generate an audio profile
  if (!(caps = gst_caps_from_string (info->audio_caps))) {
    GST_WARNING ("can't parse caps \"%s\" for \"%s\"", info->audio_caps,
        info->name);
    return NULL;
  }
  if (!(a_profile = gst_encoding_audio_profile_new (caps, NULL, NULL, 1))) {
    GST_WARNING ("can't create audio profile for caps \"%s\" for \"%s\"",
        info->audio_caps, info->name);
  }
  gst_caps_unref (caps);

  if (c_profile) {
    gst_encoding_container_profile_add_profile (c_profile,
        (GstEncodingProfile *)
        a_profile);
    return (GstEncodingProfile *) c_profile;
  } else {
    return (GstEncodingProfile *) a_profile;
  }
}

//-- helper methods

/*
 * bt_sink_bin_activate_analyzers:
 * @self: the sink-bin
 *
 * Add all analyzers to the bin and link them.
 */
static void
bt_sink_bin_activate_analyzers (const BtSinkBin * const self)
{
  GstState state;
  gboolean is_playing = TRUE;

  if (!self->priv->analyzers)
    return;

  //g_object_get(self->priv->song,"is-playing",&is_playing,NULL);
  if ((gst_element_get_state (GST_ELEMENT (self), &state, NULL,
              GST_MSECOND) == GST_STATE_CHANGE_SUCCESS)) {
    if (state < GST_STATE_PAUSED)
      is_playing = FALSE;
  }
  bt_bin_activate_tee_chain (GST_BIN (self), self->priv->tee,
      self->priv->analyzers, is_playing);
}

/*
 * bt_sink_bin_deactivate_analyzers:
 * @self: the sink-bin
 *
 * Remove all analyzers to the bin and unlink them.
 */
static void
bt_sink_bin_deactivate_analyzers (const BtSinkBin * const self)
{
  GstState state;
  gboolean is_playing = TRUE;

  if (!self->priv->analyzers)
    return;

  //g_object_get(self->priv->song,"is-playing",&is_playing,NULL);
  if ((gst_element_get_state (GST_ELEMENT (self), &state, NULL,
              GST_MSECOND) == GST_STATE_CHANGE_SUCCESS)) {
    if (state < GST_STATE_PAUSED)
      is_playing = FALSE;
  }
  bt_bin_deactivate_tee_chain (GST_BIN (self), self->priv->tee,
      self->priv->analyzers, is_playing);
}

static void
bt_sink_bin_configure_latency (const BtSinkBin * const self)
{
  GstElement *sink = self->priv->audio_sink;
  if (GST_IS_AUDIO_BASE_SINK (sink)) {
    if (self->priv->beats_per_minute && self->priv->ticks_per_beat
        && self->priv->subticks_per_tick) {
      // we configure stpb in machine.c
      const gdouble div =
          self->priv->beats_per_minute * self->priv->ticks_per_beat *
          self->priv->subticks_per_tick;
      // configure buffer size
      gint64 chunk = GST_TIME_AS_USECONDS ((GST_SECOND * 60) / div);

      GST_INFO_OBJECT (sink,
          "changing audio chunk-size to %" G_GUINT64_FORMAT " µs = %"
          G_GUINT64_FORMAT " ms", chunk, (chunk / G_GINT64_CONSTANT (1000)));
      g_object_set (sink, "latency-time", chunk, "buffer-time", chunk << 1,
          NULL);
    }
  }
}

static void
bt_sink_bin_set_audio_sink (const BtSinkBin * const self, GstElement * sink)
{
  // disable deep notify
  {
    GObjectClass *gobject_class = G_OBJECT_GET_CLASS (sink);
    GObjectClass *parent_class = g_type_class_peek_static (G_TYPE_OBJECT);
    gobject_class->dispatch_properties_changed =
        parent_class->dispatch_properties_changed;
  }
  g_object_set (sink,
      /* enable syncing buffer timestamps to the clock */
      "sync", TRUE,
      /* no need to keep the last rendered buffer */
      "enable-last-sample", FALSE,
      /* try to avoid any gaps, see design/gst/underrun.c */
      "max-lateness", G_GUINT64_CONSTANT (0),
      /* if we do this, live pipelines go playing, but:
       * - non-live sources don't start
       * - scrubbing on timeline and fast seeking is broken
       */
      //"async", FALSE,
      /* this does not lockup anymore, but does not give us better latencies */
      //"can-activate-pull",TRUE,  // default is FALSE
      NULL);
  if (GST_IS_AUDIO_BASE_SINK (sink)) {
    g_object_set (sink,
        /* try to avoid any gaps, see design/gst/underrun.c */
        "discont-wait", G_GUINT64_CONSTANT (0),
        /* default is 1: skew, this should not be required if the audio sink
         * provides the clock
         * trying 2: none, this helps trickplay and scrubbing, but this should be
         * auto-selected by audiobasesink anyway since clock==provided_clock
         */
        "slave-method", 2,
        //"provide-clock", FALSE,  // default is TRUE
        NULL);
  }
  self->priv->audio_sink = sink;
  bt_sink_bin_configure_latency (self);
}

static void
bt_sink_bin_clear (const BtSinkBin * const self)
{
  GstBin *const bin = GST_BIN (self);
  GstElement *elem;

  GST_DEBUG_OBJECT (self, "clearing sink-bin : %d", GST_BIN_NUMCHILDREN (bin));
  if (bin->children) {
    GstStateChangeReturn res;

    self->priv->audio_sink = NULL;
    self->priv->caps_filter = NULL;
    self->priv->tee = NULL;

    // does not seem to be needed
    //gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),NULL);
    //GST_DEBUG("released ghost-pad");

    while (bin->children) {
      elem = GST_ELEMENT_CAST (bin->children->data);
      GST_DEBUG_OBJECT (elem, "  removing elem=%" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (elem));
      gst_element_set_locked_state (elem, FALSE);
      if ((res =
              gst_element_set_state (elem,
                  GST_STATE_NULL)) == GST_STATE_CHANGE_FAILURE)
        GST_WARNING_OBJECT (self, "can't go to null state");
      else
        GST_DEBUG_OBJECT (self, "->NULL state change returned '%s'",
            gst_element_state_change_return_get_name (res));
      gst_bin_remove (bin, elem);
    }

    if (self->priv->src) {
      gst_pad_set_active (self->priv->src, FALSE);
      gst_element_remove_pad (GST_ELEMENT (self), self->priv->src);
      self->priv->src = NULL;
    }
  }
  GST_DEBUG_OBJECT (self, "done");
}

static gboolean
bt_sink_bin_add_many (const BtSinkBin * const self, GList * const list)
{
  const GList *node;
  GstElement *elem;

  GST_DEBUG_OBJECT (self, "add elements: list=%p", list);

  for (node = list; node; node = node->next) {
    elem = GST_ELEMENT_CAST (node->data);
    GST_DEBUG_OBJECT (elem, "  adding elem=%" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (elem));
    if (!gst_bin_add (GST_BIN (self), elem)) {
      GST_WARNING_OBJECT (self, "can't add element: elem=%s, parent=%s",
          GST_OBJECT_NAME (elem), GST_OBJECT_NAME (GST_OBJECT_PARENT (elem)));
    } else {
      if (!gst_element_sync_state_with_parent (elem)) {
        GST_WARNING_OBJECT (self, "can't sync element to parent state: elem=%s",
            GST_OBJECT_NAME (elem));
      }
    }
  }
  return TRUE;
}

static void
bt_sink_bin_link_many (const BtSinkBin * const self, GstElement * last_elem,
    GList * const list)
{
  const GList *node;

  GST_DEBUG_OBJECT (self, "link elements: last_elem=%s, list=%p",
      (last_elem ? GST_OBJECT_NAME (last_elem) : "<NULL>"), list);
  if (!last_elem || !list)
    return;

  for (node = list; node; node = node->next) {
    GstElement *const cur_elem = GST_ELEMENT (node->data);

    if (!gst_element_link (last_elem, cur_elem)) {
      GST_WARNING_OBJECT (self,
          "can't link elements: last_elem=%s, cur_elem=%s",
          GST_OBJECT_NAME (last_elem), GST_OBJECT_NAME (cur_elem));
    }
    last_elem = cur_elem;
  }
}

static GList *
bt_sink_bin_get_player_elements (const BtSinkBin * const self)
{
  GList *list = NULL;
  gchar *element_name, *device_name;
  GstElement *element;

  GST_DEBUG_OBJECT (self, "get playback elements");

  // need to also get the "audiosink-device" setting
  if (!bt_settings_determine_audiosink_name (self->priv->settings,
          &element_name, &device_name)) {
    return NULL;
  }
  element = bt_audio_session_get_sink_for (element_name, device_name);
  if (!element) {
    GST_WARNING ("No session sink for '%s'", element_name);
    element = gst_element_factory_make (element_name, NULL);
    if (BT_IS_STRING (device_name)) {
      g_object_set (element, "device", device_name, NULL);
    }
  }
  if (!element) {
    /* bt_settings_determine_audiosink() does all kind of sanity checks already */
    GST_WARNING ("Can't instantiate '%s' element", element_name);
    goto Error;
  }
  if (GST_IS_BASE_SINK (element)) {
    bt_sink_bin_set_audio_sink (self, element);
  } else if (GST_IS_BIN (element)) {
    g_signal_connect (element, "element-added",
        G_CALLBACK (on_audio_sink_child_added), (gpointer) self);
  }
  list = g_list_append (list, element);

Error:
  g_free (element_name);
  g_free (device_name);
  return list;
}

GstElement *
bt_sink_bin_make_and_configure_encodebin (GstEncodingProfile * profile)
{
  GstBus *bus = gst_bus_new ();
  GstElement *element;
  GstMessage *message;
  gboolean has_error = FALSE;

  element = gst_element_factory_make ("encodebin", "sink-encodebin");
  gst_element_set_bus (element, bus);

  GST_DEBUG_OBJECT (element, "set profile");
  // TODO(ensonic): this will post missing element mesages if the profile
  // cannot be satisfied, we catch them here right now, as the elements is not
  // yet in the pipeline. See https://bugzilla.gnome.org/show_bug.cgi?id=722980
  g_object_set (element, "profile", profile, NULL);
  while ((message = gst_bus_pop_filtered (bus,
              GST_MESSAGE_ERROR | GST_MESSAGE_ELEMENT))) {
    GST_INFO_OBJECT (element, "error message %" GST_PTR_FORMAT, message);
    if (gst_is_missing_plugin_message (message)) {
      GstObject *src = GST_MESSAGE_SRC (message);
      gchar *detail = gst_missing_plugin_message_get_description (message);

      GST_WARNING_OBJECT (src, "%s", detail);
      // TODO(ensonic): tell the app (forward the message?)
      has_error = TRUE;
    }
  }

  gst_element_set_bus (element, NULL);
  gst_object_unref (bus);

  if (has_error) {
    gst_object_unref (element);
    element = NULL;
  }

  return element;
}

static GList *
bt_sink_bin_get_recorder_elements (const BtSinkBin * const self)
{
  GList *list = NULL;
  GstElement *element;
  GstEncodingProfile *profile = NULL;

  GST_DEBUG_OBJECT (self, "get record elements");

  // TODO(ensonic): check extension ?

  // generate recorder profile and set encodebin accordingly
  profile =
      bt_sink_bin_create_recording_profile (&formats[self->priv->
          record_format]);
  if (profile) {
    if ((element = bt_sink_bin_make_and_configure_encodebin (profile))) {
      list = g_list_append (list, element);
    }
    g_object_unref (profile);
  } else {
    GST_DEBUG_OBJECT (self, "no profile, do raw recording");
    // encodebin starts with a queue already
    element = gst_element_factory_make ("queue", "record-queue");
    if (element) {
      g_object_set (element, "max-size-buffers", 1, "max-size-bytes", 0,
          "max-size-time", G_GUINT64_CONSTANT (0), "silent", TRUE, NULL);
      list = g_list_append (list, element);
    } else {
      GST_WARNING_OBJECT (self, "failed to create 'queue'");
    }
  }
  // create filesink, set location property
  GST_DEBUG_OBJECT (self, "recording to: %s", self->priv->record_file_name);
  element = gst_element_factory_make ("filesink", "filesink");
  if (element) {
    g_object_set (element, "location", self->priv->record_file_name,
        // only for recording in in realtime and not as fast as we can
        // "sync", TRUE,
        /* this avoids the prerolling */
        "async", FALSE, NULL);
    list = g_list_append (list, element);
  } else {
    GST_WARNING_OBJECT (self, "failed to create 'filesink'");
  }
  return list;
}

#ifdef BT_MONITOR_SINK_DATA_FLOW
static GstClockTime sink_probe_last_ts = GST_CLOCK_TIME_NONE;
static gboolean
sink_probe (GstPad * pad, GstMiniObject * mini_obj, gpointer user_data)
{
  if (GST_IS_BUFFER (mini_obj)) {
    if (sink_probe_last_ts == GST_CLOCK_TIME_NONE) {
      GST_WARNING ("got SOS at %" GST_TIME_FORMAT,
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (mini_obj)));
    }
    sink_probe_last_ts =
        GST_BUFFER_TIMESTAMP (mini_obj) + GST_BUFFER_DURATION (mini_obj);
  } else if (GST_IS_EVENT (mini_obj)) {
    GST_WARNING ("got %s at %" GST_TIME_FORMAT,
        GST_EVENT_TYPE_NAME (mini_obj), GST_TIME_ARGS (sink_probe_last_ts));
    if (GST_EVENT_TYPE (mini_obj) == GST_EVENT_EOS) {
      sink_probe_last_ts = GST_CLOCK_TIME_NONE;
    }
  }
  return TRUE;
}
#endif

static gboolean
bt_sink_bin_format_update (const BtSinkBin * const self)
{
  GstStructure *s;
  GstCaps *sink_format_caps;

  if (!self->priv->caps_filter)
    return FALSE;

  // always add caps-filter as a first element and enforce sample rate and channels
  s = gst_structure_from_string ("audio/x-raw, "
      "format = (string) " GST_AUDIO_FORMATS_ALL ", "
      "layout = (string) interleaved, "
      "rate = (int) 44100, " "channels = (int) 2", NULL);
  gst_structure_set (s,
      "rate", G_TYPE_INT, self->priv->sample_rate,
      "channels", G_TYPE_INT, self->priv->channels, NULL);
  sink_format_caps = gst_caps_new_full (s, NULL);

  GST_INFO_OBJECT (self, "sink is using: sample-rate=%u, channels=%u",
      self->priv->sample_rate, self->priv->channels);

  g_object_set (self->priv->caps_filter, "caps", sink_format_caps, NULL);
  gst_caps_unref (sink_format_caps);

  return TRUE;
}

/*
 * bt_sink_bin_update:
 * @self: the #BtSinkBin
 *
 * Needs to be called after a series of property changes to update the internals
 * of the bin.
 *
 * Returns: %TRUE for success
 */
static gboolean
bt_sink_bin_update (const BtSinkBin * const self)
{
  BtSinkBinPrivate *p = self->priv;
  GstElement *audio_resample = NULL, *tee = NULL;
  GstStateChangeReturn scr;
  GstState state;
  gboolean defer = FALSE;

  // check current state, if ASYNC the element is changing state already/still
  if ((scr =
          gst_element_get_state (GST_ELEMENT (self), &state, NULL,
              0)) != GST_STATE_CHANGE_ASYNC) {
    if (state > GST_STATE_READY) {
      GST_INFO_OBJECT (self, "state > READY");
      defer = TRUE;
    }
  } else {
    GST_INFO_OBJECT (self, "_get_state() returned %s",
        gst_element_state_change_return_get_name (scr));
    defer = TRUE;
  }
  if (defer) {
    GST_INFO_OBJECT (self, "defer switching sink-bin elements");
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (self),
        GST_DEBUG_GRAPH_SHOW_ALL, PACKAGE_NAME "-sinkbin-error");
    p->pending_update = TRUE;
    return FALSE;
  }

  GST_INFO_OBJECT (self, "clearing sink-bin");

  // remove existing children
  bt_sink_bin_clear (self);

  GST_INFO_OBJECT (self, "initializing sink-bin");

  p->caps_filter = gst_element_factory_make ("capsfilter", "sink-format");
  bt_sink_bin_format_update (self);
  audio_resample =
      gst_element_factory_make ("audioresample", "sink-audioresample");
  tee = p->tee = gst_element_factory_make ("tee", "sink-tee");
  if (!tee || !audio_resample) {
    if (audio_resample) {
      gst_object_unref (audio_resample);
    } else {
      GST_WARNING_OBJECT (self, "Failed to create audio-resample");
    }
    if (tee) {
      gst_object_unref (tee);
      p->tee = NULL;
    } else {
      GST_WARNING_OBJECT (self, "Failed to create tee");
    }
    return FALSE;
  }

  gst_bin_add_many (GST_BIN (self), p->caps_filter, tee, audio_resample, NULL);

  if (!gst_element_link_pads (p->caps_filter, "src", audio_resample, "sink")) {
    GST_WARNING_OBJECT (self, "Can't link caps-filter and audio-resample");
    return FALSE;
  }
#ifdef BT_MONITOR_TIMESTAMPS
  {
    GstElement *identity = gst_element_factory_make ("identity", "identity");

    g_object_set (identity, "check-perfect", TRUE, "silent", FALSE, NULL);
    gst_bin_add (GST_BIN (self), identity);
    if (!gst_element_link_pads (audio_resample, "src", identity, "sink")) {
      GST_WARNING_OBJECT (self, "Can't link audio-resample and identity");
    }
    if (!gst_element_link_pads (identity, "src", tee, "sink")) {
      GST_WARNING_OBJECT (self, "Can't link identity and tee");
    }
  }
#else
  if (!gst_element_link_pads (audio_resample, "src", tee, "sink")) {
    GST_WARNING_OBJECT (self, "Can't link audio-resample and tee");
    return FALSE;
  }
#endif

  // add new children
  switch (p->mode) {
    case BT_SINK_BIN_MODE_PLAY:{
      GList *const list = bt_sink_bin_get_player_elements (self);
      if (list) {
        bt_sink_bin_add_many (self, list);
        bt_sink_bin_link_many (self, tee, list);
        g_list_free (list);
      } else {
        GST_WARNING_OBJECT (self, "Can't get playback element list");
        return FALSE;
      }
      break;
    }
    case BT_SINK_BIN_MODE_RECORD:{
      GList *const list = bt_sink_bin_get_recorder_elements (self);
      if (list) {
        bt_sink_bin_add_many (self, list);
        bt_sink_bin_link_many (self, tee, list);
        g_list_free (list);
      } else {
        GST_WARNING ("Can't get record element list");
        return FALSE;
      }
      break;
    }
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:{
      // add player elems
      GList *list1 = bt_sink_bin_get_player_elements (self);
      if (list1) {
        GstElement *element;

        /*
           element=gst_element_factory_make("audioconvert","play-convert");
           list1=g_list_prepend(list1,element);
         */

        // start with a queue
        element = gst_element_factory_make ("queue", "play-queue");
        if (element) {
          g_object_set (element, "max-size-buffers", 1, "max-size-bytes", 0,
              "max-size-time", G_GUINT64_CONSTANT (0), "silent", TRUE, NULL);
          list1 = g_list_prepend (list1, element);
        } else {
          GST_WARNING_OBJECT (self, "failed to create 'queue'");
        }

        bt_sink_bin_add_many (self, list1);
        bt_sink_bin_link_many (self, tee, list1);
        g_list_free (list1);
      } else {
        GST_WARNING_OBJECT (self, "Can't get playback element list");
        return FALSE;
      }
      // add recorder elements
      GList *const list2 = bt_sink_bin_get_recorder_elements (self);
      if (list2) {
        bt_sink_bin_add_many (self, list2);
        bt_sink_bin_link_many (self, tee, list2);
        g_list_free (list2);
      } else {
        GST_WARNING_OBJECT (self, "Can't get record element list");
        return FALSE;
      }
      break;
    }
    case BT_SINK_BIN_MODE_PASS_THRU:{
      GstPadTemplate *tmpl =
          gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (tee),
          "src_%u");
      GstPad *target_pad = gst_element_request_pad (tee, tmpl, NULL, NULL);
      if (!target_pad) {
        GST_WARNING_OBJECT (tee, "failed to get request 'src' request-pad");
      }

      p->src = gst_ghost_pad_new ("src", target_pad);
      gst_pad_set_active (p->src, TRUE);
      gst_element_add_pad (GST_ELEMENT (self), p->src);
      break;
    }
    default:
      g_assert_not_reached ();
  }

  // set new ghostpad-target
  if (p->sink) {
    GstPad *sink_pad = gst_element_get_static_pad (p->caps_filter, "sink");
    GstPad *req_sink_pad = NULL;

    GST_INFO_OBJECT (p->sink, "updating ghostpad: %p", p->sink);

    if (!sink_pad) {
      GST_INFO_OBJECT (p->caps_filter, "failed to get static 'sink' pad");
      GstElementClass *klass = GST_ELEMENT_GET_CLASS (p->caps_filter);
      sink_pad = req_sink_pad =
          gst_element_request_pad (p->caps_filter,
          gst_element_class_get_pad_template (klass, "sink_%u"), NULL, NULL);
      if (!sink_pad) {
        GST_WARNING_OBJECT (p->caps_filter,
            "failed to get request 'sink' request-pad");
      }
    }
    GST_INFO_OBJECT (p->caps_filter,
        "updating ghost pad : elem=%" G_OBJECT_REF_COUNT_FMT
        "pad=%" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (p->caps_filter),
        G_OBJECT_LOG_REF_COUNT (sink_pad));

    if (!gst_ghost_pad_set_target (GST_GHOST_PAD (p->sink), sink_pad)) {
      GST_WARNING_OBJECT (self, "failed to link internal pads");
    }

    GST_INFO_OBJECT (self, "  done, pad=%" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (sink_pad));
    // request pads need to be released
    if (!req_sink_pad) {
      gst_object_unref (sink_pad);
    }
  }
#ifdef BT_MONITOR_SINK_DATA_FLOW
  if (audio_sink) {
    GstPad *sink_pad = gst_element_get_static_pad (audio_sink, "sink");

    if (sink_pad) {
      sink_probe_last_ts = GST_CLOCK_TIME_NONE;
      gst_pad_add_data_probe (sink_pad, G_CALLBACK (sink_probe),
          (gpointer) self);

      gst_object_unref (sink_pad);
    }
  }
#endif

  GST_INFO_OBJECT (self, "done");
  return TRUE;
}

//-- event handler

static void
on_audio_sink_child_added (GstBin * bin, GstElement * element,
    gpointer user_data)
{
  BtSinkBin *self = BT_SINK_BIN (user_data);

  if (GST_IS_BASE_SINK (element)) {
    bt_sink_bin_set_audio_sink (self, element);
  }
}

static void
on_audio_sink_changed (const BtSettings * const settings,
    GParamSpec * const arg, gconstpointer const user_data)
{
  BtSinkBin *self = BT_SINK_BIN (user_data);

  GST_INFO_OBJECT (self, "audio-sink has changed");

  // exchange the machine
  switch (self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:
      bt_sink_bin_update (self);
      break;
    default:
      break;
  }
}

static void
on_system_audio_sink_changed (const BtSettings * const settings,
    GParamSpec * const arg, gconstpointer const user_data)
{
  BtSinkBin *self = BT_SINK_BIN (user_data);
  gchar *element_name, *sink_name;

  GST_INFO_OBJECT (self, "system audio-sink has changed");

  // exchange the machine (only if the system-audiosink is in use)
  bt_settings_determine_audiosink_name (self->priv->settings, &element_name,
      NULL);
  g_object_get ((gpointer) settings, "system-audiosink-name", &sink_name, NULL);

  GST_INFO_OBJECT (self, "  -> '%s' (sytem_sink is '%s')", element_name,
      sink_name);
  if (!strcmp (element_name, sink_name)) {
    // exchange the machine
    switch (self->priv->mode) {
      case BT_SINK_BIN_MODE_PLAY:
      case BT_SINK_BIN_MODE_PLAY_AND_RECORD:
        bt_sink_bin_update (self);
        break;
      default:
        break;
    }
  }
  g_free (sink_name);
  g_free (element_name);
}

static void
on_sample_rate_changed (const BtSettings * const settings,
    GParamSpec * const arg, gconstpointer const user_data)
{
  BtSinkBin *self = BT_SINK_BIN (user_data);

  GST_INFO_OBJECT (self, "sample-rate has changed");
  g_object_get ((gpointer) settings, "sample-rate", &self->priv->sample_rate,
      NULL);
  bt_sink_bin_format_update (self);
}

static void
on_channels_changed (const BtSettings * const settings, GParamSpec * const arg,
    gconstpointer const user_data)
{
  BtSinkBin *self = BT_SINK_BIN (user_data);

  GST_INFO_OBJECT (self, "channels have changed");
  g_object_get ((gpointer) settings, "channels", &self->priv->channels, NULL);
  bt_sink_bin_format_update (self);
  // TODO(ensonic): this would render all panorama/balance elements useless and also
  // affect wire patterns - how do we want to handle it
}

static GstPadProbeReturn
master_volume_sync_handler (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  gst_object_sync_values (GST_OBJECT (user_data),
      GST_BUFFER_TIMESTAMP ((GstBuffer *) info->data));
  return GST_PAD_PROBE_OK;
}

//-- methods

/**
 * bt_sink_bin_is_record_format_supported:
 * @format: the format to check
 *
 * Each record format might need a couple of GStreamer element to work. This
 * function verifies that all needed element are available.
 *
 * Returns: %TRUE if a fomat is useable
 */
gboolean
bt_sink_bin_is_record_format_supported (BtSinkBinRecordFormat format)
{
  if (format_states[format] == RECORD_FORMAT_STATE_NOT_CHECKED) {
    GstElement *element;
    GstEncodingProfile *profile =
        bt_sink_bin_create_recording_profile (&formats[format]);

    // TODO(ensonic): if we run the plugin installer, we need to re-eval the
    // profiles
    if ((element = bt_sink_bin_make_and_configure_encodebin (profile))) {
      format_states[format] = RECORD_FORMAT_STATE_WORKING;
      gst_object_unref (element);
    } else {
      format_states[format] = RECORD_FORMAT_STATE_MISSES_ELEMENTS;
    }
    g_object_unref (profile);
  }
  return format_states[format] > RECORD_FORMAT_STATE_NOT_CHECKED;
}

//-- wrapper

//-- class internals

static GstStateChangeReturn
bt_sink_bin_change_state (GstElement * element, GstStateChange transition)
{
  const BtSinkBin *const self = BT_SINK_BIN (element);
  BtAudioSession *audio_session = bt_audio_session_new ();
  GstStateChangeReturn res;

  GST_INFO_OBJECT (self, "state change on the sink-bin: %s -> %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  res =
      GST_ELEMENT_CLASS (bt_sink_bin_parent_class)->change_state (element,
      transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (self->priv->pending_update) {
        self->priv->pending_update = FALSE;
        bt_sink_bin_update (self);
      } else {
        g_object_set (audio_session, "audio-locked", TRUE, NULL);
      }
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      g_object_set (audio_session, "audio-locked", FALSE, NULL);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (self),
          GST_DEBUG_GRAPH_SHOW_ALL, PACKAGE_NAME "-encodebin");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (self),
          GST_DEBUG_GRAPH_SHOW_ALL, PACKAGE_NAME "-encodebin");
      break;
    default:
      break;
  }
  g_object_unref (audio_session);
  return res;
}

static void
bt_sink_bin_set_context (GstElement * element, GstContext * context)
{
  const BtSinkBin *const self = BT_SINK_BIN (element);
  guint bpm, tpb, stpb;

  if (gstbt_audio_tempo_context_get_tempo (context, &bpm, &tpb, &stpb)) {
    if (self->priv->beats_per_minute != bpm ||
        self->priv->ticks_per_beat != tpb ||
        self->priv->subticks_per_tick != stpb) {
      self->priv->beats_per_minute = bpm;
      self->priv->ticks_per_beat = tpb;
      self->priv->subticks_per_tick = stpb;

      GST_INFO_OBJECT (self, "audio tempo context: bmp=%u, tpb=%u, stpb=%u",
          bpm, tpb, stpb);

      if (self->priv->audio_sink) {
        bt_sink_bin_configure_latency (self);
      } else {
        GST_INFO_OBJECT (self, "no player object created yet.");
      }
    }
  }
#if GST_CHECK_VERSION (1,8,0)
  GST_ELEMENT_CLASS (bt_sink_bin_parent_class)->set_context (element, context);
#else
  if (GST_ELEMENT_CLASS (bt_sink_bin_parent_class)->set_context) {
    GST_ELEMENT_CLASS (bt_sink_bin_parent_class)->set_context (element,
        context);
  }
#endif
}

static void
bt_sink_bin_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtSinkBin *const self = BT_SINK_BIN (object);
  return_if_disposed ();
  switch (property_id) {
    case SINK_BIN_MODE:
      g_value_set_enum (value, self->priv->mode);
      break;
    case SINK_BIN_RECORD_FORMAT:
      g_value_set_enum (value, self->priv->record_format);
      break;
    case SINK_BIN_RECORD_FILE_NAME:
      g_value_set_string (value, self->priv->record_file_name);
      break;
    case SINK_BIN_INPUT_GAIN:
      g_value_set_object (value, self->priv->gain);
      break;
    case SINK_BIN_MASTER_VOLUME:
      if (self->priv->gain) {
        // FIXME(ensonic): do we need a notify (or weak ptr) to update this?
        g_object_get (self->priv->gain, "volume", &self->priv->volume, NULL);
        GST_DEBUG ("Get master volume: %lf", self->priv->volume);
      }
      g_value_set_double (value, self->priv->volume);
      break;
    case SINK_BIN_ANALYZERS:
      g_value_set_pointer (value, self->priv->analyzers);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sink_bin_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtSinkBin *const self = BT_SINK_BIN (object);
  return_if_disposed ();

  switch (property_id) {
    case SINK_BIN_MODE:
      self->priv->mode = g_value_get_enum (value);
      if ((self->priv->mode == BT_SINK_BIN_MODE_PLAY)
          || (self->priv->mode == BT_SINK_BIN_MODE_PASS_THRU)) {
        if (self->priv->record_file_name) {
          g_free (self->priv->record_file_name);
          self->priv->record_file_name = NULL;
        }
        bt_sink_bin_update (self);
      } else {
        if (self->priv->record_file_name) {
          bt_sink_bin_update (self);
        }
      }
      break;
    case SINK_BIN_RECORD_FORMAT:
      self->priv->record_format = g_value_get_enum (value);
      if (((self->priv->mode == BT_SINK_BIN_MODE_RECORD)
              || (self->priv->mode == BT_SINK_BIN_MODE_PLAY_AND_RECORD))
          && self->priv->record_file_name) {
        bt_sink_bin_update (self);
      }
      break;
    case SINK_BIN_RECORD_FILE_NAME:
      g_free (self->priv->record_file_name);
      self->priv->record_file_name = g_value_dup_string (value);
      GST_INFO_OBJECT (self, "recording to '%s'", self->priv->record_file_name);
      if ((self->priv->mode == BT_SINK_BIN_MODE_RECORD)
          || (self->priv->mode == BT_SINK_BIN_MODE_PLAY_AND_RECORD)) {
        bt_sink_bin_update (self);
      }
      break;
    case SINK_BIN_INPUT_GAIN:{
      GstPad *sink_pad;

      g_object_try_weak_unref (self->priv->gain);
      self->priv->gain = GST_ELEMENT (g_value_get_object (value));
      GST_DEBUG_OBJECT (self->priv->gain, "Set master gain element: %p",
          self->priv->gain);
      g_object_try_weak_ref (self->priv->gain);
      g_object_set (self->priv->gain, "volume", self->priv->volume, NULL);
      sink_pad = gst_element_get_static_pad (self->priv->gain, "sink");
      self->priv->mv_handler_id =
          gst_pad_add_probe (sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
          master_volume_sync_handler, (gpointer) self, NULL);
      gst_object_unref (sink_pad);
      break;
    }
    case SINK_BIN_MASTER_VOLUME:
      if (self->priv->gain) {
        self->priv->volume = g_value_get_double (value);
        g_object_set (self->priv->gain, "volume", self->priv->volume, NULL);
        GST_DEBUG_OBJECT (self, "Set master volume: %lf", self->priv->volume);
      }
      break;
    case SINK_BIN_ANALYZERS:
      bt_sink_bin_deactivate_analyzers (self);
      self->priv->analyzers = g_value_get_pointer (value);
      bt_sink_bin_activate_analyzers (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sink_bin_dispose (GObject * const object)
{
  const BtSinkBin *const self = BT_SINK_BIN (object);
  GstStateChangeReturn res;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_INFO ("!!!! self=%p", self);

  if ((res =
          gst_element_set_state (GST_ELEMENT (self),
              GST_STATE_NULL)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("can't go to null state");
  }
  GST_DEBUG ("->NULL state change returned '%s'",
      gst_element_state_change_return_get_name (res));

  g_object_unref (self->priv->settings);

  if (self->priv->mv_handler_id && self->priv->gain) {
    GstPad *sink_pad = gst_element_get_static_pad (self->priv->gain, "sink");

    if (sink_pad) {
      gst_pad_remove_probe (sink_pad, self->priv->mv_handler_id);
      gst_object_unref (sink_pad);
    }
  }
  g_object_try_weak_unref (self->priv->gain);

  GST_INFO ("self->sink=%" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->sink));
  gst_element_remove_pad (GST_ELEMENT (self), self->priv->sink);
  GST_INFO ("sink-bin : children=%d", GST_BIN_NUMCHILDREN (self));

  GST_INFO ("chaining up");
  G_OBJECT_CLASS (bt_sink_bin_parent_class)->dispose (object);
  GST_INFO ("done");
}

static void
bt_sink_bin_finalize (GObject * const object)
{
  const BtSinkBin *const self = BT_SINK_BIN (object);

  GST_DEBUG ("!!!! self=%p", self);
  GST_INFO ("sink-bin : children=%d", GST_BIN_NUMCHILDREN (self));

  g_free (self->priv->record_file_name);

  G_OBJECT_CLASS (bt_sink_bin_parent_class)->finalize (object);
}

static void
bt_sink_bin_init (BtSinkBin * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SINK_BIN, BtSinkBinPrivate);

  GST_INFO ("!!!! self=%p", self);

  // watch settings changes
  self->priv->settings = bt_settings_make ();
  //GST_DEBUG("listen to settings changes (%p)",self->priv->settings);
  g_signal_connect_object (self->priv->settings, "notify::audiosink",
      G_CALLBACK (on_audio_sink_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->settings, "notify::audiosink-device",
      G_CALLBACK (on_audio_sink_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->settings, "notify::system-audiosink",
      G_CALLBACK (on_system_audio_sink_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->settings, "notify::sample-rate",
      G_CALLBACK (on_sample_rate_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->settings, "notify::channels",
      G_CALLBACK (on_channels_changed), (gpointer) self, 0);

  g_object_get (self->priv->settings,
      "sample-rate", &self->priv->sample_rate,
      "channels", &self->priv->channels, NULL);
  self->priv->volume = 1.0;

  self->priv->sink = gst_ghost_pad_new_no_target ("sink", GST_PAD_SINK);
  gst_element_add_pad (GST_ELEMENT (self), self->priv->sink);
  bt_sink_bin_update (self);

  GST_OBJECT_FLAG_SET (self, GST_ELEMENT_FLAG_SINK);

  GST_INFO ("done");
}

static void
bt_sink_bin_class_init (BtSinkBinClass * klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *const element_class = GST_ELEMENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSinkBinPrivate));

  gobject_class->set_property = bt_sink_bin_set_property;
  gobject_class->get_property = bt_sink_bin_get_property;
  gobject_class->dispose = bt_sink_bin_dispose;
  gobject_class->finalize = bt_sink_bin_finalize;

  element_class->change_state = bt_sink_bin_change_state;
  element_class->set_context = bt_sink_bin_set_context;

  g_object_class_install_property (gobject_class, SINK_BIN_MODE,
      g_param_spec_enum ("mode", "Mode", "mode of operation",
          BT_TYPE_SINK_BIN_MODE, BT_SINK_BIN_MODE_PLAY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SINK_BIN_RECORD_FORMAT,
      g_param_spec_enum ("record-format", "Record format",
          "format to use when in record mode", BT_TYPE_SINK_BIN_RECORD_FORMAT,
          BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SINK_BIN_RECORD_FILE_NAME,
      g_param_spec_string ("record-file-name", "Record filename",
          "the file-name to use for recording", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SINK_BIN_INPUT_GAIN,
      g_param_spec_object ("input-gain", "Input gain",
          "the input-gain element, if any",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SINK_BIN_MASTER_VOLUME,
      g_param_spec_double ("master-volume",
          "Master volume", "master volume for the song",
          0.0, 1.0, 1.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, SINK_BIN_ANALYZERS,
      g_param_spec_pointer ("analyzers",
          "Analyzers", "list of master analyzers",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_metadata (element_class,
      "Master AudioSink",
      "Audio/Bin", "Play/Record audio", "Stefan Kost <ensonic@users.sf.net>");
}

//-- plugin handling

gboolean
bt_sink_bin_plugin_init (GstPlugin * const plugin)
{
  gst_element_register (plugin, "bt-sink-bin", GST_RANK_NONE, BT_TYPE_SINK_BIN);
  return TRUE;
}
