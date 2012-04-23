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
 * SECTION:btwirepattern
 * @short_description: class for an event pattern of a #BtMachine instance
 *
 * A wire-pattern contains a grid of events. Events are parameter changes in
 * #BtWire objects. The events are stored as #GValues. Cells contaning %NULL
 * have no event for the parameter at the time.
 *
 * The wire-patterns are used in normal #BtPattern objects as a group for each
 * input of the #BtMachine that is the owner of the pattern.
 *
 * Wire-patterns synchronize their length to the length of the pattern they
 * belong to.
 */
/* we need wire_params (volume,panning ) per input
 * - only for processor machine and sink machine patterns
 * - volume: is input-volume for the wire
 *   - one volume per input
 * - panning: is panorama on the wire, if we connect e.g. mono -> stereo
 *   - panning parameters can change, if the connection changes
 *   - mono-to-stereo (1->2): 1 parameter
 *   - mono-to-suround (1->4): 2 parameters
 *   - stereo-to-surround (2->4): 1 parameter
 */
/* TODO(ensonic): BtWirePattern is not a good name :/
 * - see comments in pattern.c
 */
#define BT_CORE
#define BT_WIRE_PATTERN_C

#include "core_private.h"

//-- signal ids

enum {
  PARAM_CHANGED_EVENT,
  PATTERN_CHANGED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  WIRE_PATTERN_SONG=1,
  WIRE_PATTERN_WIRE,
  WIRE_PATTERN_PATTERN
};

struct _BtWirePatternPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the pattern belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* the number of ticks */
  gulong length;
  /* the number of dynamic params */
  gulong num_params;
  /* the wire the wire-pattern belongs to */
  G_POINTER_ALIAS(BtWire *,wire);
  BtParameterGroup *param_group;
  /* the pattern the wire-pattern belongs to */
  G_POINTER_ALIAS(BtPattern *,pattern);

  /* the pattern data */
  BtValueGroup *value_group;
  
  /* signal handler ids */
  gint pattern_length_changed;
};

static GQuark error_domain=0;

static guint signals[LAST_SIGNAL]={0,};

//-- the class

static void bt_wire_pattern_persistence_interface_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtWirePattern, bt_wire_pattern, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
    bt_wire_pattern_persistence_interface_init));


//-- enums

//-- helper methods

//-- event handler

static void on_pattern_length_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
  BtWirePattern *self=BT_WIRE_PATTERN(user_data);
  const gulong length=self->priv->length;

  GST_INFO("pattern length changed : %p",self->priv->pattern);
  g_object_get((gpointer)(self->priv->pattern),"length",&self->priv->length,NULL);
  if(length!=self->priv->length) {
    GST_DEBUG("set the length for wire-pattern: %lu",self->priv->length);
    g_object_set(self->priv->value_group,"length",self->priv->length,NULL);
  }
}

//-- constructor methods

/**
 * bt_wire_pattern_new:
 * @song: the song the new instance belongs to
 * @wire: the wire the pattern belongs to
 * @pattern: the pattern that gets extended
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWirePattern *bt_wire_pattern_new(const BtSong * const song, const BtWire * const wire, const BtPattern * const pattern) {
  return(BT_WIRE_PATTERN(g_object_new(BT_TYPE_WIRE_PATTERN,"song",song,"wire",wire,"pattern",pattern,NULL)));
}

/**
 * bt_wire_pattern_copy:
 * @self: the wire pattern to create a copy from
 * @pattern: the new pattern for the copy
 *
 * Create a new instance as a copy of the given instance. This is usualy done in
 * sync with bt_pattern_copy().
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWirePattern *bt_wire_pattern_copy(const BtWirePattern * const self, const BtPattern * const pattern) {
  BtWirePattern *wire_pattern;

  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),NULL);
  
  wire_pattern=bt_wire_pattern_new(self->priv->song,self->priv->wire,pattern);
  g_object_try_unref(wire_pattern->priv->value_group);
  wire_pattern->priv->value_group=bt_value_group_copy(self->priv->value_group);
  
  return(wire_pattern);
}
 
//-- methods

/**
 * bt_wire_pattern_get_event_data:
 * @self: the pattern to search for the param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Do not modify the contents!
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_wire_pattern_get_event_data(const BtWirePattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),NULL);
  g_return_val_if_fail(self->priv->value_group,NULL);

  return(bt_value_group_get_event_data(self->priv->value_group,tick,param));
}

/**
 * bt_wire_pattern_set_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean bt_wire_pattern_set_event(const BtWirePattern * const self, const gulong tick, const gulong param, const gchar * const value) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  g_return_val_if_fail(self->priv->value_group,FALSE);

  gboolean res=bt_value_group_set_event(self->priv->value_group,tick,param,value);
  if(res) {
    // notify others that the data has been changed
    g_signal_emit((gpointer)self,signals[PARAM_CHANGED_EVENT],0,tick,param);
  }
  return(res);
}

/**
 * bt_wire_pattern_get_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *bt_wire_pattern_get_event(const BtWirePattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),NULL);
  g_return_val_if_fail(self->priv->value_group,NULL);

  return(bt_value_group_get_event(self->priv->value_group,tick,param));
}

/**
 * bt_wire_pattern_test_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 */
gboolean bt_wire_pattern_test_event(const BtWirePattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  g_return_val_if_fail(self->priv->value_group,FALSE);

  return(bt_value_group_test_event(self->priv->value_group,tick,param));
}

/**
 * bt_wire_pattern_tick_has_data:
 * @self: the pattern to check
 * @tick: the tick index in the pattern
 *
 * Check if there are any event in the given pattern-row.
 *
 * Returns: %TRUE is there are events, %FALSE otherwise
 */
gboolean bt_wire_pattern_tick_has_data(const BtWirePattern * const self, const gulong tick) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  g_return_val_if_fail(self->priv->value_group,FALSE);

  return(bt_value_group_tick_has_data(self->priv->value_group,tick));
}

/**
 * bt_wire_pattern_insert_row:
 * @self: the pattern
 * @tick: the postion to insert at
 * @param: the parameter
 *
 * Insert one empty row for given @param.
 *
 * Since: 0.3
 */
void bt_wire_pattern_insert_row(const BtWirePattern * const self, const gulong tick, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_insert_row(self->priv->value_group,tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_insert_full_row:
 * @self: the pattern
 * @tick: the postion to insert at
 *
 * Insert one empty row for all parameters.
 *
 * Since: 0.3
 */
void bt_wire_pattern_insert_full_row(const BtWirePattern * const self, const gulong tick) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_insert_full_row(self->priv->value_group,tick);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_delete_row:
 * @self: the pattern
 * @tick: the postion to delete
 * @param: the parameter
 *
 * Delete row for given @param.
 *
 * Since: 0.3
 */
void bt_wire_pattern_delete_row(const BtWirePattern * const self, const gulong tick, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_delete_row(self->priv->value_group,tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_delete_full_row:
 * @self: the pattern
 * @tick: the postion to delete
 *
 * Delete row for all parameters.
 *
 * Since: 0.3
 */
void bt_wire_pattern_delete_full_row(const BtWirePattern * const self, const gulong tick) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_delete_full_row(self->priv->value_group,tick);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_delete_column:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @param: the parameter
 *
 * Randomize values from @start_tick to @end_tick for @param.
 *
 * Since: 0.6
 */
void bt_wire_pattern_delete_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_delete_column(self->priv->value_group,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_delete_columns:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 *
 * Clear values from @start_tick to @end_tick for all params.
 *
 * Since: 0.6
 */
void bt_wire_pattern_delete_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  bt_value_group_delete_columns(self->priv->value_group,start_tick,end_tick);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}
                          
/**
 * bt_wire_pattern_blend_column:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @param: the parameter
 *
 * Fade values from @start_tick to @end_tick for @param.
 *
 * Since: 0.3
 */
void bt_wire_pattern_blend_column(const BtWirePattern * const self, const gulong start_tick,const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));
  
  bt_value_group_blend_column(self->priv->value_group,start_tick,end_tick,param);
  g_signal_emit((gpointer)self->priv->value_group,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_blend_columns:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 *
 * Fade values from @start_tick to @end_tick for all params.
 *
 * Since: 0.3
 */
void bt_wire_pattern_blend_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));
  
  bt_value_group_blend_columns(self->priv->value_group,start_tick,end_tick);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_flip_column:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @param: the parameter
 *
 * Flips values from @start_tick to @end_tick for @param up-side down.
 *
 * Since: 0.6
 */
void bt_wire_pattern_flip_column(const BtWirePattern * const self, const gulong start_tick,const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));
  
  bt_value_group_flip_column(self->priv->value_group,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_flip_columns:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 *
 * Flips values from @start_tick to @end_tick for all params up-side down.
 *
 * Since: 0.6
 */
void bt_wire_pattern_flip_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));
  
  bt_value_group_flip_columns(self->priv->value_group,start_tick,end_tick);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_randomize_column:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @param: the parameter
 *
 * Randomize values from @start_tick to @end_tick for @param.
 *
 * Since: 0.3
 */
void bt_wire_pattern_randomize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  bt_value_group_randomize_column(self->priv->value_group,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_randomize_columns:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 *
 * Randomize values from @start_tick to @end_tick for all params.
 *
 * Since: 0.3
 */
void bt_wire_pattern_randomize_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  bt_value_group_randomize_columns(self->priv->value_group,start_tick,end_tick);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}

/**
 * bt_wire_pattern_serialize_column:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @param: the parameter
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for @param into @data.
 *
 * Since: 0.6
 */
void bt_wire_pattern_serialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  bt_value_group_serialize_column(self->priv->value_group,start_tick,end_tick,param,data);
}

/**
 * bt_wire_pattern_serialize_columns:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for all params into @data.
 *
 * Since: 0.6
 */
void bt_wire_pattern_serialize_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, GString *data) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  bt_value_group_serialize_columns(self->priv->value_group,start_tick,end_tick,data);
}

/**
 * bt_wire_pattern_deserialize_column:
 * @self: the pattern
 * @start_tick: the start postion for the range
 * @end_tick: the end postion for the range
 * @param: the parameter
 * @data: the source data
 *
 * Deserializes values to @start_tick to @end_tick for @param from @data.
 *
 * Returns: %TRUE for success, %FALSE e.g. to indicate incompative GType values
 * for the column specified by @param and the @data. 
 *
 * Since: 0.6
 */
gboolean bt_wire_pattern_deserialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, const gchar *data) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  
  return(bt_value_group_deserialize_column(self->priv->value_group,start_tick,end_tick,param,data));
}


//-- io interface

static xmlNodePtr bt_wire_pattern_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(persistence);
  gchar *id;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node,child_node2;

  GST_DEBUG("PERSISTENCE::wire-pattern");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("wire-pattern"),NULL))) {
    gulong i,k;
    gchar *value;
    BtWire *wire;
    const gulong length=self->priv->length;
    const gulong num_params=self->priv->num_params;
    BtParameterGroup *pg;
    
    g_object_get((gpointer)(self->priv->pattern),"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("pattern-id"),XML_CHAR_PTR(id));
    g_free(id);

    g_object_get((gpointer)self,"wire",&wire,NULL);

    // save pattern data
    for(i=0;i<length;i++) {
      // check if there are any GValues stored ?
      if(bt_wire_pattern_tick_has_data(self,i)) {
        child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("tick"),NULL);
        xmlNewProp(child_node,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
        // save tick data
        pg=self->priv->param_group;
        for(k=0;k<num_params;k++) {
          if((value=bt_wire_pattern_get_event(self,i,k))) {
            child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("wiredata"),NULL);
            xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(bt_parameter_group_get_param_name(pg,k)));
            xmlNewProp(child_node2,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));g_free(value);
          }
        }
      }
    }
    g_object_unref(wire);
  }
  return(node);
}

static BtPersistence *bt_wire_pattern_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  BtWirePattern *self;
  BtPersistence *result;
  BtMachine *dst_machine=NULL;
  BtPattern *pattern;
  xmlChar *name,*pattern_id,*tick_str,*value;
  gulong tick,param;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wire-pattern");
  g_assert(node);
  
  pattern_id=xmlGetProp(node,XML_CHAR_PTR("pattern-id"));
  if(!pattern_id) /* "patterns" is legacy, it is conflicting with a node type in the xsd */
    pattern_id=xmlGetProp(node,XML_CHAR_PTR("pattern"));
  
  if(!persistence) {
    BtSong *song=NULL;
    BtWire *wire=NULL;
    gchar *param_name;

    // we need to get parameters from var_args (need to handle all baseclass params
    param_name=va_arg(var_args,gchar*);
    while(param_name) {
      if(!strcmp(param_name,"song")) {
        song=va_arg(var_args, gpointer);
      }
      else if(!strcmp(param_name,"wire")) {
        wire=va_arg(var_args, gpointer);
      }
      else {
        GST_WARNING("unhandled argument: %s",param_name);
        break;
      }
      param_name=va_arg(var_args,gchar*);
    }

    g_object_get(wire,"dst",&dst_machine,NULL);
    pattern=(BtPattern *)bt_machine_get_pattern_by_id(dst_machine,(gchar *)pattern_id);
    
    self=bt_wire_pattern_new(song,wire,pattern);
    result=BT_PERSISTENCE(self);
    
    if(pattern) {
      g_object_unref(pattern);
    }
    else {
      goto NoPatternError;
    }
  }
  else {
    self=BT_WIRE_PATTERN(persistence);
    result=BT_PERSISTENCE(self);
  }
  
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
          //GST_DEBUG("     \"%s\" -> \"%s\"",safe_string(name),safe_string(value));
          if(!strncmp((char *)child_node->name,"wiredata\0",9)) {
            param=bt_parameter_group_get_param_index(self->priv->param_group,(gchar *)name);
            if(param!=-1) {
              bt_wire_pattern_set_event(self,tick,param,(gchar *)value);
            }
            else {
              GST_WARNING("error while loading wire pattern data at tick %lu, param %lu: %s",tick,param);
            }
          }
          xmlFree(name);
	      xmlFree(value);
        }
      }
      xmlFree(tick_str);
    }
  }
  
Done:
  xmlFree(pattern_id);
  g_object_try_unref(dst_machine);
  return(result);
NoPatternError:
  GST_WARNING("No pattern with id='%s'",pattern_id);
  if(err) {
    g_set_error(err, error_domain, /* errorcode= */0,
             "No pattern with id='%s'",pattern_id);
  }
  goto Done;
}

static void bt_wire_pattern_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wire_pattern_persistence_load;
  iface->save = bt_wire_pattern_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void bt_wire_pattern_constructed(GObject *object) {
  BtWirePattern *self=BT_WIRE_PATTERN(object);
  
  if(G_OBJECT_CLASS(bt_wire_pattern_parent_class)->constructed)
    G_OBJECT_CLASS(bt_wire_pattern_parent_class)->constructed(object);

  g_return_if_fail(BT_IS_SONG(self->priv->song));
  g_return_if_fail(BT_IS_WIRE(self->priv->wire));
  g_return_if_fail(BT_IS_PATTERN(self->priv->pattern));
  
  // now that we have wire::param_group and pattern::length create the data
  self->priv->value_group=bt_value_group_new(self->priv->param_group, self->priv->length);

  // add the pattern to the wire
  bt_wire_add_wire_pattern(self->priv->wire,self->priv->pattern,self);
}

static void bt_wire_pattern_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case WIRE_PATTERN_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case WIRE_PATTERN_WIRE: {
      g_value_set_object(value, self->priv->wire);
    } break;
    case WIRE_PATTERN_PATTERN: {
      g_value_set_object(value, self->priv->pattern);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_pattern_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case WIRE_PATTERN_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for pattern: %p",self->priv->song);
    } break;
    case WIRE_PATTERN_WIRE: {
      if((self->priv->wire = BT_WIRE(g_value_get_object(value)))) {
        g_object_try_weak_ref(self->priv->wire);
        g_object_get((gpointer)(self->priv->wire),"num-params",&self->priv->num_params,NULL);
        self->priv->param_group=bt_wire_get_param_group(self->priv->wire);
        GST_DEBUG("set the wire for the wire-pattern: %p",self->priv->wire);
      }
    } break;
    case WIRE_PATTERN_PATTERN: {
      g_object_try_weak_unref(self->priv->pattern);
      if((self->priv->pattern = BT_PATTERN(g_value_get_object(value)))) {
        g_object_try_weak_ref(self->priv->pattern);
        g_object_get((gpointer)(self->priv->pattern),"length",&self->priv->length,NULL);
        // watch the pattern
        self->priv->pattern_length_changed=g_signal_connect((gpointer)(self->priv->pattern),"notify::length",G_CALLBACK(on_pattern_length_changed),(gpointer)self);
        GST_DEBUG("set the pattern for the wire-pattern: %p",self->priv->pattern);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_pattern_dispose(GObject * const object) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  g_object_try_unref(self->priv->value_group);

  g_object_try_weak_unref(self->priv->song);
  g_object_try_weak_unref(self->priv->wire);
  
  if(self->priv->pattern) {
    g_signal_handler_disconnect(self->priv->pattern,self->priv->pattern_length_changed);
    g_object_try_weak_unref(self->priv->pattern);
  }

  G_OBJECT_CLASS(bt_wire_pattern_parent_class)->dispose(object);
}

//-- class internals

static void bt_wire_pattern_init(BtWirePattern *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE_PATTERN, BtWirePatternPrivate);
}

static void bt_wire_pattern_class_init(BtWirePatternClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  error_domain=g_type_qname(BT_TYPE_WIRE_PATTERN);
  g_type_class_add_private(klass,sizeof(BtWirePatternPrivate));

  gobject_class->constructed  = bt_wire_pattern_constructed;
  gobject_class->set_property = bt_wire_pattern_set_property;
  gobject_class->get_property = bt_wire_pattern_get_property;
  gobject_class->dispose      = bt_wire_pattern_dispose;

  /**
   * BtWirePattern::param-changed:
   * @self: the wire-pattern object that emitted the signal
   * @tick: the tick position inside the pattern
   * @wire: the wire for which it changed
   * @param: the parameter index
   *
   * Signals that a param of this wire-pattern has been changed.
   */
  signals[PARAM_CHANGED_EVENT] = g_signal_new("param-changed",
                                 G_TYPE_FROM_CLASS(klass),
                                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                 0,
                                 NULL, // accumulator
                                 NULL, // acc data
                                 bt_marshal_VOID__ULONG_OBJECT_ULONG,
                                 G_TYPE_NONE, // return type
                                 3, // n_params
                                 G_TYPE_ULONG,BT_TYPE_WIRE,G_TYPE_ULONG // param data
                                 );

  /**
   * BtWirePattern::pattern-changed:
   * @self: the wire-pattern object that emitted the signal
   * @intermediate: flag that is %TRUE to signal that more change are coming
   *
   * Signals that this wire-pattern has been changed (more than in one place).
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
                                    0, // n_params
                                    G_TYPE_BOOLEAN
                                    );

  g_object_class_install_property(gobject_class,WIRE_PATTERN_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Song object, the pattern belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_PATTERN_WIRE,
                                  g_param_spec_object("wire",
                                     "wire construct prop",
                                     "Wire object, the wire-pattern belongs to",
                                     BT_TYPE_WIRE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_PATTERN_PATTERN,
                                  g_param_spec_object("pattern",
                                     "pattern construct prop",
                                     "Pattern object, the wire-pattern belongs to",
                                     BT_TYPE_PATTERN, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

