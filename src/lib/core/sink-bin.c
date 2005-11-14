// $Id: sink-bin.c,v 1.2 2005-11-14 14:46:35 ensonic Exp $
/**
 * SECTION:btsinkbin
 * @short_description: bin to be used by #BtSinkMachine
 *
 * The sink-bin provides switchable play and record facillities.
 */ 
 
/* @todo detect supported encoders
 * get gst mimetype from the extension
 * and then look at all encoders which supply that mimetype
 * check elements in "codec/encoder/audio", "codec/muxer/audio"
 * build caps using this mimetype
 * gst_element_factory_can_src_caps()
 * problem here is that we need extra option for each encoder (e.g. quality)
 */
 
#define BT_CORE
#define BT_SINK_BIN_C

#include <libbtcore/core.h>
#include <libbtcore/sink-bin.h>

//-- property ids

enum {
  SINK_BIN_MODE=1,
  SINK_BIN_RECORD_FORMAT
};

struct _BtSinkBinPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* mode of operation */
  BtSinkBinMode mode;
  BtSinkBinRecordFormat record_format;
  
  GstPad *sink;
};

static GstBinClass *parent_class=NULL;

//-- enums

GType bt_sink_bin_mode_get_type(void) {
  static GType type = 0;
  if(type==0) {
    static GEnumValue values[] = {
      { BT_SINK_BIN_MODE_PLAY,            "BT_SINK_BIN_MODE_PLAY",            "play the song" },
      { BT_SINK_BIN_MODE_RECORD,          "BT_SINK_BIN_MODE_RECORD",          "record to file" },
      { BT_SINK_BIN_MODE_PLAY_AND_RECORD, "BT_SINK_BIN_MODE_PLAY_AND_RECORD", "play and record together" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSinkBinModeType", values);
  }
  return type;
}

GType bt_sink_bin_record_format_get_type(void) {
  static GType type = 0;
  if(type==0) {
    static GEnumValue values[] = {
      { BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, "BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS", "ogg vorbis" },
      { BT_SINK_BIN_RECORD_FORMAT_MP3,        "BT_SINK_BIN_RECORD_FORMAT_MP3",        "mp3" },
      { BT_SINK_BIN_RECORD_FORMAT_WAV,        "BT_SINK_BIN_RECORD_FORMAT_WAV",        "wav" },
      { BT_SINK_BIN_RECORD_FORMAT_FLAC,       "BT_SINK_BIN_RECORD_FORMAT_FLAC",       "flac" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSinkBinMRecordFormatType", values);
  }
  return type;
}

//-- helper methods

static void bt_sink_bin_clear(self) {
  GstBin *bin=GST_BIN(self);
  GST_LOCK(self);
  while(bin->children) {
    gst_bin_remove (bin, GST_ELEMENT_CAST (bin->children->data));
  }  
  GST_UNLOCK(self);
}

static gboolean bt_sink_bin_add_many(const BtSinkBin *self,GList *list) {
  GList *node;
  
  for(node=list;node;node=node->next) {
    gst_bin_add(GST_BIN(self),GST_ELEMENT(node->data));
  }
  return(TRUE);
}

static void bt_sink_bin_link_many(const BtSinkBin *self,GstElement *last_elem,GList *list) {
  GList *node;
  GstElement *cur_elem;
 
  for(node=list;node;node=node->next) {
    cur_elem=GST_ELEMENT(node->data);
    gst_element_link(last_elem,cur_elem);
    last_elem=cur_elem;
  }
}

static gchar *bt_sink_bin_determine_plugin_name(const BtSettings *settings) {
  gchar *audiosink_name,*system_audiosink_name;
  gchar *plugin_name=NULL;
  
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
    plugin_name=sink_name;
  }
  if (!BT_IS_STRING(plugin_name)) {
    GST_INFO("get audiosink from gst registry by rank");
    // iterate over gstreamer-audiosink list and choose element with highest rank
    GList *node,*audiosink_names=bt_gst_registry_get_element_names_by_class("Sink/Audio");
    GstElementFactory *factory;
    guint max_rank=0,cur_rank;
    
    for(node=audiosink_names;node;node=g_list_next(node)) {
      factory=gst_element_factory_find(node->data);
      cur_rank=gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
      //GST_INFO("  trying audio sink: \"%s\" with rank: %d",node->data,cur_rank);
      if((cur_rank>max_rank) || (!plugin_name)) {
        plugin_name=g_strdup(node->data);
        max_rank=cur_rank;
      }
    }
    g_list_free(audiosink_names);
  }
  GST_INFO("using audio sink : \"%s\"",plugin_name);

  g_free(system_audiosink_name);
  g_free(audiosink_name);
  return(plugin_name);
}

static GList *bt_sink_bin_get_player_elements(const BtSinkBin *self) {
  GList *list=NULL;
  BtSettings *settings=NULL;
  gchar *plugin_name;

  // @todo: get settings
  plugin_name=bt_sink_bin_determine_plugin_name(settings);
  list=g_list_append(list,gst_element_factory_make("player",plugin_name));
  g_free(plugin_name);
  
  return(list);
}

static GList *bt_sink_bin_get_recorder_elements(const BtSinkBin *self) {
  GList *list=NULL;

  // @todo: generate recorder elements
  switch(self->priv->record_format) {
    case BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS:
      // oggmux ! vorbis ! filesink location="song.ogg"
      break;
    case BT_SINK_BIN_RECORD_FORMAT_MP3:
      // lame ! filesink location="song.mp3"
      break;
    case BT_SINK_BIN_RECORD_FORMAT_WAV:
      // wavenc ! filesink location="song.wav"
      break;
    case BT_SINK_BIN_RECORD_FORMAT_FLAC:
      // flacenc ! filesink location="song.flac"
      break;
  }
  // @todo: create filesink, we need the filename here
  list=g_list_append(list,gst_element_factory_make("filesink","filesink"));
  return(list);
}

static gboolean bt_sink_bin_change(const BtSinkBin *self) {
  GList *list;
  GstElement *first_elem=NULL;
  GstPad *sink_pad;
  gchar *name;

  // remove existing children
  bt_sink_bin_clear(self);

  // add new children
  switch(self->priv->mode) {
    case BT_SINK_BIN_MODE_PLAY:
      list=bt_sink_bin_get_player_elements(self);
      bt_sink_bin_add_many(self,list);
      first_elem=GST_ELEMENT(list->data);
      bt_sink_bin_link_many(self,first_elem,list->next);
      g_list_free(list);
      break;
    case BT_SINK_BIN_MODE_RECORD:
      list=bt_sink_bin_get_recorder_elements(self);
      bt_sink_bin_add_many(self,list);
      first_elem=GST_ELEMENT(list->data);
      bt_sink_bin_link_many(self,first_elem,list->next);
      g_list_free(list);
      break;
    case BT_SINK_BIN_MODE_PLAY_AND_RECORD:
      // add a tee element
      name=g_strdup_printf("tee_%p",self);
      first_elem=gst_element_factory_make("tee",name);
      g_free(name);
      gst_bin_add(GST_BIN(self),first_elem);
      // add player elems
      list=bt_sink_bin_get_player_elements(self);
      bt_sink_bin_add_many(self,list);
      bt_sink_bin_link_many(self,first_elem,list);
      g_list_free(list);
      // add recorder elems
      list=bt_sink_bin_get_recorder_elements(self);
      bt_sink_bin_add_many(self,list);
      bt_sink_bin_link_many(self,first_elem,list);
      g_list_free(list);
      break;
    default:
      g_assert_not_reached();
  }
  // set new ghostpad-target
  sink_pad=gst_element_get_pad(first_elem,"Sink");
  gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink),sink_pad);
  gst_object_unref(sink_pad);
  
  return(TRUE);
}

//-- constructor methods

/**
 * bt_sink_bin_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkBin *bt_sink_bin_new(void) {
  BtSinkBin *self=NULL;

  if(!(self=BT_SINK_BIN(g_object_new(BT_TYPE_SINK_BIN,NULL)))) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_sink_bin_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSinkBin *self = BT_SINK_BIN(object);
  return_if_disposed();
  switch (property_id) {
    case SINK_BIN_MODE: {
      g_value_set_enum(value, self->priv->mode);
    } break;
    case SINK_BIN_RECORD_FORMAT: {
      g_value_set_enum(value, self->priv->record_format);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_sink_bin_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSinkBin *self = BT_SINK_BIN(object);
  return_if_disposed();
  switch (property_id) {
    case SINK_BIN_MODE: {
      self->priv->mode=g_value_get_enum(value);
      bt_sink_bin_change(self);
    } break;
    case SINK_BIN_RECORD_FORMAT: {
      self->priv->record_format=g_value_get_enum(value);
      bt_sink_bin_change(self);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sink_bin_dispose(GObject *object) {
  BtSinkBin *self = BT_SINK_BIN(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_sink_bin_finalize(GObject *object) {
  BtSinkBin *self = BT_SINK_BIN(object);

  GST_DEBUG("!!!! self=%p",self);
  
  gst_object_unref(self->priv->sink);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sink_bin_init(GTypeInstance *instance, gpointer g_class) {
  BtSinkBin *self = BT_SINK_BIN(instance);
  self->priv = g_new0(BtSinkBinPrivate,1);
  self->priv->dispose_has_run = FALSE;
  
  self->priv->sink=gst_ghost_pad_new_notarget("Sink",GST_PAD_SINK);
  gst_element_add_pad(GST_ELEMENT(self),self->priv->sink);
}

static void bt_sink_bin_class_init(BtSinkBinClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GST_TYPE_BIN);
  
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
                                     "ecord-format prop",
                                     "format to use when in record mode",
                                     BT_TYPE_SINK_BIN_RECORD_FORMAT,  /* enum type */
                                     BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, /* default value */
                                     G_PARAM_READWRITE));

}

GType bt_sink_bin_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
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

static gboolean bt_sink_bin_plugin_init (GstPlugin * plugin) {
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
