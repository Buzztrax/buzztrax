/* $Id: song.c,v 1.23 2004-07-06 15:44:57 ensonic Exp $
 * song 
 *   holds all song related globals
 *
 */

#define BT_CORE
#define BT_SONG_C

#include <libbtcore/core.h>

enum {
  SONG_NAME=1,
	SONG_BIN
};

struct _BtSongPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the name for the song */
  gchar *name;
	
	BtSongInfo* song_info; 
	BtSequence* sequence;
	BtSetup*    setup;

	/* the main gstreamer container element */
	GstElement *bin;
};

//-- methods

/** @todo add playback code from design/gst/play_sequence */

/** 
 * bt_song_start_play:
 * @self: the song that should be played
 *
 * Starts to play the specified song instance from beginning.
 */
void bt_song_start_play(const BtSong *self) {
  /* emitting signal if we start play */
  g_signal_emit(G_OBJECT(self), 
                BT_SONG_GET_CLASS(self)->play_signal_id,
                0);
  /* @todo call bt_sequence_play(); */
  /* how should the player work:
  bt_sequence_play() {
    BTPlayLine *pl;
    foreach(timeline in sequence.rows) {
      //enter new patterns into the playline
      //and stop or mute patterns
      bt_playline_play();
    }
  }
  // see net2.c::play_timeline
  bt_playline_play() {
    for(tick=0...ticks) {
      for(track=0...tracks) {
        // set dparams for pattern 
      }
    }
  }
    
  */
}

//-- wrapper

/**
 * bt_song_get_song_info:
 * @self: the song to get the information from
 *
 * get the #BtSongInfo for the song
 *
 * Returns: the #BtSongInfo instance
 */
BtSongInfo *bt_song_get_song_info(const BtSong *self) {
	return(self->private->song_info);
}

/**
 * bt_song_get_setup:
 * @self: the song to get the setup from
 *
 * get the #BtSetup for the song
 *
 * Returns: the #BtSetup instance
 */
BtSetup *bt_song_get_setup(const BtSong *self) {
	return(self->private->setup);
}

/**
 * bt_song_get_sequence:
 * @self: the song to get the sequence from
 *
 * get the #BtSequence for the song
 *
 * Returns: the #BtSequence instance
 */
BtSequence *bt_song_get_sequence(const BtSong *self) {
	return(self->private->sequence);
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
    case SONG_BIN: {
      g_value_set_object(value, G_OBJECT(self->private->bin));
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
      GST_DEBUG("set the name for song: %s",self->private->name);
    } break;
		case SONG_BIN: {
			self->private->bin = g_object_ref(G_OBJECT(g_value_get_object(value)));
      GST_DEBUG("set the master bin for song: %p",self->private->bin);
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
	
  //GST_DEBUG("song_init self=%p",self);
  self->private = g_new0(BtSongPrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->song_info = BT_SONG_INFO(g_object_new(BT_TYPE_SONG_INFO,"song",self,NULL));
  self->private->sequence  = BT_SEQUENCE(g_object_new(BT_TYPE_SEQUENCE,"song",self,NULL));
  self->private->setup     = BT_SETUP(g_object_new(BT_TYPE_SETUP,"song",self,NULL));
}

static void bt_song_class_init(BtSongClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose      = bt_song_dispose;
  gobject_class->finalize     = bt_song_finalize;
  
  /** 
	 * BtSong::play:
	 *
	 * signals that this song has started to play
	 */
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
  
  g_object_class_install_property(gobject_class,SONG_NAME,
																	g_param_spec_string("name",
                                     "name contsruct prop",
                                     "Set songs name",
                                     "unnamed song", /* default value */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SONG_BIN,
																	g_param_spec_object("bin",
                                     "bin construct prop",
                                     "songs top-level GstElement container",
                                     GST_TYPE_BIN, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
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
		type = g_type_register_static(G_TYPE_OBJECT,"BtSong",&info,0);
  }
  return type;
}

