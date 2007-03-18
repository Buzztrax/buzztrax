/* $Id: sink-bin.c,v 1.29 2007-03-18 19:23:45 ensonic Exp $
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
 */

 /* @todo: detect supported encoders
 * get gst mimetype from the extension
 * and then look at all encoders which supply that mimetype
 * check elements in "codec/encoder/audio", "codec/muxer/audio"
 * build caps using this mimetype
 * gst_element_factory_can_src_caps()
 * problem here is that we need extra option for each encoder (e.g. quality)
 *
 * @todo: listen to settings changes to switch audio-sink
 *
 * @todo: add properties for bpm and master volume,
 * - implementing the tempo iface here wont help
 * - we could make it a dummy property and from the song listen to property
 *   notify
 * - we could implement the child-bin iface and add song-info as a child, in
 *   song-info we make tempo controlable
 */

#define BT_CORE
#define BT_SINK_BIN_C

#include <libbtcore/core.h>
#include <libbtcore/sink-bin.h>

//-- property ids

enum {
  SINK_BIN_MODE=1,
  SINK_BIN_RECORD_FORMAT,
  SINK_BIN_RECORD_FILE_NAME,
  SINK_BIN_TEMPO_BPM
};

struct _BtSinkBinPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* change configuration at next convinient point? */
  gboolean pending_update;

  /* mode of operation */
  BtSinkBinMode mode;
  BtSinkBinRecordFormat record_format;

  gchar *record_file_name;

  GstPad *sink;

  /* we need to hold the reference to not kill the notifies */
  BtSettings *settings;

  gulong bus_handler_id;
};

static GstBinClass *parent_class=NULL;

static void on_song_state_changed(const GstBus * const bus, GstMessage *message, gconstpointer user_data);

//-- enums

GType bt_sink_bin_mode_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_SINK_BIN_MODE_PLAY,            "BT_SINK_BIN_MODE_PLAY",            "play the song" },
      { BT_SINK_BIN_MODE_RECORD,          "BT_SINK_BIN_MODE_RECORD",          "record to file" },
      { BT_SINK_BIN_MODE_PLAY_AND_RECORD, "BT_SINK_BIN_MODE_PLAY_AND_RECORD", "play and record together" },
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
      { BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, "BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS", "ogg vorbis" },
      { BT_SINK_BIN_RECORD_FORMAT_MP3,        "BT_SINK_BIN_RECORD_FORMAT_MP3",        "mp3" },
      { BT_SINK_BIN_RECORD_FORMAT_WAV,        "BT_SINK_BIN_RECORD_FORMAT_WAV",        "wav" },
      { BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC,   "BT_SINK_BIN_RECORD_FORMAT_OGG_FLAC",   "ogg flac" },
      { BT_SINK_BIN_RECORD_FORMAT_RAW,        "BT_SINK_BIN_RECORD_FORMAT_RAW",        "raw" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSinkBinMRecordFormat", values);
  }
  return type;
}

//-- helper methods

static void bt_sink_bin_clear(const BtSinkBin * const self) {
  GstBin * const bin=GST_BIN(self);
  GstElement *elem;

  GST_INFO("clearing sink-bin : %d",g_list_length(bin->children));
  if(bin->children) {
    //gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),NULL);
    GST_DEBUG("released ghost-pad");

    while(bin->children) {
      elem=GST_ELEMENT_CAST (bin->children->data);
      GST_DEBUG("  removing elem=%p (ref_ct=%d),'%s'",
        elem,(G_OBJECT(elem)->ref_count),GST_OBJECT_NAME(elem));
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
  }
  return(TRUE);
}

static void bt_sink_bin_link_many(const BtSinkBin * const self, GstElement *last_elem, GList * const list) {
  const GList *node;

  GST_DEBUG("link elements: list=%p",list);
  if(!list) return;

  for(node=list;node;node=node->next) {
    GstElement * const cur_elem=GST_ELEMENT(node->data);
    gst_element_link(last_elem,cur_elem);
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

  plugin_name=bt_sink_bin_determine_plugin_name();
  GstElement * const element=gst_element_factory_make(plugin_name,"player");
  if(!element) {
    GST_INFO("Can't instantiate '%d' element",plugin_name);goto Error;
  }
  if(GST_IS_BASE_SINK(element)) {
    // enable syncing to timestamps
    gst_base_sink_set_sync(GST_BASE_SINK(element),TRUE);
  }
  //else {
  //g_object_set(G_OBJECT (element), "sync", FALSE, NULL);
  //}
  list=g_list_append(list,element);

Error:
  g_free(plugin_name);
  return(list);
}

static GList *bt_sink_bin_get_recorder_elements(const BtSinkBin * const self) {
  GList *list=NULL;
  GstElement *element;

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
  GstElement *first_elem=NULL;
  GstPad *sink_pad;
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

  // add new children
  switch(self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:{
      GList * const list=bt_sink_bin_get_player_elements(self);
      if(list) {
        bt_sink_bin_add_many(self,list);
        first_elem=GST_ELEMENT(list->data);
        bt_sink_bin_link_many(self,first_elem,list->next);
        g_list_free(list);
      }
      else {
        GST_WARNING("Can't get playback elemnt list");
        return(FALSE);
      }
      break;}
    case BT_SINK_BIN_MODE_RECORD:{
      GList * const list=bt_sink_bin_get_recorder_elements(self);
      if(list) {
        bt_sink_bin_add_many(self,list);
        first_elem=GST_ELEMENT(list->data);
        bt_sink_bin_link_many(self,first_elem,list->next);
        g_list_free(list);
      }
      else {
        GST_WARNING("Can't get record elemnt list");
        return(FALSE);
      }
      break;}
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:{
      // add a tee element
      gchar * const name=g_strdup_printf("tee_%p",self);
      first_elem=gst_element_factory_make("tee",name);
      g_free(name);
      gst_bin_add(GST_BIN(self),first_elem);
      // add player elems
      GList * const list1=bt_sink_bin_get_player_elements(self);
      if(list1) {
        bt_sink_bin_add_many(self,list1);
        bt_sink_bin_link_many(self,first_elem,list1);
        g_list_free(list1);
      }
      else {
        GST_WARNING("Can't get playback elemnt list");
        return(FALSE);
      }
      // add recorder elems
      GList * const list2=bt_sink_bin_get_recorder_elements(self);
      if(list2) {
        bt_sink_bin_add_many(self,list2);
        bt_sink_bin_link_many(self,first_elem,list2);
        g_list_free(list2);
      }
      else {
        GST_WARNING("Can't get record elemnt list");
        return(FALSE);
      }
      break;}
    default:
      g_assert_not_reached();
  }

  // set new ghostpad-target
  sink_pad=gst_element_get_pad(first_elem,"sink");
  if(!sink_pad) {
    GST_INFO("failed to get 'sink' pad for element '%s'",GST_OBJECT_NAME(first_elem));
    sink_pad=gst_element_get_request_pad(first_elem,"sink_%d");
    if(!sink_pad) {
      GST_INFO("failed to get 'sink' request-pad for element '%s'",GST_OBJECT_NAME(first_elem));
    }
  }
  GST_INFO("updating ghost pad : elem=%p (ref_ct=%d),'%s', pad=%p (ref_ct=%d)",
    first_elem,(G_OBJECT(first_elem)->ref_count),GST_OBJECT_NAME(first_elem),
    sink_pad,(G_OBJECT(sink_pad)->ref_count));
  gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),sink_pad);
  GST_INFO("  done, pad=%p (ref_ct=%d)",sink_pad,(G_OBJECT(sink_pad)->ref_count));
  gst_object_unref(sink_pad);

  GST_INFO("done");
  return(TRUE);
}

//-- event handler

static void on_song_state_changed(const GstBus * const bus, GstMessage *message, gconstpointer user_data) {
  const BtSinkBin * const self = BT_SINK_BIN(user_data);

  g_assert(user_data);
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
  //BtSinkBin *self = BT_SINK_BIN(user_data);

  g_assert(user_data);

  GST_INFO("system audio-sink has changed");
  gchar * const plugin_name=bt_sink_bin_determine_plugin_name();
  GST_INFO("  -> '%s'",plugin_name);

  // @todo exchange the machine (only if the system-audiosink is in use)
  //bt_sink_bin_update(self);
  g_free(plugin_name);
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

  // @todo avoid non-sense updates
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
    g_signal_handler_disconnect(bus,self->priv->bus_handler_id);
    //g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_song_state_changed,(gpointer)self);
    gst_object_unref(bus);
  }

  g_signal_handlers_disconnect_matched(self->priv->settings,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_audio_sink_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(self->priv->settings,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_system_audio_sink_changed,(gpointer)self);
  g_object_unref(self->priv->settings);

  GST_INFO("self->sink=%p, refct=%d",self->priv->sink,(G_OBJECT(self->priv->sink))->ref_count);
  gst_element_remove_pad(GST_ELEMENT(self),self->priv->sink);

  g_free(self->priv->record_file_name);

  GST_INFO("chaining up");

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
  GST_INFO("done");
}

static void bt_sink_bin_finalize(GObject * const object) {
  const BtSinkBin * const self = BT_SINK_BIN(object);

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sink_bin_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSinkBin * const self = BT_SINK_BIN(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SINK_BIN, BtSinkBinPrivate);

  self->priv->sink=gst_ghost_pad_new_no_target("sink",GST_PAD_SINK);
  gst_element_add_pad(GST_ELEMENT(self),self->priv->sink);
  bt_sink_bin_update(self);

  // watch settings changes
  self->priv->settings=bt_settings_new();
  GST_DEBUG("listen to settings changes (%p)",self->priv->settings);
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

  g_object_class_install_property(gobject_class,SINK_BIN_MODE,
                                  g_param_spec_enum("mode",
                                     "mode prop",
                                     "mode of operation",
                                     BT_TYPE_SINK_BIN_MODE,  /* enum type */
                                     BT_SINK_BIN_MODE_PLAY, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SINK_BIN_RECORD_FORMAT,
                                  g_param_spec_enum("record-format",
                                     "record-format prop",
                                     "format to use when in record mode",
                                     BT_TYPE_SINK_BIN_RECORD_FORMAT,  /* enum type */
                                     BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SINK_BIN_RECORD_FILE_NAME,
                                  g_param_spec_string("record-file-name",
                                     "record-file-name contruct prop",
                                     "the file-name to use for recording",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));
}

GType bt_sink_bin_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtSinkBinClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sink_bin_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSinkBin),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_sink_bin_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GST_TYPE_BIN,"BtSinkBin",&info,0);
  }
  return type;
}

//-- plugin handling

static gboolean bt_sink_bin_plugin_init (GstPlugin * const plugin) {
  //GST_INFO("registering sink_bin plugin");

  gst_element_register(plugin,"bt-sink-bin",GST_RANK_NONE,BT_TYPE_SINK_BIN);
  return TRUE;
}

GST_PLUGIN_DEFINE_STATIC(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "bt-sink-bin",
    "buzztard sink bin - encapsulates play and record functionality",
    bt_sink_bin_plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztard.org"
)
