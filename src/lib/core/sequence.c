/* $Id: sequence.c,v 1.25 2004-09-15 16:57:58 ensonic Exp $
 * class for the pattern sequence
 */
 
#define BT_CORE
#define BT_SEQUENCE_C

#include <libbtcore/core.h>

enum {
  SEQUENCE_SONG=1,
  SEQUENCE_LENGTH,
  SEQUENCE_TRACKS
};

struct _BtSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the sequence belongs to */
	BtSong *song;

  /* the number of timeline entries */
  glong length;

  /* the number of tracks */
  glong tracks;

  /* <tracks> machine entries that are the heading of the sequence */
  BtMachine **machines;
  
  /* <length> timeline entries that form the sequence */
  BtTimeLine **timelines;

  /* flag to externally abort playing */
  GMutex *is_playing_mutex;
  gboolean is_playing;
};

//-- helper methods

/**
 * bt_sequence_unref_timelines:
 * @self: the sequence that holds the timelines
 *
 * Unref the old timelines when we create/load a new song
 */

static void bt_sequence_unref_timelines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_unref_timelines");
  if(self->private->timelines && self->private->length) {
    glong i;
    for(i=0;i<self->private->length;i++) {
      g_object_try_unref(self->private->timelines[i]);
    }
    self->private->length=0;
  }
}
  
/**
 * bt_sequence_free_timelines:
 * @self: the sequence that holds the timelines
 *
 * Freeing old timelines when we create/load a new song
 */
static void bt_sequence_free_timelines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_free_timelines");
  if(self->private->timelines) {
    g_free(self->private->timelines);
    self->private->timelines=NULL;
  }
}

/**
 * bt_sequence_init_timelines:
 * @self: the sequence that holds the timelines
 *
 * Prepare new timelines when we create/load a new song
 */
static void bt_sequence_init_timelines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_init_timelines");
  if(self->private->length) {
    self->private->timelines=g_new(BtTimeLine*,self->private->length);
    if(self->private->timelines) {
      glong i;
      for(i=0;i<self->private->length;i++) {
        self->private->timelines[i]=bt_timeline_new(self->private->song);
      }
    }
  }
}

/**
 * bt_sequence_init_timelinetracks:
 * @self: the sequence that holds the timelinetracks
 *
 * Initialize the timelinetracks when we create/load a new song
 */
static void bt_sequence_init_timelinetracks(const BtSequence *self) {
  GST_DEBUG("bt_sequence_init_timelinetracks");
  if(self->private->timelines) {
    if(self->private->length && self->private->tracks) {
      glong i;

      for(i=0;i<self->private->length;i++) {
        bt_g_object_set_long_property(G_OBJECT(self->private->timelines[i]),"tracks",self->private->tracks);
      }
    }
  }
}

/**
 * bt_sequence_init_machines:
 * @self: the sequence that holds the machines-list
 *
 * Initialize the machines-list when we create/load a new song
 */
static void bt_sequence_init_machines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_init_machines");
  if(self->private->tracks) {
    self->private->machines=g_new0(BtMachine*,self->private->tracks);
  }
}

//-- constructor methods

/**
 * bt_sequence_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtSequence *bt_sequence_new(const BtSong *song) {
  BtSequence *self;
  self=BT_SEQUENCE(g_object_new(BT_TYPE_SEQUENCE,"song",song,NULL));
  if(self) {
    bt_sequence_init_timelines(self);
  }
  return(self);
}

//-- methods

/**
 * bt_sequence_get_timeline_by_time:
 * @self: the #BtSequence that holds the #BtTimeLine objects
 * @time: the requested index
 *
 * fetches the required timeline.
 *
 * Returns: the #BtTimeLine pointer or NULL in case of an error
 */
BtTimeLine *bt_sequence_get_timeline_by_time(const BtSequence *self,const glong time) {
  if(time<self->private->length) {
    return(self->private->timelines[time]);
  }
  else {
    GST_ERROR("index out of bounds, %d should be < %d",time,self->private->length);
  }
  return(NULL);
}

/**
 * bt_sequence_get_machine_by_track:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 *
 * fetches the required #BtMachine.
 *
 * Returns: the #BtMachine pointer or NULL in case of an error
 */
BtMachine *bt_sequence_get_machine_by_track(const BtSequence *self,const glong track) {
  if(track<self->private->tracks) {
    return(self->private->machines[track]);
  }
  else {
    GST_ERROR("index out of bounds, %d should be < %d",time,self->private->tracks);
  }
  return(NULL);
}

/**
 * bt_sequence_set_machine_by_track:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 * @machine: the #BtMachine
 *
 * sets #BtMachine for the respective track.
 */
void bt_sequence_set_machine_by_track(const BtSequence *self,const glong track,const BtMachine *machine) {

  g_assert(machine);

  // @todo shouldn't we better make self->private->tracks a readonly property and offer methods to insert/remove tracks
  // as it should not be allowed to change the machine later on
  
  if(track<self->private->tracks) {
    if(!self->private->machines[track]) {
      self->private->machines[track]=machine;
    }
    else {
      GST_ERROR("machine has already be set!");
    }
  }
  else {
    GST_ERROR("index out of bounds, %d should be < %d",time,self->private->tracks);
  }
}

/**
 * bt_sequence_get_bar_time:
 * @self: the #BtSequence of the song
 *
 * calculates the length of one sequence bar in milliseconds
 *
 * Returns: the length of one sequence bar in milliseconds
 *
 */
gulong bt_sequence_get_bar_time(const BtSequence *self) {
  BtSongInfo *song_info=bt_song_get_song_info(self->private->song);
  glong wait_per_position,bars;
  glong beats_per_minute,ticks_per_beat;
  gdouble ticks_per_minute;
  gulong res;
  
  ticks_per_beat=bt_g_object_get_long_property(G_OBJECT(song_info),"tpb");
  beats_per_minute=bt_g_object_get_long_property(G_OBJECT(song_info),"bpm");
  bars=bt_g_object_get_long_property(G_OBJECT(song_info),"bars");

  ticks_per_minute=(gdouble)beats_per_minute*(gdouble)ticks_per_beat;
  wait_per_position=(glong)((GST_SECOND*60.0)/(gdouble)ticks_per_minute);

  res=(gulong)(((guint64)bars*(guint64)wait_per_position)/G_USEC_PER_SEC);
  return(res);
}

/**
 * bt_sequence_get_loop_time:
 * @self: the #BtSequence of the song
 *
 * calculates the length of the song loop in milliseconds
 *
 * Returns: the length of the song loop in milliseconds
 *
 */
gulong bt_sequence_get_loop_time(const BtSequence *self) {
  gulong res;

  res=(gulong)(self->private->length*bt_sequence_get_bar_time(self));
  return(res);
}

/**
 * bt_sequence_play:
 * @self: the #BtSequence that should be played
 *
 * starts playback for the sequence
 *
 * Returns: TRUE for success
 *
 */
gboolean bt_sequence_play(const BtSequence *self) {
  glong i;
  BtSongInfo *song_info=bt_song_get_song_info(self->private->song);
  BtTimeLine **timeline=self->private->timelines;
  GstElement *master=GST_ELEMENT(bt_g_object_get_object_property(G_OBJECT(self->private->song),"master"));
  GstElement *bin=GST_ELEMENT(bt_g_object_get_object_property(G_OBJECT(self->private->song),"bin"));
  BtPlayLine *playline;
  glong wait_per_position,bars;
  glong beats_per_minute,ticks_per_beat;
  gdouble ticks_per_minute;
  gboolean res=TRUE;
  time_t tm1,tm2;
  
  if(!self->private->tracks) return(res);
  if(!self->private->length) return(res);
  
  ticks_per_beat=bt_g_object_get_long_property(G_OBJECT(song_info),"tpb");
  beats_per_minute=bt_g_object_get_long_property(G_OBJECT(song_info),"bpm");
  bars=bt_g_object_get_long_property(G_OBJECT(song_info),"bars");
  /* the number of pattern-events for one playline-step,
   * when using 4 ticks_per_beat then
   * for 4/4 bars it is 16 (standart dance rhythm)
   * for 3/4 bars it is 12 (walz)
   */
  //ticks_per_minute=((gdouble)beats_per_minute*(gdouble)ticks_per_beat)/(gdouble)bars;
  ticks_per_minute=(gdouble)beats_per_minute*(gdouble)ticks_per_beat;
  wait_per_position=(glong)((GST_SECOND*60.0)/(gdouble)ticks_per_minute);
  playline=bt_playline_new(self->private->song,master,self->private->tracks,bars,wait_per_position);
  
  GST_INFO("pattern.duration = %d * %d usec = %ld sec",bars,wait_per_position,(gulong)(((guint64)bars*(guint64)wait_per_position)/GST_SECOND));
  GST_INFO("song.duration = %d * %d * %d usec = %ld sec",self->private->length,bars,wait_per_position,(gulong)(((guint64)self->private->length*(guint64)bars*(guint64)wait_per_position)/GST_SECOND));

  if(gst_element_set_state(bin,GST_STATE_PLAYING)!=GST_STATE_FAILURE) {
    g_mutex_lock(self->private->is_playing_mutex);
    self->private->is_playing=TRUE;
    g_mutex_unlock(self->private->is_playing_mutex);
    // DEBUG {
    tm1=time(NULL);
    // }
    for(i=0;((i<self->private->length) && (self->private->is_playing));i++,timeline++) {
      // DEBUG {
      tm2=time(NULL);
      GST_INFO("Playing sequence : position = %d, time elapsed = %lf sec",i,difftime(tm2,tm1));
      // }
      // emit a tick signal
      g_signal_emit(G_OBJECT(self), BT_SEQUENCE_GET_CLASS(self)->tick_signal_id, 0, i);
      // enter new patterns into the playline and stop or mute patterns
      bt_timeline_update_playline(*timeline,playline);
      // play the patterns in the cursor
      bt_playline_play(playline);
    }
    // emit a tick signal
    g_signal_emit(G_OBJECT(self), BT_SEQUENCE_GET_CLASS(self)->tick_signal_id, 0, i);
    if(gst_element_set_state(bin,GST_STATE_NULL)==GST_STATE_FAILURE) {
      GST_ERROR("can't stop playing");res=FALSE;
    }
  }
  else {
    GST_ERROR("can't start playing");res=FALSE;
  }
  g_object_unref(playline); 
  return(res);
}

/**
 * bt_sequence_stop:
 * @self: the #BtSequence that should be stopped
 *
 * stops playback for the sequence
 *
 * Returns: TRUE for success
 *
 */
gboolean bt_sequence_stop(const BtSequence *self) {
  g_mutex_lock(self->private->is_playing_mutex);
  self->private->is_playing=FALSE;
  g_mutex_unlock(self->private->is_playing_mutex);

  return(TRUE);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_sequence_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSequence *self = BT_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    case SEQUENCE_LENGTH: {
      g_value_set_long(value, self->private->length);
    } break;
    case SEQUENCE_TRACKS: {
      g_value_set_long(value, self->private->tracks);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_sequence_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSequence *self = BT_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_SONG: {
      g_object_try_unref(self->private->song);
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the song for sequence: %p",self->private->song);
    } break;
    case SEQUENCE_LENGTH: {
      bt_sequence_unref_timelines(self);
      bt_sequence_free_timelines(self);
      self->private->length = g_value_get_long(value);
      GST_DEBUG("set the length for sequence: %d",self->private->length);
      bt_sequence_init_timelines(self);
    } break;
    case SEQUENCE_TRACKS: {
      self->private->tracks = g_value_get_long(value);
      GST_DEBUG("set the tracks for sequence: %d",self->private->tracks);
      bt_sequence_init_machines(self);
      bt_sequence_init_timelinetracks(self);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_sequence_dispose(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
	g_object_try_unref(self->private->song);
  bt_sequence_unref_timelines(self);
}

static void bt_sequence_finalize(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->private->machines);
  g_free(self->private);
  g_free(self->private->timelines);
}

static void bt_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtSequence *self = BT_SEQUENCE(instance);
  self->private = g_new0(BtSequencePrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->length=1;
  self->private->is_playing_mutex=g_mutex_new();

}

static void bt_sequence_class_init(BtSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_sequence_set_property;
  gobject_class->get_property = bt_sequence_get_property;
  gobject_class->dispose      = bt_sequence_dispose;
  gobject_class->finalize     = bt_sequence_finalize;

  /** 
	 * BtSequence::tick:
   * @self: the sequence object that emitted the signal
   * @pos: the postion in the sequence
	 *
	 * This sequence has reached a new play position (with the number of ticks
   * determined by the bars-property) during playback
	 */
  klass->tick_signal_id = g_signal_new("tick",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        NULL, // class closure
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__LONG,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        G_TYPE_LONG // param data
                                        );

  g_object_class_install_property(gobject_class,SEQUENCE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the sequence belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LENGTH,
																	g_param_spec_long("length",
                                     "length prop",
                                     "length of the sequence in timeline bars",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_TRACKS,
																	g_param_spec_long("tracks",
                                     "tracks prop",
                                     "number of tracks in the sequence",
                                     0,
                                     G_MAXLONG,
                                     0,
                                     G_PARAM_READWRITE));

}

GType bt_sequence_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSequence),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_sequence_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSequence",&info,0);
  }
  return type;
}

