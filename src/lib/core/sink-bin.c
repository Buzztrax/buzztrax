/* $Id$
 *
 * Buzztard
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
 * The sink-bin provides switchable play and record facillities.
 * It also provides controlable master-volume.
 */

/* @todo: detect supported encoders
 * get gst mimetype from the extension
 * and then look at all encoders which supply that mimetype
 * check elements in "codec/encoder/audio", "codec/muxer/audio"
 * build caps using this mimetype
 * gst_element_factory_can_src_caps()
 * problem here is that we need extra option for each encoder (e.g. quality)
 *
 * @todo: add properties for bpm and master volume,
 * - bpm
 *   - tempo-iface is implemented, but is hidden from the ui
 *     (the iface properties are not controlable)
 *   - we could have separate properties and forward the changes
 *
 * @todo: for upnp it would be nice to stream on-demand
 *
 * @todo: add parameters for sampling rate and channels
 *   - channels can be used in the capsfilter
 *   - sampling rate could be used there too 
 *   - both should be sink-bin properties, so that we can configure them
 *     externaly
 */

#define BT_CORE
#define BT_SINK_BIN_C

#include "core_private.h"
#include <libbuzztard-core/sink-bin.h>

/* define this to get diagnostics of the sink data flow */
//#define BT_MONITOR_SINK_DATA_FLOW
/* define this to verify continuos timestamps */
//#define BT_MONITOR_TIMESTAMPS

/* this requires gstreamer-0.10.16 */
#ifndef GST_TIME_AS_USECONDS
#define GST_TIME_AS_USECONDS(time) ((time) / G_GINT64_CONSTANT (1000))
#endif

//-- property ids

enum {
  SINK_BIN_MODE=1,
  SINK_BIN_RECORD_FORMAT,
  SINK_BIN_RECORD_FILE_NAME,
  SINK_BIN_INPUT_GAIN,
  SINK_BIN_MASTER_VOLUME,
  // tempo iface
  SINK_BIN_TEMPO_BPM,
  SINK_BIN_TEMPO_TPB,
  SINK_BIN_TEMPO_STPT,
};

struct _BtSinkBinPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* change configuration at next convinient point? */
  gboolean pending_update;

  /* mode of operation */
  BtSinkBinMode mode;
  BtSinkBinRecordFormat record_format;
  guint sample_rate;

  gchar *record_file_name;

  /* sink ghost-pad of the bin */
  GstPad *sink;

  /* we need to hold the reference to not kill the notifies */
  BtSettings *settings;

  gulong bus_handler_id;
  
  /* master volume */
  G_POINTER_ALIAS(GstElement *,gain);
  gulong mv_handler_id;
  
  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
};

static GstBinClass *parent_class=NULL;

static void bt_sink_bin_configure_latency(const BtSinkBin * const self,GstElement *sink);
static void on_song_state_changed(const GstBus * const bus, GstMessage *message, gconstpointer user_data);

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
    GstElement *element = gst_bin_get_by_name(GST_BIN(self),"player");

    GST_DEBUG("changing tempo to %d BPM  %d TPB  %d STPT",self->priv->beats_per_minute,self->priv->ticks_per_beat,self->priv->subticks_per_tick);
    
    if(element) {
      bt_sink_bin_configure_latency(self,element);
      gst_object_unref(element);
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
      { BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC,   ".flac",   "ogg-flac" },
      { BT_SINK_BIN_RECORD_FORMAT_RAW,        ".raw",        "raw" },
      /*
      { BT_SINK_BIN_RECORD_FORMAT_MP4_AAC,   ".m4a",   "mp4 aac" },
      */
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSinkBinMRecordFormat", values);
  }
  return type;
}

//-- helper methods

static void bt_sink_bin_configure_latency(const BtSinkBin * const self,GstElement *sink) {
  if(GST_IS_BASE_AUDIO_SINK(sink)) {
    if(self->priv->beats_per_minute && self->priv->ticks_per_beat) {
      // configure buffer size (e.g.  GST_SECONG*60/120*4
      gint64 chunk=GST_TIME_AS_USECONDS((GST_SECOND*60)/(self->priv->beats_per_minute*self->priv->ticks_per_beat));
      GST_INFO("changing audio chunk-size for sink to %"G_GUINT64_FORMAT" µs = %"G_GUINT64_FORMAT" ms",
        chunk, (chunk/G_GINT64_CONSTANT(1000)));
      // @todo: bah, pulseaudio does no like those at all
      g_object_set(sink,"latency-time",chunk,"buffer-time",chunk<<1,NULL);
    }
  }
}

static void bt_sink_bin_clear(const BtSinkBin * const self) {
  GstBin * const bin=GST_BIN(self);
  GstElement *elem;

  GST_INFO("clearing sink-bin : %d",GST_BIN_NUMCHILDREN(bin));
  if(bin->children) {
    GstStateChangeReturn res;

    //gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),NULL);
    //GST_DEBUG("released ghost-pad");

    while(bin->children) {
      elem=GST_ELEMENT_CAST (bin->children->data);
      GST_DEBUG("  removing elem=%p (ref_ct=%d),'%s'",
        elem,(G_OBJECT(elem)->ref_count),GST_OBJECT_NAME(elem));
      if((res=gst_element_set_state(elem,GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
        GST_WARNING("can't go to null state");
      else
        GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));
      gst_bin_remove (bin, elem);
    }
  }
  GST_DEBUG("done");
}

static gboolean bt_sink_bin_add_many(const BtSinkBin * const self, GList * const list) {
  const GList *node;

  GST_DEBUG("add elements: list=%p",list);

  for(node=list;node;node=node->next) {
    gst_bin_add(GST_BIN(self),GST_ELEMENT(node->data));
    gst_element_set_state(GST_ELEMENT(node->data),GST_STATE_READY);
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

static gchar *bt_sink_bin_determine_plugin_name(void) {
  BtSettings *settings;
  gchar *audiosink_name,*system_audiosink_name;
  gchar *plugin_name=NULL;

  settings=bt_settings_new();
  g_object_get(G_OBJECT(settings),"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);

  if(BT_IS_STRING(audiosink_name)) {
    GST_INFO("get audiosink from config");
    plugin_name=audiosink_name;
    audiosink_name=NULL;
  }
  else if(BT_IS_STRING(system_audiosink_name)) {
    GST_INFO("get audiosink from system config");
    plugin_name=system_audiosink_name;
    system_audiosink_name=NULL;
  }
  if(plugin_name) {
    gchar *sink_name,*eon;
    // this can be a whole pipeline like "audioconvert ! osssink sync=false"
    // seek for the last '!'
    if(!(sink_name=g_strrstr(plugin_name,"!"))) {
      sink_name=plugin_name;
    }
    else {
      // skip '!' and spaces
      sink_name++;
      while(*sink_name==' ') sink_name++;
    }
    // if there is a space following put '\0' in there
    if((eon=strstr(sink_name," "))) {
      *eon='\0';
    }
    // no g_free() to partial memory later
    gchar * const temp=plugin_name;
    plugin_name=g_strdup(sink_name);
    g_free(temp);
  }
  if (!BT_IS_STRING(plugin_name)) {
    // @todo: try autoaudiosink (if it exists)
    // iterate over gstreamer-audiosink list and choose element with highest rank
    const GList *node;
    GList * const audiosink_names=bt_gst_registry_get_element_names_matching_all_categories("Sink/Audio");
    guint max_rank=0,cur_rank;
    GstCaps *caps1=gst_caps_from_string(GST_AUDIO_INT_PAD_TEMPLATE_CAPS);
    GstCaps *caps2=gst_caps_from_string(GST_AUDIO_FLOAT_PAD_TEMPLATE_CAPS);

    GST_INFO("get audiosink from gst registry by rank");

    for(node=audiosink_names;node;node=g_list_next(node)) {
      GstElementFactory * const factory=gst_element_factory_find(node->data);

      // can the sink accept raw audio?
      if(gst_element_factory_can_sink_caps(factory,caps1) || gst_element_factory_can_sink_caps(factory,caps2)) {
        // get element max(rank)
        cur_rank=gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
        GST_INFO("  trying audio sink: \"%s\" with rank: %d",node->data,cur_rank);
        if((cur_rank>=max_rank) || (!plugin_name)) {
          plugin_name=g_strdup(node->data);
          max_rank=cur_rank;
        }
      }
      else {
        GST_INFO("  skipping audio sink: \"%s\" because of incompatible caps",node->data);
      }
    }
    g_list_free(audiosink_names);
    gst_caps_unref(caps1);
    gst_caps_unref(caps2);
  }
  GST_INFO("using audio sink : \"%s\"",plugin_name);

  g_free(system_audiosink_name);
  g_free(audiosink_name);
  g_object_unref(settings);

  return(plugin_name);
}

static GList *bt_sink_bin_get_player_elements(const BtSinkBin * const self) {
  GList *list=NULL;
  gchar *plugin_name;

  GST_DEBUG("get playback elements");

  plugin_name=bt_sink_bin_determine_plugin_name();
  GstElement * const element=gst_element_factory_make(plugin_name,"player");
  if(!element) {
    /* todo: if this fails
     * check if it was audiosink in settings and if so, unset it and retry
     * else check if ir was system-audiosink and if so, what?
     */
    GST_WARNING("Can't instantiate '%s' element",plugin_name);goto Error;
  }
  if(GST_IS_BASE_SINK(element)) {
    // enable syncing to timestamps
    g_object_set(element,
      "sync",TRUE,
      /*"slave-method",2,*/
      NULL);
    bt_sink_bin_configure_latency(self,element);
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

  // @todo: check extension ?
  // generate recorder elements
  switch(self->priv->record_format) {
    case BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS:
      // vorbisenc ! oggmux ! filesink location="song.ogg"
      if(!(element=gst_element_factory_make("audioconvert","makefloat"))) {
        GST_INFO("Can't instantiate 'audioconvert' element");goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("vorbisenc","vorbisenc"))) {
        GST_INFO("Can't instantiate 'vorbisenc' element");goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("oggmux","oggmux"))) {
        GST_INFO("Can't instantiate 'oggmux' element");goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_MP3:
      // lame ! id3mux ! filesink location="song.mp3"
      if(!(element=gst_element_factory_make("lame","lame"))) {
        GST_INFO("Can't instantiate 'lame' element");goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("id3mux","id3mux"))) {
        GST_INFO("Can't instantiate 'id3mux' element");goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_WAV:
      // wavenc ! filesink location="song.wav"
      if(!(element=gst_element_factory_make("wavenc","wavenc"))) {
        GST_INFO("Can't instantiate 'wavenc' element");goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC:
      // flacenc ! oggmux ! filesink location="song.flac"
      if(!(element=gst_element_factory_make("flacenc","flacenc"))) {
        GST_INFO("Can't instantiate 'flacenc' element");goto Error;
      }
      list=g_list_append(list,element);
      if(!(element=gst_element_factory_make("oggmux","oggmux"))) {
        GST_INFO("Can't instantiate 'oggmux' element");goto Error;
      }
      list=g_list_append(list,element);
      break;
    case BT_SINK_BIN_RECORD_FORMAT_RAW:
      // no element needed
      break;
  }
  // create filesink, set location property
  GstElement * const filesink=gst_element_factory_make("filesink","filesink");
  g_object_set(filesink,"location",self->priv->record_file_name,NULL);
  list=g_list_append(list,filesink);
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
  GstElement *caps_filter,*audio_resample,*first_elem;
  GstStructure *stereo_structures[2];
  GstCaps *stereo_caps;
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
    if(!self->priv->bus_handler_id) {
      // watch the bus
      GstObject *parent=gst_object_get_parent(GST_OBJECT(self));
      GstBus *bus=gst_element_get_bus(GST_ELEMENT(parent));
      GST_DEBUG("listen to bus messages (%p)",bus);
      self->priv->bus_handler_id=g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_song_state_changed), (gpointer)self);
      gst_object_unref(bus);
      gst_object_unref(parent);
    }
    return(FALSE);
  }

  GST_INFO("clearing sink-bin");

  // remove existing children
  bt_sink_bin_clear(self);

  GST_INFO("initializing sink-bin");
  
  // always add caps-filter as a first element and enforce stereo
  /* @todo: we would also enforce samplerate here
   * - for that we need to check the caps of the audiosink for offered caps
   *   - not sure how well that would work with autoaudiosink/gconfaudiosink
   * - we could plug audioresample right behind and then use any sampling rate
   * - sensible values : 8000, 11025, 22050, 32000, 44100, 48000, 96000
   */
  stereo_structures[0]=gst_structure_from_string("audio/x-raw-int, "
    "channels = (int) 2, "
    "rate = (int) 44100, "
    "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }, "
    "width = (int) { 8, 16, 24, 32 }, "
    "depth = (int) [ 1, 32 ], "
    "signed = (boolean) { true, false };",
    NULL
  );
  stereo_structures[1]=gst_structure_from_string("audio/x-raw-float, "
    "channels = (int) 2, "
    "rate = (int) 44100, "
    "width = (int) { 32, 64 }, "
    "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }",
    NULL
  );
  gst_structure_set(stereo_structures[0],
    "rate",G_TYPE_INT,self->priv->sample_rate,
    NULL);
  gst_structure_set(stereo_structures[1],
    "rate",G_TYPE_INT,self->priv->sample_rate,
    NULL);
  stereo_caps=gst_caps_new_full(stereo_structures[0],stereo_structures[1],NULL);
  
#if 0
  gst_caps_from_string(
    "audio/x-raw-int, "
      "channels = (int) 2, "
      "rate = (int) [ 1, MAX ], "
      "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }, "
      "width = (int) { 8, 16, 24, 32 }, "
      "depth = (int) [ 1, 32 ], "
      "signed = (boolean) { true, false };"
    "audio/x-raw-float, "
      "channels = (int) 2, "
      "rate = (int) [ 1, MAX ], "
      "width = (int) { 32, 64 }, "
      "endianness = (int) { LITTLE_ENDIAN, BIG_ENDIAN }"
  );
#endif
  
  caps_filter=gst_element_factory_make("capsfilter","capsfilter");
  g_object_set(caps_filter,"caps",stereo_caps,NULL);
  gst_caps_unref(stereo_caps);
  gst_bin_add(GST_BIN(self),caps_filter);
  
  /* add audioresample */ 
  first_elem=audio_resample=gst_element_factory_make("audioresample","audioresample");
  //first_elem=audio_resample=gst_element_factory_make("speexresample","audioresample");
  gst_bin_add(GST_BIN(self),audio_resample);
  if(!gst_element_link(caps_filter,audio_resample)) {
    GST_WARNING("Can't link caps-filter and audio-resample");
  }

#ifdef BT_MONITOR_TIMESTAMPS
  {
    GstElement *identity;

    first_elem=identity=gst_element_factory_make("identity","identity");
    g_object_set(identity,"check-perfect",TRUE,NULL);
    gst_bin_add(GST_BIN(self),identity);
    if(!gst_element_link(audio_resample,identity)) {
      GST_WARNING("Can't link caps-filter and audio-resample");
    }
  }
#endif

  // add new children
  switch(self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:{
      GList * const list=bt_sink_bin_get_player_elements(self);
      if(list) {
        bt_sink_bin_add_many(self,list);
        bt_sink_bin_link_many(self,first_elem,list);
        g_list_free(list);
      }
      else {
        GST_WARNING("Can't get playback element list");
        return(FALSE);
      }
      break;}
    case BT_SINK_BIN_MODE_RECORD:{
      GList * const list=bt_sink_bin_get_recorder_elements(self);
      if(list) {
        bt_sink_bin_add_many(self,list);
        bt_sink_bin_link_many(self,first_elem,list);
        g_list_free(list);
      }
      else {
        GST_WARNING("Can't get record element list");
        return(FALSE);
      }
      break;}
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:{
      GstElement *tee;
      // add a tee element
      gchar * const name=g_strdup_printf("tee_%p",self);
      tee=gst_element_factory_make("tee",name);
      g_free(name);
      gst_bin_add(GST_BIN(self),tee);
      gst_element_link(first_elem,tee);
      // add player elems
      GList * const list1=bt_sink_bin_get_player_elements(self);
      if(list1) {
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
      break;}
    default:
      g_assert_not_reached();
  }

  // set new ghostpad-target
  if(self->priv->sink) {
    GstPad *sink_pad=gst_element_get_static_pad(caps_filter,"sink");
    GstPad *req_sink_pad=NULL;

    GST_INFO("updating ghostpad: %p", self->priv->sink);

    if(!sink_pad) {
      GST_INFO("failed to get static 'sink' pad for element '%s'",GST_OBJECT_NAME(caps_filter));
      sink_pad=req_sink_pad=gst_element_get_request_pad(caps_filter,"sink_%d");
      if(!sink_pad) {
        GST_INFO("failed to get request 'sink' request-pad for element '%s'",GST_OBJECT_NAME(caps_filter));
      }
    }
    GST_INFO ("updating ghost pad : elem=%p (ref_ct=%d),'%s', pad=%p (ref_ct=%d)",
      caps_filter,(G_OBJECT(caps_filter)->ref_count),GST_OBJECT_NAME(caps_filter),
      sink_pad,(G_OBJECT(sink_pad)->ref_count));
    gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),sink_pad);
    GST_INFO("  done, pad=%p (ref_ct=%d)",sink_pad,(G_OBJECT(sink_pad)->ref_count));
    // request pads need to be released
    if(!req_sink_pad) {
      gst_object_unref(sink_pad);
    }
  }
  
#ifdef BT_MONITOR_SINK_DATA_FLOW
  {
    GstElement *audio_sink=gst_bin_get_by_name(GST_BIN(self),"player");
    
    if(audio_sink) {
      GstPad *sink_pad=gst_element_get_static_pad(audio_sink,"sink");
      
      if(sink_pad) {
        sink_probe_last_ts=GST_CLOCK_TIME_NONE;
        gst_pad_add_data_probe(sink_pad,G_CALLBACK(sink_probe),(gpointer)self);
        
        gst_object_unref(sink_pad);
      }
      gst_object_unref(audio_sink);
    }
  }
#endif

  GST_INFO("done");
  return(TRUE);
}

//-- event handler

static void on_song_state_changed(const GstBus * const bus, GstMessage *message, gconstpointer user_data) {
  if(!user_data) {
    // @todo: what went wrong here?
    GST_WARNING("!! invalid args");
    return;
  }

  const BtSinkBin * const self = BT_SINK_BIN(user_data);

  if(!self->priv->pending_update)
    return;

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self)) {
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    GST_INFO("state change on the sink-bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_PAUSED_TO_READY:
        self->priv->pending_update=FALSE;
        bt_sink_bin_update(self);
        break;
      default:
        break;
    }
  }
}

static void on_audio_sink_changed(const BtSettings * const settings, GParamSpec * const arg, gconstpointer const user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  g_assert(user_data);

  GST_INFO("audio-sink has changed");
  gchar * const plugin_name=bt_sink_bin_determine_plugin_name();
  GST_INFO("  -> '%s'",plugin_name);

  // exchange the machine
  switch(self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:
      bt_sink_bin_update(self);
      break;
    default:
      break;
  }
  g_free(plugin_name);
}

static void on_system_audio_sink_changed(const BtSettings * const settings, GParamSpec * const arg, gconstpointer const user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);

  g_assert(user_data);

  GST_INFO("system audio-sink has changed");
  gchar * const plugin_name=bt_sink_bin_determine_plugin_name();
  gchar *sink_name;

  // exchange the machine (only if the system-audiosink is in use)
  g_object_get(G_OBJECT(settings),"system_audiosink_name",&sink_name,NULL);

  GST_INFO("  -> '%s' (sytem_sink is '%s')",plugin_name,sink_name);
  if (!strcmp(plugin_name,sink_name)) {
    bt_sink_bin_update(self);
  }
  g_free(sink_name);
  g_free(plugin_name);
}

static gboolean master_volume_sync_handler(GstPad *pad,GstBuffer *buffer, gpointer user_data) {
  BtSinkBin *self = BT_SINK_BIN(user_data);
  
  gst_object_sync_values(G_OBJECT(self),GST_BUFFER_TIMESTAMP(buffer));
  return(TRUE);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_sink_bin_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
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
        gdouble volume;

        g_object_get(self->priv->gain,"volume",&volume,NULL);
        g_value_set_double(value,volume);
      }
      else g_value_set_double(value,1.0);
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

/* sets the given properties for this object */
static void bt_sink_bin_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSinkBin * const self = BT_SINK_BIN(object);
  return_if_disposed();

  switch (property_id) {
    case SINK_BIN_MODE: {
      self->priv->mode=g_value_get_enum(value);
      if(self->priv->mode==BT_SINK_BIN_MODE_PLAY) {
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
      g_object_try_weak_ref(self->priv->gain);
      sink_pad=gst_element_get_static_pad(self->priv->gain,"sink");
      self->priv->mv_handler_id=gst_pad_add_buffer_probe(sink_pad,G_CALLBACK(master_volume_sync_handler),(gpointer)self);
      gst_object_unref(sink_pad);
    } break;
    case SINK_BIN_MASTER_VOLUME: {
      if(self->priv->gain) {
        g_object_set(self->priv->gain,"volume",g_value_get_double(value),NULL);
      }
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

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  // disconnect handlers
  if(self->priv->bus_handler_id) {
    GstBus *bus=gst_element_get_bus(GST_ELEMENT(self));
    if(bus) {
      g_signal_handler_disconnect(bus,self->priv->bus_handler_id);
      //g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_song_state_changed,(gpointer)self);
      gst_object_unref(bus);
    }
    else {
      GST_WARNING("element has no bus ?!?");
    }
  }

  g_signal_handlers_disconnect_matched(self->priv->settings,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_audio_sink_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(self->priv->settings,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_system_audio_sink_changed,(gpointer)self);
  g_object_unref(self->priv->settings);
 
  if(self->priv->mv_handler_id && self->priv->gain) {
    GstPad *sink_pad=gst_element_get_static_pad(self->priv->gain,"sink");
    
    if(sink_pad) {
      gst_pad_remove_buffer_probe(sink_pad,self->priv->mv_handler_id);
      gst_object_unref(sink_pad);
    }
  }  
  g_object_try_weak_unref(self->priv->gain);

  GST_INFO("self->sink=%p, refct=%d",self->priv->sink,(G_OBJECT(self->priv->sink))->ref_count);
  gst_element_remove_pad(GST_ELEMENT(self),self->priv->sink);
  //gst_object_unref(self->priv->sink);
  
  GST_INFO("sink-bin : children=%d",GST_BIN_NUMCHILDREN(self));

  GST_INFO("chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_INFO("done");
}

static void bt_sink_bin_finalize(GObject * const object) {
  const BtSinkBin * const self = BT_SINK_BIN(object);

  GST_DEBUG("!!!! self=%p",self);
  GST_INFO("sink-bin : children=%d",GST_BIN_NUMCHILDREN(self));

  g_free(self->priv->record_file_name);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_sink_bin_base_init (gpointer klass) {
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  const GstElementDetails element_details =
  GST_ELEMENT_DETAILS ("Master AudioSink",
    "Audio/Bin",
    "Play/Record audio",
    "Stefan Kost <ensonic@users.sf.net>");
  
  gst_element_class_set_details (element_class, &element_details);
}

static void bt_sink_bin_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSinkBin * const self = BT_SINK_BIN(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SINK_BIN, BtSinkBinPrivate);
  
  /* @todo: get this from settings and follow changes */
  self->priv->sample_rate=GST_AUDIO_DEF_RATE;

  self->priv->sink=gst_ghost_pad_new_no_target("sink",GST_PAD_SINK);
  gst_element_add_pad(GST_ELEMENT(self),self->priv->sink);
  bt_sink_bin_update(self);

  // watch settings changes
  self->priv->settings=bt_settings_new();
  //GST_DEBUG("listen to settings changes (%p)",self->priv->settings);
  g_signal_connect(G_OBJECT(self->priv->settings), "notify::audiosink", G_CALLBACK(on_audio_sink_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->settings), "notify::system-audiosink", G_CALLBACK(on_system_audio_sink_changed), (gpointer)self);
}

static void bt_sink_bin_class_init(BtSinkBinClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSinkBinPrivate));

  gobject_class->set_property = bt_sink_bin_set_property;
  gobject_class->get_property = bt_sink_bin_get_property;
  gobject_class->dispose      = bt_sink_bin_dispose;
  gobject_class->finalize     = bt_sink_bin_finalize;

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
                                     10.0,
                                     1.0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|GST_PARAM_CONTROLLABLE));
}

GType bt_sink_bin_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) bt_sink_bin_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GTypeInfo info = {
      sizeof(BtSinkBinClass),
      (GBaseInitFunc)bt_sink_bin_base_init, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sink_bin_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSinkBin),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_sink_bin_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GST_TYPE_BIN,"BtSinkBin",&info,0);
    g_type_add_interface_static(type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
  }
  return type;
}

//-- plugin handling

#if !GST_CHECK_VERSION(0,10,16)
static
#endif
gboolean bt_sink_bin_plugin_init (GstPlugin * const plugin) {
  gst_element_register(plugin,"bt-sink-bin",GST_RANK_NONE,BT_TYPE_SINK_BIN);
  return TRUE;
}

#if !GST_CHECK_VERSION(0,10,16)
GST_PLUGIN_DEFINE_STATIC(
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  "bt-sink-bin",
  "buzztard sink bin - encapsulates play and record functionality",
  bt_sink_bin_plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztard.org"
);
#endif

