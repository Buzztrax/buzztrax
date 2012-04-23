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

  /* the pattern data */
  BtValueGroup *global_value_group;
  BtValueGroup **voice_value_groups;
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
static void bt_pattern_resize_data_length(const BtPattern * const self) {
  gulong i;
  
  if(!self->priv->global_value_group)
    return;
  
  g_object_set(self->priv->global_value_group,"length",self->priv->length,NULL);
  for(i=0;i<self->priv->voices;i++) {
    g_object_set(self->priv->voice_value_groups[i],"length",self->priv->length,NULL);
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
  gulong i;
  
  if(voices<self->priv->voices) {
    // unref old voices
    for(i=self->priv->voices;i<voices;i++) {
      g_object_try_unref(self->priv->voice_value_groups[i]);
    }
  }

  self->priv->voice_param_groups=g_renew(BtParameterGroup *,self->priv->voice_param_groups,self->priv->voices);
  self->priv->voice_value_groups=g_renew(BtValueGroup *,self->priv->voice_value_groups,self->priv->voices);

  for(i=voices;i<self->priv->voices;i++) {
    self->priv->voice_param_groups[i]=bt_machine_get_voice_param_group(self->priv->machine,i);
    self->priv->voice_value_groups[i]=bt_value_group_new(self->priv->voice_param_groups[i], self->priv->length);
  }
}

static BtValueGroup *bt_pattern_get_value_group_for_param(const BtPattern * const self, gulong *param) {
  if(*param<self->priv->global_params) {
    return self->priv->global_value_group;
  }
  else {
    *param=*param-self->priv->global_params;
    gulong voice=*param/self->priv->voice_params;
    *param-=(voice*self->priv->voice_params);
    return self->priv->voice_value_groups[voice];
  }
}

//-- signal handler

static void bt_pattern_on_voices_changed(BtMachine * const machine, const GParamSpec * const arg, gconstpointer const user_data) {
  const BtPattern * const self=BT_PATTERN(user_data);
  gulong old_voices=self->priv->voices;

  g_object_get((gpointer)machine,"voices",&self->priv->voices,NULL);
  if(old_voices!=self->priv->voices) {
    GST_DEBUG_OBJECT(self->priv->machine,"set the voices for pattern: %lu -> %lu",old_voices,self->priv->voices);
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
  gulong i;

  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);

  GST_INFO("copying pattern");

  g_object_get((gpointer)(self->priv->machine),"id",&mid,NULL);

  name=bt_machine_get_unique_pattern_name(self->priv->machine);
  id=g_strdup_printf("%s %s",mid,name);

  pattern=bt_pattern_new(self->priv->song,id,name,self->priv->length,self->priv->machine);
  g_object_try_unref(pattern->priv->global_value_group);
  pattern->priv->global_value_group=bt_value_group_copy(self->priv->global_value_group);
  for(i=0;i<self->priv->voices;i++) {
    g_object_try_unref(pattern->priv->voice_value_groups[i]);
    pattern->priv->voice_value_groups[i]=bt_value_group_copy(self->priv->voice_value_groups[i]);    
  }

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
 * Do not modify the contents!
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_global_event_data(const BtPattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);

  return(bt_value_group_get_event_data(self->priv->global_value_group,tick,param));
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
 * Do not modify the contents!
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_voice_event_data(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param) {
  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(voice<self->priv->voices,NULL);

  return(bt_value_group_get_event_data(self->priv->voice_value_groups[voice],tick,param));
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
  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);

  gboolean res=bt_value_group_set_event(self->priv->global_value_group,tick,param,value);
  if(res) {
    // notify others that the data has been changed
    g_signal_emit((gpointer)self,signals[GLOBAL_PARAM_CHANGED_EVENT],0,tick,param);
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
  g_return_val_if_fail(voice<self->priv->voices,FALSE);

  gboolean res=bt_value_group_set_event(self->priv->voice_value_groups[voice],tick,param,value);
  if(res) {
    // notify others that the data has been changed
    g_signal_emit((gpointer)self,signals[VOICE_PARAM_CHANGED_EVENT],0,tick,voice,param);
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

  return(bt_value_group_get_event(self->priv->global_value_group,tick,param));
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
  g_return_val_if_fail(voice<self->priv->voices,NULL);

  return(bt_value_group_get_event(self->priv->voice_value_groups[voice],tick,param));
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

  return(bt_value_group_test_event(self->priv->global_value_group,tick,param));
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
  g_return_val_if_fail(voice<self->priv->voices,FALSE);

  return(bt_value_group_test_event(self->priv->voice_value_groups[voice],tick,param));
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
gboolean bt_pattern_test_tick(const BtPattern * const self, const gulong tick) {
  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  
  if(bt_value_group_test_tick(self->priv->global_value_group,tick)) {
    return(TRUE);
  }

  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    if(bt_value_group_test_tick(self->priv->voice_value_groups[j],tick)) {
      return(TRUE);
    }
  }
  return(FALSE);
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

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_insert_row(self->priv->global_value_group,tick,param);
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

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_insert_full_row(self->priv->global_value_group,tick);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_insert_full_row(self->priv->voice_value_groups[j],tick);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
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

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_delete_row(self->priv->global_value_group,tick,param);
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

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_delete_full_row(self->priv->global_value_group,tick);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_delete_full_row(self->priv->voice_value_groups[j],tick);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
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
void bt_pattern_clear_column(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_PATTERN(self));

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_clear_column(bt_pattern_get_value_group_for_param(self,(gulong *)&param),start_tick,end_tick,param);
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
void bt_pattern_clear_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_PATTERN(self));

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_clear_columns(self->priv->global_value_group,start_tick,end_tick);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_clear_columns(self->priv->voice_value_groups[j],start_tick,end_tick);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
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

  // irks need both group and remaining param
  bt_value_group_blend_column(bt_pattern_get_value_group_for_param(self,(gulong *)&param),start_tick,end_tick,param);
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

  bt_value_group_blend_columns(self->priv->global_value_group,start_tick,end_tick);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_blend_columns(self->priv->voice_value_groups[j],start_tick,end_tick);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
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

  bt_value_group_flip_column(bt_pattern_get_value_group_for_param(self,(gulong *)&param),start_tick,end_tick,param);
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

  bt_value_group_flip_columns(self->priv->global_value_group,start_tick,end_tick);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_flip_columns(self->priv->voice_value_groups[j],start_tick,end_tick);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
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

  bt_value_group_randomize_column(bt_pattern_get_value_group_for_param(self,(gulong *)&param),start_tick,end_tick,param);
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

  bt_value_group_randomize_columns(self->priv->global_value_group,start_tick,end_tick);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_randomize_columns(self->priv->voice_value_groups[j],start_tick,end_tick);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
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

  bt_value_group_serialize_column(bt_pattern_get_value_group_for_param(self,(gulong *)&param),start_tick,end_tick,param,data);
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

  bt_value_group_serialize_columns(self->priv->global_value_group,start_tick,end_tick,data);
  const gulong voices=self->priv->voices;
  gulong j;
  for(j=0;j<voices;j++) {
    bt_value_group_serialize_columns(self->priv->voice_value_groups[j],start_tick,end_tick,data);
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
  
  return(bt_value_group_deserialize_column(bt_pattern_get_value_group_for_param(self,(gulong *)&param),start_tick,end_tick,param,data));
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
      if(bt_pattern_test_tick(self,i)) {
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
  self->priv->global_value_group=bt_value_group_new(self->priv->global_param_group, self->priv->length);

  g_signal_connect(self->priv->machine,"notify::voices",G_CALLBACK(bt_pattern_on_voices_changed),(gpointer)self);
  // need to do that so that data is reallocated
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
        bt_pattern_resize_data_length(self);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_dispose(GObject * const object) {
  const BtPattern * const self = BT_PATTERN(object);
  gulong i;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  g_object_try_unref(self->priv->global_value_group);
  for(i=0;i<self->priv->voices;i++) {
    g_object_try_unref(self->priv->voice_value_groups[i]);
  }

  if(self->priv->machine) {
    g_signal_handlers_disconnect_matched(self->priv->machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_pattern_on_voices_changed,(gpointer)self);
  }

  g_object_try_weak_unref(self->priv->song);
  g_object_try_weak_unref(self->priv->machine);

  G_OBJECT_CLASS(bt_pattern_parent_class)->dispose(object);
}

static void bt_pattern_finalize(GObject * const object) {
  const BtPattern * const self = BT_PATTERN(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->voice_value_groups);

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
   * Signals that a global param of this pattern has been changed.
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
   * Signals that a voice param of this pattern has been changed.
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
   * @intermediate: flag that is %TRUE to signal that more change are coming
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

