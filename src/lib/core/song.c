/* $Id: song.c,v 1.37 2004-08-18 16:55:08 ensonic Exp $
 * song 
 *   holds all song related globals
 *
 */

#define BT_CORE
#define BT_SONG_C

#include <libbtcore/core.h>

enum {
	SONG_BIN=1,
  SONG_MASTER
};

struct _BtSongPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
	BtSongInfo* song_info; 
	BtSequence* sequence;
	BtSetup*    setup;

	/* the main gstreamer container element */
	GstBin *bin;
  /* the element that has the clock */
  GstElement *master;
};

//-- constructor methods

/**
 * bt_song_new:
 * @bin: the gst root element to hold the song. 
 *
 * Create a new instance. The bin object can be retrieved from the bin property
 * of an #BtApplication instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtSong *bt_song_new(const GstBin *bin) {
  BtSong *self=NULL;
  if(bin) {
    self=BT_SONG(g_object_new(BT_TYPE_SONG,"bin",bin,NULL));
  }
  return(self);
}

//-- methods

/** 
 * bt_song_play:
 * @self: the song that should be played
 *
 * Starts to play the specified song instance from beginning.
 * This methods emits the "play" signal.
 *
 * Returns: TRUE for success
 *
 */
gboolean bt_song_play(const BtSong *self) {
  gboolean res;
  // emit signal that we start playing
  g_signal_emit(G_OBJECT(self), BT_SONG_GET_CLASS(self)->play_signal_id, 0);
  res=bt_sequence_play(self->private->sequence);
  // emit signal that we have finished playing
  g_signal_emit(G_OBJECT(self), BT_SONG_GET_CLASS(self)->stop_signal_id, 0);
  return(res);
}

/**
 * bt_song_stop:
 * @self: the song that should be stopped
 *
 * Stops the playback of the specified song instance.
 *
 * Returns: TRUE for success
 *
 */
gboolean bt_song_stop(const BtSong *self) {
  gboolean res;
  
  res=bt_sequence_stop(self->private->sequence);
  // emit signal that we have finished playing
  g_signal_emit(G_OBJECT(self), BT_SONG_GET_CLASS(self)->stop_signal_id, 0);
  return(res);
}

/**
 * bt_song_pause:
 * @self: the song that should be paused
 *
 * Pauses the playback of the specified song instance
 *
 * Returns: TRUE for success
 *
 */
gboolean bt_song_pause(const BtSong *self) {
  // @todo remember play position
  return(gst_element_set_state(GST_ELEMENT(self->private->bin),GST_STATE_PAUSED)!=GST_STATE_FAILURE);
}

/**
 * bt_song_continue:
 * @self: the song that should be paused
 *
 * Continues the playback of the specified song instance
 *
 * Returns: TRUE for success
 *
 */
gboolean bt_song_continue(const BtSong *self) {
  // @todo reuse play position
  return(gst_element_set_state(GST_ELEMENT(self->private->bin),GST_STATE_PLAYING)!=GST_STATE_FAILURE);
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
    case SONG_BIN: {
      g_value_set_object(value, G_OBJECT(self->private->bin));
    } break;
    case SONG_MASTER: {
      g_value_set_object(value, G_OBJECT(self->private->master));
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
		case SONG_BIN: {
			self->private->bin = g_object_ref(G_OBJECT(g_value_get_object(value)));
      GST_DEBUG("set the bin for the song: %p",self->private->bin);
		} break;
		case SONG_MASTER: {
			self->private->master = g_object_ref(G_OBJECT(g_value_get_object(value)));
      GST_DEBUG("set the master for the song: %p",self->private->master);
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
	g_object_unref(G_OBJECT(self->private->sequence));
	g_object_unref(G_OBJECT(self->private->setup));
  gst_object_unref(GST_OBJECT(self->private->master));
	gst_object_unref(GST_OBJECT(self->private->bin));
  g_free(self->private);
}

static void bt_song_init(GTypeInstance *instance, gpointer g_class) {
  BtSong *self = BT_SONG(instance);
	
  GST_DEBUG("song_init self=%p",self);
  self->private = g_new0(BtSongPrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->song_info = bt_song_info_new(self);
  self->private->sequence  = bt_sequence_new(self);
  self->private->setup     = bt_setup_new(self);
}

static void bt_song_class_init(BtSongClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose      = bt_song_dispose;
  gobject_class->finalize     = bt_song_finalize;
  
  /** 
	 * BtSong::play:
   * @self: the song object that emitted the signal
	 *
	 * signals that this song has started to play
	 */
  klass->play_signal_id = g_signal_newv("play",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        NULL, // class closure
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );
  
  /** 
	 * BtSong::stop:
   * @self: the song object that emitted the signal
	 *
	 * signals that this song has finished to play
	 */
  klass->stop_signal_id = g_signal_newv("stop",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        NULL, // class closure
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );

  g_object_class_install_property(gobject_class,SONG_BIN,
																	g_param_spec_object("bin",
                                     "bin construct prop",
                                     "songs top-level GstElement container",
                                     GST_TYPE_BIN, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_MASTER,
																	g_param_spec_object("master",
                                     "master prop",
                                     "songs clocking GstElement",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READWRITE));
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

