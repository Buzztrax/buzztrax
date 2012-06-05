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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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

/* TODO(ensonic): use encodebin
 *
 * TODO(ensonic): add properties for bpm and master volume,
 * - bpm
 *   - tempo-iface is implemented, but is hidden from the ui
 *     (the iface properties are not controllable)
 *   - we could have separate properties and forward the changes
 *
 * TODO(ensonic): for upnp it would be nice to stream on-demand
 *
 * TODO(ensonic): add a metronome
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

#define BT_CORE
#define BT_SINK_BIN_C

#include "core_private.h"
#include "sink-bin.h"

/* define this to get diagnostics of the sink data flow */
//#define BT_MONITOR_SINK_DATA_FLOW
/* define this to verify continuous timestamps */
//#define BT_MONITOR_TIMESTAMPS

//-- property ids

enum {
  SINK_BIN_MODE=1,
  SINK_BIN_RECORD_FORMAT,
  SINK_BIN_RECORD_FILE_NAME,
  SINK_BIN_INPUT_GAIN,
  SINK_BIN_MASTER_VOLUME,
  SINK_BIN_ANALYZERS,
  // tempo iface
  SINK_BIN_TEMPO_BPM,
  SINK_BIN_TEMPO_TPB,
  SINK_BIN_TEMPO_STPT,
};

struct _BtSinkBinPrivate {
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
  GstPad *sink,*src;

  /* we need to hold the reference to not kill the notifies */
  BtSettings *settings;

  gulong bus_handler_id;

  /* player sink or NULL */
  GstElement *audio_sink;

  /* tee */
  G_POINTER_ALIAS(GstElement *,tee);

  /* master volume */
  G_POINTER_ALIAS(GstElement *,gain);
  gulong mv_handler_id;
  gdouble volume;

  /* sink format */
  G_POINTER_ALIAS(GstElement *,caps_filter);

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;

  /* master analyzers */
  GList *analyzers;
};

//-- prototypes

static void bt_sink_bin_configure_latency(const BtSinkBin * const self);
static void on_audio_sink_child_added(GstBin *bin, GstElement *element, gpointer user_data);

//-- the class

static void bt_sink_bin_tempo_interface_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSinkBin, bt_sink_bin, GST_TYPE_BIN,
  G_IMPLEMENT_INTERFACE (GSTBT_TYPE_TEMPO,
    bt_sink_bin_tempo_interface_init));


//-- tempo interface implementations

static void bt_sink_bin_tempo_change_tempo(GstBtTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  BtSinkBin *self=BT_SINK_BIN(tempo);
  gboolean changed=FALSE;

  if(beats_per_minute>=0) {
    if(self->priv->beats_per_minute!=beats_per_minute) {
      self->priv->beats_per_minute=(gulong)beats_per_minute;
      g_object_notify(G_OBJECT(self),"beats-per-minute");
      changed=TRUE;
    }
  }
  if(ticks_per_beat>=0) {
    if(self->priv->ticks_per_beat!=ticks_per_beat) {
      self->priv->ticks_per_beat=(gulong)ticks_per_beat;
      g_object_notify(G_OBJECT(self),"ticks-per-beat");
      changed=TRUE;
    }
  }
  if(subticks_per_tick>=0) {
    if(self->priv->subticks_per_tick!=subticks_per_tick) {
      self->priv->subticks_per_tick=(gulong)subticks_per_tick;
      g_object_notify(G_OBJECT(self),"subticks-per-tick");
      changed=TRUE;
    }
  }
  if(changed) {
    GST_DEBUG("changing tempo to %lu BPM  %lu TPB  %lu STPT",self->priv->beats_per_minute,self->priv->ticks_per_beat,self->priv->subticks_per_tick);
    if(self->priv->audio_sink) {
      bt_sink_bin_configure_latency(self);
    }
    else {
      GST_INFO("no player object created yet.");
    }
  }
}

static void bt_sink_bin_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtTempoInterface *iface = g_iface;

  iface->change_tempo = bt_sink_bin_tempo_change_tempo;
}

//-- enums

GType bt_sink_bin_mode_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_SINK_BIN_MODE_PLAY,            "BT_SINK_BIN_MODE_PLAY",            "play" },
      { BT_SINK_BIN_MODE_RECORD,          "BT_SINK_BIN_MODE_RECORD",          "record" },
      { BT_SINK_BIN_MODE_PLAY_AND_RECORD, "BT_SINK_BIN_MODE_PLAY_AND_RECORD", "play-and-record" },
      { BT_SINK_BIN_MODE_PASS_THRU,       "BT_SINK_BIN_MODE_PASS_THRU",       "pass-thru" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSinkBinMode", values);
  }
  return type;
}

GType bt_sink_bin_record_format_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, ".ogg", "ogg-vorbis" },
      { BT_SINK_BIN_RECORD_FORMAT_MP3,        ".mp3",        "mp3" },
      { BT_SINK_BIN_RECORD_FORMAT_WAV,        ".wav",        "wav" },
      { BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC,   ".flac",  "ogg-flac" },
      { BT_SINK_BIN_RECORD_FORMAT_RAW,        ".raw",        "raw" },
      { BT_SINK_BIN_RECORD_FORMAT_MP4_AAC,    ".m4a",    "mp4-aac" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSinkBinRecordFormat", values);
  }
  return type;
}

//-- helper methods

/*
 * bt_sink_bin_activate_analyzers:
 * @self: the sink-bin
 *
 * Add all analyzers to the bin and link them.
 */
static void bt_sink_bin_activate_analyzers(const BtSinkBin * const self) {
  GstState state;
  gboolean is_playing=TRUE;

  if(!self->priv->analyzers) return;

  //g_object_get(self->priv->song,"is-playing",&is_playing,NULL);
  if((gst_element_get_state(GST_ELEMENT(self),&state,NULL,GST_MSECOND)==GST_STATE_CHANGE_SUCCESS)) {
      if(state<GST_STATE_PAUSED) is_playing=FALSE;
  }
  bt_bin_activate_tee_chain(GST_BIN(self),self->priv->tee,self->priv->analyzers,is_playing);
}

/*
 * bt_sink_bin_deactivate_analyzers:
 * @self: the sink-bin
 *
 * Remove all analyzers to the bin and unlink them.
 */
static void bt_sink_bin_deactivate_analyzers(const BtSinkBin * const self) {
  GstState state;
  gboolean is_playing=TRUE;

  if(!self->priv->analyzers) return;

  //g_object_get(self->priv->song,"is-playing",&is_playing,NULL);
  if((gst_element_get_state(GST_ELEMENT(self),&state,NULL,GST_MSECOND)==GST_STATE_CHANGE_SUCCESS)) {
      if(state<GST_STATE_PAUSED) is_playing=FALSE;
  }
  bt_bin_deactivate_tee_chain(GST_BIN(self),self->priv->tee,self->priv->analyzers,is_playing);
}

static void bt_sink_bin_configure_latency(const BtSinkBin * const self) {
  GstElement *sink=self->priv->audio_sink;
  if(GST_IS_BASE_AUDIO_SINK(sink)) {
    if(self->priv->beats_per_minute && self->priv->ticks_per_beat && self->priv->subticks_per_tick) {
      // we configure stpb in machine.c
      const gdouble div=self->priv->beats_per_minute*self->priv->ticks_per_beat*self->priv->subticks_per_tick;
      // configure buffer size
      gint64 chunk=GST_TIME_AS_USECONDS((GST_SECOND*60)/div);

      GST_INFO_OBJECT(sink,
        "changing audio chunk-size to %"G_GUINT64_FORMAT" µs = %"G_GUINT64_FORMAT" ms",
        chunk, (chunk/G_GINT64_CONSTANT(1000)));
      g_object_set(sink,
        "latency-time",chunk,
        "buffer-time",chunk<<1,
        NULL);
    }
  }
}

static void bt_sink_bin_set_audio_sink(const BtSinkBin * const self, GstElement *sink) {
  // enable syncing to timestamps
  g_object_set(sink,
    "sync",TRUE,
    /* if we do this, live pipelines go playing, but still sources don't start */
    /*"async", FALSE,*/ /* <- this breaks scrubbing on timeline */
     /* this helps trickplay and scrubbing, there seems to be some time issue
      * in baseaudiosink
      */
    /*"slave-method",2,*/
    /*"provide-clock",FALSE, */
    /* this does not lockup anymore, but does not give us better latencies */
    /*"can-activate-pull",TRUE,*/
    NULL);
  self->priv->audio_sink=sink;
  bt_sink_bin_configure_latency(self);  
}

static void bt_sink_bin_clear(const BtSinkBin * const self) {
  GstBin * const bin=GST_BIN(self);
  GstElement *elem;

  GST_DEBUG_OBJECT(self,"clearing sink-bin : %d",GST_BIN_NUMCHILDREN(bin));
  if(bin->children) {
    GstStateChangeReturn res;

    self->priv->audio_sink=NULL;
    self->priv->caps_filter=NULL;
    self->priv->tee=NULL;

    // does not seem to be needed
    //gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),NULL);
    //GST_DEBUG("released ghost-pad");

    while(bin->children) {
      elem=GST_ELEMENT_CAST (bin->children->data);
      GST_DEBUG_OBJECT(elem,"  removing elem=%p (ref_ct=%d),'%s'",
        elem,(G_OBJECT_REF_COUNT(elem)),GST_OBJECT_NAME(elem));
      gst_element_set_locked_state(elem,FALSE);
      if((res=gst_element_set_state(elem,GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
        GST_WARNING("can't go to null state");
      else
        GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));
      gst_bin_remove (bin, elem);
    }

    if(self->priv->src) {
      gst_pad_set_active(self->priv->src,FALSE);
      gst_element_remove_pad(GST_ELEMENT(self),self->priv->src);
      self->priv->src=NULL;
    }
  }
  GST_DEBUG("done");
}

static gboolean bt_sink_bin_add_many(const BtSinkBin * const self, GList * const list) {
  const GList *node;
  GstElement *elem;

  GST_DEBUG("add elements: list=%p",list);

  for(node=list;node;node=node->next) {
    elem=GST_ELEMENT_CAST(node->data);
    GST_DEBUG_OBJECT(elem,"  adding elem=%p (ref_ct=%d),'%s'",
        elem,(G_OBJECT_REF_COUNT(elem)),GST_OBJECT_NAME(elem));
    if(!gst_bin_add(GST_BIN(self),elem)) {
      GST_WARNING_OBJECT(self,"can't add element: elem=%s, parent=%s",
        GST_OBJECT_NAME(elem),GST_OBJECT_NAME(GST_OBJECT_PARENT(elem)));
    }
  }
  return(TRUE);
}

static void bt_sink_bin_link_many(const BtSinkBin * const self, GstElement *last_elem, GList * const list) {
  const GList *node;

  GST_DEBUG("link elements: last_elem=%s, list=%p",GST_OBJECT_NAME(last_elem),list);
  if(!list) return;

  for(node=list;node;node=node->next) {
    GstElement * const cur_elem=GST_ELEMENT(node->data);

    if(!gst_element_link(last_elem,cur_elem)) {
      GST_WARNING("can't link elements: last_elem=%s, cur_elem=%s",
        GST_OBJECT_NAME(last_elem),GST_OBJECT_NAME(cur_elem));
    }
    last_elem=cur_elem;
  }
}

#define BT_SINK_BIN_MISSING_ELEMENT(elem_name) bt_sink_bin_missing_element(self,elem_name,__FILE__,GST_FUNCTION,__LINE__)

static void bt_sink_bin_missing_element(const BtSinkBin * const self, const gchar *elem_name, const gchar *file, const gchar *func, const gint line) {
  gchar *msg=g_strdup_printf("Can't instantiate %s element", elem_name);

  gst_element_message_full(GST_ELEMENT(self),GST_MESSAGE_ERROR,
    GST_CORE_ERROR,GST_CORE_ERROR_MISSING_PLUGIN,
    msg,g_strdup (msg),file,func,line);
}

static GList *bt_sink_bin_get_player_elements(const BtSinkBin * const self) {
  GList *list=NULL;
  gchar *plugin_name;
  GstElement *element;
  BtAudioSession *audio_session;

  GST_DEBUG("get playback elements");

  if(!(plugin_name=bt_settings_determine_audiosink_name(self->priv->settings))) {
    return(NULL);
  }
  audio_session=bt_audio_session_new();
  g_object_set(audio_session,"audio-sink-name",plugin_name,NULL);
  g_object_get(audio_session,"audio-sink",&element,NULL);
  g_object_unref(audio_session);
  if(!element) {
    GST_WARNING("No session sink for '%s'",plugin_name);
    element=gst_element_factory_make(plugin_name,NULL);
  }
  if(!element) {
    /* bt_settings_determine_audiosink_name() does all kind of sanity checks already */
    GST_WARNING("Can't instantiate '%s' element",plugin_name);goto Error;
  }
  if(GST_IS_BASE_SINK(element)) {
    bt_sink_bin_set_audio_sink(self,element);
  } else if(GST_IS_BIN(element)) {
    g_signal_connect(element, "element-added", G_CALLBACK(on_audio_sink_child_added), (gpointer)self);
  }
  list=g_list_append(list,element);

Error:
  g_free(plugin_name);
  return(list);
}

static GList *bt_sink_bin_get_recorder_elements(const BtSinkBin * const self) {
  GList *list=NULL;
  GstElement *element;

  GST_DEBUG("get record elements");

  // TODO(ensonic): check extension ?
  // TODO(ensonic): need to lookup encoders by caps

  // start with a queue
  element=gst_element_factory_make("queue","record-queue");
  // TODO(ensonic): if we have/require gstreamer-0.10.31 ret rid of the check
  if(g_object_class_find_property(G_OBJECT_GET_CLASS(element),"silent")) {
    g_object_set(element,"silent",TRUE,NULL);
  }
  list=g_list_append(list,element);

  /*
  element=gst_element_factory_make("audioconvert","record-convert");
  list=g_list_append(list,element);
  */

  // generate recorder elements
  switch(self->priv->record_format) {
    case BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS:
      // vorbisenc ! oggmux ! filesink location="song.ogg"
      /* we use float internaly anyway
      if(!(element=gst_element_factory_make("audioconvert","makefloat"))) {
        GST_INFO("Can't instantiate 'audioconvert' element");goto Error;
      }
      list=g_list_append(list,element);
      */
      if(!(element=gst_element_factory_make("vorbisenc","vorbisenc"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'vorbisenc'");
        goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("oggmux","oggmux"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'oggmux'");
        goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_MP3:
      // lame ! id3mux ! filesink location="song.mp3"
      if(!(element=gst_element_factory_make("lamemp3enc","lame"))) {
        if(!(element=gst_element_factory_make("lame","lame"))) {
          // FIXME(ensonic): for some reason this does not show up on the bus
          /*GstCaps *caps=gst_caps_from_string("audio/mpeg, mpegversion=1, layer=3");

          gst_element_post_message(GST_ELEMENT(self),
            gst_missing_encoder_message_new(GST_ELEMENT(self), caps));
          gst_caps_unref(caps);
          */
          BT_SINK_BIN_MISSING_ELEMENT("'lamemp3enc'/'lame'");
          goto Error;
        }
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("id3mux","id3mux"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'id3mux'");
        goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_WAV:
      // wavenc ! filesink location="song.wav"
      if(!(element=gst_element_factory_make("wavenc","wavenc"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'wavenc'");
        goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC:
      // flacenc ! oggmux ! filesink location="song.flac"
      if(!(element=gst_element_factory_make("flacenc","flacenc"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'flacenc'");
        goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("oggmux","oggmux"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'oggmux'");
        goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_MP4_AAC:
      // faac ! mp4mux ! filesink location="song.m4a"
      if(!(element=gst_element_factory_make("faac","faac"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'faac'");
        goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("mp4mux","mp4mux"))) {
        BT_SINK_BIN_MISSING_ELEMENT("'mp4mux'");
        goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_RAW:
      // no element needed
      break;
  }
  // create filesink, set location property
  element=gst_element_factory_make("filesink","filesink");
  g_object_set(element,
    "location",self->priv->record_file_name,
    // only for recording in in realtime and not as fast as we can
    // "sync",TRUE,
    NULL);
  list=g_list_append(list,element);
  return(list);
Error:
  for(;list;list=list->next) {
    gst_object_unref(GST_OBJECT(list->data));
  }
  g_list_free(g_list_first(list));
  return(NULL);
}

#ifdef BT_MONITOR_SINK_DATA_FLOW	 
static GstClockTime sink_probe_last_ts=GST_CLOCK_TIME_NONE;	 
static gboolean sink_probe(GstPad *pad, GstMiniObject *mini_obj, gpointer user_data) {	 
  if(GST_IS_BUFFER(mini_obj)) {	 
    if(sink_probe_last_ts==GST_CLOCK_TIME_NONE) {	 
      GST_WARNING("got SOS at %"GST_TIME_FORMAT,GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(mini_obj)));	 
    }	 
    sink_probe_last_ts=GST_BUFFER_TIMESTAMP(mini_obj)+GST_BUFFER_DURATION(mini_obj);	 
  }	 
  else if(GST_IS_EVENT(mini_obj)) {	 
    GST_WARNING("got %s at %"GST_TIME_FORMAT,	 
      GST_EVENT_TYPE_NAME(mini_obj),	 
      GST_TIME_ARGS(sink_probe_last_ts));	 
    if(GST_EVENT_TYPE(mini_obj)==GST_EVENT_EOS) {	 
      sink_probe_last_ts=GST_CLOCK_TIME_NONE;	 
    }	 
  }	 
  return(TRUE);	 
}	 
#endif

static gboolean bt_sink_bin_format_update(const BtSinkBin * const self) {
  GstStructure *sink_format_structures[2];
  GstCaps *sink_format_caps;

  if(!self->priv->caps_filter) return(FALSE);

  // always add caps-filter as a first element and enforce sample rate and channels
  sink_format_structures[0]=gst_structure_from_string("audio/x-raw-int, "
    "channels = (int) 2, "
    "rate = (int) 44100, "
    "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }, "
    "width = (int) { 8, 16, 24, 32 }, "
    "depth = (int) [ 1, 32 ], "
    "signed = (boolean) { true, false };",
    NULL
  );
  sink_format_structures[1]=gst_structure_from_string("audio/x-raw-float, "
    "channels = (int) 2, "
    "rate = (int) 44100, "
    "width = (int) { 32, 64 }, "
    "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }",
    NULL
  );
  gst_structure_set(sink_format_structures[0],
    "rate",G_TYPE_INT,self->priv->sample_rate,
    "channels",G_TYPE_INT,self->priv->channels,
    NULL);
  gst_structure_set(sink_format_structures[1],
    "rate",G_TYPE_INT,self->priv->sample_rate,
    "channels",G_TYPE_INT,self->priv->channels,
    NULL);
  sink_format_caps=gst_caps_new_full(sink_format_structures[0],sink_format_structures[1],NULL);

  GST_INFO("sink is using: sample-rate=%u, channels=%u",self->priv->sample_rate,self->priv->channels);

  g_object_set(self->priv->caps_filter,"caps",sink_format_caps,NULL);
  gst_caps_unref(sink_format_caps);

  return(TRUE);
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
static gboolean bt_sink_bin_update(const BtSinkBin * const self) {
  GstElement *audio_resample,*tee;
  GstState state;
  gboolean defer=FALSE;

  // check current state
  if(gst_element_get_state(GST_ELEMENT(self),&state,NULL,0)==GST_STATE_CHANGE_SUCCESS) {
    if(state>GST_STATE_READY) defer=TRUE;
  }
  else defer=TRUE;
  if(defer) {
    GST_INFO("defer switching sink-bin elements");
    self->priv->pending_update=TRUE;
    return(FALSE);
  }

  GST_INFO_OBJECT(self,"clearing sink-bin");

  // remove existing children
  bt_sink_bin_clear(self);

  GST_INFO_OBJECT(self,"initializing sink-bin");

  self->priv->caps_filter=gst_element_factory_make("capsfilter","sink-format");
  bt_sink_bin_format_update(self);
  audio_resample=gst_element_factory_make("audioresample","sink-audioresample");
  tee=self->priv->tee=gst_element_factory_make("tee","sink-tee");

  gst_bin_add_many(GST_BIN(self),self->priv->caps_filter,tee,audio_resample,NULL);

  if(!gst_element_link_pads(self->priv->caps_filter,"src",audio_resample,"sink")) {
    GST_WARNING("Can't link caps-filter and audio-resample");
  }
#ifdef BT_MONITOR_TIMESTAMPS
  {	 
    GstElement *identity=gst_element_factory_make("identity","identity");	 

    g_object_set(identity,"check-perfect",TRUE,"silent",FALSE,NULL);	 
    gst_bin_add(GST_BIN(self),identity);
		if(!gst_element_link_pads(audio_resample,"src",identity,"sink")) {
			GST_WARNING("Can't link audio-resample and identity");
		}
		if(!gst_element_link_pads(identity,"src",tee,"sink")) {
			GST_WARNING("Can't link identity and tee");
		}
	}
#else
  if(!gst_element_link_pads(audio_resample,"src",tee,"sink")) {
    GST_WARNING("Can't link audio-resample and tee");
  }
#endif

  // add new children
  switch(self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:{
      GList * const list=bt_sink_bin_get_player_elements(self);
      if(list) {
        bt_sink_bin_add_many(self,list);
        bt_sink_bin_link_many(self,tee,list);
        g_list_free(list);
      }
      else {
        GST_WARNING("Can't get playback element list");
        return(FALSE);
      }
      break;
    }
    case BT_SINK_BIN_MODE_RECORD:{
      GList * const list=bt_sink_bin_get_recorder_elements(self);
      if(list) {
        bt_sink_bin_add_many(self,list);
        bt_sink_bin_link_many(self,tee,list);
        g_list_free(list);
      }
      else {
        GST_WARNING("Can't get record element list");
        return(FALSE);
      }
      break;
    }
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:{
      // add player elems
      GList *list1=bt_sink_bin_get_player_elements(self);
      if(list1) {
        GstElement *element;

        /*
        element=gst_element_factory_make("audioconvert","play-convert");
        list1=g_list_prepend(list1,element);
        */

        // start with a queue
        element=gst_element_factory_make("queue","play-queue");
        // TODO(ensonic): if we have/require gstreamer-0.10.31 ret rid of the check
        if(g_object_class_find_property(G_OBJECT_GET_CLASS(element),"silent")) {
          g_object_set(element,"silent",TRUE,NULL);
        }
        list1=g_list_prepend(list1,element);

        bt_sink_bin_add_many(self,list1);
        bt_sink_bin_link_many(self,tee,list1);
        g_list_free(list1);
      }
      else {
        GST_WARNING("Can't get playback element list");
        return(FALSE);
      }
      // add recorder elems
      GList * const list2=bt_sink_bin_get_recorder_elements(self);
      if(list2) {
        bt_sink_bin_add_many(self,list2);
        bt_sink_bin_link_many(self,tee,list2);
        g_list_free(list2);
      }
      else {
        GST_WARNING("Can't get record element list");
        return(FALSE);
      }
      break;
    }
    case BT_SINK_BIN_MODE_PASS_THRU:{
      GstPad *target_pad=gst_element_get_request_pad(tee,"src%d");

      self->priv->src=gst_ghost_pad_new("src",target_pad);
      gst_pad_set_active(self->priv->src,TRUE);
      gst_element_add_pad(GST_ELEMENT(self),self->priv->src);
      break;
    }
    default:
      g_assert_not_reached();
  }

  // set new ghostpad-target
  if(self->priv->sink) {
    GstPad *sink_pad=gst_element_get_static_pad(self->priv->caps_filter,"sink");
    GstPad *req_sink_pad=NULL;

    GST_INFO("updating ghostpad: %p", self->priv->sink);

    if(!sink_pad) {
      GST_INFO("failed to get static 'sink' pad for element '%s'",GST_OBJECT_NAME(self->priv->caps_filter));
      sink_pad=req_sink_pad=gst_element_get_request_pad(self->priv->caps_filter,"sink_%d");
      if(!sink_pad) {
        GST_INFO("failed to get request 'sink' request-pad for element '%s'",GST_OBJECT_NAME(self->priv->caps_filter));
      }
    }
    GST_INFO ("updating ghost pad : elem=%p (ref_ct=%d),'%s', pad=%p (ref_ct=%d)",
      self->priv->caps_filter,(G_OBJECT_REF_COUNT(self->priv->caps_filter)),GST_OBJECT_NAME(self->priv->caps_filter),
      sink_pad,(G_OBJECT_REF_COUNT(sink_pad)));

    if(!gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),sink_pad)) {
      GST_WARNING("failed to link internal pads");
    }

    GST_INFO("  done, pad=%p (ref_ct=%d)",sink_pad,(G_OBJECT_REF_COUNT(sink_pad)));
    // request pads need to be released
    if(!req_sink_pad) {
      gst_object_unref(sink_pad);
    }
  }

#ifdef BT_MONITOR_SINK_DATA_FLOW	 
  if(audio_sink) {
    GstPad *sink_pad=gst_element_get_static_pad(audio_sink,"sink");	 
 	 
    if(sink_pad) {	 
      sink_probe_last_ts=GST_CLOCK_TIME_NONE;	 
      gst_pad_add_data_probe(sink_pad,G_CALLBACK(sink_probe),(gpointer)self);	 
 	 
      gst_object_unref(sink_pad);	 
    }
  }
#endif

  GST_INFO("done");
  return(TRUE);
}

//-- event handler

static void on_audio_sink_child_added(GstBin *bin, GstElement *element, gpointer user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);
  
  if(GST_IS_BASE_SINK(element)) {
    bt_sink_bin_set_audio_sink(self,element);
  }
}

static void on_audio_sink_changed(const BtSettings * const settings, GParamSpec * const arg, gconstpointer const user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  GST_INFO("audio-sink has changed");
  gchar * const plugin_name=bt_settings_determine_audiosink_name(self->priv->settings);
  GST_INFO("  -> '%s'",plugin_name);
  g_free(plugin_name);

  // exchange the machine
  switch(self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:
      bt_sink_bin_update(self);
      break;
    default:
      break;
  }
}

static void on_system_audio_sink_changed(const BtSettings * const settings, GParamSpec * const arg, gconstpointer const user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  GST_INFO("system audio-sink has changed");
  gchar * const plugin_name=bt_settings_determine_audiosink_name(self->priv->settings);
  gchar *sink_name;

  // exchange the machine (only if the system-audiosink is in use)
  g_object_get((gpointer)settings,"system-audiosink-name",&sink_name,NULL);

  GST_INFO("  -> '%s' (sytem_sink is '%s')",plugin_name,sink_name);
  if (!strcmp(plugin_name,sink_name)) {
    // exchange the machine
    switch(self->priv->mode) {
      case BT_SINK_BIN_MODE_PLAY:
      case BT_SINK_BIN_MODE_PLAY_AND_RECORD:
        bt_sink_bin_update(self);
        break;
      default:
        break;
    }
  }
  g_free(sink_name);
  g_free(plugin_name);
}

static void on_sample_rate_changed(const BtSettings * const settings, GParamSpec * const arg, gconstpointer const user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  GST_INFO("sample-rate has changed");
  g_object_get((gpointer)settings,"sample-rate",&self->priv->sample_rate,NULL);
  bt_sink_bin_format_update(self);
}

static void on_channels_changed(const BtSettings * const settings, GParamSpec * const arg, gconstpointer const user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  GST_INFO("channels have changed");
  g_object_get((gpointer)settings,"channels",&self->priv->channels,NULL);
  bt_sink_bin_format_update(self);
  // TODO(ensonic): this would render all panorama/balance elements useless and also
  // affect wire patterns - how do we want to handle it
}

static gboolean master_volume_sync_handler(GstPad *pad,GstBuffer *buffer, gpointer user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  gst_object_sync_values(G_OBJECT(self),GST_BUFFER_TIMESTAMP(buffer));
  return(TRUE);
}

//-- methods

//-- wrapper

//-- class internals

static GstStateChangeReturn bt_sink_bin_change_state(GstElement * element, GstStateChange transition) {
  const BtSinkBin * const self = BT_SINK_BIN(element);
  BtAudioSession *audio_session=bt_audio_session_new();
  GstStateChangeReturn res;

  GST_INFO_OBJECT(self,"state change on the sink-bin: %s -> %s",
    gst_element_state_get_name(GST_STATE_TRANSITION_CURRENT (transition)),
    gst_element_state_get_name(GST_STATE_TRANSITION_NEXT (transition)));

  res = GST_ELEMENT_CLASS(bt_sink_bin_parent_class)->change_state(element,transition);

  switch(transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if(self->priv->pending_update) {
        self->priv->pending_update=FALSE;
        bt_sink_bin_update(self);
      }
      else {
        g_object_set(audio_session,"audio-locked",TRUE,NULL);
      }
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      g_object_set(audio_session,"audio-locked",FALSE,NULL);
      break;
    default:
      break;
  }
  g_object_unref(audio_session);
  return(res);
}

static void bt_sink_bin_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtSinkBin * const self = BT_SINK_BIN(object);
  return_if_disposed();
  switch (property_id) {
    case SINK_BIN_MODE: {
      g_value_set_enum(value, self->priv->mode);
    } break;
    case SINK_BIN_RECORD_FORMAT: {
      g_value_set_enum(value, self->priv->record_format);
    } break;
    case SINK_BIN_RECORD_FILE_NAME: {
      g_value_set_string(value, self->priv->record_file_name);
    } break;
    case SINK_BIN_INPUT_GAIN: {
      g_value_set_object(value, self->priv->gain);
    } break;
    case SINK_BIN_MASTER_VOLUME: {
      if(self->priv->gain) {
        // FIXME(ensonic): do we need a notify (or weak ptr) to update this?
        g_object_get(self->priv->gain,"volume",&self->priv->volume,NULL);
        GST_DEBUG("Get master volume: %lf",self->priv->volume);
      }
      g_value_set_double(value,self->priv->volume);
    } break;
    case SINK_BIN_ANALYZERS: {
      g_value_set_pointer(value, self->priv->analyzers);
    } break;
    // tempo iface
    case SINK_BIN_TEMPO_BPM:
      g_value_set_ulong(value, self->priv->beats_per_minute);
      break;
    case SINK_BIN_TEMPO_TPB:
      g_value_set_ulong(value, self->priv->ticks_per_beat);
      break;
    case SINK_BIN_TEMPO_STPT:
      g_value_set_ulong(value, self->priv->subticks_per_tick);
      break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sink_bin_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtSinkBin * const self = BT_SINK_BIN(object);
  return_if_disposed();

  switch (property_id) {
    case SINK_BIN_MODE: {
      self->priv->mode=g_value_get_enum(value);
      if((self->priv->mode==BT_SINK_BIN_MODE_PLAY) || (self->priv->mode==BT_SINK_BIN_MODE_PASS_THRU)) {
        if(self->priv->record_file_name) {
          g_free(self->priv->record_file_name);
          self->priv->record_file_name=NULL;
        }
        bt_sink_bin_update(self);
      }
      else {
        if(self->priv->record_file_name) {
          bt_sink_bin_update(self);
        }
      }
    } break;
    case SINK_BIN_RECORD_FORMAT: {
      self->priv->record_format=g_value_get_enum(value);
      if(((self->priv->mode==BT_SINK_BIN_MODE_RECORD) || (self->priv->mode==BT_SINK_BIN_MODE_PLAY_AND_RECORD)) && self->priv->record_file_name) {
        bt_sink_bin_update(self);
      }
    } break;
    case SINK_BIN_RECORD_FILE_NAME: {
      g_free(self->priv->record_file_name);
      self->priv->record_file_name = g_value_dup_string(value);
      if((self->priv->mode==BT_SINK_BIN_MODE_RECORD) || (self->priv->mode==BT_SINK_BIN_MODE_PLAY_AND_RECORD)) {
        bt_sink_bin_update(self);
      }
    } break;
    case SINK_BIN_INPUT_GAIN: {
      GstPad *sink_pad;

      g_object_try_weak_unref(self->priv->gain);
      self->priv->gain = GST_ELEMENT(g_value_get_object(value));
      GST_DEBUG_OBJECT(self->priv->gain,"Set master gain element: %p",self->priv->gain);
      g_object_try_weak_ref(self->priv->gain);
      g_object_set(self->priv->gain,"volume",self->priv->volume,NULL);
      sink_pad=gst_element_get_static_pad(self->priv->gain,"sink");
      self->priv->mv_handler_id=gst_pad_add_buffer_probe(sink_pad,G_CALLBACK(master_volume_sync_handler),(gpointer)self);
      gst_object_unref(sink_pad);
    } break;
    case SINK_BIN_MASTER_VOLUME: {
      if(self->priv->gain) {
        self->priv->volume=g_value_get_double(value);
        g_object_set(self->priv->gain,"volume",self->priv->volume,NULL);
        GST_DEBUG("Set master volume: %lf",self->priv->volume);
      }
    } break;
    case SINK_BIN_ANALYZERS: {
      bt_sink_bin_deactivate_analyzers(self);
      self->priv->analyzers=g_value_get_pointer(value);
      bt_sink_bin_activate_analyzers(self);
    } break;
    // tempo iface
    case SINK_BIN_TEMPO_BPM:
    case SINK_BIN_TEMPO_TPB:
    case SINK_BIN_TEMPO_STPT:
      GST_WARNING("use gstbt_tempo_change_tempo()");
      break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sink_bin_dispose(GObject * const object) {
  const BtSinkBin * const self = BT_SINK_BIN(object);
  GstStateChangeReturn res;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_INFO("!!!! self=%p",self);

  if((res=gst_element_set_state(GST_ELEMENT(self),GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to null state");
  }
  GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));

  g_signal_handlers_disconnect_matched(self->priv->settings,G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,(gpointer)self);
  g_object_unref(self->priv->settings);

  if(self->priv->mv_handler_id && self->priv->gain) {
    GstPad *sink_pad=gst_element_get_static_pad(self->priv->gain,"sink");

    if(sink_pad) {
      gst_pad_remove_buffer_probe(sink_pad,self->priv->mv_handler_id);
      gst_object_unref(sink_pad);
    }
  }
  g_object_try_weak_unref(self->priv->gain);

  GST_INFO("self->sink=%p, refct=%d",self->priv->sink,G_OBJECT_REF_COUNT(self->priv->sink));
  gst_element_remove_pad(GST_ELEMENT(self),self->priv->sink);
  GST_INFO("sink-bin : children=%d",GST_BIN_NUMCHILDREN(self));

  GST_INFO("chaining up");
  G_OBJECT_CLASS(bt_sink_bin_parent_class)->dispose(object);
  GST_INFO("done");
}

static void bt_sink_bin_finalize(GObject * const object) {
  const BtSinkBin * const self = BT_SINK_BIN(object);

  GST_DEBUG("!!!! self=%p",self);
  GST_INFO("sink-bin : children=%d",GST_BIN_NUMCHILDREN(self));

  g_free(self->priv->record_file_name);

  G_OBJECT_CLASS(bt_sink_bin_parent_class)->finalize(object);
}

static void bt_sink_bin_init(BtSinkBin *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SINK_BIN, BtSinkBinPrivate);

  GST_INFO("!!!! self=%p",self);

  // watch settings changes
  self->priv->settings=bt_settings_make();
  //GST_DEBUG("listen to settings changes (%p)",self->priv->settings);
  g_signal_connect(self->priv->settings, "notify::audiosink", G_CALLBACK(on_audio_sink_changed), (gpointer)self);
  g_signal_connect(self->priv->settings, "notify::system-audiosink", G_CALLBACK(on_system_audio_sink_changed), (gpointer)self);
  g_signal_connect(self->priv->settings, "notify::sample-rate", G_CALLBACK(on_sample_rate_changed), (gpointer)self);
  g_signal_connect(self->priv->settings, "notify::channels", G_CALLBACK(on_channels_changed), (gpointer)self);

  g_object_get(self->priv->settings,
    "sample-rate",&self->priv->sample_rate,
    "channels",&self->priv->channels,
    NULL);
  self->priv->volume=1.0;

  self->priv->sink=gst_ghost_pad_new_no_target("sink",GST_PAD_SINK);
  gst_element_add_pad(GST_ELEMENT(self),self->priv->sink);
  bt_sink_bin_update(self);

  GST_INFO("done");
}

static void bt_sink_bin_class_init(BtSinkBinClass * klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass * const element_class = GST_ELEMENT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtSinkBinPrivate));

  gobject_class->set_property = bt_sink_bin_set_property;
  gobject_class->get_property = bt_sink_bin_get_property;
  gobject_class->dispose      = bt_sink_bin_dispose;
  gobject_class->finalize     = bt_sink_bin_finalize;

  element_class->change_state = bt_sink_bin_change_state;

  // override interface properties
  g_object_class_override_property(gobject_class, SINK_BIN_TEMPO_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, SINK_BIN_TEMPO_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, SINK_BIN_TEMPO_STPT, "subticks-per-tick");

  g_object_class_install_property(gobject_class,SINK_BIN_MODE,
                                  g_param_spec_enum("mode",
                                     "mode prop",
                                     "mode of operation",
                                     BT_TYPE_SINK_BIN_MODE,  /* enum type */
                                     BT_SINK_BIN_MODE_PLAY, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SINK_BIN_RECORD_FORMAT,
                                  g_param_spec_enum("record-format",
                                     "record-format prop",
                                     "format to use when in record mode",
                                     BT_TYPE_SINK_BIN_RECORD_FORMAT,  /* enum type */
                                     BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SINK_BIN_RECORD_FILE_NAME,
                                  g_param_spec_string("record-file-name",
                                     "record-file-name contruct prop",
                                     "the file-name to use for recording",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SINK_BIN_INPUT_GAIN,
                                  g_param_spec_object("input-gain",
                                     "input-gain prop",
                                     "the input-gain element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SINK_BIN_MASTER_VOLUME,
                                  g_param_spec_double("master-volume",
                                     "master volume prop",
                                     "master volume for the song",
                                     0.0,
                                     1.0,
                                     1.0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,SINK_BIN_ANALYZERS,
                                  g_param_spec_pointer("analyzers",
                                     "analyzers prop",
                                     "list of master analyzers",
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  gst_element_class_set_details_simple (element_class,
    "Master AudioSink",
    "Audio/Bin",
    "Play/Record audio",
    "Stefan Kost <ensonic@users.sf.net>");
}

//-- plugin handling

gboolean bt_sink_bin_plugin_init(GstPlugin * const plugin) {
  gst_element_register(plugin,"bt-sink-bin",GST_RANK_NONE,BT_TYPE_SINK_BIN);
  return TRUE;
}

