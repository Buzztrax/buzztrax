/* $Id: song-info.c,v 1.24 2004-10-08 13:50:04 ensonic Exp $
 * class for a machine to machine connection
 */
 
#define BT_CORE
#define BT_SONG_INFO_C

#include <libbtcore/core.h>

enum {
  SONG_INFO_SONG=1,
	SONG_INFO_INFO,
	SONG_INFO_NAME,
	SONG_INFO_GENRE,
  SONG_INFO_BPM,
  SONG_INFO_TPB,
  SONG_INFO_BARS
};

struct _BtSongInfoPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the song-info belongs to */
	BtSong *song;
	
  /* freeform info about the song */
  gchar *info;
  /* the name of the tune */
  gchar *name;
  /* the genre of the tune */
  gchar *genre;
  /* how many beats should be played in a minute */
  glong beats_per_minute;
  /* how many event fire in one fraction of a beat */
  glong ticks_per_beat;
  /* how many bars per beat */
  glong bars;
};

//-- constructor methods

/**
 * bt_song_info_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtSongInfo *bt_song_info_new(const BtSong *song) {
  BtSongInfo *self;

  g_assert(BT_IS_SONG(song));
  
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
    case SONG_INFO_INFO: {
      g_value_set_string(value, self->priv->info);
    } break;
    case SONG_INFO_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case SONG_INFO_GENRE: {
      g_value_set_string(value, self->priv->genre);
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
}

static void bt_song_info_finalize(GObject *object) {
  BtSongInfo *self = BT_SONG_INFO(object);

  GST_DEBUG("!!!! self=%p",self);

	g_free(self->priv->info);
	g_free(self->priv->name);
	g_free(self->priv->genre);
  g_free(self->priv);
}

static void bt_song_info_init(GTypeInstance *instance, gpointer g_class) {
  BtSongInfo *self = BT_SONG_INFO(instance);
	
	//GST_DEBUG("song_info_init self=%p",self);
  self->priv = g_new0(BtSongInfoPrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->name=g_strdup("unamed song");
  // @idea alternate that all a little at new_song
  self->priv->beats_per_minute=125;
  self->priv->ticks_per_beat=4;
  self->priv->bars=16;
}

static void bt_song_info_class_init(BtSongInfoClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
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
}

GType bt_song_info_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongInfoClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_info_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSongInfo),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_info_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSongInfo",&info,0);
  }
  return type;
}

