// $Id: song-info.c,v 1.35 2005-09-19 16:14:06 ensonic Exp $
/**
 * SECTION:btsonginfo
 * @short_description: class that keeps the meta-data for a #BtSong instance
 *
 */ 
 
#define BT_CORE
#define BT_SONG_INFO_C

#include <libbtcore/core.h>

enum {
  SONG_INFO_SONG=1,
  SONG_INFO_FILE_NAME,
  SONG_INFO_INFO,
  SONG_INFO_NAME,
  SONG_INFO_GENRE,
  SONG_INFO_AUTHOR,
  SONG_INFO_BPM,
  SONG_INFO_TPB,
  SONG_INFO_BARS,
  SONG_INFO_CREATE_DTS,
  SONG_INFO_CHANGE_DTS
};

struct _BtSongInfoPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the song the song-info belongs to */
  BtSong *song;

  /* the file name of the song */
  gchar *file_name;
  
  /* freeform info about the song */
  gchar *info;
  /* the name of the tune */
  gchar *name;
  /* the genre of the tune */
  gchar *genre;
  /* the author of the tune */
  gchar *author;
  /* how many beats should be played in a minute */
  gulong beats_per_minute;
  /* how many event fire in one fraction of a beat */
  gulong ticks_per_beat;
  /* how many bars per beat */
  gulong bars;
  /* date stamps */
  gchar *create_dts,*change_dts;
};

static GObjectClass *parent_class=NULL;

// date time stamp format YYYY-MM-DDThh:mm:ssZ
#define DTS_LEN 20

//-- constructor methods

/**
 * bt_song_info_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSongInfo *bt_song_info_new(const BtSong *song) {
  BtSongInfo *self;

  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  
  self=BT_SONG_INFO(g_object_new(BT_TYPE_SONG_INFO,"song",song,NULL));
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_info_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSongInfo *self = BT_SONG_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case SONG_INFO_FILE_NAME: {
      g_value_set_string(value, self->priv->file_name);
    } break;
    case SONG_INFO_INFO: {
      g_value_set_string(value, self->priv->info);
    } break;
    case SONG_INFO_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case SONG_INFO_GENRE: {
      g_value_set_string(value, self->priv->genre);
    } break;
    case SONG_INFO_AUTHOR: {
      g_value_set_string(value, self->priv->author);
    } break;
    case SONG_INFO_BPM: {
      g_value_set_ulong(value, self->priv->beats_per_minute);
    } break;
    case SONG_INFO_TPB: {
      g_value_set_ulong(value, self->priv->ticks_per_beat);
    } break;
    case SONG_INFO_BARS: {
      g_value_set_ulong(value, self->priv->bars);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
    case SONG_INFO_CREATE_DTS: {
      g_value_set_string(value, self->priv->create_dts);
    } break;
    case SONG_INFO_CHANGE_DTS: {
      g_value_set_string(value, self->priv->change_dts);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_song_info_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSongInfo *self = BT_SONG_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {  
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for song-info: %p",self->priv->song);
    } break;
    case SONG_INFO_FILE_NAME: {
      g_free(self->priv->file_name);
      self->priv->file_name = g_value_dup_string(value);
      GST_DEBUG("set the file-name for song_info: %s",self->priv->file_name);
    } break;
    case SONG_INFO_INFO: {
      g_free(self->priv->info);
      self->priv->info = g_value_dup_string(value);
      GST_DEBUG("set the info for song_info: %s",self->priv->info);
    } break;
    case SONG_INFO_NAME: {
      g_free(self->priv->name);
      self->priv->name = g_value_dup_string(value);
      GST_DEBUG("set the name for song_info: %s",self->priv->name);
    } break;
    case SONG_INFO_GENRE: {
      g_free(self->priv->genre);
      self->priv->genre = g_value_dup_string(value);
      GST_DEBUG("set the genre for song_info: %s",self->priv->genre);
    } break;
    case SONG_INFO_AUTHOR: {
      g_free(self->priv->author);
      self->priv->author = g_value_dup_string(value);
      GST_DEBUG("set the author for song_info: %s",self->priv->author);
    } break;
    case SONG_INFO_BPM: {
      self->priv->beats_per_minute = g_value_get_ulong(value);
      GST_DEBUG("set the bpm for song_info: %d",self->priv->beats_per_minute);
    } break;
    case SONG_INFO_TPB: {
      self->priv->ticks_per_beat = g_value_get_ulong(value);
      GST_DEBUG("set the tpb for song_info: %d",self->priv->ticks_per_beat);
    } break;
    case SONG_INFO_BARS: {
      self->priv->bars = g_value_get_ulong(value);
      GST_DEBUG("set the bars for song_info: %d",self->priv->bars);
    } break;
    case SONG_INFO_CREATE_DTS: {
      const gchar *dts = g_value_get_string(value);

      if(dts) {
        if(strlen(dts)==DTS_LEN) {
          strcpy(self->priv->create_dts,dts);
        }
      }
      else {
        time_t now=time(NULL);
        strftime(self->priv->create_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
      }
    } break;
    case SONG_INFO_CHANGE_DTS: {
      const gchar *dts = g_value_get_string(value);

      if(dts) {
        if(strlen(dts)==DTS_LEN) {
          strcpy(self->priv->change_dts,dts);
        }
      }
      else {
        time_t now=time(NULL);
        strftime(self->priv->change_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_info_dispose(GObject *object) {
  BtSongInfo *self = BT_SONG_INFO(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->song);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_song_info_finalize(GObject *object) {
  BtSongInfo *self = BT_SONG_INFO(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->file_name);
  g_free(self->priv->info);
  g_free(self->priv->name);
  g_free(self->priv->genre);
  g_free(self->priv->author);
  g_free(self->priv->create_dts);
  g_free(self->priv->change_dts);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_song_info_init(GTypeInstance *instance, gpointer g_class) {
  BtSongInfo *self = BT_SONG_INFO(instance);
  time_t now=time(NULL);
  
  //GST_DEBUG("song_info_init self=%p",self);
  self->priv = g_new0(BtSongInfoPrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->name=g_strdup("unamed song");
  // @idea alternate bpm's a little at new_song (user defined range?)
  self->priv->beats_per_minute=125;  // 1..1000
  self->priv->ticks_per_beat=4;      // 1..128
  self->priv->bars=4;                // 1..16
  // init dates 'YYYY-MM-DDThh:mm:ssZ'
  self->priv->create_dts=g_new0(gchar,DTS_LEN+1);
  self->priv->change_dts=g_new0(gchar,DTS_LEN+1);
  strftime(self->priv->create_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
  strcpy(self->priv->change_dts,self->priv->create_dts);
  
}

static void bt_song_info_class_init(BtSongInfoClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_song_info_set_property;
  gobject_class->get_property = bt_song_info_get_property;
  gobject_class->dispose      = bt_song_info_dispose;
  gobject_class->finalize     = bt_song_info_finalize;
  
  g_object_class_install_property(gobject_class,SONG_INFO_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "song object, the song-info belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_FILE_NAME,
                                  g_param_spec_string("file-name",
                                     "file name prop",
                                     "songs file name",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_INFO,
                                  g_param_spec_string("info",
                                     "info prop",
                                     "songs freeform info",
                                     "comment me!", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "songs name",
                                     "unnamed", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_GENRE,
                                  g_param_spec_string("genre",
                                     "genre prop",
                                     "songs genre",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_AUTHOR,
                                  g_param_spec_string("author",
                                     "author prop",
                                     "songs author",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_BPM,
                                  g_param_spec_ulong("bpm",
                                     "bpm prop",
                                     "how many beats should be played in a minute",
                                     1,
                                     1000,
                                     125,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_TPB,
                                  g_param_spec_ulong("tpb",
                                     "tpb prop",
                                     "how many event fire in one fraction of a beat",
                                     1,
                                     128,
                                     4,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_BARS,
                                  g_param_spec_ulong("bars",
                                     "bars prop",
                                     "how many bars per beat",
                                     1,
                                     16,
                                     4,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_CREATE_DTS,
                                  g_param_spec_string("create-dts",
                                     "creation dts prop",
                                     "song creation date time stamp",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_CHANGE_DTS,
                                  g_param_spec_string("change-dts",
                                     "changed dts prop",
                                     "song changed date time stamp",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));
}

GType bt_song_info_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSongInfoClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_info_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSongInfo),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_info_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSongInfo",&info,0);
  }
  return type;
}
