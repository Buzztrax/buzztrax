/* $Id: song.c,v 1.17 2004-05-11 16:16:38 ensonic Exp $
 * song 
 *   holds all song related globals
 *
 */

#define BT_CORE
#define BT_SONG_C

#include <libbtcore/core.h>

enum {
  SONG_NAME=1
};

struct _BtSongPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the name for the song */
  gchar *name;
	
	BtSongInfo* song_info; 
	BtSequence* sequence;
	BtSetup*    setup;
};

//-- methods

static void bt_song_real_start_play(const BtSong *self) {
  /* emitting signal if we start play */
  g_signal_emit(G_OBJECT(self), 
                BT_SONG_GET_CLASS(self)->play_signal_id,
                0);
}

//-- wrapper

/** 
 * bt_song_start_play:
 * @self: the #BtSong that should be played
 *
 * Starts to play the specified song instance from beginning.
 */
void bt_song_start_play(const BtSong *self) {
  BT_SONG_GET_CLASS(self)->start_play(self);
}

/**
 * bt_song_get_song_info:
 *
 * get the #BtSongInfo for the song
 */
BtSongInfo *bt_song_get_song_info(const BtSong *self) {
	return(self->private->song_info);
}

/**
 * bt_song_get_setup:
 *
 * get the #BtSetup for the song
 */
BtSetup *bt_song_get_setup(const BtSong *self) {
	return(self->private->setup);
}

/**
 * bt_song_get_sequence:
 *
 * get the #BtSequence for the song
 */
BtSequence *bt_song_get_sequence(const BtSong *self) {
	return(self->private->sequence);
}

/**
 * bt_song_get_bin:
 *
 * get the root #GstBin for the song
 *
 * Returns: the root #GstBin to insert #GstElements for this song
 */
GstElement *bt_song_get_bin(const BtSong *self) {
	GST_WARNING("implement me!");
	return(NULL);
}

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSong *self = BT_SONG(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_NAME: {
      g_value_set_string(value, self->private->name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_song_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSong *self = BT_SONG(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_NAME: {
      g_free(self->private->name);
      self->private->name = g_value_dup_string(value);
      GST_INFO("set the name for song: %s",self->private->name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_song_dispose(GObject *object) {
  BtSong *self = BT_SONG(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_song_finalize(GObject *object) {
  BtSong *self = BT_SONG(object);
	g_object_unref(G_OBJECT(self->private->song_info));
	g_free(self->private->name);
  g_free(self->private);
}

static void bt_song_init(GTypeInstance *instance, gpointer g_class) {
  BtSong *self = BT_SONG(instance);
	
	//GST_INFO("song_init self=%p",self);
  self->private = g_new0(BtSongPrivate,1);
  self->private->dispose_has_run = FALSE;
	self->private->song_info = BT_SONG_INFO(g_object_new(BT_SONG_INFO_TYPE,"song",self,NULL));
	self->private->sequence  = BT_SEQUENCE(g_object_new(BT_SEQUENCE_TYPE,"song",self,NULL));
	self->private->setup     = BT_SETUP(g_object_new(BT_SETUP_TYPE,"song",self,NULL));
}

static void bt_song_class_init(BtSongClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose      = bt_song_dispose;
  gobject_class->finalize     = bt_song_finalize;
  
  klass->start_play = bt_song_real_start_play;
  
  /* adding simple signal */
  klass->play_signal_id = g_signal_newv("play",
                                        G_TYPE_FROM_CLASS (gobject_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        NULL, // class closure
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );
  
  g_param_spec = g_param_spec_string("name",
                                     "name contruct prop",
                                     "Set songs name",
                                     "unnamed song", /* default value */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class,SONG_NAME,g_param_spec);
}

/**
 * bt_song_get_type:
 *
 * Returns: the type of #BtSong
 */
GType bt_song_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSong),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSongType",&info,0);
  }
  return type;
}

