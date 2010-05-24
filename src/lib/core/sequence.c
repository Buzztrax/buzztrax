/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btsequence
 * @short_description: class for the event timeline of a #BtSong instance
 *
 * A sequence holds grid of #BtPatterns, with labels on the time axis and
 * #BtMachine instances on the track axis. It supports looping a section of the
 * sequence (see #BtSequence:loop, #BtSequence:loop-start, #BtSequence:loop-end).
 *
 * The #BtSequence manages the #GstController event queues for the #BtMachines
 * and #BtWires.
 * It uses a damage-repair based two phase algorithm to update the controller
 * queues whenever patterns or the sequence changes.
 */

#define BT_CORE
#define BT_SEQUENCE_C

#include "core_private.h"

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
  G_POINTER_ALIAS(BtSong *,song);

  /* the number of timeline entries */
  gulong length;

  /* the number of tracks */
  gulong tracks;

  /* loop mode on/off */
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

  /* playback range variables */
  gulong play_start,play_end;
  
  /* cached */
  GstClockTime wait_per_position;

  /* manages damage regions for updating gst-controller queues after changes
   * each entry (per machine) has another GHashTable
   *   each entry (per parameter) has another GHashTable
   *     with time:changed
   * @todo: there is quite some overhead becasue of the nested HashTables
   * and because of repeted lookups in repair_{global,voice,wire}_damage_entry:
   * - maybe we could have this instead:
   *   each entry (per machine) has another GHashTable
   *     each entry (per time) has list of changed parameters
   * - the list of changed parameters could be a fixed bit-array, allocated on 
   *   first use
   * - benefits would be
   *   - one less hashtable (but then the array)
   *   - repair_damage could do the prev-pattern lookup once and reuse for each
   *     changed parameter
   */
  GHashTable *damage;
};

static GObjectClass *parent_class=NULL;

//static guint signals[LAST_SIGNAL]={0,};

//-- helper methods

static BtPattern *bt_sequence_get_pattern_unchecked(const BtSequence * const self, const gulong time, const gulong track) {
  //GST_DEBUG("get pattern at time %d, track %d",time, track);
  return(self->priv->patterns[time*self->priv->tracks+track]);
}


static BtMachine *bt_sequence_get_machine_unchecked(const BtSequence * const self,const gulong track) {
  //GST_DEBUG("getting machine : %p,ref_count=%d",self->priv->machines[track],(self->priv->machines[track]?G_OBJECT_REF_COUNT(self->priv->machines[track]):-1));
  return(self->priv->machines[track]);
}

/*
 * bt_sequence_resize_data_length:
 * @self: the sequence to resize the length
 * @length: the old length
 *
 * Resizes the pattern data grid to the new length. Keeps previous values.
 */
static void bt_sequence_resize_data_length(const BtSequence * const self, const gulong length) {
  const gulong old_data_count=length*self->priv->tracks;
  const gulong new_data_count=self->priv->length*self->priv->tracks;
  BtPattern ** const patterns=self->priv->patterns;
  gchar ** const labels=self->priv->labels;

  // allocate new space
  if((self->priv->patterns=(BtPattern **)g_try_new0(gpointer,new_data_count))) {
    if(patterns) {
      const gulong count=MIN(old_data_count,new_data_count);
      // copy old values over
      memcpy(self->priv->patterns,patterns,count*sizeof(gpointer));
      // free old data
      if(length>self->priv->length) {
        gulong i,j,k;
        k=new_data_count;
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
    GST_INFO("extending sequence length from %lu to %lu failed : data_count=%lu = length=%lu * tracks=%lu",
      length,self->priv->length,
      new_data_count,self->priv->length,self->priv->tracks);
  }
  // allocate new space
  if((self->priv->labels=(gchar **)g_try_new0(gpointer,self->priv->length))) {
    if(labels) {
      const gulong count=MIN(length,self->priv->length);
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
    GST_INFO("extending sequence labels from %lu to %lu failed",length,self->priv->length);
  }
}

/*
 * bt_sequence_resize_data_tracks:
 * @self: the sequence to resize the tracks number
 * @old_tracks: the old number of tracks
 *
 * Resizes the pattern data grid to the new number of tracks (add removes at the
 * end). Keeps previous values.
 */
static void bt_sequence_resize_data_tracks(const BtSequence * const self, const gulong old_tracks) {
  //gulong old_data_count=self->priv->length*tracks;
  const gulong new_data_count=self->priv->length*self->priv->tracks;
  BtPattern ** const patterns=self->priv->patterns;
  BtMachine ** const machines=self->priv->machines;
  const gulong count=MIN(old_tracks,self->priv->tracks);

  GST_DEBUG("resize tracks %lu -> %lu to new_data_count=%lu",old_tracks,self->priv->tracks,new_data_count);

  // allocate new space
  if((self->priv->patterns=(BtPattern **)g_try_new0(GValue,new_data_count))) {
    if(patterns) {
      gulong i;
      BtPattern **src,**dst;

      // copy old values over
      src=patterns;
      dst=self->priv->patterns;
      for(i=0;i<self->priv->length;i++) {
        memcpy(dst,src,count*sizeof(gpointer));
        src=&src[old_tracks];
        dst=&dst[self->priv->tracks];
      }
      // free old data
      if(old_tracks>self->priv->tracks) {
        gulong j,k;
        for(i=0;i<self->priv->length;i++) {
          k=i*old_tracks+self->priv->tracks;
          for(j=self->priv->tracks;j<old_tracks;j++,k++) {
            g_object_try_unref(patterns[k]);
          }
        }
      }
      g_free(patterns);
    }
  }
  else {
    GST_INFO("extending sequence tracks from %lu to %lu failed : data_count=%lu = length=%lu * tracks=%lu",
      old_tracks,self->priv->tracks,
      new_data_count,self->priv->length,self->priv->tracks);
  }
  // allocate new space
  if(self->priv->tracks) {
    if((self->priv->machines=(BtMachine **)g_try_new0(gpointer,self->priv->tracks))) {
      if(machines) {
        // copy old values over
        memcpy(self->priv->machines,machines,count*sizeof(gpointer));
        // free old data
        if(old_tracks>self->priv->tracks) {
          gulong i;
          for(i=self->priv->tracks;i<old_tracks;i++) {
            GST_INFO("release machine %p,ref_count=%d for track %lu",
              machines[i],(machines[i]?G_OBJECT_REF_COUNT(machines[i]):-1),i);
            g_object_try_unref(machines[i]);
          }
        }
        g_free(machines);
      }
    }
    else self->priv->machines=NULL;
  }
  else {
    GST_INFO("extending sequence machines from %lu to %lu failed",old_tracks,self->priv->tracks);
  }
}

/*
 * bt_sequence_get_track_by_machine:
 * @self: the sequence to search in
 * @machine: the machine to find the first track for
 *
 * Gets the first track this @machine is on.
 *
 * Returns: the track-index or -1 if there is no track for this @machine.
 */
static glong bt_sequence_get_track_by_machine(const BtSequence * const self,const BtMachine * const machine) {
  gulong track;

  for(track=0;track<self->priv->tracks;track++) {
    if(self->priv->machines[track]==machine) {
      return((glong)track);
    }
  }
  return(-1);
}

/*
 * bt_sequence_limit_play_pos_internal:
 * @self: the sequence to trim the play position of
 *
 * Enforce the playback position to be within loop start and end or the song
 * bounds if there is no loop.
 */
static void bt_sequence_limit_play_pos_internal(const BtSequence * const self) {
  gulong old_play_pos,new_play_pos;

  g_object_get(self->priv->song,"play-pos",&old_play_pos,NULL);
  new_play_pos=bt_sequence_limit_play_pos(self,old_play_pos);
  if(new_play_pos!=old_play_pos) {
    GST_WARNING("limit play pos: %lu -> %lu",old_play_pos,new_play_pos);
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
 * @max_num: don't count more that this
 *
 * Count the number of times a pattern is in use. If @max_num is not -1, return
 * when the @max_num is reached.
 *
 * Returns: the pattern count
 */
static gulong bt_sequence_get_number_of_pattern_uses(const BtSequence * const self,const BtPattern * const this_pattern, const glong max_num) {
  gulong res=0;
  BtMachine *machine;
  BtPattern *that_pattern;
  gulong i,j=0;

  g_object_get((gpointer)this_pattern,"machine",&machine,NULL);
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      for(j=0;j<self->priv->length;j++) {
        // time has a pattern
        if((that_pattern=bt_sequence_get_pattern_unchecked(self,j,i))) {
          if(that_pattern==this_pattern) {
            res++;
            if(res==max_num) goto done;
          }
        }
      }
    }
  }
done:
  g_object_unref(machine);
  //GST_INFO("get pattern uses = %d",res);
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
static gboolean bt_sequence_test_pattern(const BtSequence * const self, const gulong time, const gulong track) {
  /*g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);
  g_return_val_if_fail(time<self->priv->length,FALSE);
  g_return_val_if_fail(track<self->priv->tracks,FALSE);
  */
  return(self->priv->patterns[time*self->priv->tracks+track]!=NULL);
}

static void bt_sequence_invalidate_param(const BtSequence * const self, const BtMachine * const machine, const gulong time, const gulong param) {
  GHashTable *hash,*param_hash=NULL;

  // mark region covered by change as damaged
  hash=g_hash_table_lookup(self->priv->damage,machine);
  if(!hash) {
    hash=g_hash_table_new_full(NULL,NULL,NULL,(GDestroyNotify)g_hash_table_destroy);
    g_hash_table_insert(self->priv->damage,(gpointer)machine,hash);
  }
  else {
    param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER(param));
  }
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
static void bt_sequence_invalidate_global_param(const BtSequence * const self, const BtMachine * const machine, const gulong time, const gulong param) {
  bt_sequence_invalidate_param(self,machine,time,param);
}

/*
 * bt_sequence_invalidate_voice_param:
 *
 * Marks the given tick for the voice param of the given machine and voice as invalid.
 */
static void bt_sequence_invalidate_voice_param(const BtSequence * const self, const BtMachine * const machine, const gulong time, const gulong voice, const gulong param) {
  bt_sequence_invalidate_param(self,machine,time,param);
}

/*
 * bt_sequence_invalidate_wire_param:
 *
 * Marks the given tick for the wire param of the given machine and voice as invalid.
 */
static void bt_sequence_invalidate_wire_param(const BtSequence * const self, const BtMachine * const machine, const gulong time, const BtWire *wire, const gulong param) {
  bt_sequence_invalidate_param(self,machine,time,param);
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
static void bt_sequence_invalidate_pattern_region(const BtSequence * const self, const gulong time, const gulong track, const BtPattern * const pattern) {
  BtMachine *machine;
  BtWire *wire;
  BtWirePattern *wire_pattern;
  GList *node;
  gulong i,j,k;
  gulong length;
  gulong global_params,voice_params,voices,wire_params;
  gulong param_offset;

  GST_DEBUG("invalidate pattern %p region for tick=%5lu, track=%3lu",pattern,time,track);
  /* @todo: if we load a song and thus set a lot of patterns, this is called a
   * lot. While doing this, there are a few thing that don't change. If we set
   * 100 patterns for one machine, we query, the machine, its parameters and its
   * list of incoming wires (and its pattern) again and again.
   *
   * It also involves a lot of creating and destoying of hashtables
   *
   * @todo: couldn't we just invalidate the regio where we have a event
   * bt_wire_pattern_test_event(wire_pattern,i,j);
   * bt_pattern_test_global_event(pattern,i,j);
   * bt_pattern_test_voice_event(pattern,i,k,j);
   */

  // determine region of change
  g_object_get((gpointer)pattern,"length",&length,"machine",&machine,NULL);
  if(G_UNLIKELY(!length)) {
    g_object_unref(machine);
    GST_WARNING("pattern has length 0");
    return;
  }
  g_assert(machine);
  g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,"voices",&voices,NULL);
  // check if from time+1 to time+length another pattern starts (in this track)
  for(i=1;((i<length) && (time+i<self->priv->length));i++) {
    if(bt_sequence_test_pattern(self,time+i,track)) break;
  }
  length=i;
  // mark region covered by new pattern as damaged
  // check wires
  param_offset=global_params+voices*voice_params;
  for(node=machine->dst_wires;node;node=g_list_next(node),param_offset++) {
    wire=BT_WIRE(node->data);
    g_object_get(wire,"num-params",&wire_params,NULL);
    if((wire_pattern=bt_wire_get_pattern(wire,pattern))) {
      for(i=0;i<length;i++) {
        // check wire params
        for(j=0;j<wire_params;j++,k++) {
          // mark region covered by change as damaged
          if(bt_wire_pattern_test_event(wire_pattern,i,j)) {
            bt_sequence_invalidate_wire_param(self,machine,time+i,wire,param_offset+j);
          }
        }
      }
      g_object_unref(wire_pattern);
    }
  }
  for(i=0;i<length;i++) {
    // check global params
    for(j=0;j<global_params;j++) {
      // mark region covered by change as damaged
      if(bt_pattern_test_global_event(pattern,i,j)) {
        bt_sequence_invalidate_global_param(self,machine,time+i,j);
      }
    }
    // check voices
    param_offset=global_params;
    for(k=0;k<voices;k++,param_offset+=voice_params) {
      // check voice params
      for(j=0;j<voice_params;j++) {
        // mark region covered by change as damaged
        if(bt_pattern_test_voice_event(pattern,i,k,j)) {
          bt_sequence_invalidate_voice_param(self,machine,time+i,k,param_offset+j);
        }
      }
    }
  }
  g_object_unref(machine);
  GST_DEBUG("done");
}

/*
 * bt_sequence_get_tick_time:
 * @self: the #BtSequence of the song
 * @tick: tick for the event
 *
 * Calculate a timestamp for the event quantizes to samples
 */
static GstClockTime bt_sequence_get_tick_time(const BtSequence * const self,const gulong tick) {
  GstClockTime timestamp=bt_sequence_get_bar_time(self)*tick;
#if 0
  /* @todo: get this from settings and follow changes */
  guint sample_rate=GST_AUDIO_DEF_RATE;
  guint64 samples=gst_util_uint64_scale(timestamp,(guint64)sample_rate,GST_SECOND);
  
  timestamp=gst_util_uint64_scale(samples,GST_SECOND,(guint64)sample_rate);
#endif
  return(timestamp);
}

/*
 * bt_sequence_repair_global_damage_entry:
 *
 * Lookup the global parameter value that needs to become effective for the given
 * time and update the machines global controller.
 */
static gboolean bt_sequence_repair_global_damage_entry(gpointer key,gpointer _value,gpointer user_data) {
  gconstpointer * const hash_params=(gconstpointer *)user_data;
  const BtSequence * const self=(BtSequence *)(hash_params[0]);
  const BtMachine * const machine=(BtMachine *)(hash_params[1]);
  const gulong param=GPOINTER_TO_UINT(hash_params[2]);
  const gulong tick=GPOINTER_TO_UINT(key);
  glong i,j;
  GValue *value=NULL,*cur_value;
  BtPattern *pattern;

  GST_LOG("repair global damage entry for tick=%5lu",tick);

  // find all patterns with tick-offsets that are intersected by the tick of the damage
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      // go from tick position upwards to find pattern for track
      pattern=NULL;
      for(j=tick;j>=0;j--) {
        if((pattern=bt_sequence_get_pattern_unchecked(self,j,i))) break;
      }
      if(pattern) {
        gulong length,pos=tick-j;

        g_object_get(pattern,"length",&length,NULL);
        if(pos<length) {
          // get value at tick position or NULL
          if((cur_value=bt_pattern_get_global_event_data(pattern,pos,param)) && G_IS_VALUE(cur_value)) {
            value=cur_value;
          }
        }
      }
    }
  }
  // set controller value
  //if(value) {
    const GstClockTime timestamp=bt_sequence_get_tick_time(self,tick);
    bt_machine_global_controller_change_value(machine,param,timestamp,value);
  //}
  return(TRUE);
}

/*
 * bt_sequence_repair_voice_damage_entry:
 *
 * Lookup the voice parameter value that needs to become effective for the given
 * time and update the machines voice controller.
 */
static gboolean bt_sequence_repair_voice_damage_entry(gpointer key,gpointer _value,gpointer user_data) {
  gconstpointer *hash_params=(gconstpointer *)user_data;
  const BtSequence * const self=(BtSequence *)(hash_params[0]);
  const BtMachine * const machine=(BtMachine *)(hash_params[1]);
  const gulong param=GPOINTER_TO_UINT(hash_params[2]);
  const gulong voice=GPOINTER_TO_UINT(hash_params[3]);
  const gulong tick=GPOINTER_TO_UINT(key);
  glong i,j;
  GValue *value=NULL,*cur_value;
  BtPattern *pattern;

  GST_LOG("repair voice damage entry for tick=%5lu",tick);

  // find all patterns with tick-offsets that are intersected by the tick of the damage
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      // go from tick position upwards to find pattern for track
      pattern=NULL;
      for(j=tick;j>=0;j--) {
        if((pattern=bt_sequence_get_pattern_unchecked(self,j,i))) break;
      }
      if(pattern) {
        gulong length,pos=tick-j;

        g_object_get(pattern,"length",&length,NULL);
        if(pos<length) {
          // get value at tick position or NULL
          if((cur_value=bt_pattern_get_voice_event_data(pattern,tick-j,voice,param)) && G_IS_VALUE(cur_value)) {
            value=cur_value;
          }
        }
      }
    }
  }
  // set controller value
  //if(value) {
    const GstClockTime timestamp=bt_sequence_get_tick_time(self,tick);
    bt_machine_voice_controller_change_value(machine,voice,param,timestamp,value);
  //}
  return(TRUE);
}

/*
 * bt_sequence_repair_wire_damage_entry:
 *
 * Lookup the wire parameter value that needs to become effective for the given
 * time and updates the wire controller.
 */
static gboolean bt_sequence_repair_wire_damage_entry(gpointer key,gpointer _value,gpointer user_data) {
  gconstpointer *hash_params=(gconstpointer *)user_data;
  const BtSequence * const self=(BtSequence *)(hash_params[0]);
  const BtMachine * const machine=(BtMachine *)(hash_params[1]);
  const gulong param=GPOINTER_TO_UINT(hash_params[2]);
  const BtWire *wire=BT_WIRE(hash_params[3]);
  const gulong tick=GPOINTER_TO_UINT(key);
  glong i,j;
  GValue *value=NULL,*cur_value;
  BtPattern *pattern;

  GST_LOG("repair wire damage entry for tick=%5lu",tick);

  // find all patterns with tick-offsets that are intersected by the tick of the damage
  for(i=0;i<self->priv->tracks;i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      // go from tick position upwards to find pattern for track
      pattern=NULL;
      for(j=tick;j>=0;j--) {
        if((pattern=bt_sequence_get_pattern_unchecked(self,j,i))) break;
      }
      if(pattern) {
        gulong length,pos=tick-j;

        g_object_get(pattern,"length",&length,NULL);
        if(pos<length) {
          BtWirePattern *wire_pattern=bt_wire_get_pattern(wire,pattern);
          // get value at tick position or NULL
          if((cur_value=bt_wire_pattern_get_event_data(wire_pattern,tick-j,param)) && G_IS_VALUE(cur_value)) {
            value=cur_value;
          }
          g_object_unref(wire_pattern);
        }
      }
    }
  }
  // set controller value
  //if(value) {
    const GstClockTime timestamp=bt_sequence_get_tick_time(self,tick);
    bt_wire_controller_change_value(wire,param,timestamp,value);
  //}
  return(TRUE);
}

static void bt_sequence_calculate_wait_per_position(const BtSequence * const self) {
  BtSongInfo * const song_info;
  gulong beats_per_minute,ticks_per_beat;

  g_object_get((gpointer)(self->priv->song),"song-info",&song_info,NULL);
  g_object_get(song_info,"tpb",&ticks_per_beat,"bpm",&beats_per_minute,NULL);
  /* the number of pattern-events for one playline-step,
   * when using 4 ticks_per_beat then
   *   for 4/4 bars it is 16 (standart dance rhythm)
   *   for 3/4 bars it is 12 (walz)
   * correlation of values:
   *   bpm    60     60  
   *   tpb     4      8
   *   tpm   240    480 bpm*tpb
   *   wpp  0.25  0.125 60/tpm
   */

  const gdouble ticks_per_minute=(gdouble)(beats_per_minute*ticks_per_beat);
  //self->priv->wait_per_position=((GST_SECOND*60)/(GstClockTime)ticks_per_minute);
  //self->priv->wait_per_position=(GST_SECOND*60.0)/ticks_per_minute;
  self->priv->wait_per_position=(GstClockTime)(0.5+((GST_SECOND*60.0)/ticks_per_minute));

  // release the references
  g_object_unref(song_info);

  GST_INFO("calculating songs bar-time %"G_GUINT64_FORMAT,self->priv->wait_per_position);
}

//-- event handler

/*
 * bt_sequence_on_pattern_global_param_changed:
 *
 * Invalidate the global param change in all pattern uses.
 */
static void bt_sequence_on_pattern_global_param_changed(const BtPattern * const pattern, const gulong tick, const gulong param, gconstpointer user_data) {
  const BtSequence * const self=BT_SEQUENCE(user_data);
  BtMachine *this_machine;
  gulong i,j,k;

  g_object_get((gpointer)pattern,"machine",&this_machine,NULL);

  GST_INFO("pattern %p changed",pattern);

  // for all occurences of pattern
  for(i=0;i<self->priv->tracks;i++) {
    BtMachine * const that_machine=bt_sequence_get_machine_unchecked(self,i);
    if(that_machine==this_machine) {
      for(j=0;j<self->priv->length;j++) {
        BtPattern * const that_pattern=bt_sequence_get_pattern_unchecked(self,j,i);
        if(that_pattern==pattern) {
          // check if pattern plays long enough for the damage to happen
          for(k=1;((k<tick) && (j+k<self->priv->length));k++) {
            if(bt_sequence_test_pattern(self,j+k,i)) break;
          }
          // for tick==0 we always invalidate
          if(!tick || k==tick) {
            bt_sequence_invalidate_global_param(self,this_machine,j+tick,param);
          }
        }
      }
    }
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
static void bt_sequence_on_pattern_voice_param_changed(const BtPattern * const pattern, const gulong tick, const gulong voice, const gulong param, gconstpointer user_data) {
  const BtSequence * const self=BT_SEQUENCE(user_data);
  BtMachine *this_machine;
  gulong i,j,k;
  
  g_object_get((gpointer)pattern,"machine",&this_machine,NULL);
  // for all occurences of pattern
  for(i=0;i<self->priv->tracks;i++) {
    BtMachine * const that_machine=bt_sequence_get_machine_unchecked(self,i);
    if(that_machine==this_machine) {
      for(j=0;j<self->priv->length;j++) {
        BtPattern * const that_pattern=bt_sequence_get_pattern_unchecked(self,j,i);
        if(that_pattern==pattern) {
          // check if pattern plays long enough for the damage to happen
          for(k=1;((k<tick) && (j+k<self->priv->length));k++) {
            if(bt_sequence_test_pattern(self,j+k,i)) break;
          }
          // for tick==0 we always invalidate
          if(!tick || k==tick) {
            gulong global_params,voice_params,param_offset;
            g_object_get(this_machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
            param_offset=global_params+voice*voice_params;
            bt_sequence_invalidate_voice_param(self,this_machine,j+tick,voice,param_offset+param);
          }
        }
      }
    }
  }
  g_object_unref(this_machine);
  // repair damage
  bt_sequence_repair_damage(self);
}

/*
 * bt_sequence_on_pattern_wire_param_changed:
 *
 * Invalidate the wire param change in all pattern uses.
 */
static void bt_sequence_on_wire_pattern_wire_param_changed(const BtWirePattern * const wire_pattern, const gulong tick, const BtWire * const wire, const gulong param, gconstpointer user_data) {
  const BtSequence * const self=BT_SEQUENCE(user_data);
  BtPattern *pattern;
  BtMachine *this_machine;
  gulong i,j,k;
  
  g_object_get((gpointer)wire_pattern,"pattern",&pattern,NULL);
  g_object_get(pattern,"machine",&this_machine,NULL);
  // for all occurences of pattern
  for(i=0;i<self->priv->tracks;i++) {
    BtMachine * const that_machine=bt_sequence_get_machine_unchecked(self,i);
    if(that_machine==this_machine) {
      for(j=0;j<self->priv->length;j++) {
        BtPattern * const that_pattern=bt_sequence_get_pattern_unchecked(self,j,i);
        if(that_pattern==pattern) {
          // check if pattern plays long enough for the damage to happen
          for(k=1;((k<tick) && (j+k<self->priv->length));k++) {
            if(bt_sequence_test_pattern(self,j+k,i)) break;
          }
          // for tick==0 we always invalidate
          if(!tick || k==tick) {
            gulong global_params,voice_params,voices,param_offset;
            gulong l;
            GList *node;

            g_object_get(this_machine,"global-params",&global_params,"voice-params",&voice_params,"voices",&voices,NULL);
            // get the index for the given wires
            for(l=0,node=this_machine->dst_wires;node;node=g_list_next(node),l++) {
              if(node->data==(gpointer)wire) break;
            }
            param_offset=(global_params+voices*voice_params)+(BT_WIRE_MAX_NUM_PARAMS*l);
            bt_sequence_invalidate_wire_param(self,this_machine,j+tick,wire,param_offset+param);
          }
        }
      }
    }
  }
  g_object_unref(this_machine);
  g_object_unref(pattern);
  // repair damage
  bt_sequence_repair_damage(self);
}

static void bt_sequence_on_pattern_changed(const BtPattern * const pattern, gconstpointer user_data) {
  const BtSequence * const self=BT_SEQUENCE(user_data);
  BtMachine *machine;
  gulong i,j;

  GST_DEBUG("repair damage after a pattern %p has been changed",pattern);
  g_object_get((gpointer)pattern,"machine",&machine,NULL);

  // for all tracks
  for(i=0;i<self->priv->tracks;i++) {
    BtMachine * const that_machine=bt_sequence_get_machine_unchecked(self,i);
    // does the track belong to the given machine?
    if(that_machine==machine) {
      // for all occurance of pattern
      for(j=0;j<self->priv->length;j++) {
        BtPattern * const that_pattern=bt_sequence_get_pattern_unchecked(self,j,i);
        if(that_pattern==pattern) {
          // mark region covered by change as damaged
          bt_sequence_invalidate_pattern_region(self,j,i,pattern);
        }
      }
    }
  }
  g_object_unref(machine);
  // repair damage
  bt_sequence_repair_damage(self);
  GST_DEBUG("Done");
}

static void bt_sequence_on_wire_pattern_changed(const BtWirePattern * const wire_pattern, gconstpointer user_data) {
  const BtSequence * const self=BT_SEQUENCE(user_data);
  BtPattern *pattern;
  BtMachine *machine;
  gulong i,j;

  GST_DEBUG("repair damage after a wire-pattern %p has been changed",wire_pattern); 
  g_object_get((gpointer)wire_pattern,"pattern",&pattern,NULL);
  g_object_get(pattern,"machine",&machine,NULL);

  // for all tracks
  for(i=0;i<self->priv->tracks;i++) {
    BtMachine * const that_machine=bt_sequence_get_machine_unchecked(self,i);
    // does the track belong to the given machine?
    if(that_machine==machine) {
      // for all occurance of pattern
      for(j=0;j<self->priv->length;j++) {
        BtPattern * const that_pattern=bt_sequence_get_pattern_unchecked(self,j,i);
        if(that_pattern==pattern) {
          // mark region covered by change as damaged
          bt_sequence_invalidate_pattern_region(self,j,i,pattern);
        }
      }
    }
  }
  g_object_unref(machine);
  g_object_unref(pattern);
  // repair damage
  bt_sequence_repair_damage(self);
  GST_DEBUG("Done");
}

/*
 * bt_sequence_on_pattern_removed:
 *
 * Invalidate the pattern region in all occurences.
 */
static void bt_sequence_on_pattern_removed(const BtMachine * const machine, const BtPattern * const pattern, gconstpointer user_data) {
  const BtSequence * const self=BT_SEQUENCE(user_data);
  gulong i,j;
  gboolean sequence_changed=FALSE;

  GST_DEBUG("repair damage after a pattern %p has been removed from machine %p",pattern,machine);

  // for all tracks
  for(i=0;i<self->priv->tracks;i++) {
    BtMachine * const that_machine=bt_sequence_get_machine_unchecked(self,i);
    // does the track belong to the given machine?
    if(that_machine==machine) {
      // for all occurance of pattern
      for(j=0;j<self->priv->length;j++) {
        BtPattern * const that_pattern=bt_sequence_get_pattern_unchecked(self,j,i);
        if(that_pattern==pattern) {
          sequence_changed|=bt_sequence_set_pattern_quick(self,j,i,NULL);
        }
      }
    }
  }
  if(sequence_changed) {
    // repair damage
    bt_sequence_repair_damage(self);
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  GST_DEBUG("Done");
}

static void on_wire_pattern_added(BtWire *wire,BtWirePattern *wire_pattern,gpointer user_data) {
  BtSequence *self=BT_SEQUENCE(user_data);

  g_signal_connect(wire_pattern,"param-changed",G_CALLBACK(bt_sequence_on_wire_pattern_wire_param_changed),(gpointer)self);
  g_signal_handlers_disconnect_matched(wire,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_wire_pattern_added,NULL);
}

static void on_wire_added(BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtSequence *self=BT_SEQUENCE(user_data);
  BtMachine *machine;
  BtPattern *pattern;
  GList *patterns,*node;
  
  g_object_get(wire,"dst",&machine,NULL);
  g_object_get(machine,"patterns",&patterns,NULL);
  // go over all patterns of the wire.dst
  for(node=patterns;node;node=g_list_next(node)) {
    pattern=BT_PATTERN(node->data);
    if(bt_sequence_get_number_of_pattern_uses(self,pattern,1)>0) {
      // we need to wait for the first wire-pattern
      g_signal_connect(wire,"pattern-created",G_CALLBACK(on_wire_pattern_added),(gpointer)self);
    }
  }
  g_list_free(patterns);
  g_object_unref(machine);
}

static void on_wire_removed(BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtSequence *self=BT_SEQUENCE(user_data);
  BtMachine *machine;
  BtPattern *pattern;
  BtWirePattern *wire_pattern;
  GList *patterns,*node;
  
  g_object_get(wire,"dst",&machine,NULL);
  g_object_get(machine,"patterns",&patterns,NULL);
  // go over all patterns of the wire.dst
  for(node=patterns;node;node=g_list_next(node)) {
    pattern=BT_PATTERN(node->data);
    if(bt_sequence_get_number_of_pattern_uses(self,pattern,1)>0) {
      if((wire_pattern=bt_wire_get_pattern(wire,pattern))) {
        g_signal_handlers_disconnect_matched(wire_pattern,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_wire_pattern_wire_param_changed,NULL);
        g_object_unref(wire_pattern);
      }
    }
  }
  g_list_free(patterns);
  g_object_unref(machine);
}

//-- helper methods

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
BtSequence *bt_sequence_new(const BtSong * const song) {
  return(BT_SEQUENCE(g_object_new(BT_TYPE_SEQUENCE,"song",song,NULL)));
}

//-- methods

/**
 * bt_sequence_repair_damage:
 * @self: the #BtSequence
 *
 * Works through the repair queue and rebuilds controller queues, where needed.
 *
 * There is usualy no need to call that manualy. Only call after soing mass
 * updates using bt_sequence_set_pattern_quick() functions.
 *
 * Since: 0.5
 */
void bt_sequence_repair_damage(const BtSequence * const self) {
  gulong i,j,k,l;
  BtMachine *machine;
  BtWire *wire;
  GList *node;
  gulong global_params,voice_params,voices,wire_params;
  GHashTable *hash,*param_hash;
  gconstpointer hash_params[4];

  GST_DEBUG("repair damage");
  hash_params[0]=(gpointer)self;

  // repair damage
  for(i=0;i<self->priv->tracks;i++) {
    if((machine=bt_sequence_get_machine_unchecked(self,i))) {
      GST_DEBUG("check damage for track %lu",i);
      if((hash=g_hash_table_lookup(self->priv->damage,machine))) {
        GST_DEBUG("repair damage for track %lu",i);
        g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,"voices",&voices,NULL);
        hash_params[1]=machine;

        // repair damage of wires
        for(k=0,node=machine->dst_wires;node;node=g_list_next(node)) {
          wire=BT_WIRE(node->data);
          g_object_get(wire,"num-params",&wire_params,NULL);
          hash_params[3]=GUINT_TO_POINTER(wire);
          // repair damage of wire params
          for(j=0;j<wire_params;j++) {
            l=(global_params+voices*voice_params+BT_WIRE_MAX_NUM_PARAMS*k)+j;
            if((param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER(l)))) {
              hash_params[2]=GUINT_TO_POINTER(j);
              g_hash_table_foreach_remove(param_hash,bt_sequence_repair_wire_damage_entry,hash_params);
              g_hash_table_remove(hash,GUINT_TO_POINTER(l));
            }
          }
        }

        // repair damage of global params
        for(j=0;j<global_params;j++) {
          if((param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER(j)))) {
            hash_params[2]=GUINT_TO_POINTER(j);
            g_hash_table_foreach_remove(param_hash,bt_sequence_repair_global_damage_entry,&hash_params);
            g_hash_table_remove(hash,GUINT_TO_POINTER(j));
          }
        }

        // repair damage of voices
        for(k=0;k<voices;k++) {
          hash_params[3]=GUINT_TO_POINTER(k);
          // repair damage of voice params
          for(j=0;j<voice_params;j++) {
            l=(global_params+k*voice_params)+j;
            if((param_hash=g_hash_table_lookup(hash,GUINT_TO_POINTER(l)))) {
              hash_params[2]=GUINT_TO_POINTER(j);
              g_hash_table_foreach_remove(param_hash,bt_sequence_repair_voice_damage_entry,hash_params);
              g_hash_table_remove(hash,GUINT_TO_POINTER(l));
            }
          }
        }
        
        g_hash_table_remove(self->priv->damage,machine);
      }
    }
  }
}

/**
 * bt_sequence_get_machine:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 *
 * Fetches the #BtMachine for the given @track. Unref when done.
 *
 * Returns: a reference to the #BtMachine pointer or %NULL in case of an error
 */
BtMachine *bt_sequence_get_machine(const BtSequence * const self,const gulong track) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),NULL);

  if(track>=self->priv->tracks) return(NULL);

  GST_DEBUG("getting machine : %p,ref_count=%d",self->priv->machines[track],(self->priv->machines[track]?G_OBJECT_REF_COUNT(self->priv->machines[track]):-1));
  return(g_object_try_ref(bt_sequence_get_machine_unchecked(self,track)));
}

/*
@todo shouldn't we better make self->priv->tracks a readonly property and offer methods to insert/remove tracks
as it should not be allowed to change the machine later on
*/

/**
 * bt_sequence_add_track:
 * @self: the #BtSequence that holds the tracks
 * @machine: the #BtMachine
 *
 * Adds a new track with the @machine to the end.
 *
 * Returns: %TRUE for success
 */
gboolean bt_sequence_add_track(const BtSequence * const self, const BtMachine * const machine) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);
  g_return_val_if_fail(BT_IS_MACHINE(machine),FALSE);

  const gulong track=self->priv->tracks;
  GST_INFO("add track for machine %p,ref_count=%d at position %lu",machine,G_OBJECT_REF_COUNT(machine),track);

  g_object_set((gpointer)self,"tracks",(gulong)(track+1),NULL);
  self->priv->machines[track]=g_object_ref((gpointer)machine);
  // check if that has already been connected
  if(!g_signal_handler_find((gpointer)machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_sequence_on_pattern_removed,(gpointer)self)) {
    g_signal_connect((gpointer)machine,"pattern-removed",G_CALLBACK(bt_sequence_on_pattern_removed),(gpointer)self);
  }
  return(TRUE);
}

/**
 * bt_sequence_remove_track_by_ix:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 *
 * Removes the specified @track.
 *
 * Returns: %TRUE for success
 */
gboolean bt_sequence_remove_track_by_ix(const BtSequence * const self, const gulong track) {
  BtPattern **src,**dst;
  gulong i;
  glong other_track;

  g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);
  g_return_val_if_fail(track<self->priv->tracks,FALSE);

  const gulong count=(self->priv->tracks-1)-track;
  GST_INFO("remove track %lu/%lu (shift %lu tracks)",track,self->priv->tracks,count);

  src=&self->priv->patterns[track+1];
  dst=&self->priv->patterns[track];
  for(i=0;i<self->priv->length;i++) {
    // unref patterns
    if(*dst) {
      GST_INFO("unref pattern: %p,refs=%d at timeline %lu", *dst,G_OBJECT_REF_COUNT(*dst),i);
      g_object_unref(*dst);
    }
    if(count) {
      memmove(dst,src,count*sizeof(gpointer));
    }
    src[count-1]=NULL;
    src=&src[self->priv->tracks];
    dst=&dst[self->priv->tracks];
  }
  // disconnect signal handler if its the last of this machine
  other_track=bt_sequence_get_track_by_machine(self,self->priv->machines[track]);
  if(other_track==-1 || other_track==track) {
    g_signal_handlers_disconnect_matched(self->priv->machines[track],G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA, 0, 0, NULL, bt_sequence_on_pattern_removed, (gpointer)self);
  }
  GST_INFO("and release machine %p,ref_count=%d",self->priv->machines[track],G_OBJECT_REF_COUNT(self->priv->machines[track]));
  g_object_unref(self->priv->machines[track]);
  memmove(&self->priv->machines[track],&self->priv->machines[track+1],count*sizeof(gpointer));
  self->priv->machines[self->priv->tracks-1]=NULL;
  g_object_set((gpointer)self,"tracks",(gulong)(self->priv->tracks-1),NULL);

  GST_INFO("done");
  return(TRUE);
}

/**
 * bt_sequence_move_track_left:
 * @self: the #BtSequence that holds the tracks
 * @track: the track to move
 *
 * Move the selected track on column left.
 *
 * Returns: @TRUE for success
 */
gboolean bt_sequence_move_track_left(const BtSequence * const self, const gulong track) {
  BtPattern *pattern;
  BtMachine *machine;
  gulong i,ix=track;

  g_return_val_if_fail(track>0,FALSE);
  
  for(i=0;i<self->priv->length;i++) {
    pattern=self->priv->patterns[ix];
    self->priv->patterns[ix]=self->priv->patterns[ix-1];
    self->priv->patterns[ix-1]=pattern;
    ix+=self->priv->tracks;
  }
  machine=self->priv->machines[track];
  self->priv->machines[track]=self->priv->machines[track-1];
  self->priv->machines[track-1]=machine;
  
  return(TRUE);
}

/**
 * bt_sequence_move_track_right:
 * @self: the #BtSequence that holds the tracks
 * @track: the track to move
 *
 * Move the selected track on column left.
 *
 * Returns: @TRUE for success
 */
gboolean bt_sequence_move_track_right(const BtSequence * const self, const gulong track) {
  BtPattern *pattern;
  BtMachine *machine;
  gulong i,ix=track;

  g_return_val_if_fail(track<(self->priv->tracks-1),FALSE);
  
  for(i=0;i<self->priv->length;i++) {
    pattern=self->priv->patterns[ix];
    self->priv->patterns[ix]=self->priv->patterns[ix+1];
    self->priv->patterns[ix+1]=pattern;
    ix+=self->priv->tracks;
  }
  machine=self->priv->machines[track];
  self->priv->machines[track]=self->priv->machines[track+1];
  self->priv->machines[track+1]=machine;

  return(TRUE);
}

/**
 * bt_sequence_remove_track_by_machine:
 * @self: the #BtSequence that holds the tracks
 * @machine: the #BtMachine
 *
 * Removes all tracks that belong the the given @machine.
 *
 * Returns: %TRUE for success
 */
gboolean bt_sequence_remove_track_by_machine(const BtSequence * const self,const BtMachine * const machine) {
  gboolean res=TRUE;
  glong track;

  g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);
  g_return_val_if_fail(BT_IS_MACHINE(machine),FALSE);

  GST_INFO("remove tracks for machine %p,ref_count=%d",machine,G_OBJECT_REF_COUNT(machine));

  // do bt_sequence_remove_track_by_ix() for each occurance
  while(((track=bt_sequence_get_track_by_machine(self,machine))>-1) && res) {
    res=bt_sequence_remove_track_by_ix(self,(gulong)track);
  }
  GST_INFO("removed tracks for machine %p,ref_count=%d,res=%d",machine,G_OBJECT_REF_COUNT(machine),res);
  return(res);
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
gchar *bt_sequence_get_label(const BtSequence * const self, const gulong time) {
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
void bt_sequence_set_label(const BtSequence * const self, const gulong time, const gchar * const label) {
  g_return_if_fail(BT_IS_SEQUENCE(self));
  g_return_if_fail(time<self->priv->length);

  GST_DEBUG("set label for time %lu",time);

  g_free(self->priv->labels[time]);
  self->priv->labels[time]=g_strdup(label);

  bt_song_set_unsaved(self->priv->song,TRUE);
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
BtPattern *bt_sequence_get_pattern(const BtSequence * const self, const gulong time, const gulong track) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),NULL);
  g_return_val_if_fail(time<self->priv->length,NULL);
  g_return_val_if_fail(track<self->priv->tracks,NULL);

  //GST_DEBUG("get pattern at time %d, track %d",time, track);
  return(g_object_try_ref(bt_sequence_get_pattern_unchecked(self,time,track)));
}

/**
 * bt_sequence_set_pattern_quick:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 * @pattern: the #BtPattern or %NULL to unset
 *
 * A quick version of bt_sequence_set_pattern() that does not repair damaged
 * area. Useful when doing mass updates.
 *
 * Returns: %TRUE if a change has been made. One should call
 * bt_sequence_repair_damage() in that case.
 *
 * Since: 0.5
 */
gboolean bt_sequence_set_pattern_quick(const BtSequence * const self, const gulong time, const gulong track, const BtPattern * const pattern) {
  BtMachine * const machine;
  BtWire *wire;
  BtWirePattern *wire_pattern;
  GList *node;
  gboolean changed=FALSE;

  if(pattern) {
    g_return_val_if_fail(BT_IS_PATTERN(pattern),FALSE);
    g_object_get((gpointer)pattern,"machine",&machine,NULL);
    if(self->priv->machines[track]!=machine) {
      GST_WARNING("adding a pattern to a track with different machine!");
      g_object_unref(machine);
      return(FALSE);
    }
    g_object_unref(machine);
  }

  const gulong index=time*self->priv->tracks+track;
  BtPattern *old_pattern=self->priv->patterns[index];

  GST_DEBUG("set pattern from %p to %p for time %lu, track %lu",
    self->priv->patterns[index],pattern,time,track);
  
  // take out the old pattern
  if(old_pattern) {
    GST_DEBUG("clean up for old pattern");

    // detach a signal handler if this was the last usage
    // @todo: cache the number of pattern uses within the pattern
    //   or have a quick variant that returns one the 2nd pattern
    if(bt_sequence_get_number_of_pattern_uses(self,old_pattern,2)==1) {
      g_signal_handlers_disconnect_matched(old_pattern,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_pattern_global_param_changed,NULL);
      g_signal_handlers_disconnect_matched(old_pattern,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_pattern_voice_param_changed,NULL);
      g_signal_handlers_disconnect_matched(old_pattern,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_pattern_changed,NULL);

      g_object_get(old_pattern,"machine",&machine,NULL);
      for(node=machine->dst_wires;node;node=g_list_next(node)) {
        wire=BT_WIRE(node->data);
        if((wire_pattern=bt_wire_get_pattern(wire,old_pattern))) {
          g_signal_handlers_disconnect_matched(wire_pattern,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_wire_pattern_wire_param_changed,NULL);
          g_signal_handlers_disconnect_matched(wire_pattern,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_sequence_on_wire_pattern_changed,NULL);
          g_object_unref(wire_pattern);
        }
      }
      g_object_unref(machine);
    }
    // mark region covered by old pattern as damaged
    bt_sequence_invalidate_pattern_region(self,time,track,self->priv->patterns[index]);
    changed=TRUE;
    g_object_unref(self->priv->patterns[index]);
    self->priv->patterns[index]=NULL;
  }
  if(pattern) {
    GST_DEBUG("set new pattern");
    // enter the new pattern
    self->priv->patterns[index]=g_object_ref((gpointer)pattern);
    //g_object_add_weak_pointer((gpointer)pattern,(gpointer *)(&self->priv->patterns[index]));

    // attach a signal handler if this is the first usage
    // @todo: cache the number of pattern uses within the pattern
    //   or have a quick variant that returns once the 2nd pattern use is found
    if(bt_sequence_get_number_of_pattern_uses(self,pattern,2)==1) {
      //GST_INFO("subscribing to changes for pattern %p",pattern);
      g_signal_connect((gpointer)pattern,"global-param-changed",G_CALLBACK(bt_sequence_on_pattern_global_param_changed),(gpointer)self);
      g_signal_connect((gpointer)pattern,"voice-param-changed",G_CALLBACK(bt_sequence_on_pattern_voice_param_changed),(gpointer)self);
      g_signal_connect((gpointer)pattern,"pattern-changed",G_CALLBACK(bt_sequence_on_pattern_changed),(gpointer)self);

      g_object_get((gpointer)pattern,"machine",&machine,NULL);
      for(node=machine->dst_wires;node;node=g_list_next(node)) {
        wire=BT_WIRE(node->data);
        if((wire_pattern=bt_wire_get_pattern(wire,pattern))) {
          g_signal_connect(wire_pattern,"param-changed",G_CALLBACK(bt_sequence_on_wire_pattern_wire_param_changed),(gpointer)self);
          g_signal_connect(wire_pattern,"pattern-changed",G_CALLBACK(bt_sequence_on_wire_pattern_changed),(gpointer)self);
          g_object_unref(wire_pattern);
        }
        else {
          // we need to wait for the first wire-pattern
          g_signal_connect(wire,"pattern-created",G_CALLBACK(on_wire_pattern_added),(gpointer)self);
        }
      }
      g_object_unref(machine);
    }
    // mark region covered by new pattern as damaged
    bt_sequence_invalidate_pattern_region(self,time,track,pattern);
    changed=TRUE;
  }
  GST_DEBUG("done: %d",changed);
  return(changed);
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
void bt_sequence_set_pattern(const BtSequence * const self, const gulong time, const gulong track, const BtPattern * const pattern) {
  g_return_if_fail(BT_IS_SEQUENCE(self));
  g_return_if_fail(time<self->priv->length);
  g_return_if_fail(track<self->priv->tracks);
  g_return_if_fail(self->priv->machines[track]);

  if(bt_sequence_set_pattern_quick(self,time,track,pattern)) {
    // repair damage
    bt_sequence_repair_damage(self);
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
}

/**
 * bt_sequence_get_bar_time:
 * @self: the #BtSequence of the song
 *
 * Calculates the length of one sequence bar in microseconds.
 * Divide it by %G_USEC_PER_SEC to get it in milliseconds.
 *
 * Returns: the length of one sequence bar in microseconds
 */
/* @todo rename to bt_sequence_get_tick_duration(), or turn into property ?*/ 
GstClockTime bt_sequence_get_bar_time(const BtSequence * const self) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),0.0);
  
  if(G_UNLIKELY(self->priv->wait_per_position==0.0)) {
    bt_sequence_calculate_wait_per_position(self);
  }
  
  return(self->priv->wait_per_position);
}

/**
 * bt_sequence_get_loop_time:
 * @self: the #BtSequence of the song
 *
 * Calculates the length of the song loop in microseconds.
 * Divide it by %G_USEC_PER_SEC to get it in milliseconds.
 *
 * Returns: the length of the song loop in microseconds
 */
GstClockTime bt_sequence_get_loop_time(const BtSequence * const self) {
  g_return_val_if_fail(BT_IS_SEQUENCE(self),0);
  
  GST_DEBUG("%lu .. %lu : %"GST_TIME_FORMAT" : %"GST_TIME_FORMAT,
    self->priv->play_start,
    self->priv->play_end,
    GST_TIME_ARGS(self->priv->wait_per_position),
    GST_TIME_ARGS((self->priv->play_end-self->priv->play_start)*self->priv->wait_per_position));

  const GstClockTime res=bt_sequence_get_tick_time(self,self->priv->play_end-self->priv->play_start);
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
gulong bt_sequence_limit_play_pos(const BtSequence * const self, gulong play_pos) {
  if(play_pos>self->priv->play_end) {
    play_pos=self->priv->play_end;
  }
  if(play_pos<self->priv->play_start) {
    play_pos=self->priv->play_start;
  }
  return(play_pos);
}

/**
 * bt_sequence_is_pattern_used:
 * @self: the sequence to check for pattern use
 * @pattern: the pattern to check for
 *
 * Checks if the @pattern is used in the sequence.
 *
 * Returns: %TRUE if @pattern is used.
 */
gboolean bt_sequence_is_pattern_used(const BtSequence * const self,const BtPattern * const pattern) {
  gboolean res=FALSE;
  BtMachine *machine;
  BtPattern *that_pattern;
  gulong i,j=0;

  g_return_val_if_fail(BT_IS_SEQUENCE(self),0);
  g_return_val_if_fail(BT_IS_PATTERN(pattern),0);

  g_object_get((gpointer)pattern,"machine",&machine,NULL);
  for(i=0;(i<self->priv->tracks && !res);i++) {
    // track uses the same machine
    if(self->priv->machines[i]==machine) {
      for(j=0;(j<self->priv->length && !res);j++) {
        // time has a pattern
        if((that_pattern=bt_sequence_get_pattern_unchecked(self,j,i))) {
          if(that_pattern==pattern) res=TRUE;
        }
      }
    }
  }
  g_object_unref(machine);
  GST_INFO("is pattern used = %d",res);
  return(res);
}

/*
 * insert_rows:
 * @self: the sequence
 * @time: the postion to insert at
 * @track: the track
 * @rows: the number of rows to insert
 *
 * Insert one empty row for given @track.
 */
static void insert_rows(const BtSequence * const self, const gulong time, const gulong track, const gulong rows) {
  BtPattern **src=&self->priv->patterns[track+self->priv->tracks*(self->priv->length-(1+rows))];
  BtPattern **dst=&self->priv->patterns[track+self->priv->tracks*(self->priv->length-1)];
  gulong i;
  
  /* ins 3     0 1 2 3 4
   * 0 a       a a a a . 
   * 1 b       b b b . .
   * 2 c       c c . . .
   * 3 d       d . . . a
   * 4 e src   . . . b b
   * 5 f    \  f f c c c
   * 6 g     v g d d d d
   * 7 h dst e e e e e e
   */
  
  for(i=1;i<=rows;i++) {
    g_object_try_unref(src[i*self->priv->tracks]);
  }
  /* copy patterns and move upwards */
  for(i=(self->priv->length-1);i>=(time+rows);i--) {
    if(*src) {
      bt_sequence_invalidate_pattern_region(self,i-rows,track,*src);
      bt_sequence_invalidate_pattern_region(self,i,track,*src);
    }
    if(*dst) bt_sequence_invalidate_pattern_region(self,i,track,*dst);
    *dst=*src;
    *src=NULL;
    src-=self->priv->tracks;
    dst-=self->priv->tracks;
  }
}

/**
 * bt_sequence_insert_rows:
 * @self: the sequence
 * @time: the postion to insert at
 * @track: the track
 * @rows: the number of rows to insert
 *
 * Insert one empty row for given @track.
 *
 * Since: 0.3
 */
void bt_sequence_insert_rows(const BtSequence * const self, const gulong time, const gulong track, const gulong rows) {
  g_return_if_fail(BT_IS_SEQUENCE(self));
  
  GST_INFO("insert %lu rows at %lu,%lu", rows, time, track);

  insert_rows(self,time,track,rows);
  bt_sequence_repair_damage(self);
}

/**
 * bt_sequence_insert_full_rows:
 * @self: the sequence
 * @time: the postion to insert at
 * @rows: the number of rows to insert
 *
 * Insert one empty row for all tracks.
 *
 * Since: 0.3
 */
void bt_sequence_insert_full_rows(const BtSequence * const self, const gulong time, const gulong rows) {
  g_return_if_fail(BT_IS_SEQUENCE(self));

  gulong j=0;

  GST_DEBUG("insert %lu full-rows at %lu / %lu", rows, time, self->priv->length);

  g_object_set((gpointer)self,"length",self->priv->length+rows,NULL);

  // shift label down
  memmove(&self->priv->labels[time+rows],&self->priv->labels[time],((self->priv->length-rows)-time)*sizeof(gpointer));
  for(j=0;j<rows;j++) {
    self->priv->labels[time+j]=NULL;
  }
  for(j=0;j<self->priv->tracks;j++) {
    insert_rows(self,time,j,rows);
  }
  bt_sequence_repair_damage(self);
}

/*
 * delete_rows:
 * @self: the sequence
 * @time: the postion to delete
 * @track: the track
 * @rows: the number of rows to remove
 *
 * Delete row for given @track.
 */
static void delete_rows(const BtSequence * const self, const gulong time, const gulong track, const gulong rows) {
  BtPattern **src=&self->priv->patterns[track+self->priv->tracks*(time+rows)];
  BtPattern **dst=&self->priv->patterns[track+self->priv->tracks*time];
  gulong i;

  /* del 3     0 1 2 3 4 5
   * 0 a dst d d d d d d d
   * 1 b     ^ b e e e e e
   * 2 c    /  c c f f f r
   * 3 d src   d d d g g g
   * 4 e       e e e e h h
   * 5 f       f f f f f .
   * 6 g       g g g g g .
   * 7 h       h h h h h .
   */
  // not sure why we need the final loop and can't use this scheme
  /* del 3     0 1 2 3 4
   * 0 a dst d d d d d d
   * 1 b     ^ b e e e e
   * 2 c    /  c c f f f
   * 3 d src   . . . g g
   * 4 e       e . . . h
   * 5 f       f f . . .
   * 6 g       g g g . .
   * 7 h       h h h h .
   */
  for(i=0;i<rows;i++) {
    g_object_try_unref(dst[i*self->priv->tracks]);
  }
  /* copy patterns and move downwards */
  for(i=time;i<(self->priv->length-rows);i++) {
    if(*src) {
      bt_sequence_invalidate_pattern_region(self,i,track,*src);
      bt_sequence_invalidate_pattern_region(self,i+rows,track,*src);
    }
    if(*dst) bt_sequence_invalidate_pattern_region(self,i,track,*dst);
    *dst=*src;
    //*src=NULL;
    src+=self->priv->tracks;
    dst+=self->priv->tracks;
  }
  for(i=0;i<rows;i++) {
    *dst=NULL;
    dst+=self->priv->tracks;
  }
}

/**
 * bt_sequence_delete_rows:
 * @self: the sequence
 * @time: the postion to delete
 * @track: the track
 * @rows: the number of rows to remove
 *
 * Delete row for given @track.
 *
 * Since: 0.3
 */
void bt_sequence_delete_rows(const BtSequence * const self, const gulong time, const gulong track, const gulong rows) {
  g_return_if_fail(BT_IS_SEQUENCE(self));
  
  GST_INFO("delete %lu rows at %lu,%lu", rows, time, track);

  delete_rows(self,time,track,rows);
  
  bt_sequence_repair_damage(self);
}

/**
 * bt_sequence_delete_full_rows:
 * @self: the sequence
 * @time: the postion to delete
 * @rows: the number of rows to remove
 *
 * Delete row for all tracks.
 *
 * Since: 0.3
 */
void bt_sequence_delete_full_rows(const BtSequence * const self, const gulong time, const gulong rows) {
  g_return_if_fail(BT_IS_SEQUENCE(self));

  gulong j=0;
  
  GST_DEBUG("delete %lu full-rows at %lu / %lu", rows, time, self->priv->length);

  // shift label up
  for(j=0;j<rows;j++) {
    g_free(self->priv->labels[time+j]);
  }
  memmove(&self->priv->labels[time],&self->priv->labels[time+rows],((self->priv->length-rows)-time)*sizeof(gpointer));
  for(j=0;j<rows;j++) {
    self->priv->labels[self->priv->length-(1+j)]=NULL;
  }
  for(j=0;j<self->priv->tracks;j++) {
    bt_sequence_delete_rows(self,time,j,rows);
  }

  // don't make it shorter because of loop-end ?
  g_object_set((gpointer)self,"length",self->priv->length-rows,NULL);

  bt_sequence_repair_damage(self);
}

/**
 * bt_sequence_update_tempo:
 * @self: the sequence
 *
 * Refresh sequence after tempo changes.
 */
void bt_sequence_update_tempo(const BtSequence * const self) {
  g_return_if_fail(BT_IS_SEQUENCE(self));
  
  BtPattern **pattern=self->priv->patterns;
  gulong i,j;
  
  GST_INFO("updating gst-controller queues");
  bt_sequence_calculate_wait_per_position(self);

  /* @todo: this is very slow */
  for(i=0;i<self->priv->length;i++) {
    for(j=0;j<self->priv->tracks;j++) {
      if(*pattern) {
        bt_sequence_invalidate_pattern_region(self,i,j,*pattern);
      }
      pattern++;
    }
  }
  bt_sequence_repair_damage(self);
}

//-- io interface

static xmlNodePtr bt_sequence_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  BtSequence * const self = BT_SEQUENCE(persistence);
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
      // iterate over timelines
      for(i=0;i<self->priv->length;i++) {
	      const gchar * const label=self->priv->labels[i];
        if(label) {
          child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("label"),NULL);
          xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(label));
          xmlNewProp(child_node2,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
        }
      }
    }
    else goto Error;
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("tracks"),NULL))) {
      gchar * const machine_id, * const pattern_id;
      BtMachine *machine;
      BtPattern *pattern;

      // iterate over tracks
      for(j=0;j<self->priv->tracks;j++) {
        child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("track"),NULL);
        machine=self->priv->machines[j];
        g_object_get(machine,"id",&machine_id,NULL);
        xmlNewProp(child_node2,XML_CHAR_PTR("index"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(j)));
        xmlNewProp(child_node2,XML_CHAR_PTR("machine"),XML_CHAR_PTR(machine_id));
        g_free(machine_id);
        // iterate over timelines
        for(i=0;i<self->priv->length;i++) {
          // get pattern
          pattern=self->priv->patterns[i*self->priv->tracks+j];
          if(pattern) {
            g_object_get(pattern,"id",&pattern_id,NULL);
            child_node3=xmlNewChild(child_node2,NULL,XML_CHAR_PTR("position"),NULL);
            xmlNewProp(child_node3,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
            xmlNewProp(child_node3,XML_CHAR_PTR("pattern"),XML_CHAR_PTR(pattern_id));
            g_free(pattern_id);
          }
        }
      }
    }
    else goto Error;
  }
Error:
  return(node);
}

static BtPersistence *bt_sequence_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  BtSequence * const self = BT_SEQUENCE(persistence);
  xmlNodePtr child_node,child_node2;
  gboolean sequence_changed=FALSE;

  GST_DEBUG("PERSISTENCE::sequence");
  g_assert(node);

  xmlChar * const length_str     =xmlGetProp(node,XML_CHAR_PTR("length"));
  xmlChar * const tracks_str     =xmlGetProp(node,XML_CHAR_PTR("tracks"));
  xmlChar * const loop_str       =xmlGetProp(node,XML_CHAR_PTR("loop"));
  xmlChar * const loop_start_str =xmlGetProp(node,XML_CHAR_PTR("loop-start"));
  xmlChar * const loop_end_str   =xmlGetProp(node,XML_CHAR_PTR("loop-end"));

  const gulong length=length_str?atol((char *)length_str):0;
  const gulong tracks=tracks_str?atol((char *)tracks_str):0;
  const gulong loop_start=loop_start_str?atol((char *)loop_start_str):-1;
  const gulong loop_end=loop_end_str?atol((char *)loop_end_str):-1;
  const gboolean loop=loop_str?!strncasecmp((char *)loop_str,"on\0",3):FALSE;

  g_object_set(self,"length",length,"tracks",tracks,
    "loop",loop,"loop-start",loop_start,"loop-end",loop_end,
    NULL);
  xmlFree(length_str);
  xmlFree(tracks_str);
  xmlFree(loop_str);
  xmlFree(loop_start_str);
  xmlFree(loop_end_str);

  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      if(!strncmp((gchar *)node->name,"labels\0",7)) {
        for(child_node=node->children;child_node;child_node=child_node->next) {
          if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"label\0",6))) {
            xmlChar * const time_str=xmlGetProp(child_node,XML_CHAR_PTR("time"));
            xmlChar * const name=xmlGetProp(child_node,XML_CHAR_PTR("name"));
            bt_sequence_set_label(self,atol((const char *)time_str),(const gchar *)name);
            xmlFree(time_str);
	        xmlFree(name);
          }
        }
      }
      else if(!strncmp((const gchar *)node->name,"tracks\0",7)) {
        BtSetup * const setup;

        g_object_get(self->priv->song,"setup",&setup,NULL);

        for(child_node=node->children;child_node;child_node=child_node->next) {
          if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"track\0",6))) {
            xmlChar * const machine_id=xmlGetProp(child_node,XML_CHAR_PTR("machine"));
            xmlChar * const index_str=xmlGetProp(child_node,XML_CHAR_PTR("index"));
            const gulong index=index_str?atol((char *)index_str):0;
            BtMachine * const machine=bt_setup_get_machine_by_id(setup, (gchar *)machine_id);

            if(machine) {
              GST_INFO("add track for machine %p,ref_count=%d at position %lu",machine,G_OBJECT_REF_COUNT(machine),index);
              if(index<tracks) {
                self->priv->machines[index]=machine;
                GST_DEBUG("loading track with index=%s for machine=\"%s\"",index_str,machine_id);
                for(child_node2=child_node->children;child_node2;child_node2=child_node2->next) {
                  if((!xmlNodeIsText(child_node2)) && (!strncmp((char *)child_node2->name,"position\0",9))) {
                    xmlChar * const time_str=xmlGetProp(child_node2,XML_CHAR_PTR("time"));
                    xmlChar * const pattern_id=xmlGetProp(child_node2,XML_CHAR_PTR("pattern"));
                    GST_DEBUG("  at %s, machinepattern \"%s\"",time_str,safe_string(pattern_id));
                    if(pattern_id) {
                      // get pattern by name from machine
                      BtPattern * const pattern=bt_machine_get_pattern_by_id(machine,(gchar *)pattern_id);
                      if(pattern) {
                        // this refs the pattern
                        sequence_changed|=bt_sequence_set_pattern_quick(self,atol((char *)time_str),index,pattern);
                        g_object_unref(pattern);
                      }
                      else {
                        GST_WARNING("  unknown pattern \"%s\"",pattern_id);
                      }
                      xmlFree(pattern_id);
                    }
                    xmlFree(time_str);
                  }
                }
                // we keep the ref in self->priv->machines[index]
              }
              else {
                GST_WARNING("index beyond tracks: %lu>=%lu",index,tracks); 
                g_object_unref(machine);
              }
            }
            else {
              GST_INFO("invalid or missing machine %s referenced at track %lu",(gchar *)machine_id,index);
            }
            xmlFree(index_str);
	        xmlFree(machine_id);
          }
        }
        g_object_unref(setup);
      }
    }
  }
  
  if(sequence_changed) {
    // repair damage
    bt_sequence_repair_damage(self);
  }

  return(BT_PERSISTENCE(persistence));
}

static void bt_sequence_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_sequence_persistence_load;
  iface->save = bt_sequence_persistence_save;
}

//-- wrapper

//-- default signal handler

//-- g_object overrides

static void bt_sequence_constructed(GObject *object) {
  BtSequence *self=BT_SEQUENCE(object);
  BtSetup *setup;
  
  if(G_OBJECT_CLASS(parent_class)->constructed)
    G_OBJECT_CLASS(parent_class)->constructed(object);

  g_return_if_fail(BT_IS_SONG(self->priv->song));

  g_object_get((gpointer)(self->priv->song),"setup",&setup,NULL);
  if(setup) {
    g_signal_connect(setup,"wire-added",G_CALLBACK(on_wire_added),(gpointer)self);
    g_signal_connect(setup,"wire-removed",G_CALLBACK(on_wire_removed),(gpointer)self);
    g_object_unref(setup);
  }
  else {
    GST_WARNING("no setup yet");
  }
}

/* returns a property for the given property_id for this object */
static void bt_sequence_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtSequence * const self = BT_SEQUENCE(object);
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
static void bt_sequence_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtSequence * const self = BT_SEQUENCE(object);

  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for sequence: %p",self->priv->song);
    } break;
    case SEQUENCE_LENGTH: {
      // @todo remove or better stop the song
      // if(self->priv->is_playing) bt_sequence_stop(self);
      // prepare new data
      const gulong length=self->priv->length;
      self->priv->length = g_value_get_ulong(value);
      if(length!=self->priv->length) {
        GST_DEBUG("set the length for sequence: %lu",self->priv->length);
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
      const gulong tracks=self->priv->tracks;
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
      else {
        self->priv->play_start=0;
        self->priv->play_end=self->priv->length;
        bt_sequence_limit_play_pos_internal(self);
      }
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_LOOP_START: {
      self->priv->loop_start = g_value_get_long(value);
      if(self->priv->loop_start!=-1) {
        // make sure its less then loop_end/length
        if(self->priv->loop_end>0) {
          if(self->priv->loop_start>=self->priv->loop_end)
            self->priv->loop_start=self->priv->loop_end-1;
        }
        else if(self->priv->length>0) {
          if(self->priv->loop_start>=self->priv->length)
            self->priv->loop_start=self->priv->length-1;
        }
        else
          self->priv->loop_start=-1;
      }
      GST_DEBUG("set the loop-start for sequence: %ld",self->priv->loop_start);
      self->priv->play_start=(self->priv->loop_start!=-1)?self->priv->loop_start:0;
      bt_sequence_limit_play_pos_internal(self);
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case SEQUENCE_LOOP_END: {
      self->priv->loop_end = g_value_get_long(value);
      if(self->priv->loop_end!=-1) {
        // make sure its more then loop-start
        if(self->priv->loop_start>-1) {
          if(self->priv->loop_end<self->priv->loop_start)
            self->priv->loop_end=self->priv->loop_start+1;
        }
        // make sure its less then or equal to length
        if(self->priv->length>0) {
          if(self->priv->loop_end>self->priv->length)
            self->priv->loop_end=self->priv->length;          
        }
        else
          self->priv->loop_end=-1;
      }
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

static void bt_sequence_dispose(GObject * const object) {
  BtSequence * const self = BT_SEQUENCE(object);
  gulong i,j,k;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->song);
  // unref the machines
  GST_DEBUG("unref %lu machines",self->priv->tracks);
  for(i=0;i<self->priv->tracks;i++) {
    GST_INFO("releasing machine %p,ref_count=%d",
      self->priv->machines[i],(self->priv->machines[i]?G_OBJECT_REF_COUNT(self->priv->machines[i]):-1));
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

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void bt_sequence_finalize(GObject * const object) {
  const BtSequence * const self = BT_SEQUENCE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->machines);
  g_free(self->priv->labels);
  g_free(self->priv->patterns);
  g_hash_table_destroy(self->priv->damage);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

//-- class internals

static void bt_sequence_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSequence * const self = BT_SEQUENCE(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SEQUENCE, BtSequencePrivate);
  self->priv->loop_start=-1;
  self->priv->loop_end=-1;
  self->priv->damage=g_hash_table_new_full(NULL,NULL,NULL,(GDestroyNotify)g_hash_table_destroy);
  self->priv->wait_per_position=0.0;
}

static void bt_sequence_class_init(BtSequenceClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSequencePrivate));

  gobject_class->constructed  = bt_sequence_constructed;
  gobject_class->set_property = bt_sequence_set_property;
  gobject_class->get_property = bt_sequence_get_property;
  gobject_class->dispose      = bt_sequence_dispose;
  gobject_class->finalize     = bt_sequence_finalize;

  g_object_class_install_property(gobject_class,SEQUENCE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the sequence belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SEQUENCE_LENGTH,
                                  g_param_spec_ulong("length",
                                     "length prop",
                                     "length of the sequence in timeline bars",
                                     0,
                                     G_MAXLONG,  // loop-pos are LONG as well
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SEQUENCE_TRACKS,
                                  g_param_spec_ulong("tracks",
                                     "tracks prop",
                                     "number of tracks in the sequence",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SEQUENCE_LOOP,
                                  g_param_spec_boolean("loop",
                                     "loop prop",
                                     "is loop activated",
                                     FALSE,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SEQUENCE_LOOP_START,
                                  g_param_spec_long("loop-start",
                                     "loop-start prop",
                                     "start of the repeat sequence on the timeline",
                                     -1,
                                     G_MAXLONG,
                                     -1,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SEQUENCE_LOOP_END,
                                  g_param_spec_long("loop-end",
                                     "loop-end prop",
                                     "end of the repeat sequence on the timeline",
                                     -1,
                                     G_MAXLONG,
                                     -1,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

}

GType bt_sequence_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSequence),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_sequence_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_sequence_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSequence",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
