/* $Id: song.c,v 1.49 2004-10-01 16:01:46 ensonic Exp $
 * song 
 *   holds all song related globals
 *
 */

#define BT_CORE
#define BT_SONG_C

#include <libbtcore/core.h>

//-- signal ids

enum {
  PLAY_EVENT,
  STOP_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
	SONG_BIN=1,
  SONG_MASTER,
  SONG_SONG_INFO,
  SONG_SEQUENCE,
  SONG_SETUP
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

static guint signals[LAST_SIGNAL]={0,};

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
	
  g_assert(bin);
  
  self=BT_SONG(g_object_new(BT_TYPE_SONG,"bin",bin,NULL));
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

  g_assert(self);

  // emit signal that we start playing
  g_signal_emit(G_OBJECT(self), signals[PLAY_EVENT], 0);
  res=bt_sequence_play(self->priv->sequence);
  // emit signal that we have finished playing
  g_signal_emit(G_OBJECT(self), signals[STOP_EVENT], 0);
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

  g_assert(self);
  
  res=bt_sequence_stop(self->priv->sequence);
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
  g_assert(self);
  // @todo remember play position
  return(gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PAUSED)!=GST_STATE_FAILURE);
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
  g_assert(self);
  // @todo reuse play position
  return(gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PLAYING)!=GST_STATE_FAILURE);
}

//-- wrapper

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
      g_value_set_object(value, self->priv->bin);
    } break;
    case SONG_MASTER: {
      g_value_set_object(value, self->priv->master);
    } break;
    case SONG_SONG_INFO: {
      g_value_set_object(value, self->priv->song_info);
    } break;
    case SONG_SEQUENCE: {
      g_value_set_object(value, self->priv->sequence);
    } break;
    case SONG_SETUP: {
      g_value_set_object(value, self->priv->setup);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
      g_object_try_unref(self->priv->bin);
			self->priv->bin = GST_BIN(g_object_try_ref(g_value_get_object(value)));
      GST_DEBUG("set the bin for the song: %p",self->priv->bin);
		} break;
		case SONG_MASTER: {
      g_object_try_weak_unref(self->priv->master);
			self->priv->master = GST_ELEMENT(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->master);
      GST_DEBUG("set the master for the song: %p",self->priv->master);
		} break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_dispose(GObject *object) {
  BtSong *self = BT_SONG(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_weak_unref(self->priv->master);
	g_object_try_unref(self->priv->song_info);
	g_object_try_unref(self->priv->sequence);
	g_object_try_unref(self->priv->setup);
	g_object_try_unref(self->priv->bin);
}

static void bt_song_finalize(GObject *object) {
  BtSong *self = BT_SONG(object);
  
  GST_DEBUG("!!!! self=%p",self);
  
  g_free(self->priv);
}

static void bt_song_init(GTypeInstance *instance, gpointer g_class) {
  BtSong *self = BT_SONG(instance);
	
  GST_DEBUG("song_init self=%p",self);
  self->priv = g_new0(BtSongPrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->song_info = bt_song_info_new(self);
  self->priv->sequence  = bt_sequence_new(self);
  self->priv->setup     = bt_setup_new(self);
}

static void bt_song_class_init(BtSongClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose      = bt_song_dispose;
  gobject_class->finalize     = bt_song_finalize;

  klass->play_event = NULL;
  klass->stop_event = NULL;
  /** 
	 * BtSong::play:
   * @self: the song object that emitted the signal
	 *
	 * signals that this song has started to play
	 */
  signals[PLAY_EVENT] = g_signal_new("play",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_STRUCT_OFFSET(BtSongClass,play_event),
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
  signals[STOP_EVENT] = g_signal_new("stop",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_STRUCT_OFFSET(BtSongClass,stop_event),
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

  g_object_class_install_property(gobject_class,SONG_SONG_INFO,
																	g_param_spec_object("song-info",
                                     "song-info prop",
                                     "songs metadata sub object",
                                     BT_TYPE_SONG_INFO, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,SONG_SEQUENCE,
																	g_param_spec_object("sequence",
                                     "sequence prop",
                                     "songs sequence sub object",
                                     BT_TYPE_SEQUENCE, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,SONG_SETUP,
																	g_param_spec_object("setup",
                                     "setup prop",
                                     "songs setup sub object",
                                     BT_TYPE_SETUP, /* object type */
                                     G_PARAM_READABLE));
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

