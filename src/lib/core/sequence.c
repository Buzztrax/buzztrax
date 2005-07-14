/* $Id: sequence.c,v 1.67 2005-07-14 22:03:35 ensonic Exp $
 * class for the pattern sequence
 */
 
#define BT_CORE
#define BT_SEQUENCE_C

#include <libbtcore/core.h>

//-- signal ids

//-- property ids

enum {
  SEQUENCE_SONG=1,
  SEQUENCE_LENGTH,
  SEQUENCE_TRACKS,
  SEQUENCE_LOOP,
  SEQUENCE_LOOP_START,
  SEQUENCE_LOOP_END,
	SEQUENCE_PLAY_POS
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
  /* <length> label entries that are the description of the time axis */
  gchar **labels;
  /* <length>*<tracks> BtPattern pointers */
  BtPattern **patterns;

  /* OBSOLETE <length> timeline entries that form the sequence */
  //BtTimeLine **timelines;

  /* flag to externally abort playing */
  GMutex *is_playing_mutex;
  gboolean volatile is_playing;
	
	gulong play_pos,play_start,play_end;
};

static GObjectClass *parent_class=NULL;

//static guint signals[LAST_SIGNAL]={0,};

//-- helper methods

/*
 * bt_sequence_init_data:
 * @self: the sequence to initialize the pattern data for
 *
 * Allocates and initializes the memory for the pattern data grid.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_sequence_init_data(const BtSequence *self) {
  gulong data_count=self->priv->length*self->priv->tracks;

  if(data_count==0) return(TRUE);
		
  if(self->priv->patterns) {
    GST_INFO("data has already been initialized");
    return(TRUE);
  }

  GST_DEBUG("sizes : %d*%d=%d",self->priv->length,self->priv->tracks,data_count);
  if(!(self->priv->patterns=(BtPattern **)g_try_new0(gpointer,data_count))) {
    GST_WARNING("failed to allocate memory for patterns grid");
    goto Error;
  }
  if(!(self->priv->labels=(gchar **)g_try_new0(gpointer,self->priv->length))) {
    GST_WARNING("failed to allocate memory for labels array");
    goto Error;
  }
  if(!(self->priv->machines=(BtMachine **)g_try_new0(gpointer,self->priv->tracks))) {
    GST_WARNING("failed to allocate memory for machines array");
    goto Error;
  }
  return(TRUE);
Error:
  g_free(self->priv->patterns);self->priv->patterns=NULL;
  g_free(self->priv->labels);self->priv->labels=NULL;
  g_free(self->priv->machines);self->priv->machines=NULL;
  return(FALSE);
}

/*
 * bt_sequence_resize_data_length:
 * @self: the sequence to resize the length
 * @length: the old length
 *
 * Resizes the pattern data grid to the new length. Keeps previous values.
 */
static void bt_sequence_resize_data_length(const BtSequence *self, gulong length) {
	gulong old_data_count=length*self->priv->tracks;
	gulong new_data_count=self->priv->length*self->priv->tracks;
	BtPattern **patterns=self->priv->patterns;
  gchar **labels=self->priv->labels;
	
  // allocate new space
	if((self->priv->patterns=(BtPattern **)g_try_new0(gpointer,new_data_count))) {
		if(patterns) {
      gulong count=MIN(old_data_count,new_data_count);
			// copy old values over
			memcpy(self->priv->patterns,patterns,count*sizeof(gpointer));
			// free old data
      if(length>self->priv->length) {
        gulong i,j,k;
        k=self->priv->length*self->priv->tracks;
        for(i=self->priv->length;i<length;i++) {
          for(j=0;j<self->priv->tracks;j++,k++) {
            g_object_try_unref(patterns[k]);
          }
        }
      }
			g_free(patterns);
		}
  }
	else {
		GST_WARNING("extending sequence length from %d to %d failed : data_count=%d = length=%d * tracks=%d",
			length,self->priv->length,
			new_data_count,self->priv->length,self->priv->tracks);
	}
  // allocate new space
	if((self->priv->labels=(gchar **)g_try_new0(gpointer,self->priv->length))) {
		if(labels) {
      gulong count=MIN(length,self->priv->length);
			// copy old values over
			memcpy(self->priv->labels,labels,count*sizeof(gpointer));
			// free old data
      if(length>self->priv->length) {
        gulong i;
        for(i=self->priv->length;i<length;i++) {
          g_free(labels[i]);
        }
      }
			g_free(labels);
		}
  }
	else {
		GST_WARNING("extending sequence labels from %d to %d failed",length,self->priv->length);
	}  
}

/*
 * bt_sequence_resize_data_tracks:
 * @self: the sequence to resize the tracks number
 * @tracks: the old number of tracks
 *
 * Resizes the pattern data grid to the new number of tracks. Keeps previous values.
 */
static void bt_sequence_resize_data_tracks(const BtSequence *self, gulong tracks) {
	//gulong old_data_count=self->priv->length*tracks;
	gulong new_data_count=self->priv->length*self->priv->tracks;
	BtPattern **patterns=self->priv->patterns;
  BtMachine **machines=self->priv->machines;
	
  // allocate new space
	if((self->priv->patterns=(BtPattern **)g_try_new0(GValue,new_data_count))) {
		if(patterns) {
			gulong i;
			gulong count=MIN(tracks,self->priv->tracks);
			BtPattern **src,**dst;
		
			// copy old values over
      src=patterns;
      dst=self->priv->patterns;
			for(i=0;i<self->priv->length;i++) {
        memcpy(dst,src,count*sizeof(gpointer));
				src=&src[tracks];
				dst=&dst[self->priv->tracks];			
			}
			// free old data
      if(tracks>self->priv->tracks) {
        gulong j,k;
        for(i=0;i<self->priv->length;i++) {
          k=i*tracks+self->priv->tracks;
          for(j=self->priv->tracks;j<tracks;j++,k++) {
            g_object_try_unref(patterns[k]);
          }
        }        
      }
			g_free(patterns);
		}
  }
	else {
		GST_WARNING("extending sequence tracks from %d to %d failed : data_count=%d = length=%d * tracks=%d",
			tracks,self->priv->tracks,
			new_data_count,self->priv->length,self->priv->tracks);
	}
  // allocate new space
	if((self->priv->machines=(BtMachine **)g_try_new0(gpointer,self->priv->tracks))) {
		if(machines) {
      gulong count=MIN(tracks,self->priv->tracks);
			// copy old values over
			memcpy(self->priv->machines,machines,count*sizeof(gpointer));
			// free old data
      if(tracks>self->priv->tracks) {
        gulong i;
        for(i=self->priv->tracks;i<tracks;i++) {
          g_object_unref(machines[i]);
        }
      }
			g_free(machines);
		}
  }
	else {
		GST_WARNING("extending sequence machines from %d to %d failed",tracks,self->priv->tracks);
	}  
}

/*
 * bt_sequence_limit_play_pos:
 * @self: the sequence to trim the play position of
 *
 * Enforce the playback position to be within loop start and end or the song
 * bounds if there is no loop.
 */
static void bt_sequence_limit_play_pos(const BtSequence *self) {
	if(self->priv->play_pos>self->priv->play_end) {
		self->priv->play_pos=self->priv->play_end;
		g_object_notify(G_OBJECT(self),"play-pos");
	}
	if(self->priv->play_pos<self->priv->play_start) {
		self->priv->play_pos=self->priv->play_start;
		g_object_notify(G_OBJECT(self),"play-pos");
	}
}

/**
 * bt_sequence_update_playline:
 * @self: the sequence instance that holds the tracks
 * @playline: the current play-cursor
 *
 * Enter new patterns into the playline and stop or mute patterns
 */
static void bt_sequence_update_playline(const BtSequence *self, const BtPlayLine *playline) {
  gulong i;
  BtPattern *pattern;
  
  g_assert(BT_IS_SEQUENCE(self));
  g_assert(BT_IS_PLAYLINE(playline));

  /* DEBUG
  if(self->priv->labels[self->priv->play_pos]) {
    GST_INFO("  label=\"%s\"",self->priv->labels[self->priv->play_pos]);
  }*/
  
  for(i=0;i<self->priv->tracks;i++) {
    // enter new patterns into the playline and stop or mute patterns
    if((pattern=bt_sequence_get_pattern(self,self->priv->play_pos,i))) {
      /*
      g_object_get_property(G_OBJECT(*timelinetrack),"type", &pattern_type);
      switch(g_value_get_enum(&pattern_type)) {
        case BT_TIMELINETRACK_TYPE_EMPTY:
          break;
        case BT_TIMELINETRACK_TYPE_PATTERN:
          g_object_get(G_OBJECT(*timelinetrack),"pattern",&pattern,NULL);
          bt_playline_set_pattern(playline,i,pattern);
          g_object_try_unref(pattern);
          break;
        case BT_TIMELINETRACK_TYPE_STOP:
          bt_playline_set_pattern(playline,i,NULL);
          break;
        default:
          GST_ERROR("implement me");
      }
      */
      g_object_unref(pattern);
    }
  }
}

//-- constructor methods

/**
 * bt_sequence_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance. One would not call this directly, but rather get this
 * from a #BtSong instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSequence *bt_sequence_new(const BtSong *song) {
  BtSequence *self;
  
	g_return_val_if_fail(BT_IS_SONG(song),NULL);
  
  if(!(self=BT_SEQUENCE(g_object_new(BT_TYPE_SEQUENCE,"song",song,NULL)))) {
		goto Error;
	}
  if(!bt_sequence_init_data(self)) {
    goto Error;
  }
  return(self);
Error:
	g_object_try_unref(self);
	return(NULL);
}

//-- methods

/**
 * bt_sequence_get_machine:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 *
 * Fetches the #BtMachine for the given @track. Unref when done.
 *
 * Returns: a reference to the #BtMachine pointer or %NULL in case of an error
 */
BtMachine *bt_sequence_get_machine(const BtSequence *self,const gulong track) {
	g_return_val_if_fail(BT_IS_SEQUENCE(self),NULL);
  g_return_val_if_fail(track<self->priv->tracks,NULL);

  return(g_object_try_ref(self->priv->machines[track]));
}

/**
 * bt_sequence_set_machine:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 * @machine: the #BtMachine
 *
 * Sets the #BtMachine for the respective @track.
 * This should only be done once for each track.
 */
void bt_sequence_set_machine(const BtSequence *self,const gulong track,const BtMachine *machine) {
	g_return_if_fail(BT_IS_SEQUENCE(self));
	g_return_if_fail(BT_IS_MACHINE(machine));
  g_return_if_fail(track<self->priv->tracks);

  // @todo shouldn't we better make self->priv->tracks a readonly property and offer methods to insert/remove tracks
  // as it should not be allowed to change the machine later on
  if(!self->priv->machines[track]) {
    self->priv->machines[track]=g_object_ref((gpointer)machine);
  }
  else {
    GST_ERROR("machine has already be set!");
  }
}

/**
 * bt_sequence_get_label:
 * @self: the #BtSequence that holds the labels
 * @time: the requested time position
 *
 * Fetches the label for the given @time position. Free when done.
 *
 * Returns: a copy of the label or %NULL in case of an error
 */
gchar *bt_sequence_get_label(const BtSequence *self,const gulong time) {
	g_return_val_if_fail(BT_IS_SEQUENCE(self),NULL);
  g_return_val_if_fail(time<self->priv->length,NULL);
  
  return(g_strdup(self->priv->labels[time]));
}

/**
 * bt_sequence_set_label:
 * @self: the #BtSequence that holds the labels
 * @time: the requested time position
 * @label: the new label
 *
 * Sets a new label for the respective @time position.
 */
void bt_sequence_set_label(const BtSequence *self,const gulong time, gchar *label) {
	g_return_if_fail(BT_IS_SEQUENCE(self));
  g_return_if_fail(time<self->priv->length);
  
  g_free(self->priv->labels[time]);
  self->priv->labels[time]=g_strdup(label);
}

/**
 * bt_sequence_get_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 *
 * Fetches the pattern for the given @time and @track position. Unref when done.
 *
 * Returns: a reference to the #BtPattern or %NULL when empty
 */
BtPattern *bt_sequence_get_pattern(const BtSequence *self,const gulong time,const gulong track) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),NULL);
  g_return_val_if_fail(time<self->priv->length,NULL);
  g_return_val_if_fail(track<self->priv->tracks,NULL);
  
  return(g_object_try_ref(self->priv->patterns[time*self->priv->tracks+track]));
}

/**
 * bt_sequence_set_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 * @pattern: the #BtPattern
 *
 * Sets the #BtPattern for the respective @time and @track position.
 */
void bt_sequence_set_pattern(const BtSequence *self,const gulong time,const gulong track,BtPattern *pattern) {
  gulong index;
  g_return_if_fail(BT_IS_SEQUENCE(self));
  g_return_if_fail(time<self->priv->length);
  g_return_if_fail(track<self->priv->tracks);
  
  index=time*self->priv->tracks+track;
  g_object_try_unref(self->priv->patterns[index]);
  self->priv->patterns[index]=g_object_try_ref(pattern);
}

/**
 * bt_sequence_get_bar_time:
 * @self: the #BtSequence of the song
 *
 * Calculates the length of one sequence bar in microseconds.
 * Divide it by G_USEC_PER_SEC to get it in milliseconds.
 *
 * Returns: the length of one sequence bar in microseconds
 */
GstClockTime bt_sequence_get_bar_time(const BtSequence *self) {
  BtSongInfo *song_info;
  GstClockTime wait_per_position;
  gulong beats_per_minute,ticks_per_beat;
  gdouble ticks_per_minute;

	g_return_val_if_fail(BT_IS_SEQUENCE(self),(GstClockTime)0);

  g_object_get(G_OBJECT(self->priv->song),"song-info",&song_info,NULL);
	g_object_get(G_OBJECT(song_info),"tpb",&ticks_per_beat,"bpm",&beats_per_minute,NULL);

  ticks_per_minute=(gdouble)beats_per_minute*(gdouble)ticks_per_beat;
  wait_per_position=(GstClockTime)((GST_SECOND*60.0)/(gdouble)ticks_per_minute);

  //res=(gulong)(wait_per_position/G_USEC_PER_SEC);
  // release the references
  g_object_try_unref(song_info);
  return(wait_per_position);
}

/**
 * bt_sequence_get_loop_time:
 * @self: the #BtSequence of the song
 *
 * Calculates the length of the song loop in microseconds.
 * Divide it by G_USEC_PER_SEC to get it in milliseconds.
 *
 * Returns: the length of the song loop in microseconds
 *
 */
GstClockTime bt_sequence_get_loop_time(const BtSequence *self) {
  GstClockTime res;

	g_return_val_if_fail(BT_IS_SEQUENCE(self),(GstClockTime)0);

  res=(GstClockTime)(self->priv->play_end-self->priv->play_start)*bt_sequence_get_bar_time(self);
  return(res);
}

/**
 * bt_sequence_get_pattern_positions:
 * @self: the #BtSequence of the song
 * @machine: the #BtMachine the pattern belongs to
 * @pattern: the #BtPattern to lookup positions
 *
 * Searches all tick positions the pattern will be played.
 *
 * Returns: a newly allocated #GList holding the tick positions. Get them by
 * calling: tick=GPOINTER_TO_UINT(node->data);. Free the list when done.
 */
GList *bt_sequence_get_pattern_positions(const BtSequence *self,const BtMachine *machine,const BtPattern *pattern) {
  // @todo implement bt_sequence_get_pattern_positions()
  /* playline algorithm?
  foreach(timeline) {
    
  }
  */
  return(NULL);
}

/**
 * bt_sequence_play:
 * @self: the #BtSequence that should be played
 *
 * starts playback for the sequence
 *
 * Returns: %TRUE for success
 *
 */
gboolean bt_sequence_play(const BtSequence *self) {
  gboolean res=TRUE;
  
	g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);

  if((!self->priv->tracks) || (!self->priv->length)) {
		GST_INFO("skip playing (no tracks or no timeline at all");
		return(res);
	}
  else {
    BtSongInfo *song_info;
    GstElement *bin;
    BtPlayLine *playline;
    gulong wait_per_position;
    gulong beats_per_minute,ticks_per_beat;
    gdouble ticks_per_minute;
    // DEBUG {
    //GTimer *timer;
    // }

    g_object_get(G_OBJECT(self->priv->song),"bin",&bin,"song-info",&song_info,NULL);
		g_object_get(G_OBJECT(song_info),"tpb",&ticks_per_beat,"bpm",&beats_per_minute,NULL);
    /* the number of pattern-events for one playline-step,
     * when using 4 ticks_per_beat then
     * for 4/4 bars it is 16 (standart dance rhythm)
     * for 3/4 bars it is 12 (walz)
     */
    ticks_per_minute=(gdouble)beats_per_minute*(gdouble)ticks_per_beat;
    wait_per_position=(gulong)((GST_SECOND*60.0)/(gdouble)ticks_per_minute);
		playline=bt_playline_new(self->priv->song,self->priv->tracks,wait_per_position);
    
    //GST_INFO("pattern.duration = %d * %d usec = %ld sec",bars,wait_per_position,(gulong)(((guint64)bars*(guint64)wait_per_position)/GST_SECOND));
    GST_INFO("song.duration = %d * %d usec = %ld sec",self->priv->length,wait_per_position,(gulong)(((guint64)self->priv->length*(guint64)wait_per_position)/GST_SECOND));
		GST_INFO("  play_start,pos,end: %d,%d,%d",self->priv->play_start,self->priv->play_pos,self->priv->play_end);
		GST_INFO("  loop_start,end: %d,%d",self->priv->loop_start,self->priv->loop_end);
		GST_INFO("  length: %d",self->priv->length);
  
    if(gst_element_set_state(bin,GST_STATE_PLAYING)!=GST_STATE_FAILURE) {
      g_mutex_lock(self->priv->is_playing_mutex);
      self->priv->is_playing=TRUE;
      g_mutex_unlock(self->priv->is_playing_mutex);
      //timer=g_timer_new();
			//g_timer_start(timer);

      do {
				for(;((self->priv->play_pos<self->priv->play_end) && (self->priv->is_playing));self->priv->play_pos++) {
          //GST_INFO("Playing sequence : position = %d, time elapsed = %lf sec",i,g_timer_elapsed(timer,NULL));

					g_object_notify(G_OBJECT(self),"play-pos");
          // enter new patterns into the playline and stop or mute patterns
          bt_sequence_update_playline(self,playline);
          // play the patterns in the cursor
          bt_playline_play(playline);
        }
				self->priv->play_pos=self->priv->play_start;
				g_object_notify(G_OBJECT(self),"play-pos");
      } while((self->priv->loop) && (self->priv->is_playing));

      //g_timer_destroy(timer);

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
 * Returns: %TRUE for success
 *
 */
gboolean bt_sequence_stop(const BtSequence *self) {
	
	g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);

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
    case SEQUENCE_PLAY_POS: {
      g_value_set_ulong(value, self->priv->play_pos);
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
  gulong length,tracks;
  
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for sequence: %p",self->priv->song);
    } break;
    case SEQUENCE_LENGTH: {
			// check if song is playing
			if(self->priv->is_playing) bt_sequence_stop(self);
			// prepare new data
      length=self->priv->length;
      self->priv->length = g_value_get_ulong(value);
      if(length!=self->priv->length) {
				GST_DEBUG("set the length for sequence: %d",self->priv->length);
				bt_sequence_resize_data_length(self,length);
				bt_song_set_unsaved(self->priv->song,TRUE);
        if(self->priv->loop_end!=-1) {
          if(self->priv->loop_end>self->priv->length) {
            self->priv->play_end=self->priv->loop_end=self->priv->length;
          }
        }
        else {
          self->priv->play_end=self->priv->length;
        }
        bt_sequence_limit_play_pos(self);
      }
    } break;
    case SEQUENCE_TRACKS: {
			// check if song is playing
			if(self->priv->is_playing) bt_sequence_stop(self);
			// prepare new data			
			tracks=self->priv->tracks;
      self->priv->tracks = g_value_get_ulong(value);
      if(tracks!=self->priv->tracks) {
        GST_DEBUG("set the tracks for sequence: %lu",self->priv->tracks);
        //bt_sequence_init_timelinetracks(self);
        bt_sequence_resize_data_tracks(self,tracks);
			  bt_song_set_unsaved(self->priv->song,TRUE);
      }
    } break;
    case SEQUENCE_LOOP: {
      self->priv->loop = g_value_get_boolean(value);
      GST_DEBUG("set the loop for sequence: %d",self->priv->loop);
			bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_LOOP_START: {
      self->priv->loop_start = g_value_get_long(value);
      GST_DEBUG("set the loop-start for sequence: %ld",self->priv->loop_start);
			self->priv->play_start=(self->priv->loop_start!=-1)?self->priv->loop_start:0;
			bt_sequence_limit_play_pos(self);
			bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_LOOP_END: {
      self->priv->loop_end = g_value_get_long(value);
      GST_DEBUG("set the loop-end for sequence: %ld",self->priv->loop_end);
			self->priv->play_end=(self->priv->loop_end!=-1)?self->priv->loop_end:self->priv->length;
			bt_sequence_limit_play_pos(self);
			bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_PLAY_POS: {
      self->priv->play_pos = g_value_get_ulong(value);
			bt_sequence_limit_play_pos(self);
      GST_DEBUG("set the play-pos for sequence: %lu",self->priv->play_pos);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sequence_dispose(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);
  gulong i,j,k;

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
	g_object_try_weak_unref(self->priv->song);
  // unref the machines
  for(i=0;i<self->priv->tracks;i++) {
    g_object_try_unref(self->priv->machines[i]);
  }
  // free the labels
  for(i=0;i<self->priv->length;i++) {
    g_free(self->priv->labels[i]);
  }
  // unref the patterns
  for(k=i=0;i<self->priv->length;i++) {
    for(j=0;j<self->priv->tracks;j++,k++) {
      g_object_try_unref(self->priv->patterns[k]);
    }
  }

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_sequence_finalize(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);

  GST_DEBUG("!!!! self=%p",self);

	g_mutex_free(self->priv->is_playing_mutex);
  //g_free(self->priv->timelines);
  g_free(self->priv->machines);
  g_free(self->priv->labels);
  g_free(self->priv->patterns);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtSequence *self = BT_SEQUENCE(instance);
  self->priv = g_new0(BtSequencePrivate,1);
  self->priv->dispose_has_run = FALSE;
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
                                     0,
                                     G_MAXLONG,	// loop-pos are LONG as well
                                     0,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_TRACKS,
																	g_param_spec_ulong("tracks",
                                     "tracks prop",
                                     "number of tracks in the sequence",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LOOP,
																	g_param_spec_boolean("loop",
                                     "loop prop",
                                     "is loop activated",
                                     FALSE,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LOOP_START,
																	g_param_spec_long("loop-start",
                                     "loop-start prop",
                                     "start of the repeat sequence on the timeline",
                                     -1,
                                     G_MAXLONG,
                                     -1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LOOP_END,
																	g_param_spec_long("loop-end",
                                     "loop-end prop",
                                     "end of the repeat sequence on the timeline",
                                     -1,
                                     G_MAXLONG,
                                     -1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_PLAY_POS,
																	g_param_spec_ulong("play-pos",
                                     "play-pos prop",
                                     "position of the play cursor of the sequence in timeline bars",
                                     0,
                                     G_MAXLONG,	// loop-pos are LONG as well
                                     0,
                                     G_PARAM_READWRITE));
}

GType bt_sequence_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSequence),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_sequence_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSequence",&info,0);
  }
  return type;
}
