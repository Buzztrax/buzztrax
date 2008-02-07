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
 * SECTION:btpattern
 * @short_description: class for an event pattern of a #BtMachine instance
 *
 * A pattern contains a grid of events. Events are parameter changes in
 * #BtMachine objects. The events are stored aas #GValues.
 *
 * The patterns are used in the #BtSequence to form the score of a song.
 */
/* @todo:
 * - we need wire_params (volume,panning ) per input
 *   - only for processor machines sink machine patterns
 *   - volume: is input-volume for the wire
 *     - one volume per input
 *   - panning: is panorama on the wire, if we connect e.g. mono -> stereo
 *     - panning parameters can change, if the connection changes
 *     - mono-to-stereo (1->2): 1 parameter
 *     - mono-to-suround (1->4): 2 parameters
 *     - stereo-to-surround (2->4): 1 parameter
 * - BtWirePattern is not a good name :/
 *   - maybe we can make BtPattern a base class and also have BtMachinePattern
 */
#define BT_CORE
#define BT_WIRE_PATTERN_C

#include <libbtcore/core.h>

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
  /* the pattern the wire-pattern belongs to */
  G_POINTER_ALIAS(BtPattern *,pattern);

  /* an array of GValues (not pointing to)
   * with the size of length*num_params
   */
  GValue *data;
  
  /* signal handler ids */
  gint pattern_length_changed;
};

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- enums

//-- helper methods

/*
 * bt_wire_pattern_init_data:
 * @self: the pattern to initialize the event data for
 *
 * Allocates and initializes the memory for the event data grid.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_wire_pattern_init_data(const BtWirePattern * const self) {
  gboolean ret=FALSE;
  const gulong data_count=self->priv->length*self->priv->num_params;

  if(self->priv->wire==NULL) return(TRUE);
  if(data_count==0) return(TRUE);

  if(self->priv->data) {
    GST_INFO("data has already been initialized");
    return(TRUE);
  }

  GST_DEBUG("sizes : %d*%d=%d",self->priv->length,self->priv->num_params);
  if((self->priv->data=g_try_new0(GValue,data_count))) {
    ret=TRUE;
  }
  return(ret);
}

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
      GST_DEBUG("keeping data count=%d, old=%d, new=%d",count,old_data_count,new_data_count);
      // copy old values over
      memcpy(self->priv->data,data,count*sizeof(GValue));
      // free old data
      g_free(data);
    }
    GST_DEBUG("extended pattern length from %d to %d : data_count=%d = length=%d * np=%d)",
      length,self->priv->length,
      new_data_count,self->priv->length,self->priv->num_params);
  }
  else {
    GST_INFO("extending pattern length from %d to %d failed : data_count=%d = length=%d * np=%d)",
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
  g_value_init(event,bt_wire_get_param_type(self->priv->wire,param));
  g_assert(G_IS_VALUE(event));
}

//-- event handler

static void on_pattern_length_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
  BtWirePattern *self=BT_WIRE_PATTERN(user_data);
  const gulong length=self->priv->length;

  GST_INFO("pattern length changed : %p",self->priv->pattern);
  g_object_get(G_OBJECT(self->priv->pattern),"length",&self->priv->length,NULL);
  if(length!=self->priv->length) {
    GST_DEBUG("set the length for wire-pattern: %d",self->priv->length);
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
  BtWirePattern *self;

  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_WIRE(wire),NULL);

  if(!(self=BT_WIRE_PATTERN(g_object_new(BT_TYPE_WIRE_PATTERN,"song",song,"wire",wire,"pattern",pattern,NULL)))) {
    goto Error;
  }
  if(!bt_wire_pattern_init_data(self)) {
    goto Error;
  }
  // add the pattern to the wire
  bt_wire_add_wire_pattern(wire,pattern,self);
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_wire_pattern_get_event_data:
 * @self: the pattern to search for the param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with G_IS_VALUE(event).
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_wire_pattern_get_event_data(const BtWirePattern * const self, const gulong tick, const gulong param) {

  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);
  g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick=%d beyond length=%d in wire-pattern",tick,self->priv->length);return(NULL); }
  if(!(param<self->priv->num_params)) { GST_ERROR("param=%d beyond num_params=%d in wire-pattern",param,self->priv->num_params);return(NULL); }

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
    if(value) {
      if(!G_IS_VALUE(event)) {
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
      if(G_IS_VALUE(event)) {
        g_value_unset(event);
      }
      res=TRUE;
    }
    if(res) {
      // notify others that the data has been changed
      g_signal_emit(G_OBJECT(self),signals[PARAM_CHANGED_EVENT],0,tick,self->priv->wire,param);
      bt_song_set_unsaved(self->priv->song,TRUE);
    }
  }
  else {
    GST_DEBUG("no GValue found for cell at tick=%lu, param=%lu",tick,param);
  }
  return(res);
}

/**
 * bt_wire_pattern_get__event:
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

  if(event && G_IS_VALUE(event)) {
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

  if(event && G_IS_VALUE(event)) {
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
  gulong k;
  GValue *data;

  g_return_val_if_fail(BT_IS_WIRE_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  data=&self->priv->data[tick*self->priv->num_params];
  for(k=0;k<self->priv->num_params;k++) {
    if(G_IS_VALUE(data)) {
      return(TRUE);
    }
    data++;
  }
  return(FALSE);
}

static void _insert_row(const BtWirePattern * const self, const gulong tick, const gulong param) {
  GValue *src=&self->priv->data[param+self->priv->num_params*(self->priv->length-2)];
  GValue *dst=&self->priv->data[param+self->priv->num_params*(self->priv->length-1)];
  gulong i;
  
  GST_INFO("insert row at %lu,%lu", tick, param);

  for(i=tick;i<self->priv->length-1;i++) {
    if(G_IS_VALUE(src)) {
      if(!G_IS_VALUE(dst))
        g_value_init(dst,G_VALUE_TYPE(src));
      g_value_copy(src,dst);
      g_value_unset(src);
    }
    else {
      if(G_IS_VALUE(dst))
        g_value_unset(dst);      
    }

    src-=self->priv->num_params;
    dst-=self->priv->num_params;
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

  _insert_row(self,tick,param);
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
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

  gulong j;

  GST_DEBUG("insert full-row at %lu", time);

  for(j=0;j<self->priv->num_params;j++) {
    _insert_row(self,tick,j);
  }
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
}

static void _delete_row(const BtWirePattern * const self, const gulong tick, const gulong param) {
  GValue *src=&self->priv->data[param+self->priv->num_params*(tick+1)];
  GValue *dst=&self->priv->data[param+self->priv->num_params*tick];
  gulong i;
  
  GST_INFO("insert row at %lu,%lu", tick, param);
  //GST_INFO("one full row has %d params", self->priv->num_params);

  for(i=tick;i<self->priv->length-1;i++) {
    if(G_IS_VALUE(src)) {
      if(!G_IS_VALUE(dst))
        g_value_init(dst,G_VALUE_TYPE(src));
      g_value_copy(src,dst);
      g_value_unset(src);
    }
    else {
      if(G_IS_VALUE(dst))
        g_value_unset(dst);      
    }
    src+=self->priv->num_params;
    dst+=self->priv->num_params;
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

  _delete_row(self,tick,param);
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
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

  gulong j;

  GST_DEBUG("insert full-row at %lu", time);

  for(j=0;j<self->priv->num_params;j++) {
    _delete_row(self,tick,j);
  }
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
}

static void _blend_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  GValue *beg=&self->priv->data[param+self->priv->num_params*start_tick];
  GValue *end=&self->priv->data[param+self->priv->num_params*end_tick];
  gulong i,ticks=end_tick-start_tick;

  if(!G_IS_VALUE(beg) || !G_IS_VALUE(end)) {
    GST_INFO("Can't blend, beg or end is empty");
    return;
  }
  
  GST_INFO("blending gvalue type %s",G_VALUE_TYPE_NAME(end));
  
  // @todo: should this honour the cursor stepping? e.g. enter only every second value
  
  switch(G_VALUE_TYPE(end)) {
    case G_TYPE_INT: {
      gint val=g_value_get_int(beg);
      gdouble step=(gdouble)(g_value_get_int(end)-val)/(gdouble)ticks;
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_INT);
        g_value_set_int(beg,val+(gint)(step*i));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_UINT: {
      gint val=g_value_get_uint(beg);
      gdouble step=(gdouble)(g_value_get_uint(end)-val)/(gdouble)ticks;
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_UINT);
        g_value_set_uint(beg,val+(guint)(step*i));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_FLOAT: {
      gfloat val=g_value_get_float(beg);
      gdouble step=(gdouble)(g_value_get_float(end)-val)/(gdouble)ticks;
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_FLOAT);
        g_value_set_float(beg,val+(gfloat)(step*i));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_DOUBLE: {
      gdouble val=g_value_get_double(beg);
      gdouble step=(gdouble)(g_value_get_double(end)-val)/(gdouble)ticks;
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_DOUBLE);
        g_value_set_double(beg,val+(step*i));
        beg+=self->priv->num_params;
      }
    } break;
    // @todo: need this for more types
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
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
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

  gulong j;
  
  for(j=0;j<self->priv->num_params;j++) {
    _blend_column(self,start_tick,end_tick,j);
  }
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
}

static void _randomize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param) {
  GValue *beg=&self->priv->data[param+self->priv->num_params*start_tick];
  GValue *end=&self->priv->data[param+self->priv->num_params*end_tick];
  gulong i,ticks=(end_tick+1)-start_tick;
  GParamSpec *property;
  GType base_type;
  gdouble rnd;
  
  property=bt_wire_get_param_spec(self->priv->wire, param);
  base_type=bt_g_type_get_base_type(property->value_type);
  
  GST_INFO("blending gvalue type %s",g_type_name(property->value_type));
  
  // @todo: should this honour the cursor stepping? e.g. enter only every second value
  // @todo: if beg and end are not empty, shall we use them as upper and lower
  
  switch(base_type) {
    case G_TYPE_INT: {
      const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_INT);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_int(beg, (gint) (int_property->minimum +
          ((int_property->maximum - int_property->minimum) * rnd)));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_UINT: {
      const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_UINT);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_uint(beg, (guint) (uint_property->minimum +
          ((uint_property->maximum - uint_property->minimum) * rnd)));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_FLOAT: {
      const GParamSpecFloat *float_property = G_PARAM_SPEC_FLOAT (property);
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_FLOAT);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_float(beg, (gfloat) (float_property->minimum +
          ((float_property->maximum - float_property->minimum) * rnd)));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_DOUBLE: {
      const GParamSpecDouble *double_property = G_PARAM_SPEC_DOUBLE (property);
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,G_TYPE_DOUBLE);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_double(beg, (gdouble) (double_property->minimum +
          ((double_property->maximum - double_property->minimum) * rnd)));
        beg+=self->priv->num_params;
      }
    } break;
    case G_TYPE_ENUM:{
      const GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (property);
      const GEnumClass *enum_class = enum_property->enum_class;
      for(i=0;i<ticks;i++) {
        if(!G_IS_VALUE(beg))
          g_value_init(beg,property->value_type);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_enum (beg, (gulong) (enum_class->minimum +
          ((enum_class->maximum - enum_class->minimum) * rnd)));
        beg+=self->priv->num_params;
      }
    } break;
    // @todo: need this for more types
    default:
      GST_WARNING("unhandled gvalue type %s",G_VALUE_TYPE_NAME(end));
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
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
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

  // don't add internal_params here, bt_pattern_insert_row does already
  gulong j;

  for(j=0;j<self->priv->num_params;j++) {
    _randomize_column(self,start_tick,end_tick,j);
  }
  g_signal_emit(G_OBJECT(self),signals[PATTERN_CHANGED_EVENT],0);
}

//-- io interface

static xmlNodePtr bt_wire_pattern_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(persistence);
  gchar *id;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node,child_node2;

  GST_DEBUG("PERSISTENCE::wire-pattern");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("pattern"),NULL))) {
    gulong i,k;
    gchar *value;
    BtWire *wire;
    
    g_object_get(G_OBJECT(self->priv->pattern),"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("pattern"),XML_CHAR_PTR(id));
    g_free(id);

    g_object_get(G_OBJECT(self),"wire",&wire,NULL);

    // save pattern data
    for(i=0;i<self->priv->length;i++) {
      // check if there are any GValues stored ?
      if(bt_wire_pattern_tick_has_data(self,i)) {
        child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("tick"),NULL);
        xmlNewProp(child_node,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
        // save tick data
        for(k=0;k<self->priv->num_params;k++) {
          if((value=bt_wire_pattern_get_event(self,i,k))) {
            child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("wiredata"),NULL);
            xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(bt_wire_get_param_name(wire,k)));
            xmlNewProp(child_node2,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));g_free(value);
          }
        }
      }
    }
    g_object_unref(wire);
  }
  return(node);
}

static gboolean bt_wire_pattern_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(persistence);
  gboolean res=FALSE;
  xmlChar *name,*pattern_id,*tick_str,*value;
  gulong tick,param;
  BtMachine *dst_machine;
  BtWire *wire;
  xmlNodePtr child_node;
  GError *error=NULL;

  GST_DEBUG("PERSISTENCE::wire-pattern");
  g_assert(node);
  
  pattern_id=xmlGetProp(node,XML_CHAR_PTR("pattern"));
  g_object_get(self->priv->wire,"dst",&dst_machine,NULL);
  self->priv->pattern=bt_machine_get_pattern_by_id(dst_machine,(gchar *)pattern_id);
  //g_object_set(G_OBJECT(self),"pattern",pattern,NULL);
  g_object_try_weak_ref(self->priv->pattern);
  // @todo shouldn't we just listen to notify::length and resize patterns automatically
  g_object_get(G_OBJECT(self->priv->pattern),"length",&self->priv->length,NULL);
  xmlFree(pattern_id);
  g_object_unref(dst_machine);
  
  if(!bt_wire_pattern_init_data(self)) {
    GST_WARNING("Can't init wire-pattern data");
    goto Error;
  }
  
  g_object_get(G_OBJECT(self),"wire",&wire,NULL);
  
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
            param=bt_wire_get_param_index(wire,(gchar *)name,&error);
            if(!error) {
              bt_wire_pattern_set_event(self,tick,param,(gchar *)value);
            }
            else {
              GST_WARNING("error while loading wire pattern data at tick %d, param %d: %s",tick,param,error->message);
              g_error_free(error);error=NULL;
              BT_PERSISTENCE_ERROR(Error);
            }  
          }
          xmlFree(name);
	      xmlFree(value);
        }
      }
      xmlFree(tick_str);
    }
  }
  res=TRUE;
Error:
  g_object_unref(wire);
  return(res);
}

static void bt_wire_pattern_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wire_pattern_persistence_load;
  iface->save = bt_wire_pattern_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wire_pattern_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
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

/* sets the given properties for this object */
static void bt_wire_pattern_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtWirePattern * const self = BT_WIRE_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case WIRE_PATTERN_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for pattern: %p",self->priv->song);
    } break;
    case WIRE_PATTERN_WIRE: {
      g_object_try_weak_unref(self->priv->wire);
      if((self->priv->wire = BT_WIRE(g_value_get_object(value)))) {
        g_object_try_weak_ref(self->priv->wire);
        g_object_get(G_OBJECT(self->priv->wire),"num-params",&self->priv->num_params,NULL);
      }
    } break;
    case WIRE_PATTERN_PATTERN: {
      g_object_try_weak_unref(self->priv->pattern);
      if((self->priv->pattern = BT_PATTERN(g_value_get_object(value)))) {
        g_object_try_weak_ref(self->priv->pattern);
        g_object_get(G_OBJECT(self->priv->pattern),"length",&self->priv->length,NULL);
        // watch the pattern
        self->priv->pattern_length_changed=g_signal_connect(G_OBJECT(self->priv->pattern),"notify::length",G_CALLBACK(on_pattern_length_changed),(gpointer)self);
        GST_INFO("set the length for the wire-pattern: %p",self->priv->length);
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

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_wire_pattern_finalize(GObject * const object) {
  const BtWirePattern * const self = BT_WIRE_PATTERN(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->data);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_wire_pattern_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtWirePattern * const self = BT_WIRE_PATTERN(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE_PATTERN, BtWirePatternPrivate);
}

static void bt_wire_pattern_class_init(BtWirePatternClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWirePatternPrivate));

  gobject_class->set_property = bt_wire_pattern_set_property;
  gobject_class->get_property = bt_wire_pattern_get_property;
  gobject_class->dispose      = bt_wire_pattern_dispose;
  gobject_class->finalize     = bt_wire_pattern_finalize;

  klass->param_changed_event = NULL;

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
                                 (guint)G_STRUCT_OFFSET(BtWirePatternClass,param_changed_event),
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
                                        (guint)G_STRUCT_OFFSET(BtWirePatternClass,pattern_changed_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0 // n_params
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

GType bt_wire_pattern_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtWirePatternClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_pattern_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtWirePattern),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wire_pattern_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_wire_pattern_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtWirePattern",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
