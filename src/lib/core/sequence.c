/* $Id: sequence.c,v 1.39 2004-11-03 09:35:16 ensonic Exp $
 * class for the pattern sequence
 */
 
#define BT_CORE
#define BT_SEQUENCE_C

#include <libbtcore/core.h>

//-- signal ids

enum {
  TICK,
  LAST_SIGNAL
};

//-- property ids

enum {
  SEQUENCE_SONG=1,
  SEQUENCE_LENGTH,
  SEQUENCE_TRACKS,
  SEQUENCE_LOOP,
  SEQUENCE_LOOP_START,
  SEQUENCE_LOOP_END
};

struct _BtSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the sequence belongs to */
	BtSong *song;

  /* the number of timeline entries */
  gulong length;

  /* the number of tracks */
  gulong tracks;

  /* loop mode on ? */
  gboolean loop;
  
  /* the timeline entries where the loop starts and ends
   * -1 for both mean to use 0...length
   */
  glong loop_start,loop_end;

  /* <tracks> machine entries that are the heading of the sequence */
  BtMachine **machines;
  
  /* <length> timeline entries that form the sequence */
  BtTimeLine **timelines;

  /* flag to externally abort playing */
  GMutex *is_playing_mutex;
  gboolean is_playing;
};

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- helper methods

/**
 * bt_sequence_unref_timelines:
 * @self: the sequence that holds the timelines
 *
 * Unref the old timelines when we create/load a new song
 */

static void bt_sequence_unref_timelines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_unref_timelines");
  if(self->priv->timelines && self->priv->length) {
    glong i;
    for(i=0;i<self->priv->length;i++) {
      g_object_try_unref(self->priv->timelines[i]);
    }
    self->priv->length=0;
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
  if(self->priv->timelines) {
    g_free(self->priv->timelines);
    self->priv->timelines=NULL;
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
  if(self->priv->length) {
    self->priv->timelines=g_new(BtTimeLine*,self->priv->length);
    if(self->priv->timelines) {
      glong i;
      for(i=0;i<self->priv->length;i++) {
        self->priv->timelines[i]=bt_timeline_new(self->priv->song);
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
  if(self->priv->timelines) {
    if(self->priv->length && self->priv->tracks) {
      glong i;

      for(i=0;i<self->priv->length;i++) {
        g_object_set(G_OBJECT(self->priv->timelines[i]),"tracks",self->priv->tracks,NULL);
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
  if(self->priv->tracks) {
    self->priv->machines=g_new0(BtMachine*,self->priv->tracks);
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
  
  g_assert(BT_IS_SONG(song));
  
  self=BT_SEQUENCE(g_object_new(BT_TYPE_SEQUENCE,"song",song,NULL));
  if(self) {
    // @todo check result
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
  g_assert(BT_IS_SEQUENCE(self));

  if(time<self->priv->length) {
    return(self->priv->timelines[time]);
  }
  else {
    GST_ERROR("index out of bounds, %d should be < %d",time,self->priv->length);
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
  g_assert(BT_IS_SEQUENCE(self));

  if(track<self->priv->tracks) {
    return(self->priv->machines[track]);
  }
  else {
    GST_ERROR("index out of bounds, %d should be < %d",track,self->priv->tracks);
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

  g_assert(BT_IS_SEQUENCE(self));
  g_assert(BT_IS_MACHINE(machine));

  // @todo shouldn't we better make self->priv->tracks a readonly property and offer methods to insert/remove tracks
  // as it should not be allowed to change the machine later on
  
  if(track<self->priv->tracks) {
    if(!self->priv->machines[track]) {
      // @todo ref the machine
      self->priv->machines[track]=(BtMachine *)machine;
    }
    else {
      GST_ERROR("machine has already be set!");
    }
  }
  else {
    GST_ERROR("index out of bounds, %d should be < %d",track,self->priv->tracks);
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
  BtSongInfo *song_info;
  glong wait_per_position,bars;
  glong beats_per_minute,ticks_per_beat;
  gdouble ticks_per_minute;
  gulong res;

  g_assert(BT_IS_SEQUENCE(self));

  g_object_get(G_OBJECT(self->priv->song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"tpb",&ticks_per_beat,"bpm",&beats_per_minute,"bars",&bars,NULL);

  ticks_per_minute=(gdouble)beats_per_minute*(gdouble)ticks_per_beat;
  wait_per_position=(glong)((GST_SECOND*60.0)/(gdouble)ticks_per_minute);

  res=(gulong)(((guint64)bars*(guint64)wait_per_position)/G_USEC_PER_SEC);
  // release the references
  g_object_try_unref(song_info);
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

  g_assert(BT_IS_SEQUENCE(self));

  res=(gulong)(self->priv->length*bt_sequence_get_bar_time(self));
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
  gboolean res=TRUE;
  
  g_assert(BT_IS_SEQUENCE(self));

  if((!self->priv->tracks) || (!self->priv->length)) return(res);
  else {
    BtTimeLine **timeline;
    BtSongInfo *song_info;
    GstElement *bin;
    BtPlayLine *playline;
    glong i;
    glong wait_per_position,bars;
    glong beats_per_minute,ticks_per_beat;
    gdouble ticks_per_minute;
    // DEBUG {
    GTimer *timer;
    // }
    
    g_object_get(G_OBJECT(self->priv->song),"bin",&bin,"song-info",&song_info,NULL);
    g_object_get(G_OBJECT(song_info),"tpb",&ticks_per_beat,"bpm",&beats_per_minute,"bars",&bars,NULL);
    /* the number of pattern-events for one playline-step,
     * when using 4 ticks_per_beat then
     * for 4/4 bars it is 16 (standart dance rhythm)
     * for 3/4 bars it is 12 (walz)
     */
    //ticks_per_minute=((gdouble)beats_per_minute*(gdouble)ticks_per_beat)/(gdouble)bars;
    ticks_per_minute=(gdouble)beats_per_minute*(gdouble)ticks_per_beat;
    wait_per_position=(glong)((GST_SECOND*60.0)/(gdouble)ticks_per_minute);
    playline=bt_playline_new(self->priv->song,self->priv->tracks,bars,wait_per_position);
    
    GST_INFO("pattern.duration = %d * %d usec = %ld sec",bars,wait_per_position,(gulong)(((guint64)bars*(guint64)wait_per_position)/GST_SECOND));
    GST_INFO("song.duration = %d * %d * %d usec = %ld sec",self->priv->length,bars,wait_per_position,(gulong)(((guint64)self->priv->length*(guint64)bars*(guint64)wait_per_position)/GST_SECOND));
  
    if(gst_element_set_state(bin,GST_STATE_PLAYING)!=GST_STATE_FAILURE) {
      g_mutex_lock(self->priv->is_playing_mutex);
      self->priv->is_playing=TRUE;
      g_mutex_unlock(self->priv->is_playing_mutex);
      // DEBUG {
      timer=g_timer_new();
      g_timer_start(timer);
      // }
      do {
        // @todo handle loop-start/end
        timeline=self->priv->timelines;
        for(i=0;((i<self->priv->length) && (self->priv->is_playing));i++,timeline++) {
          // DEBUG {
          GST_INFO("Playing sequence : position = %d, time elapsed = %lf sec",i,g_timer_elapsed(timer,NULL));
          // }
          // emit a tick signal
          g_signal_emit(G_OBJECT(self), signals[TICK], 0, i);
          // enter new patterns into the playline and stop or mute patterns
          bt_timeline_update_playline(*timeline,playline);
          // play the patterns in the cursor
          bt_playline_play(playline);
        }
      } while(self->priv->loop);
      // DEBUG {
      g_timer_destroy(timer);
      // }
      // emit a tick signal
      g_signal_emit(G_OBJECT(self), signals[TICK], 0, i);
      if(gst_element_set_state(bin,GST_STATE_NULL)==GST_STATE_FAILURE) {
        GST_ERROR("can't stop playing");res=FALSE;
      }
    }
    else {
      GST_ERROR("can't start playing");res=FALSE;
    }
    // release the references
    g_object_try_unref(playline);
    g_object_try_unref(bin);
    g_object_try_unref(song_info);
  }
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
  g_assert(BT_IS_SEQUENCE(self));

  g_mutex_lock(self->priv->is_playing_mutex);
  self->priv->is_playing=FALSE;
  g_mutex_unlock(self->priv->is_playing_mutex);

  return(TRUE);
}

//-- wrapper

//-- default signal handler

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
      g_value_set_object(value, self->priv->song);
    } break;
    case SEQUENCE_LENGTH: {
      g_value_set_ulong(value, self->priv->length);
    } break;
    case SEQUENCE_TRACKS: {
      g_value_set_ulong(value, self->priv->tracks);
    } break;
    case SEQUENCE_LOOP: {
      g_value_set_boolean(value, self->priv->loop);
    } break;
    case SEQUENCE_LOOP_START: {
      g_value_set_long(value, self->priv->loop_start);
    } break;
    case SEQUENCE_LOOP_END: {
      g_value_set_long(value, self->priv->loop_end);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for sequence: %p",self->priv->song);
    } break;
    case SEQUENCE_LENGTH: {
      bt_sequence_unref_timelines(self);
      bt_sequence_free_timelines(self);
      self->priv->length = g_value_get_ulong(value);
      GST_DEBUG("set the length for sequence: %d",self->priv->length);
      bt_sequence_init_timelines(self);
    } break;
    case SEQUENCE_TRACKS: {
      self->priv->tracks = g_value_get_ulong(value);
      GST_DEBUG("set the tracks for sequence: %d",self->priv->tracks);
      bt_sequence_init_machines(self);
      bt_sequence_init_timelinetracks(self);
    } break;
    case SEQUENCE_LOOP: {
      self->priv->loop = g_value_get_boolean(value);
      GST_DEBUG("set the loop for sequence: %d",self->priv->loop);
    } break;
    case SEQUENCE_LOOP_START: {
      self->priv->loop_start = g_value_get_long(value);
      GST_DEBUG("set the loop-start for sequence: %d",self->priv->loop_start);
    } break;
    case SEQUENCE_LOOP_END: {
      self->priv->loop_end = g_value_get_long(value);
      GST_DEBUG("set the loop-end for sequence: %d",self->priv->loop_end);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sequence_dispose(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
	g_object_try_weak_unref(self->priv->song);
  bt_sequence_unref_timelines(self);
  // @todo unref the machines

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_sequence_finalize(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->machines);
  g_free(self->priv->timelines);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtSequence *self = BT_SEQUENCE(instance);
  self->priv = g_new0(BtSequencePrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->length=1;
  self->priv->loop_start=-1;
  self->priv->loop_end=-1;
  self->priv->is_playing_mutex=g_mutex_new();

}

static void bt_sequence_class_init(BtSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  
  gobject_class->set_property = bt_sequence_set_property;
  gobject_class->get_property = bt_sequence_get_property;
  gobject_class->dispose      = bt_sequence_dispose;
  gobject_class->finalize     = bt_sequence_finalize;

  klass->tick = NULL;
  
  /** 
	 * BtSequence::tick:
   * @self: the sequence object that emitted the signal
   * @pos: the postion in the sequence
	 *
	 * This sequence has reached a new play position (with the number of ticks
   * determined by the bars-property) during playback
	 */
  signals[TICK] = g_signal_new("tick",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_STRUCT_OFFSET(BtSequenceClass,tick),
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
																	g_param_spec_ulong("length",
                                     "length prop",
                                     "length of the sequence in timeline bars",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_TRACKS,
																	g_param_spec_ulong("tracks",
                                     "tracks prop",
                                     "number of tracks in the sequence",
                                     0,
                                     G_MAXLONG,
                                     0,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LOOP,
																	g_param_spec_boolean("loop",
                                     "loop prop",
                                     "is loop activated",
                                     FALSE,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LOOP_START,
																	g_param_spec_ulong("loop-start",
                                     "loop-start prop",
                                     "start of the repeat sequence on the timeline",
                                     -1,
                                     G_MAXLONG,
                                     -1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LOOP_END,
																	g_param_spec_ulong("loop-end",
                                     "loop-end prop",
                                     "end of the repeat sequence on the timeline",
                                     -1,
                                     G_MAXLONG,
                                     -1,
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

