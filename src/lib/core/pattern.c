/* Buzztard
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
 * SECTION:btpattern
 * @short_description: class for an event pattern of a #BtMachine instance
 *
 * A pattern contains a grid of events. Events are parameter changes in
 * #BtMachine objects. The events are stored as #GValues. Cells containing %NULL
 * have no event for the parameter at the time.
 *
 * Patterns can have individual lengths. The length is measured in ticks. How
 * much that is in e.g. milliseconds is determined by #BtSongInfo:bpm and 
 * #BtSongInfo:tpb.
 *
 * A pattern might consist of several groups. These are mapped to the global 
 * parameters of a machine and the voice parameters for each machine voice (if 
 * any). The number of voices (tracks) is the same in all patterns of a machine.
 * If the voices are changed on the machine patterns resize themselves.
 *
 * The patterns are used in the #BtSequence to form the score of a song.
 */
/* TODO(ensonic): BtWirePattern is not a good name :/
 * - maybe we can make BtPattern a base class and also have BtMachinePattern
 * - or maybe we can exploit BtParameterGroup
 *   - each pattern would have a list of groups (BtValueGroup) and
 *     num_wires, num_global (0/1), num_voice
 *   - each group would have a pointer to the BtParmeterGroup and a gvalue array
 *   - changing voice/wires would only need resizing the group-array
 *     BtPattern::bt_pattern_on_voices_changed -> voices
 *     BtMainPagePattern::* -> wires
 *     - right now we create wire-patterns as needed
 *   - doing length changes or line inserts/deletion would require applying
 *     this to each group, on the other hand this would avoid the need for
 *     wirepattern and the code duplication there and most likely simplify
 *     main-page pattern, where we do something similar for
 *     BtPatternEditorColumn
 *   - We store wire-patterns under wires right now and need to preserve that
 *     - loading:
 *       - create machine, load patterns
 *       - create wire, realloc wire-pattern part in pattern and load data
 */
/* TODO(ensonic): pattern editing
 *  - inc/dec cursor-cell/selection
 */
/* TODO(ensonic): cut/copy/paste api
 * - need private BtPatternFragment object (not needed with the group changes?)
 *   - copy of the data
 *   - pos and size of the region
 *   - column-types
 *   - eventually wire-pattern fragments
 * - api
 *   fragment = bt_pattern_copy_fragment (pattern, col1, col2, row1, row2);
 *     return a new fragment object, opaque for the callee
 *   fragment = bt_pattern_cut_fragment (pattern, col1, col2, row1, row2);
 *     calls copy and clears afterwards
 *   success = bt_pattern_paste_fragment(pattern, fragment, col1, row1);
 *     checks if column-types are compatible
 */
#define BT_CORE
#define BT_PATTERN_C

#include "core_private.h"

//-- signal ids

enum {
  GLOBAL_PARAM_CHANGED_EVENT,
  VOICE_PARAM_CHANGED_EVENT,
  PATTERN_CHANGED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  PATTERN_LENGTH=1,
  PATTERN_VOICES
};

struct _BtPatternPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the pattern belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* the number of ticks */
  gulong length;
  /* the number of voices */
  gulong voices;
  /* the number of dynamic params the machine provides per instance */
  gulong global_params;
  /* the number of dynamic params the machine provides per instance and voice */
  gulong voice_params;
  /* the machine the pattern belongs to */
  G_POINTER_ALIAS(BtMachine *,machine);
  BtParameterGroup *global_param_group,**voice_param_groups;

  /* an array of GValues (not pointing to) with the size of 
   * length*(global_params+voices*voice_params) for normal patterns
   * 1 for command patterns
   */
  GValue *data;
};

static guint signals[LAST_SIGNAL]={0,};

//-- the class

static void bt_pattern_persistence_interface_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtPattern, bt_pattern, BT_TYPE_CMD_PATTERN,
  G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
    bt_pattern_persistence_interface_init));

//-- helper methods

/*
 * bt_pattern_resize_data_length:
 * @self: the pattern to resize the length
 * @length: the old length
 *
 * Resizes the event data grid to the new length. Keeps previous values.
 */
static void bt_pattern_resize_data_length(const BtPattern * const self, const gulong length) {
  const gulong old_data_count=            length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
  const gulong new_data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
  GValue * const data=self->priv->data;

  // allocate new space
  if((self->priv->data=g_try_new0(GValue,new_data_count))) {
    if(data) {
      gulong count=MIN(old_data_count,new_data_count);
      GST_DEBUG("keeping data count=%lu, old=%lu, new=%lu",count,old_data_count,new_data_count);
      // copy old values over
      memcpy(self->priv->data,data,count*sizeof(GValue));
      // free gvalues
      if(old_data_count>new_data_count) {
        gulong i;

        for(i=new_data_count;i<old_data_count;i++) {
          if(BT_IS_GVALUE(&data[i]))
            g_value_unset(&data[i]);
        }
      }
      // free old data
      g_free(data);
    }
    GST_DEBUG("extended pattern length from %lu to %lu : data_count=%lu = length=%lu * ( gp=%lu + voices=%lu * vp=%lu )",
      length,self->priv->length,
      new_data_count,self->priv->length,
      self->priv->global_params,self->priv->voices,self->priv->voice_params);
  }
  else {
    GST_INFO("extending pattern length from %lu to %lu failed : data_count=%lu = length=%lu * ( gp=%lu + voices=%lu * vp=%lu )",
      length,self->priv->length,
      new_data_count,self->priv->length,
      self->priv->global_params,self->priv->voices,self->priv->voice_params);
  }
}

/*
 * bt_pattern_resize_data_voices:
 * @self: the pattern to resize the voice number
 * @voices: the old number of voices
 *
 * Resizes the event data grid to the new number of voices. Keeps previous values.
 */
static void bt_pattern_resize_data_voices(const BtPattern * const self, const gulong voices) {
  const gulong length=self->priv->length;
  const gulong new_data_count=length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
  GValue * const data=self->priv->data;

  // allocate new space
  if((self->priv->data=g_try_new0(GValue,new_data_count))) {
    if(data) {
      gulong i,j;
      // one row
      const gulong count    =sizeof(GValue)*(self->priv->global_params+self->priv->voice_params*MIN(voices,self->priv->voices));
      // one row in the old/new pattern
      const gulong src_count=self->priv->global_params+            voices*self->priv->voice_params;
      const gulong dst_count=self->priv->global_params+self->priv->voices*self->priv->voice_params;
      GValue *src=data;
      GValue *dst=self->priv->data;

      GST_DEBUG("keeping data count=%lu, src_ct=%lu, dst_ct=%lu",count,src_count,dst_count);

      // copy old values over
      for(i=0;i<length;i++) {
        memcpy(dst,src,count);
        if(src_count>dst_count) {
          // free gvalues
          for(j=dst_count;j<src_count;j++) {
            if(BT_IS_GVALUE(&src[j]))
              g_value_unset(&src[j]);
          }
        }
        src=&src[src_count];
        dst=&dst[dst_count];
      }
      // free old data
      g_free(data);
    }
    GST_DEBUG("extended pattern voices from %lu to %lu : data_count=%lu = length=%lu * ( gp=%lu + voices=%lu * vp=%lu )",
      voices,self->priv->voices,
      new_data_count,self->priv->length,
      self->priv->global_params,self->priv->voices,self->priv->voice_params);
  }
  else {
    GST_INFO("extending pattern voices from %lu to %lu failed : data_count=%lu = length=%lu * ( gp=%lu + voices=%lu * vp=%lu )",
      voices,self->priv->voices,
      new_data_count,self->priv->length,
      self->priv->global_params,self->priv->voices,self->priv->voice_params);
  }
}

/*
 * bt_pattern_init_global_event:
 * @self: the pattern that holds the cell
 * @event: the pattern-cell to initialise
 * @param: the index of the global dparam
 *
 * Initialises a pattern cell with the type of the n-th global param of the
 * machine.
 *
 */
static void bt_pattern_init_global_event(const BtPattern * const self, GValue * const event, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));

  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_parameter_group_get_param_type(self->priv->global_param_group,param));
  g_assert(G_IS_VALUE(event));
}

/*
 * bt_pattern_init_voice_event:
 * @self: the pattern that holds the cell
 * @event: the pattern-cell to initialise
 * @param: the index of the voice dparam
 *
 * Initialises a pattern cell with the type of the n-th voice param of the
 * machine.
 *
 */
static void bt_pattern_init_voice_event(const BtPattern * const self, GValue * const event, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));

  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_parameter_group_get_param_type(self->priv->voice_param_groups[0],param));
  g_assert(G_IS_VALUE(event));
}

static GParamSpec *bt_pattern_get_paramspec_for_param(const BtPattern * const self, gulong param) {
  if(param<self->priv->global_params) {
    return bt_parameter_group_get_param_spec(self->priv->global_param_group, param);
  }
  else {
    param=(param-self->priv->global_params)%self->priv->voice_params;
    return bt_parameter_group_get_param_spec(self->priv->voice_param_groups[0],param);
  }
}

static GType bt_pattern_get_type_for_param(const BtPattern * const self, gulong param) {
  if(param<self->priv->global_params) {
    return bt_parameter_group_get_param_type(self->priv->global_param_group, param);
  }
  else {
    param=(param-self->priv->global_params)%self->priv->voice_params;
    return bt_parameter_group_get_param_type(self->priv->voice_param_groups[0],param);
  }
}

//-- signal handler

static void bt_pattern_on_voices_changed(BtMachine * const machine, const GParamSpec * const arg, gconstpointer const user_data) {
  const BtPattern * const self=BT_PATTERN(user_data);
  gulong old_voices=self->priv->voices;

  g_object_get((gpointer)machine,"voices",&self->priv->voices,NULL);
  if(old_voices!=self->priv->voices) {
    gulong i;
    GST_DEBUG_OBJECT(self->priv->machine,"set the voices for pattern: %lu -> %lu",old_voices,self->priv->voices);
    self->priv->voice_param_groups=(BtParameterGroup **)g_renew(gpointer,self->priv->voice_param_groups,self->priv->voices);
    for(i=old_voices;i<self->priv->voices;i++) {
      self->priv->voice_param_groups[i]=bt_machine_get_voice_param_group(machine,i);
    }
    bt_pattern_resize_data_voices(self,old_voices);
    g_object_notify((GObject *)self,"voices");
  }
}

//-- constructor methods

/**
 * bt_pattern_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the pattern
 * @name: the display name of the pattern
 * @length: the number of ticks
 * @machine: the machine the pattern belongs to
 *
 * Create a new instance. It will be automatically added to the machines pattern
 * list.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPattern *bt_pattern_new(const BtSong * const song, const gchar * const id, const gchar * const name, const gulong length, const BtMachine * const machine) {
  return(BT_PATTERN(g_object_new(BT_TYPE_PATTERN,"song",song,"id",id,"name",name,"machine",machine,"length",length,NULL)));
}

/**
 * bt_pattern_copy:
 * @self: the pattern to create a copy from
 *
 * Create a new instance as a copy of the given instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPattern *bt_pattern_copy(const BtPattern * const self) {
  BtPattern *pattern;
  gchar *id,*name,*mid;
  gulong data_count;
  gulong i;

  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);

  GST_INFO("copying pattern");

  g_object_get((gpointer)(self->priv->machine),"id",&mid,NULL);

  name=bt_machine_get_unique_pattern_name(self->priv->machine);
  id=g_strdup_printf("%s %s",mid,name);

  pattern=bt_pattern_new(self->priv->song,id,name,self->priv->length,self->priv->machine);

  data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
  // deep copy data
  for(i=0;i<data_count;i++) {
    if(BT_IS_GVALUE(&self->priv->data[i])) {
      g_value_init(&pattern->priv->data[i],G_VALUE_TYPE(&self->priv->data[i]));
      g_value_copy(&self->priv->data[i],&pattern->priv->data[i]);
    }
  }
  GST_INFO("  data copied");

  // we also need to copy the wire patterns
  if(self->priv->machine->dst_wires) {
    GList *node;
    BtWire *wire;
    BtWirePattern *wp,*wp_new;

    GST_INFO("copying wire-patterns");

    for(node=self->priv->machine->dst_wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      wp=bt_wire_get_pattern(wire,self);
      wp_new=bt_wire_pattern_copy(wp,pattern);
      g_object_unref(wp_new);
      g_object_unref(wp);
    }
    GST_INFO("  wire-patterns copied");
  }

  g_free(mid);
  g_free(id);
  g_free(name);
  return(pattern);
}

//-- methods

/**
 * bt_pattern_get_global_event_data:
 * @self: the pattern to search for the global param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_global_event_data(const BtPattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(self->priv->data,NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);
  g_return_val_if_fail(param<self->priv->global_params,NULL);

  GST_DEBUG_OBJECT(self->priv->machine,"getting pattern gvalue at tick=%lu/%lu and global param %lu/%lu",tick,self->priv->length,param,self->priv->global_params);

  const gulong index=(tick*(self->priv->global_params+self->priv->voices*self->priv->voice_params))
       +       param;

  g_assert(index<(self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params)));
  return(&self->priv->data[index]);
}

/**
 * bt_pattern_get_voice_event_data:
 * @self: the pattern to search for the voice param
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the voice parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_voice_event_data(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(self->priv->data,NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);
  g_return_val_if_fail(voice<self->priv->voices,NULL);
  g_return_val_if_fail(param<self->priv->voice_params,NULL);

  const gulong index=(tick*(self->priv->global_params+self->priv->voices*self->priv->voice_params))
       +       self->priv->global_params+(voice*self->priv->voice_params)
       + param;

  g_assert(index<(self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params)));
  return(&self->priv->data[index]);
}

/**
 * bt_pattern_set_global_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean bt_pattern_set_global_event(const BtPattern * const self, const gulong tick, const gulong param, const gchar * const value) {
  gboolean res=FALSE;
  GValue *event;

  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  if((event=bt_pattern_get_global_event_data(self,tick,param))) {
    if(BT_IS_STRING(value)) {
      if(!BT_IS_GVALUE(event)) {
        bt_pattern_init_global_event(self,event,param);
      }
      if(bt_persistence_set_value(event,value)) {
        if(bt_parameter_group_is_param_no_value(self->priv->global_param_group,param,event)) {
          g_value_unset(event);
        }
        res=TRUE;
      }
      else {
        GST_INFO("failed to set GValue for cell at tick=%lu, param=%lu",tick,param);
      }
    }
    else {
      if(BT_IS_GVALUE(event)) {
        g_value_unset(event);
      }
      res=TRUE;
    }
    if(res) {
      // notify others that the data has been changed
      g_signal_emit((gpointer)self,signals[GLOBAL_PARAM_CHANGED_EVENT],0,tick,param);
    }
  }
  else {
    GST_DEBUG("no GValue found for cell at tick=%lu, param=%lu",tick,param);
  }
  return(res);
}

/**
 * bt_pattern_set_voice_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the global parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean bt_pattern_set_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param, const gchar * const value) {
  gboolean res=FALSE;
  GValue *event;

  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  if((event=bt_pattern_get_voice_event_data(self,tick,voice,param))) {
    if(BT_IS_STRING(value)) {
      if(!BT_IS_GVALUE(event)) {
        bt_pattern_init_voice_event(self,event,param);
      }
      if(bt_persistence_set_value(event,value)) {
        if(bt_parameter_group_is_param_no_value(self->priv->voice_param_groups[voice],param,event)) {
          g_value_unset(event);
        }
        res=TRUE;
      }
      else {
        GST_INFO("failed to set GValue for cell at tick=%lu, voice=%lu, param=%lu",tick,voice,param);
      }
    }
    else {
      if(BT_IS_GVALUE(event)) {
        g_value_unset(event);
      }
      res=TRUE;
    }
    if(res) {
      // notify others that the data has been changed
      g_signal_emit((gpointer)self,signals[VOICE_PARAM_CHANGED_EVENT],0,tick,voice,param);
    }
  }
  else {
    GST_DEBUG("no GValue found for cell at tick=%lu, voice=%lu, param=%lu",tick,voice,param);
  }
  return(res);
}

/**
 * bt_pattern_get_global_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *bt_pattern_get_global_event(const BtPattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);

  GValue * const event=bt_pattern_get_global_event_data(self,tick,param);

  if(event && BT_IS_GVALUE(event)) {
    return(bt_persistence_get_value(event));
  }
  return(NULL);
}

/**
 * bt_pattern_get_voice_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *bt_pattern_get_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);

  GValue * const event=bt_pattern_get_voice_event_data(self,tick,voice,param);

  if(event && BT_IS_GVALUE(event)) {
    return(bt_persistence_get_value(event));
  }
  return(NULL);
}

/**
 * bt_pattern_test_global_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 */
gboolean bt_pattern_test_global_event(const BtPattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  const GValue * const event=bt_pattern_get_global_event_data(self,tick,param);

  if(event && BT_IS_GVALUE(event)) {
    return(TRUE);
  }
  return(FALSE);
}

/**
 * bt_pattern_test_voice_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 */
gboolean bt_pattern_test_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  const GValue * const event=bt_pattern_get_voice_event_data(self,tick,voice,param);

  if(event && BT_IS_GVALUE(event)) {
    return(TRUE);
  }
  return(FALSE);
}

/**
 * bt_pattern_tick_has_data:
 * @self: the pattern to check
 * @tick: the tick index in the pattern
 *
 * Check if there are any event in the given pattern-row.
 *
 * Returns: %TRUE is there are events, %FALSE otherwise
 */
gboolean bt_pattern_tick_has_data(const BtPattern * const self, const gulong tick) {
  const gulong voices=self->priv->voices;
  const gulong global_params=self->priv->global_params;
  const gulong voice_params=self->priv->voice_params;
  gulong j,k;
  GValue *data;

  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  data=&self->priv->data[tick*(global_params+voices*voice_params)];
  for(k=0;k<global_params;k++) {
    if(BT_IS_GVALUE(data)) {
      return(TRUE);
    }
    data++;
  }
  for(j=0;j<voices;j++) {
    for(k=0;k<voice_params;k++) {
      if(BT_IS_GVALUE(data)) {
        return(TRUE);
      }
      data++;
    }
  }
  return(FALSE);
}


static void _insert_row(const BtPattern * const self, const gulong tick, const gulong param) {
  gulong i,length=self->priv->length;
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *src=&self->priv->data[param+params*(length-2)];
  GValue *dst=&self->priv->data[param+params*(length-1)];

  GST_INFO("insert row at %lu,%lu", tick, param);

  for(i=tick;i<length-1;i++) {
    if(BT_IS_GVALUE(src)) {
      if(!BT_IS_GVALUE(dst))
        g_value_init(dst,G_VALUE_TYPE(src));
      g_value_copy(src,dst);
      g_value_unset(src);
    }
    else {
      if(BT_IS_GVALUE(dst))
        g_value_unset(dst);
    }
    src-=params;
    dst-=params;
  }
}

/**
 * bt_pattern_insert_row:
 * @self: the pattern
 * @tick: the position to insert at
 * @param: the parameter
 *
 * Insert one empty row for given @param.
 *
 * Since: 0.3
 */
void bt_pattern_insert_row(const BtPattern * const self, const gulong tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  _insert_row(self,tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_pattern_insert_full_row:
 * @self: the pattern
 * @tick: the position to insert at
 *
 * Insert one empty row for all parameters.
 *
 * Since: 0.3
 */
void bt_pattern_insert_full_row(const BtPattern * const self, const gulong tick) {
  g_return_if_fail(BT_IS_PATTERN(self));

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  for(j=0;j<params;j++) {
    _insert_row(self,tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


static void _delete_row(const BtPattern * const self, const gulong tick, const gulong param) {
  gulong i,length=self->priv->length;
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *src=&self->priv->data[param+params*(tick+1)];
  GValue *dst=&self->priv->data[param+params*tick];

  GST_INFO("insert row at %lu,%lu", tick, param);

  for(i=tick;i<length-1;i++) {
    if(BT_IS_GVALUE(src)) {
      if(!BT_IS_GVALUE(dst))
        g_value_init(dst,G_VALUE_TYPE(src));
      g_value_copy(src,dst);
      g_value_unset(src);
    }
    else {
      if(BT_IS_GVALUE(dst))
        g_value_unset(dst);
    }
    src+=params;
    dst+=params;
  }
}

/**
 * bt_pattern_delete_row:
 * @self: the pattern
 * @tick: the position to delete
 * @param: the parameter
 *
 * Delete row for given @param.
 *
 * Since: 0.3
 */
void bt_pattern_delete_row(const BtPattern * const self, const gulong tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  _delete_row(self,tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_pattern_delete_full_row:
 * @self: the pattern
 * @tick: the position to delete
 *
 * Delete row for all parameters.
 *
 * Since: 0.3
 */
void bt_pattern_delete_full_row(const BtPattern * const self, const gulong tick) {
  g_return_if_fail(BT_IS_PATTERN(self));

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  for(j=0;j<params;j++) {
    _delete_row(self,tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


static void _delete_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  gulong i,ticks=(end_tick+1)-start_tick;

  for(i=0;i<ticks;i++) {
    if(BT_IS_GVALUE(beg))
      g_value_unset(beg);
    beg+=params;
  }
}

/**
 * bt_pattern_delete_column:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 *
 * Randomize values from @start_tick to @end_tick for @param.
 *
 * Since: 0.6
 */
void bt_pattern_delete_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  _delete_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_pattern_delete_columns:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 *
 * Clear values from @start_tick to @end_tick for all params.
 *
 * Since: 0.6
 */
void bt_pattern_delete_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  for(j=0;j<params;j++) {
    _delete_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


#define _BLEND(t,T)                                                            \
	case G_TYPE_ ## T: {                                                         \
		gdouble val=(gdouble)g_value_get_ ## t(beg);                               \
	  gdouble step=((gdouble)g_value_get_ ## t(end)-val)/(gdouble)ticks;         \
	                                                                             \
		for(i=0;i<ticks;i++) {                                                     \
			if(!BT_IS_GVALUE(beg))                                                   \
				g_value_init(beg,G_TYPE_ ## T);                                        \
			g_value_set_ ## t(beg,(g ## t)(val+(step*i)));                           \
			beg+=params;                                                             \
		}                                                                          \
	} break;

static void _blend_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  GValue *end=&self->priv->data[param+params*end_tick];
  gulong i,ticks=end_tick-start_tick;
  GParamSpec *property;
  GType base_type;

  if(!BT_IS_GVALUE(beg) || !BT_IS_GVALUE(end)) {
    GST_INFO("Can't blend, beg or end is empty");
    return;
  }
  property=bt_pattern_get_paramspec_for_param(self,param);
  base_type=bt_g_type_get_base_type(property->value_type);

  GST_INFO("blending gvalue type %s",G_VALUE_TYPE_NAME(end));

  // TODO(ensonic): should this honour the cursor stepping? e.g. enter only every second value

  switch(base_type) {
  	_BLEND(int,INT)
  	_BLEND(uint,UINT)
  	_BLEND(int64,INT64)
  	_BLEND(uint64,UINT64)
  	_BLEND(float,FLOAT)
  	_BLEND(double,DOUBLE)
    case G_TYPE_ENUM:{
      GParamSpecEnum *p=G_PARAM_SPEC_ENUM (property);
      GEnumClass *e=p->enum_class;
			gdouble step;
      gint v,v1,v2;
      
      // we need the index of the enum value and the number of values inbetween
      v=g_value_get_enum(beg);
      for(v1=0;v1<e->n_values;v1++) {
        if (e->values[v1].value==v)
          break;
      }
      v=g_value_get_enum(end);
      for(v2=0;v2<e->n_values;v2++) {
        if (e->values[v2].value==v)
          break;
      }
      step=(gdouble)(v2-v1)/(gdouble)ticks;
      //GST_DEBUG("v1 = %d, v2=%d, step=%lf",v1,v2,step);

      for(i=0;i<ticks;i++) {
        if(!BT_IS_GVALUE(beg))
          g_value_init(beg,property->value_type);
        v=(gint)(v1+(step*i));
				// handle sparse enums
        g_value_set_enum(beg,e->values[v].value);
        beg+=params;
      }    		
    } break;
    default:
      GST_WARNING("unhandled gvalue type %s",G_VALUE_TYPE_NAME(end));
  }
}

/**
 * bt_pattern_blend_column:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 *
 * Fade values from @start_tick to @end_tick for @param.
 *
 * Since: 0.3
 */
void bt_pattern_blend_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  _blend_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_pattern_blend_columns:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 *
 * Fade values from @start_tick to @end_tick for all params.
 *
 * Since: 0.3
 */
void bt_pattern_blend_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  for(j=0;j<params;j++) {
    _blend_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


static void _flip_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  GValue *end=&self->priv->data[param+params*end_tick];
  GParamSpec *property;
  GType base_type;
  GValue tmp={0,};

  property=bt_pattern_get_paramspec_for_param(self,param);
  base_type=property->value_type;

  GST_INFO("flipping gvalue type %s",G_VALUE_TYPE_NAME(base_type));

  g_value_init(&tmp,base_type);
  while(beg<end) {
		if(BT_IS_GVALUE(beg) && BT_IS_GVALUE(end)) {
			g_value_copy(beg,&tmp);
			g_value_copy(end,beg);
			g_value_copy(&tmp,end);
		} else if(!BT_IS_GVALUE(beg) && BT_IS_GVALUE(end)) {
			g_value_init(beg,base_type);
			g_value_copy(end,beg);
			g_value_unset(end);
		} else if(BT_IS_GVALUE(beg) && !BT_IS_GVALUE(end)) {
			g_value_init(end,base_type);
			g_value_copy(beg,end);
			g_value_unset(beg);
		}
    beg+=params;
    end-=params;
  }
  g_value_unset(&tmp);
}

/**
 * bt_pattern_flip_column:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 *
 * Flips values from @start_tick to @end_tick for @param up-side down.
 *
 * Since: 0.6
 */
void bt_pattern_flip_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  _flip_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_pattern_flip_columns:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 *
 * Flips values from @start_tick to @end_tick for all params up-side down.
 *
 * Since: 0.5
 */
void bt_pattern_flip_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  for(j=0;j<params;j++) {
    _flip_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


#define _RANDOMIZE(t,T,p)                                                       \
	case G_TYPE_ ## T: {                                                         \
      const GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                   \
      for(i=0;i<ticks;i++) {                                                   \
        if(!BT_IS_GVALUE(beg))                                                 \
          g_value_init(beg,G_TYPE_ ## T);                                      \
        rnd=((gdouble)rand())/(RAND_MAX+1.0);                                  \
        g_value_set_ ## t(beg,(g ## t)(p->minimum+((p->maximum-p->minimum)*rnd))); \
        beg+=params;                                                           \
      }                                                                        \
    } break;

static void _randomize_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  //GValue *end=&self->priv->data[param+params*end_tick];
  gulong i,ticks=(end_tick+1)-start_tick;
  GParamSpec *property;
  GType base_type;
  gdouble rnd;

  property=bt_pattern_get_paramspec_for_param(self,param);
  base_type=bt_g_type_get_base_type(property->value_type);

  GST_INFO("blending gvalue type %s",g_type_name(property->value_type));

  // TODO(ensonic): should this honour the cursor stepping? e.g. enter only every second value
  // TODO(ensonic): if beg and end are not empty, shall we use them as upper and lower
  // bounds instead of the pspec values (ev. have a flag on the function)

  switch(base_type) {
  	_RANDOMIZE(int,INT,Int)
  	_RANDOMIZE(uint,UINT,UInt)
  	_RANDOMIZE(int64,INT64,Int64)
  	_RANDOMIZE(uint64,UINT64,UInt64)
  	_RANDOMIZE(float,FLOAT,Float)
  	_RANDOMIZE(double,DOUBLE,Double)
    case G_TYPE_BOOLEAN:{
      for(i=0;i<ticks;i++) {
        if(!BT_IS_GVALUE(beg))
          g_value_init(beg,G_TYPE_BOOLEAN);
        rnd=((gdouble)rand())/(RAND_MAX+1.0);
        g_value_set_boolean(beg,(gboolean)(2*rnd));
      }
    } break;
    case G_TYPE_ENUM:{
      const GParamSpecEnum *p=G_PARAM_SPEC_ENUM (property);
      const GEnumClass *e=p->enum_class;
      gint nv=e->n_values-1; // don't use no_value
      gint v;

      for(i=0;i<ticks;i++) {
        if(!BT_IS_GVALUE(beg))
          g_value_init(beg,property->value_type);
        rnd=((gdouble)rand())/(RAND_MAX+1.0);
        v=(gint)(nv*rnd);
				// handle sparse enums
        g_value_set_enum(beg,e->values[v].value);
        beg+=params;
      }
    } break;
    default:
      GST_WARNING("unhandled gvalue type %s",G_VALUE_TYPE_NAME(base_type));
  }
}

/**
 * bt_pattern_randomize_column:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 *
 * Randomize values from @start_tick to @end_tick for @param.
 *
 * Since: 0.3
 */
void bt_pattern_randomize_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  _randomize_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_pattern_randomize_columns:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 *
 * Randomize values from @start_tick to @end_tick for all params.
 *
 * Since: 0.3
 */
void bt_pattern_randomize_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  for(j=0;j<params;j++) {
    _randomize_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


static void _serialize_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data) {
  gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  gulong i,ticks=(end_tick+1)-start_tick;
  gchar *val;

  g_string_append(data,g_type_name(bt_pattern_get_type_for_param(self,param)));
  for(i=0;i<ticks;i++) {
    if(BT_IS_GVALUE(beg)) {
      if((val=bt_persistence_get_value(beg))) {
        g_string_append_c(data,',');
        g_string_append(data,val);
        g_free(val);
      }
    }
    else {
      // empty cell
      g_string_append(data,", ");
    }
    beg+=params;
  }
  g_string_append_c(data,'\n');
}

/**
 * bt_pattern_serialize_column:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for @param into @data.
 *
 * Since: 0.6
 */
void bt_pattern_serialize_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);
  g_return_if_fail(data);

  _serialize_column(self,start_tick,end_tick,param,data);
}

/**
 * bt_pattern_serialize_columns:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for all params into @data.
 *
 * Since: 0.6
 */
void bt_pattern_serialize_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick, GString *data) {
  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);
  g_return_if_fail(data);

  gulong j,params=self->priv->global_params+self->priv->voices*self->priv->voice_params;

  for(j=0;j<params;j++) {
    _serialize_column(self,start_tick,end_tick,j,data);
  }
}

/**
 * bt_pattern_deserialize_column:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 * @data: the source data
 *
 * Deserializes values to @start_tick to @end_tick for @param from @data.
 *
 * Returns: %TRUE for success, %FALSE e.g. to indicate incompatible GType values
 * for the column specified by @param and the @data.
 *
 * Since: 0.6
 */
gboolean bt_pattern_deserialize_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, const gchar *data) {
  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(start_tick<self->priv->length,FALSE);
  g_return_val_if_fail(end_tick<self->priv->length,FALSE);
  g_return_val_if_fail(self->priv->data,FALSE);
  g_return_val_if_fail(data,FALSE);
  g_return_val_if_fail(param<(self->priv->global_params+self->priv->voices*self->priv->voice_params),FALSE);

  gboolean ret=TRUE;
  gchar **fields=g_strsplit_set(data,",",0);
  GType dtype=bt_pattern_get_type_for_param(self,param);
  GType stype=g_type_from_name(fields[0]);

  if(dtype==stype) {
    gint i=1;
    gulong params=self->priv->global_params+self->priv->voices*self->priv->voice_params;
    GValue *beg=&self->priv->data[param+params*start_tick];
    GValue *end=&self->priv->data[param+params*end_tick];

    GST_INFO("types match %s <-> %s",fields[0],g_type_name(dtype));

    while(fields[i] && *fields[i] && (beg<=end)) {
      if(*fields[i]!=' ') {
        if(!BT_IS_GVALUE(beg)) {
          g_value_init(beg,dtype);
        }
        bt_persistence_set_value(beg, fields[i]);
      } else {
        if(BT_IS_GVALUE(beg)) {
          g_value_unset(beg);
        }
      }
      beg+=params;
      i++;
    }
  }
  else {
    GST_INFO("types don't match in %s <-> %s",fields[0],g_type_name(dtype));
    ret=FALSE;
  }
  g_strfreev(fields);
  return(ret);
}


//-- io interface

static xmlNodePtr bt_pattern_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  const BtPattern * const self = BT_PATTERN(persistence);;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node,child_node2;

  GST_DEBUG("PERSISTENCE::pattern");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("pattern"),NULL))) {
    const gulong length=self->priv->length;
    const gulong voices=self->priv->voices;
    const gulong global_params=self->priv->global_params;
    const gulong voice_params=self->priv->voice_params;
    BtParameterGroup *pg;
    gulong i,j,k;
    gchar *value,*id,*name;
    
    g_object_get((GObject *)self,"id",&id,"name",&name,NULL);

    xmlNewProp(node,XML_CHAR_PTR("id"),XML_CHAR_PTR(id));
    xmlNewProp(node,XML_CHAR_PTR("name"),XML_CHAR_PTR(name));
    xmlNewProp(node,XML_CHAR_PTR("length"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(length)));
    g_free(id);g_free(name);

    // save pattern data
    for(i=0;i<length;i++) {
      // check if there are any GValues stored ?
      if(bt_pattern_tick_has_data(self,i)) {
        child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("tick"),NULL);
        xmlNewProp(child_node,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
        // save tick data
        pg=self->priv->global_param_group;
        for(k=0;k<global_params;k++) {
          if((value=bt_pattern_get_global_event(self,i,k))) {
            child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("globaldata"),NULL);
            xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(bt_parameter_group_get_param_name(pg,k)));
            xmlNewProp(child_node2,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));g_free(value);
          }
        }
        for(j=0;j<voices;j++) {
          const gchar * const voice_str=bt_persistence_strfmt_ulong(j);
          pg=self->priv->voice_param_groups[j];
          for(k=0;k<voice_params;k++) {
            if((value=bt_pattern_get_voice_event(self,i,j,k))) {
              child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("voicedata"),NULL);
              xmlNewProp(child_node2,XML_CHAR_PTR("voice"),XML_CHAR_PTR(voice_str));
              xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(bt_parameter_group_get_param_name(pg,k)));
              xmlNewProp(child_node2,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));g_free(value);
            }
          }
        }
      }
    }
  }
  return(node);
}

static BtPersistence *bt_pattern_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  BtPattern *self;
  BtPersistence *result;
  xmlChar *id,*name,*length_str,*tick_str,*value,*voice_str;
  glong tick,voice,param;
  gulong length;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::pattern");
  g_assert(node);

  id=xmlGetProp(node,XML_CHAR_PTR("id"));
  name=xmlGetProp(node,XML_CHAR_PTR("name"));
  length_str=xmlGetProp(node,XML_CHAR_PTR("length"));
  length=length_str?atol((char *)length_str):0;

  if(!persistence) {
    BtSong *song=NULL;
    BtMachine *machine=NULL;
    gchar *param_name;

    // we need to get parameters from var_args (need to handle all baseclass params
    param_name=va_arg(var_args,gchar*);
    while(param_name) {
      if(!strcmp(param_name,"song")) {
        song=va_arg(var_args, gpointer);
      }
      else if(!strcmp(param_name,"machine")) {
        machine=va_arg(var_args, gpointer);
      }
      else {
        GST_WARNING("unhandled argument: %s",param_name);
        break;
      }
      param_name=va_arg(var_args,gchar*);
    }

    self=bt_pattern_new(song, (gchar*)id, (gchar*)name, length, machine);
    result=BT_PERSISTENCE(self);
  }
  else {
    self=BT_PATTERN(persistence);
    result=BT_PERSISTENCE(self);

    g_object_set(self,"id",id,"name",name,"length",length,NULL);
  }
  xmlFree(id);xmlFree(name);xmlFree(length_str);

  GST_DEBUG("PERSISTENCE::pattern loading data");

  // load pattern data
  for(node=node->children;node;node=node->next) {
    if((!xmlNodeIsText(node)) && (!strncmp((gchar *)node->name,"tick\0",5))) {
      tick_str=xmlGetProp(node,XML_CHAR_PTR("time"));
      tick=atoi((char *)tick_str);
      // iterate over children
      for(child_node=node->children;child_node;child_node=child_node->next) {
        if(!xmlNodeIsText(child_node)) {
          name=xmlGetProp(child_node,XML_CHAR_PTR("name"));
          value=xmlGetProp(child_node,XML_CHAR_PTR("value"));
          //GST_LOG("     \"%s\" -> \"%s\"",safe_string(name),safe_string(value));
          if(!strncmp((char *)child_node->name,"globaldata\0",11)) {
            param=bt_parameter_group_get_param_index(self->priv->global_param_group,(gchar *)name);
            if(param!=-1) {
              bt_pattern_set_global_event(self,tick,param,(gchar *)value);
            }
            else {
              GST_WARNING("error while loading global pattern data at tick %ld, param %ld",tick,param);
            }
          }
          else if(!strncmp((char *)child_node->name,"voicedata\0",10)) {
            voice_str=xmlGetProp(child_node,XML_CHAR_PTR("voice"));
            voice=atol((char *)voice_str);
            param=bt_parameter_group_get_param_index(self->priv->voice_param_groups[voice],(gchar *)name);
            if(param!=-1) {
              bt_pattern_set_voice_event(self,tick,voice,param,(gchar *)value);
            }
            else {
              GST_WARNING("error while loading voice pattern data at tick %ld, param %ld, voice %ld",tick,param,voice);
            }
            xmlFree(voice_str);
          }
          xmlFree(name);
	      xmlFree(value);
        }
      }
      xmlFree(tick_str);
    }
  }

  return(result);
}

static void bt_pattern_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_pattern_persistence_load;
  iface->save = bt_pattern_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void bt_pattern_constructed(GObject *object) {
  BtPattern *self=BT_PATTERN(object);

  G_OBJECT_CLASS(bt_pattern_parent_class)->constructed(object);

  /* fetch pointers from base-class */
  g_object_get(self,"song",&self->priv->song,"machine",&self->priv->machine,NULL);
  g_object_try_weak_ref(self->priv->song);
  g_object_unref(self->priv->song);
  g_object_try_weak_ref(self->priv->machine);
  g_object_unref(self->priv->machine);

  g_return_if_fail(BT_IS_SONG(self->priv->song));
  g_return_if_fail(BT_IS_MACHINE(self->priv->machine));
  
  GST_DEBUG("set the machine for pattern: %p (machine-ref_ct=%d)",self->priv->machine,G_OBJECT_REF_COUNT(self->priv->machine));
  
  g_object_get((gpointer)(self->priv->machine),"global-params",&self->priv->global_params,"voice-params",&self->priv->voice_params,NULL);
  self->priv->global_param_group=bt_machine_get_global_param_group(self->priv->machine);

  g_signal_connect(self->priv->machine,"notify::voices",G_CALLBACK(bt_pattern_on_voices_changed),(gpointer)self);
  // need to do that so that data is reallocated
  bt_pattern_resize_data_length(self,0);
  bt_pattern_on_voices_changed(self->priv->machine,NULL,(gpointer)self);
  
  GST_DEBUG("add pattern to machine");
  // add the pattern to the machine
  bt_machine_add_pattern(self->priv->machine,(BtCmdPattern *)self);
}

static void bt_pattern_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtPattern * const self = BT_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case PATTERN_LENGTH: {
      g_value_set_ulong(value, self->priv->length);
    } break;
    case PATTERN_VOICES: {
      g_value_set_ulong(value, self->priv->voices);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  BtPattern * const self = BT_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case PATTERN_LENGTH: {
      const gulong length=self->priv->length;
      self->priv->length = g_value_get_ulong(value);
      if(length!=self->priv->length) {
        GST_DEBUG("set the length for pattern: %lu",self->priv->length);
        bt_pattern_resize_data_length(self,length);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_dispose(GObject * const object) {
  const BtPattern * const self = BT_PATTERN(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  if(self->priv->machine) {
    g_signal_handlers_disconnect_matched(self->priv->machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_pattern_on_voices_changed,(gpointer)self);
  }

  g_object_try_weak_unref(self->priv->song);
  g_object_try_weak_unref(self->priv->machine);

  G_OBJECT_CLASS(bt_pattern_parent_class)->dispose(object);
}

static void bt_pattern_finalize(GObject * const object) {
  const BtPattern * const self = BT_PATTERN(object);
  GValue *v;
  gulong i;

  GST_DEBUG("!!!! self=%p",self);

  if(self->priv->data) {
    const gulong data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
    // free gvalues in self->priv->data
    for(i=0;i<data_count;i++) {
      v=&self->priv->data[i];
      if(BT_IS_GVALUE(v))
        g_value_unset(v);
    }
    g_free(self->priv->data);
  }

  G_OBJECT_CLASS(bt_pattern_parent_class)->finalize(object);
}

//-- class internals

static void bt_pattern_init(BtPattern *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PATTERN, BtPatternPrivate);
}

static void bt_pattern_class_init(BtPatternClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtPatternPrivate));

  gobject_class->constructed  = bt_pattern_constructed;
  gobject_class->set_property = bt_pattern_set_property;
  gobject_class->get_property = bt_pattern_get_property;
  gobject_class->dispose      = bt_pattern_dispose;
  gobject_class->finalize     = bt_pattern_finalize;

  /**
   * BtPattern::global-param-changed:
   * @self: the pattern object that emitted the signal
   * @tick: the tick position inside the pattern
   * @param: the global parameter index
   *
   * signals that a global param of this pattern has been changed
   */
  signals[GLOBAL_PARAM_CHANGED_EVENT] = g_signal_new("global-param-changed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        0,
                                        NULL, // accumulator
                                        NULL, // acc data
                                        bt_marshal_VOID__ULONG_ULONG,
                                        G_TYPE_NONE, // return type
                                        2, // n_params
                                        G_TYPE_ULONG,G_TYPE_ULONG // param data
                                        );

  /**
   * BtPattern::voice-param-changed:
   * @self: the pattern object that emitted the signal
   * @tick: the tick position inside the pattern
   * @voice: the voice number
   * @param: the voice parameter index
   *
   * signals that a voice param of this pattern has been changed
   */
  signals[VOICE_PARAM_CHANGED_EVENT] = g_signal_new("voice-param-changed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        0,
                                        NULL, // accumulator
                                        NULL, // acc data
                                        bt_marshal_VOID__ULONG_ULONG_ULONG,
                                        G_TYPE_NONE, // return type
                                        3, // n_params
                                        G_TYPE_ULONG,G_TYPE_ULONG,G_TYPE_ULONG // param data
                                        );
  /**
   * BtPattern::pattern-changed:
   * @self: the pattern object that emitted the signal
   * @intermediate: boolean flag that is %TRUE to signal that more change are
   *                coming
   *
   * Signals that this pattern has been changed (more than in one place).
   * When doing e.g. line inserts, one will receive two updates, one before and
   * one after. The first will have @intermediate=%TRUE. Applications can use
   * that to defer change-consolidation.
   */
  signals[PATTERN_CHANGED_EVENT] = g_signal_new("pattern-changed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        0,
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__BOOLEAN,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        G_TYPE_BOOLEAN
                                        );

  g_object_class_install_property(gobject_class,PATTERN_LENGTH,
                                  g_param_spec_ulong("length",
                                     "length prop",
                                     "length of the pattern in ticks",
                                     0,
                                     G_MAXULONG,
                                     1,
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_VOICES,
                                  g_param_spec_ulong("voices",
                                     "voices prop",
                                     "number of voices in the pattern",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

