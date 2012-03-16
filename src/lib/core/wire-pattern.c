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
 * - only for processor machines sink machine patterns
 * - volume: is input-volume for the wire
 *   - one volume per input
 * - panning: is panorama on the wire, if we connect e.g. mono -> stereo
 *   - panning parameters can change, if the connection changes
 *   - mono-to-stereo (1->2): 1 parameter
 *   - mono-to-suround (1->4): 2 parameters
 *   - stereo-to-surround (2->4): 1 parameter
 */
/* TODO(ensonic): BtWirePattern is not a good name :/
 * - maybe we can make BtPattern a base class and also have BtMachinePattern
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

  /* an array of GValues (not pointing to)
   * with the size of length*num_params
   */
  GValue *data;
  
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

/*
 * bt_wire_pattern_resize_data_length:
 * @self: the pattern to resize the length
 * @length: the old length
 *
 * Resizes the event data grid to the new length. Keeps previous values.
 */
static void bt_wire_pattern_resize_data_length(const BtWirePattern * const self, const gulong length) {
  const gulong old_data_count=            length*self->priv->num_params;
  const gulong new_data_count=self->priv->length*self->priv->num_params;
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
    GST_DEBUG("extended pattern length from %lu to %lu : data_count=%lu = length=%lu * np=%lu)",
      length,self->priv->length,
      new_data_count,self->priv->length,self->priv->num_params);
  }
  else {
    GST_INFO("extending pattern length from %lu to %lu failed : data_count=%lu = length=%lu * np=%lu)",
      length,self->priv->length,
      new_data_count,self->priv->length,self->priv->num_params);
    //self->priv->data=data;
    //self->priv->length=length;
  }
}

/*
 * bt_wire_pattern_init_event:
 * @self: the pattern that holds the cell
 * @event: the pattern-cell to initialise
 * @param: the index of the param
 *
 * Initialises a pattern cell with the type of the n-th param of the wire.
 */
static void bt_wire_pattern_init_event(const BtWirePattern * const self, GValue * const event, const gulong param) {
  g_return_if_fail(BT_IS_WIRE_PATTERN(self));

  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_parameter_group_get_param_type(self->priv->param_group,param));
  g_assert(G_IS_VALUE(event));
}

//-- event handler

static void on_pattern_length_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
  BtWirePattern *self=BT_WIRE_PATTERN(user_data);
  const gulong length=self->priv->length;

  GST_INFO("pattern length changed : %p",self->priv->pattern);
  g_object_get((gpointer)(self->priv->pattern),"length",&self->priv->length,NULL);
  if(length!=self->priv->length) {
    GST_DEBUG("set the length for wire-pattern: %lu",self->priv->length);
    bt_wire_pattern_resize_data_length(self,length);
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
  gulong data_count;
  gulong i;

  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),NULL);
  
  wire_pattern=bt_wire_pattern_new(self->priv->song,self->priv->wire,pattern);

  data_count=self->priv->length*self->priv->num_params;
  // deep copy data
  for(i=0;i<data_count;i++) {
    if(BT_IS_GVALUE(&self->priv->data[i])) {
      g_value_init(&wire_pattern->priv->data[i],G_VALUE_TYPE(&self->priv->data[i]));
      g_value_copy(&self->priv->data[i],&wire_pattern->priv->data[i]);
    }
  }  
  GST_INFO("  data copied");
  
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
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_wire_pattern_get_event_data(const BtWirePattern * const self, const gulong tick, const gulong param) {
  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),NULL);
  g_return_val_if_fail(self->priv->data,NULL);

  if(G_UNLIKELY(!(tick<self->priv->length))) { GST_ERROR("tick=%lu beyond length=%lu in wire-pattern",tick,self->priv->length);return(NULL); }
  if(G_UNLIKELY(!(param<self->priv->num_params))) { GST_ERROR("param=%lu beyond num_params=%lu in wire-pattern",param,self->priv->num_params);return(NULL); }

  const gulong index=(tick*self->priv->num_params)+param;

  g_assert(index<(self->priv->length*self->priv->num_params));
  return(&self->priv->data[index]);
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
  gboolean res=FALSE;
  GValue *event;

  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  if((event=bt_wire_pattern_get_event_data(self,tick,param))) {
    if(BT_IS_STRING(value)) {
      if(!BT_IS_GVALUE(event)) {
        bt_wire_pattern_init_event(self,event,param);
      }
      if(bt_persistence_set_value(event,value)) {
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
      g_signal_emit((gpointer)self,signals[PARAM_CHANGED_EVENT],0,tick,self->priv->wire,param);
    }
  }
  else {
    GST_DEBUG("no GValue found for cell at tick=%lu, param=%lu",tick,param);
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
  g_return_val_if_fail(tick<self->priv->length,NULL);

  GValue * const event=bt_wire_pattern_get_event_data(self,tick,param);

  if(event && BT_IS_GVALUE(event)) {
    return(bt_persistence_get_value(event));
  }
  return(NULL);
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
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  const GValue * const event=bt_wire_pattern_get_event_data(self,tick,param);

  if(event && BT_IS_GVALUE(event)) {
    return(TRUE);
  }
  return(FALSE);
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
  const gulong num_params=self->priv->num_params;
  gulong k;
  GValue *data;

  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  data=&self->priv->data[tick*num_params];
  for(k=0;k<num_params;k++) {
    if(BT_IS_GVALUE(data)) {
      return(TRUE);
    }
    data++;
  }
  return(FALSE);
}


static void _insert_row(const BtWirePattern * const self, const gulong tick, const gulong param) {
  const gulong length=self->priv->length;
  const gulong num_params=self->priv->num_params;
  GValue *src=&self->priv->data[param+num_params*(length-2)];
  GValue *dst=&self->priv->data[param+num_params*(length-1)];
  gulong i;
  
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

    src-=num_params;
    dst-=num_params;
  }
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
  g_return_if_fail(tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  _insert_row(self,tick,param);
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

  const gulong num_params=self->priv->num_params;
  gulong j;

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  for(j=0;j<num_params;j++) {
    _insert_row(self,tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}


static void _delete_row(const BtWirePattern * const self, const gulong tick, const gulong param) {
  const gulong length=self->priv->length;
  const gulong num_params=self->priv->num_params;
  GValue *src=&self->priv->data[param+num_params*(tick+1)];
  GValue *dst=&self->priv->data[param+num_params*tick];
  gulong i;
  
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
    src+=num_params;
    dst+=num_params;
  }
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
  g_return_if_fail(tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  _delete_row(self,tick,param);
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

  const gulong num_params=self->priv->num_params;
  gulong j;

  GST_DEBUG("insert full-row at %lu", tick);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  for(j=0;j<num_params;j++) {
    _delete_row(self,tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,FALSE);
}



static void _delete_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  GValue *beg=&self->priv->data[param+self->priv->num_params*start_tick];
  gulong i,ticks=(end_tick+1)-start_tick;
  
  for(i=0;i<ticks;i++) {
    if(BT_IS_GVALUE(beg))
      g_value_unset(beg);
    beg+=self->priv->num_params;
  }
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  _delete_column(self,start_tick,end_tick,param);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  const gulong num_params=self->priv->num_params;
  gulong j;

  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0,TRUE);
  for(j=0;j<num_params;j++) {
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

static void _blend_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
	gulong params=self->priv->num_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  GValue *end=&self->priv->data[param+params*end_tick];
  gulong i,ticks=end_tick-start_tick;
  GParamSpec *property;
  GType base_type;

  if(!BT_IS_GVALUE(beg) || !BT_IS_GVALUE(end)) {
    GST_INFO("Can't blend, beg or end is empty");
    return;
  }
  property=bt_parameter_group_get_param_spec(self->priv->param_group, param);
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
			gdouble val1,val2,step;
      gint v;
      
      val1=g_enum_get_value(e,g_value_get_enum(beg))->value;
      val2=g_enum_get_value(e,g_value_get_enum(end))->value;
      step=(val2-val1)/(gdouble)ticks;

      for(i=0;i<ticks;i++) {
        if(!BT_IS_GVALUE(beg))
          g_value_init(beg,property->value_type);
        v=(gint)(val1+(step*i));
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);
  
  _blend_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  const gulong num_params=self->priv->num_params;
  gulong j;
  
  for(j=0;j<num_params;j++) {
    _blend_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0);
}


static void _flip_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  GValue *beg=&self->priv->data[param+self->priv->num_params*start_tick];
  GValue *end=&self->priv->data[param+self->priv->num_params*end_tick];
  GParamSpec *property;
  GType base_type;
  GValue tmp={0,};

  property=bt_parameter_group_get_param_spec(self->priv->param_group, param);
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
    beg+=self->priv->num_params;
    end-=self->priv->num_params;
  }
  g_value_unset(&tmp);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);
  
  _flip_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  const gulong num_params=self->priv->num_params;
  gulong j;
  
  for(j=0;j<num_params;j++) {
    _flip_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0);
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

static void _randomize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  gulong params=self->priv->num_params;
  GValue *beg=&self->priv->data[param+params*start_tick];
  //GValue *end=&self->priv->data[param+params*end_tick];
  gulong i,ticks=(end_tick+1)-start_tick;
  GParamSpec *property;
  GType base_type;
  gdouble rnd;
  
  property=bt_parameter_group_get_param_spec(self->priv->param_group, param);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  _randomize_column(self,start_tick,end_tick,param);
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);

  const gulong num_params=self->priv->num_params;
  gulong j;

  for(j=0;j<num_params;j++) {
    _randomize_column(self,start_tick,end_tick,j);
  }
  g_signal_emit((gpointer)self,signals[PATTERN_CHANGED_EVENT],0);
}


static void _serialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data) {
  GValue *beg=&self->priv->data[param+self->priv->num_params*start_tick];
  gulong i,ticks=(end_tick+1)-start_tick;
  gchar *val;
  
  g_string_append(data,g_type_name(bt_parameter_group_get_param_type(self->priv->param_group,param)));

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
    beg+=self->priv->num_params;
  }
  g_string_append_c(data,'\n');
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);
  g_return_if_fail(data);

  _serialize_column(self,start_tick,end_tick,param,data);
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
  g_return_if_fail(start_tick<self->priv->length);
  g_return_if_fail(end_tick<self->priv->length);
  g_return_if_fail(self->priv->data);
  g_return_if_fail(data);

  const gulong num_params=self->priv->num_params;
  gulong j;

  for(j=0;j<num_params;j++) {
    _serialize_column(self,start_tick,end_tick,j,data);
  }
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
  g_return_val_if_fail(start_tick<self->priv->length,FALSE);
  g_return_val_if_fail(end_tick<self->priv->length,FALSE);
  g_return_val_if_fail(self->priv->data,FALSE);
  g_return_val_if_fail(data,FALSE);
  
  const gulong num_params=self->priv->num_params;
  gboolean ret=TRUE;
  GType stype,dtype;
  gchar **fields;
  
  fields=g_strsplit_set(data,",",0);

  // get types in pattern and clipboard data
  dtype=bt_parameter_group_get_param_type(self->priv->param_group,param);
  
  stype=g_type_from_name(fields[0]);
  if(dtype==stype) {
    gint i=1;
    GValue *beg=&self->priv->data[param+num_params*start_tick];

    GST_INFO("types match %s <-> %s",fields[0],g_type_name(dtype));
    
    while(fields[i] && *fields[i]) {
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
      beg+=num_params;
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
    pattern=bt_machine_get_pattern_by_id(dst_machine,(gchar *)pattern_id);
    
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
        bt_wire_pattern_resize_data_length(self,self->priv->length);
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

  g_object_try_weak_unref(self->priv->song);
  g_object_try_weak_unref(self->priv->wire);
  
  if(self->priv->pattern) {
    g_signal_handler_disconnect(self->priv->pattern,self->priv->pattern_length_changed);
    g_object_try_weak_unref(self->priv->pattern);
  }

  G_OBJECT_CLASS(bt_wire_pattern_parent_class)->dispose(object);
}

static void bt_wire_pattern_finalize(GObject * const object) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->data);

  G_OBJECT_CLASS(bt_wire_pattern_parent_class)->finalize(object);
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
  gobject_class->finalize     = bt_wire_pattern_finalize;

  /**
   * BtWirePattern::param-changed:
   * @self: the wire-pattern object that emitted the signal
   * @tick: the tick position inside the pattern
   * @wire: the wire for which it changed
   * @param: the parameter index
   *
   * signals that a param of this wire-pattern has been changed
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
   *
   * signals that this wire-pattern has been changed (more than in one place)
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

