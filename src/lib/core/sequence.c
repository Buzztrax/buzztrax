// $Id: sequence.c,v 1.107 2006-06-21 16:16:39 ensonic Exp $
/**
 * SECTION:btsequence
 * @short_description: class for the event timeline of a #BtSong instance
 *
 * A sequence holds grid of #BtPatterns, with labels on the time axis and
 * #BtMachine instances on the track axis. It supports looping a section of the
 * sequence (see #BtSequence:loop, #BtSequence:loop-start, #BtSequence:loop-end).
 *
 * The #BtSequence manages the #GstController event queues for the #BtMachines.
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
  SEQUENCE_LOOP_END
};

struct _BtSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the song the sequence belongs to */
  union {
    BtSong *song;
    gpointer song_ptr;
  };

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

  /* OBSOLETE playing variables */
  //GMutex *is_playing_mutex;
  //gboolean volatile is_playing;
  //gulong play_pos;
  gulong play_start,play_end;
  
  /* manages damage regions for updating gst-controller queues after changes */
  GHashTable *damage;
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
  
	GST_DEBUG("resize to new_data_count=%d",new_data_count);
	
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
          g_object_try_unref(machines[i]);
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
 * bt_sequence_limit_play_pos_internal:
 * @self: the sequence to trim the play position of
 *
 * Enforce the playback position to be within loop start and end or the song
 * bounds if there is no loop.
 */
void bt_sequence_limit_play_pos_internal(const BtSequence *self) {
  gulong old_play_pos,new_play_pos;
  
  g_object_get(self->priv->song,"play-pos",&old_play_pos,NULL);
  new_play_pos=bt_sequence_limit_play_pos(self,old_play_pos);
  if(new_play_pos!=old_play_pos) {
    g_object_set(self->priv->song,"play-pos",new_play_pos,NULL);
  }
}

/*
 * bt_sequence_damage_hash_free:
 *
 * Helper function to cleanup the damage hashtables
 *
static void bt_sequence_damage_hash_free(gpointer user_data) {
  if(user_data) g_hash_table_destroy((GHashTable *)user_data);
}
*/

/*
 * bt_sequence_get_number_of_pattern_uses:
 * @self: the sequence to count the patterns in
 * @this_pattern: the pattern to check for
 *
 * Count the number of times a pattern is in use.
 *
 * Returns: the pattern count
 */
static gulong bt_sequence_get_number_of_pattern_uses(const BtSequence *self,const BtPattern *this_pattern) {
  gulong res=0;
  BtMachine *machine;
  BtPattern *that_pattern;
  gulong i,j=0;
  
  g_return_val_if_fail(BT_IS_SEQUENCE(self),0);
  g_return_val_if_fail(BT_IS_PATTERN(this_pattern),0);
  
  g_object_get(G_OBJECT(this_pattern),"machine",&machine,NULL);
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      for(j=0;j<self->priv->length;j++) {
        // time has a pattern
        if((that_pattern=bt_sequence_get_pattern(self,j,i))) {
          if(that_pattern==this_pattern) res++;
          g_object_unref(that_pattern);
        }
      }
    }
  }  
  g_object_unref(machine);
  GST_INFO("get pattern uses = %d",res);
  return(res);
}

/*
 * bt_sequence_test_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 *
 * Checks if there is any pattern at the given location.
 * Avoides the reffing overhead of bt_sequence_get_pattern().
 *
 * Returns: %TRUE if there is a pattern at the given location
 */
static gboolean bt_sequence_test_pattern(const BtSequence *self,const gulong time,const gulong track) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);
  g_return_val_if_fail(time<self->priv->length,FALSE);
  g_return_val_if_fail(track<self->priv->tracks,FALSE);
  
  return(self->priv->patterns[time*self->priv->tracks+track]!=NULL);
}

static void bt_sequence_invalidate_param(const BtSequence *self,const BtMachine *machine,const gulong time,gulong param) {
  GHashTable *hash,*param_hash;

  // mark region covered by change as damaged
  hash=g_hash_table_lookup(self->priv->damage,machine);
  if(!hash) {
    hash=g_hash_table_new_full(NULL,NULL,NULL,(GDestroyNotify)g_hash_table_destroy);
    g_hash_table_insert(self->priv->damage,(gpointer)machine,hash);
  }
  param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER(param));
  if(!param_hash) {
    param_hash=g_hash_table_new(NULL,NULL);
    g_hash_table_insert(hash,GUINT_TO_POINTER(param),param_hash);
  }
  g_hash_table_insert(param_hash,GUINT_TO_POINTER(time),GUINT_TO_POINTER(TRUE));
}
  
/*
 * bt_sequence_invalidate_global_param:
 *
 * Marks the given tick for the global param of the given machine as invalid.
 */
static void bt_sequence_invalidate_global_param(const BtSequence *self,const BtMachine *machine,const gulong time,gulong param) {
  bt_sequence_invalidate_param(self,machine,time,param);
}

/*
 * bt_sequence_invalidate_voice_param:
 *
 * Marks the given tick for the voice param of the given machine and voice as invalid.
 */
static void bt_sequence_invalidate_voice_param(const BtSequence *self,const BtMachine *machine,const gulong time,gulong voice,gulong param) {
  gulong global_params,voice_params;
  
  g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
  bt_sequence_invalidate_param(self,machine,time,(global_params+voice*voice_params)+param);
}

/*
 * bt_sequence_invalidate_pattern_region:
 * @self: the sequence that hold the patterns
 * @time: the sequence time-offset of the pattern
 * @track: the track of the pattern
 * @pattern: the pattern that has been added or removed
 *
 * Calculated the damage region for the given pattern and location.
 * Adds the region to the repair-queue.
 */
static void bt_sequence_invalidate_pattern_region(const BtSequence *self,const gulong time,const gulong track,const BtPattern *pattern) {
  BtMachine *machine;
  gulong i,j,k;
  gulong length;
  gulong global_params,voice_params,voices;

  GST_DEBUG("invalidate pattern %p region for tick=%5d, track=%3d",pattern,time,track);

  // determine region of change
  g_object_get(G_OBJECT(pattern),"length",&length,"machine",&machine,NULL);
	if(!length) {
		GST_WARNING("pattern has length 0");
		return;
	}
  g_assert(machine);
  g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,"voices",&voices,NULL);
  // check if from time+1 to time+length another pattern starts (in this track)
  for(i=1;((i<length) && (time+i<self->priv->length));i++) {
    if(bt_sequence_test_pattern(self,time+i,track)) break;
  }
  length=i-1;
  // mark region covered by new pattern as damaged
  for(i=0;i<length;i++) {
    // check global params
    for(j=0;j<global_params;j++) {
      if(bt_pattern_test_global_event(pattern,i,j)) {
        // mark region covered by change as damaged
        bt_sequence_invalidate_global_param(self,machine,time+i,j);
      }
    }
    // check voices
    for(k=0;k<voices;k++) {
      // check voice params
      for(j=0;j<voice_params;j++) {
        // mark region covered by change as damaged
        bt_sequence_invalidate_voice_param(self,machine,time+i,k,j);
      }
    }
  }
  g_object_unref(machine);
	GST_DEBUG("done");
}

/*
 * bt_sequence_repair_global_damage_entry:
 *
 * Lookup the global parameter value that needs to become effective for the given
 * time and updates the machines global controller.
 */
static gboolean bt_sequence_repair_global_damage_entry(gpointer key,gpointer _value,gpointer user_data) {
  gpointer *hash_params=(gpointer *)user_data;
  BtSequence *self=BT_SEQUENCE(hash_params[0]);
  BtMachine *machine=BT_MACHINE(hash_params[1]);
  gulong param=GPOINTER_TO_UINT(hash_params[2]);
  gulong tick=GPOINTER_TO_UINT(key);
  gulong i,j;
  GValue *value=NULL,*cur_value;
  BtPattern *pattern=NULL;

  GST_DEBUG("repair global damage entry for tick=%5d",tick);

  // find all patterns with tick-offsets that are intersected by the tick of the damage
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      // go from tick position upwards to find pattern for track
      for(j=tick;j>=0;j--) {
        if((pattern=bt_sequence_get_pattern(self,j,i))) break;
      }
      if(pattern) {
        // get value at tick position or NULL
        if((cur_value=bt_pattern_get_global_event_data(pattern,tick-j,param)) && G_IS_VALUE(cur_value)) {
          value=cur_value;
        }
      }      
    }
  }
  // set controller value
  if(value) {
    GstClockTime timestamp=bt_sequence_get_bar_time(self)*tick;
    bt_machine_global_controller_change_value(machine,param,timestamp,value);
  }
  return(TRUE);
}

/*
 * bt_sequence_repair_voice_damage_entry:
 *
 * Lookup the voice parameter value that needs to become effective for the given
 * time and updates the machines voice controller.
 */
static gboolean bt_sequence_repair_voice_damage_entry(gpointer key,gpointer _value,gpointer user_data) {
  gpointer *hash_params=(gpointer *)user_data;
  BtSequence *self=BT_SEQUENCE(hash_params[0]);
  BtMachine *machine=BT_MACHINE(hash_params[1]);
  gulong param=GPOINTER_TO_UINT(hash_params[2]);
  gulong voice=GPOINTER_TO_UINT(hash_params[3]);
  gulong tick=GPOINTER_TO_UINT(key);
  gulong i,j;
  GValue *value=NULL,*cur_value;
  BtPattern *pattern=NULL;

  GST_DEBUG("repair voice damage entry for tick=%5d",tick);

  // find all patterns with tick-offsets that are intersected by the tick of the damage
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      // go from tick position upwards to find pattern for track
      for(j=tick;j>=0;j--) {
        if((pattern=bt_sequence_get_pattern(self,j,i))) break;
      }
      if(pattern) {
        // get value at tick position or NULL
        if((cur_value=bt_pattern_get_voice_event_data(pattern,tick-j,voice,param)) && G_IS_VALUE(cur_value)) {
          value=cur_value;
        }
      }      
    }
  }
  // set controller value
  if(value) {
    GstClockTime timestamp=bt_sequence_get_bar_time(self)*tick;
    bt_machine_voice_controller_change_value(machine,param,voice,timestamp,value);
  }
  return(TRUE);
}

/*
 * bt_sequence_repair_damage:
 *
 * Works through the repair queue and rebuilds controller queues, where needed.
 */
static void bt_sequence_repair_damage(const BtSequence *self) {
  gulong i,j,k;
  BtMachine *machine;
  gulong global_params,voice_params,voices;
  GHashTable *hash,*param_hash;
  gpointer hash_params[4];

  GST_DEBUG("repair damage");
  
  // repair damage
  for(i=0;i<self->priv->tracks;i++) {
    if((machine=bt_sequence_get_machine(self,i))) {
      GST_DEBUG("check damage for track %d",i);
      g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,"voices",&voices,NULL);
      if((hash=g_hash_table_lookup(self->priv->damage,machine))) {
        GST_DEBUG("repair damage for track %d",i);
        // repair damage of global params
        for(j=0;j<global_params;j++) {
          if((param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER(j)))) {          
            hash_params[0]=(gpointer)self;hash_params[1]=machine;hash_params[2]=GUINT_TO_POINTER(j);
            g_hash_table_foreach_remove(param_hash,bt_sequence_repair_global_damage_entry,&hash_params);
          }
        }
        // repair damage of voices
        for(k=0;k<voices;k++) {
          // repair damage of voice params
          for(j=0;j<voice_params;j++) {
            if((param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER((global_params+k*voice_params)+j)))) {
              hash_params[0]=(gpointer)self;hash_params[1]=machine;hash_params[2]=GUINT_TO_POINTER(j);hash_params[3]=GUINT_TO_POINTER(k);
              g_hash_table_foreach_remove(param_hash,bt_sequence_repair_voice_damage_entry,hash_params);
            }
          }
        }
      }
      g_object_unref(machine);
    }
  }
}

//-- event handler

/*
 * bt_sequence_on_pattern_global_param_changed:
 *
 * Invalidate the global param change in all pattern uses.
 */
static void bt_sequence_on_pattern_global_param_changed(const BtPattern *pattern,gulong tick,gulong param,gpointer user_data) {
  BtSequence *self=BT_SEQUENCE(user_data);
  BtMachine *that_machine,*this_machine;
  BtPattern *that_pattern;
  gulong i,j,k;
  
  g_object_get(G_OBJECT(pattern),"machine",&this_machine,NULL);
  // for all occurences of pattern
  for(i=0;i<self->priv->tracks;i++) {
    that_machine=bt_sequence_get_machine(self,i);
    if(that_machine==this_machine) {
      for(j=0;j<self->priv->length;j++) {
        that_pattern=bt_sequence_get_pattern(self,j,i);
        if(that_pattern==pattern) {
          // check if pattern plays long enough for the damage to happen
          for(k=1;((k<tick) && (j+k<self->priv->length));k++) {
            if(bt_sequence_test_pattern(self,j+k,i)) break;
          }
          if(k==tick) {
            bt_sequence_invalidate_global_param(self,this_machine,j+tick,param);
          }
        }
        g_object_try_unref(that_pattern);
      }
    }
    g_object_try_unref(that_machine);
  }
  g_object_unref(this_machine);
  // repair damage
  bt_sequence_repair_damage(self);
}

/*
 * bt_sequence_on_pattern_voice_param_changed:
 *
 * Invalidate the voice param change in all pattern uses.
 */
static void bt_sequence_on_pattern_voice_param_changed(const BtPattern *pattern,gulong tick,gulong voice,gulong param,gpointer user_data) {
  BtSequence *self=BT_SEQUENCE(user_data);
  BtMachine *that_machine,*this_machine;
  BtPattern *that_pattern;
  gulong i,j,k;
  
  g_object_get(G_OBJECT(pattern),"machine",&this_machine,NULL);
  // for all occurences of pattern
  for(i=0;i<self->priv->tracks;i++) {
    that_machine=bt_sequence_get_machine(self,i);
    if(that_machine==this_machine) {
      for(j=0;j<self->priv->length;j++) {
        that_pattern=bt_sequence_get_pattern(self,j,i);
        if(that_pattern==pattern) {
          // check if pattern plays long enough for the damage to happen
          for(k=1;((k<tick) && (j+k<self->priv->length));k++) {
            if(bt_sequence_test_pattern(self,j+k,i)) break;
          }
          if(k==tick) {
            bt_sequence_invalidate_voice_param(self,this_machine,j+tick,voice,param);
          }
        }
        g_object_try_unref(that_pattern);
      }
    }
    g_object_try_unref(that_machine);
  }
  g_object_unref(this_machine);
  // repair damage
  bt_sequence_repair_damage(self);
}

/*
 * bt_sequence_on_pattern_removed:
 *
 * Invalidate the pattern region in all occurences.
 */
static void bt_sequence_on_pattern_removed(const BtMachine *machine,BtPattern *pattern,gpointer user_data) {
  BtSequence *self=BT_SEQUENCE(user_data);
  BtMachine *that_machine;
  BtPattern *that_pattern;
  gulong i,j;

  GST_DEBUG("repair damage after a pattern %p has been removed from machine %p",pattern,machine);

  // for all tracks
  for(i=0;i<self->priv->tracks;i++) {
    that_machine=bt_sequence_get_machine(self,i);
    // does the track belong to the given machine?
    if(that_machine==machine) {
      // for all occurance of pattern
      for(j=0;j<self->priv->length;j++) {
        that_pattern=bt_sequence_get_pattern(self,j,i);
        if(that_pattern==pattern) {
          bt_sequence_set_pattern(self,j,i,NULL);
          // mark region covered by change as damaged
          //bt_sequence_invalidate_pattern_region(self,j,i,pattern);
        }
        g_object_try_unref(that_pattern);
      }
    }
    g_object_try_unref(that_machine);
  }
  // repair damage
  bt_sequence_repair_damage(self);
	GST_DEBUG("Done");
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
  
  if(track>=self->priv->tracks) return(NULL);

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

  GST_INFO("set machine for track %d",track);
  
  // @todo shouldn't we better make self->priv->tracks a readonly property and offer methods to insert/remove tracks
  // as it should not be allowed to change the machine later on
  if(!self->priv->machines[track]) {
    self->priv->machines[track]=g_object_ref(G_OBJECT(machine));
    g_signal_connect(G_OBJECT(machine),"pattern-removed",G_CALLBACK(bt_sequence_on_pattern_removed),(gpointer)self);
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
void bt_sequence_set_label(const BtSequence *self,const gulong time, const gchar *label) {
  g_return_if_fail(BT_IS_SEQUENCE(self));
  g_return_if_fail(time<self->priv->length);
  
  GST_DEBUG("set label for time %d",time);
  
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
  
  //GST_DEBUG("get pattern at time %d, track %d",time, track);
  return(g_object_try_ref(self->priv->patterns[time*self->priv->tracks+track]));
}

/**
 * bt_sequence_set_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 * @pattern: the #BtPattern or %NULL to unset
 *
 * Sets the #BtPattern for the respective @time and @track position.
 */
void bt_sequence_set_pattern(const BtSequence *self,const gulong time,const gulong track,const BtPattern *pattern) {
  gulong index;
  
  g_return_if_fail(BT_IS_SEQUENCE(self));
  g_return_if_fail(time<self->priv->length);
  g_return_if_fail(track<self->priv->tracks);
  g_return_if_fail(self->priv->machines[track]);
  
  if(pattern) {
    BtMachine *machine;
    g_return_if_fail(BT_IS_PATTERN(pattern));
    g_object_get(G_OBJECT(pattern),"machine",&machine,NULL);
    if(self->priv->machines[track]!=machine) {
      GST_WARNING("adding a pattern to a track with different machine");
      g_object_unref(machine);
      return;
    }
    g_object_unref(machine);
  }
  
  GST_INFO("set pattern %p for time %d, track %d",pattern,time,track);

  index=time*self->priv->tracks+track;
  // take out the old pattern
  if(self->priv->patterns[index]) {
    GST_DEBUG("clean up for old pattern");
    // detatch a signal handler if this was the last usage
    if(bt_sequence_get_number_of_pattern_uses(self,self->priv->patterns[index])==1) {
      g_signal_handlers_disconnect_matched(self->priv->patterns[index],G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_pattern_global_param_changed,NULL);
      g_signal_handlers_disconnect_matched(self->priv->patterns[index],G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_pattern_voice_param_changed,NULL);
    }
    // mark region covered by old pattern as damaged
    bt_sequence_invalidate_pattern_region(self,time,track,self->priv->patterns[index]);
    g_object_unref(self->priv->patterns[index]);
  }
  if(pattern) {
    GST_DEBUG("set new pattern");
    // enter the new pattern
    self->priv->patterns[index]=g_object_try_ref(G_OBJECT(pattern));
    // attatch a signal handler if this is the first usage
    if(bt_sequence_get_number_of_pattern_uses(self,self->priv->patterns[index])==1) {
      g_signal_connect(G_OBJECT(pattern),"global-param-changed",G_CALLBACK(bt_sequence_on_pattern_global_param_changed),(gpointer)self);
      g_signal_connect(G_OBJECT(pattern),"voice-param-changed",G_CALLBACK(bt_sequence_on_pattern_voice_param_changed),(gpointer)self);
    }
    // mark region covered by new pattern as damaged
    bt_sequence_invalidate_pattern_region(self,time,track,pattern);
    // repair damage
    bt_sequence_repair_damage(self);
  }
  else {
    GST_DEBUG("set empty pattern");
    self->priv->patterns[index]=NULL;
  }
  GST_DEBUG("done");
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
// @todo rename to bt_sequence_get_tick_time() ?
GstClockTime bt_sequence_get_bar_time(const BtSequence *self) {
  BtSongInfo *song_info;
  GstClockTime wait_per_position;
  gulong beats_per_minute,ticks_per_beat,ticks_per_minute;

  g_return_val_if_fail(BT_IS_SEQUENCE(self),0);

  g_object_get(G_OBJECT(self->priv->song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"tpb",&ticks_per_beat,"bpm",&beats_per_minute,NULL);
  /* the number of pattern-events for one playline-step,
   * when using 4 ticks_per_beat then
   * for 4/4 bars it is 16 (standart dance rhythm)
   * for 3/4 bars it is 12 (walz)
   */

  ticks_per_minute=(gdouble)(beats_per_minute*ticks_per_beat);
  wait_per_position=((GST_SECOND*60)/(GstClockTime)ticks_per_minute);

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

  g_return_val_if_fail(BT_IS_SEQUENCE(self),0);

  res=(GstClockTime)(self->priv->play_end-self->priv->play_start)*bt_sequence_get_bar_time(self);
  return(res);
}

/**
 * bt_sequence_limit_play_pos:
 * @self: the sequence to trim the play position of
 * @play_pos: the time position to lock inbetween loop-boundaries
 *
 * Enforce the playback position to be within loop start and end or the song
 * bounds if there is no loop.
 *
 * Returns: the new @play_pos
 */
gulong bt_sequence_limit_play_pos(const BtSequence *self,gulong play_pos) {
  if(play_pos>self->priv->play_end) {
    play_pos=self->priv->play_end;
  }
  if(play_pos<self->priv->play_start) {
    play_pos=self->priv->play_start;
  }
  return(play_pos);
}

//-- io interface

static xmlNodePtr bt_sequence_persistence_save(BtPersistence *persistence, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
  BtSequence *self = BT_SEQUENCE(persistence);
  xmlNodePtr node=NULL;
  xmlNodePtr child_node,child_node2,child_node3;
  gulong i,j;
  
  GST_DEBUG("PERSISTENCE::sequence");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("sequence"),NULL))) {
    xmlNewProp(node,XML_CHAR_PTR("length"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->length)));
    xmlNewProp(node,XML_CHAR_PTR("tracks"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->tracks)));
    if(self->priv->loop) {
      xmlNewProp(node,XML_CHAR_PTR("loop"),XML_CHAR_PTR("on"));
      xmlNewProp(node,XML_CHAR_PTR("loop-start"),XML_CHAR_PTR(bt_persistence_strfmt_long(self->priv->loop_start)));
      xmlNewProp(node,XML_CHAR_PTR("loop-end"),XML_CHAR_PTR(bt_persistence_strfmt_long(self->priv->loop_end)));
    }
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("labels"),NULL))) {
      gchar *label;

      // iterate over timelines
      for(i=0;i<self->priv->length;i++) {
        if((label=self->priv->labels[i])) {
          child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("label"),NULL);
          xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(label));
          xmlNewProp(child_node2,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
        }
      }
    }
    else goto Error;
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("tracks"),NULL))) {
      BtMachine *machine;
      BtPattern *pattern;
      gchar *machine_id,*pattern_id;
      
      // iterate over tracks
      for(j=0;j<self->priv->tracks;j++) {
        child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("track"),NULL);
        machine=self->priv->machines[j];
        g_object_get(G_OBJECT(machine),"id",&machine_id,NULL);
        xmlNewProp(child_node2,XML_CHAR_PTR("index"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(j)));
        xmlNewProp(child_node2,XML_CHAR_PTR("machine"),XML_CHAR_PTR(machine_id));g_free(machine_id);
        // iterate over timelines
        for(i=0;i<self->priv->length;i++) {
          // get pattern
          if((pattern=self->priv->patterns[i*self->priv->tracks+j])) {
            g_object_get(G_OBJECT(pattern),"id",&pattern_id,NULL);
            child_node3=xmlNewChild(child_node2,NULL,XML_CHAR_PTR("position"),NULL);
            xmlNewProp(child_node3,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
            xmlNewProp(child_node3,XML_CHAR_PTR("pattern"),XML_CHAR_PTR(pattern_id));g_free(pattern_id);
          }
        }
      }
    }
    else goto Error;
  }
Error:
  return(node);
}

static gboolean bt_sequence_persistence_load(BtPersistence *persistence, xmlNodePtr node, BtPersistenceLocation *location) {
  BtSequence *self = BT_SEQUENCE(persistence);
  gboolean res=FALSE;
  xmlChar *length_str,*tracks_str,*loop_str,*loop_start_str,*loop_end_str;
  gboolean loop;
  glong loop_start,loop_end;
  gulong length,tracks;
  xmlNodePtr child_node,child_node2;
  
  GST_DEBUG("PERSISTENCE::sequence");
  
  length_str=xmlGetProp(node,XML_CHAR_PTR("length"));
  tracks_str=xmlGetProp(node,XML_CHAR_PTR("tracks"));
  loop_str=xmlGetProp(node,XML_CHAR_PTR("loop"));
  loop_start_str=xmlGetProp(node,XML_CHAR_PTR("loop-start"));
  loop_end_str=xmlGetProp(node,XML_CHAR_PTR("loop-end"));
  
  length=length_str?atol((char *)length_str):0;
  tracks=tracks_str?atol((char *)tracks_str):0;
  loop_start=loop_start_str?atol((char *)loop_start_str):-1;
  loop_end=loop_end_str?atol((char *)loop_end_str):-1;
  loop=loop_str?!strncasecmp((char *)loop_str,"on\0",3):FALSE;
  g_object_set(self,"length",length,"tracks",tracks,
    "loop",loop,"loop-start",loop_start,"loop-end",loop_end,
    NULL);
  xmlFree(length_str);xmlFree(tracks_str);
  xmlFree(loop_str);xmlFree(loop_start_str);xmlFree(loop_end_str);

  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      if(!strncmp((gchar *)node->name,"labels\0",7)) {
        xmlChar *time_str,*name;

        for(child_node=node->children;child_node;child_node=child_node->next) {
          if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"label\0",6))) {
            time_str=xmlGetProp(child_node,XML_CHAR_PTR("time"));
            name=xmlGetProp(child_node,XML_CHAR_PTR("name"));
            bt_sequence_set_label(self,atol((char *)time_str),(gchar *)name);
            xmlFree(time_str);xmlFree(name);
          }
        }
      }
      else if(!strncmp((gchar *)node->name,"tracks\0",7)) {
        BtSetup *setup;
        BtMachine *machine;
        BtPattern *pattern;
        xmlChar *index_str,*machine_id,*time_str,*pattern_id;
        gulong index;
        
        g_object_get(self->priv->song,"setup",&setup,NULL);

        for(child_node=node->children;child_node;child_node=child_node->next) {
          if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"track\0",6))) {
            machine_id=xmlGetProp(child_node,XML_CHAR_PTR("machine"));
            index_str=xmlGetProp(child_node,XML_CHAR_PTR("index"));
            index=index_str?atol((char *)index_str):0;
            if((machine=bt_setup_get_machine_by_id(setup, (gchar *)machine_id))) {
              self->priv->machines[index]=machine;
              GST_DEBUG("loading track with index=%s for machine=\"%s\"",index_str,machine_id);
              for(child_node2=child_node->children;child_node2;child_node2=child_node2->next) {
                if((!xmlNodeIsText(child_node2)) && (!strncmp((char *)child_node2->name,"position\0",9))) {
                  time_str=xmlGetProp(child_node2,XML_CHAR_PTR("time"));
                  pattern_id=xmlGetProp(child_node2,XML_CHAR_PTR("pattern"));
                  GST_DEBUG("  at %s, machinepattern \"%s\"",time_str,safe_string(pattern_id));
                  if(pattern_id) {
                    // get pattern by name from machine
                    if((pattern=bt_machine_get_pattern_by_id(machine,(gchar *)pattern_id))) {
                      bt_sequence_set_pattern(self,atol((char *)time_str),index,pattern);
                      g_object_unref(pattern);
                    }
                    else GST_ERROR("  unknown pattern \"%s\"",pattern_id);
                    xmlFree(pattern_id);
                  }
                  xmlFree(time_str);
                }
              }
            }
            else {
              GST_ERROR("invalid machine %s referenced at track %d",(gchar *)machine_id,index);
            }
            xmlFree(index_str);xmlFree(machine_id);
          }
        }
        g_object_try_unref(setup);
      }
    }
  }
  
  res=TRUE;
  return(res);
}

static void bt_sequence_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface *iface = g_iface;
  
  iface->load = bt_sequence_persistence_load;
  iface->save = bt_sequence_persistence_save;
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
      // @todo remove or better stop the song
      // if(self->priv->is_playing) bt_sequence_stop(self);
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
        bt_sequence_limit_play_pos_internal(self);
      }
    } break;
    case SEQUENCE_TRACKS: {
      // @todo remove or better stop the song
      //if(self->priv->is_playing) bt_sequence_stop(self);
      // prepare new data      
      tracks=self->priv->tracks;
      self->priv->tracks = g_value_get_ulong(value);
      if(tracks!=self->priv->tracks) {
        GST_DEBUG("set the tracks for sequence: %lu",self->priv->tracks);
        bt_sequence_resize_data_tracks(self,tracks);
        bt_song_set_unsaved(self->priv->song,TRUE);
      }
    } break;
    case SEQUENCE_LOOP: {
      self->priv->loop = g_value_get_boolean(value);
      GST_DEBUG("set the loop for sequence: %d",self->priv->loop);
      if(self->priv->loop) {
        if(self->priv->loop_start==-1) {
          self->priv->loop_start=0;
          g_object_notify(G_OBJECT(self), "loop-start");
        }
        self->priv->play_start=self->priv->loop_start;
        if(self->priv->loop_end==-1) {
          self->priv->loop_end=self->priv->length;
          g_object_notify(G_OBJECT(self), "loop-end");
        }
        self->priv->play_end=self->priv->loop_end;
        bt_sequence_limit_play_pos_internal(self);
      }
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_LOOP_START: {
      self->priv->loop_start = g_value_get_long(value);
      GST_DEBUG("set the loop-start for sequence: %ld",self->priv->loop_start);
      self->priv->play_start=(self->priv->loop_start!=-1)?self->priv->loop_start:0;
      bt_sequence_limit_play_pos_internal(self);
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_LOOP_END: {
      self->priv->loop_end = g_value_get_long(value);
      GST_DEBUG("set the loop-end for sequence: %ld",self->priv->loop_end);
      self->priv->play_end=(self->priv->loop_end!=-1)?self->priv->loop_end:self->priv->length;
      bt_sequence_limit_play_pos_internal(self);
      bt_song_set_unsaved(self->priv->song,TRUE);
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

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_sequence_finalize(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->machines);
  g_free(self->priv->labels);
  g_free(self->priv->patterns);
  g_hash_table_destroy(self->priv->damage);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtSequence *self = BT_SEQUENCE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SEQUENCE, BtSequencePrivate);
  self->priv->loop_start=-1;
  self->priv->loop_end=-1;
  self->priv->damage=g_hash_table_new_full(NULL,NULL,NULL,(GDestroyNotify)g_hash_table_destroy);
}

static void bt_sequence_class_init(BtSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSequencePrivate));
  
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
                                     G_MAXLONG,  // loop-pos are LONG as well
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

}

GType bt_sequence_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
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
    static const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_sequence_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSequence",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
