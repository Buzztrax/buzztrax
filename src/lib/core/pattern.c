/* $Id: pattern.c,v 1.95 2007-04-20 10:56:46 ensonic Exp $
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
#define BT_CORE
#define BT_PATTERN_C

#include <libbtcore/core.h>

//-- signal ids


enum {
  GLOBAL_PARAM_CHANGED_EVENT,
  VOICE_PARAM_CHANGED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  PATTERN_SONG=1,
  PATTERN_ID,
  PATTERN_NAME,
  PATTERN_LENGTH,
  PATTERN_VOICES,
  PATTERN_MACHINE,
  PATTERN_IS_INTERNAL
};

struct _BtPatternPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the pattern belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* the id, we can use to lookup the pattern */
  gchar *id;
  /* the display name of the pattern */
  gchar *name;
  /* command patterns are internal (the are not editable) */
  gboolean is_internal;

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

  /* an array of GValues (not pointing to)
   * with the size of length*(internal_params+global_params+voices*voice_params)
   */
  GValue *data;
};

static GQuark error_domain=0;

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

/*
 * @todo we need more params:
 * - the machine state (BtMachineState: normal, mute, solo, bypass)
 * - the input and output gain
 */
static gulong internal_params=1;

//-- enums

GType bt_pattern_cmd_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type == 0)) {
    static const GEnumValue values[] = {
      { BT_PATTERN_CMD_NORMAL,"BT_PATTERN_CMD_NORMAL","just working" },
      { BT_PATTERN_CMD_BREAK, "BT_PATTERN_CMD_BREAK", "no more notes" },
      { BT_PATTERN_CMD_MUTE,  "BT_PATTERN_CMD_MUTE",  "be quiet immediately" },
      { BT_PATTERN_CMD_SOLO,  "BT_PATTERN_CMD_SOLO",  "be the only one playing" },
      { BT_PATTERN_CMD_BYPASS,"BT_PATTERN_CMD_BYPASS","be uneffective (pass through)" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtPatternCmd", values);
  }
  return type;
}

//-- helper methods

/*
 * bt_pattern_init_data:
 * @self: the pattern to initialize the event data for
 *
 * Allocates and initializes the memory for the event data grid.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_pattern_init_data(const BtPattern * const self) {
  gboolean ret=FALSE;
  const gulong data_count=self->priv->length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params);

  if(self->priv->machine==NULL) return(TRUE);
  if(data_count==0) return(TRUE);

  if(self->priv->data) {
    GST_INFO("data has already been initialized");
    return(TRUE);
  }

  GST_DEBUG("sizes : %d*(%d+%d+%d*%d)=%d",self->priv->length,internal_params,self->priv->global_params,self->priv->voices,self->priv->voice_params,data_count);
  if((self->priv->data=g_try_new0(GValue,data_count))) {
    ret=TRUE;
  }
  return(ret);
}

/*
 * bt_pattern_resize_data_length:
 * @self: the pattern to resize the length
 * @length: the old length
 *
 * Resizes the event data grid to the new length. Keeps previous values.
 */
static void bt_pattern_resize_data_length(const BtPattern * const self, const gulong length) {
  const gulong old_data_count=            length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params);
  const gulong new_data_count=self->priv->length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params);
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
    GST_DEBUG("extended pattern length from %d to %d : data_count=%d = length=%d * ( ip=%d + gp=%d + voices=%d * vp=%d )",
      length,self->priv->length,
      new_data_count,self->priv->length,
      internal_params,self->priv->global_params,self->priv->voices,self->priv->voice_params);
  }
  else {
    GST_INFO("extending pattern length from %d to %d failed : data_count=%d = length=%d * ( ip=%d + gp=%d + voices=%d * vp=%d )",
      length,self->priv->length,
      new_data_count,self->priv->length,
      internal_params,self->priv->global_params,self->priv->voices,self->priv->voice_params);
    //self->priv->data=data;
    //self->priv->length=length;
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
  //gulong old_data_count=self->priv->length*(internal_params+self->priv->global_params+          voices*self->priv->voice_params);
  const gulong new_data_count=self->priv->length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params);
  GValue * const data=self->priv->data;

  // allocate new space
  if((self->priv->data=g_try_new0(GValue,new_data_count))) {
    if(data) {
      gulong i;
      const gulong count    =sizeof(GValue)*(internal_params+self->priv->global_params+self->priv->voice_params*MIN(voices,self->priv->voices));
      const gulong src_count=internal_params+self->priv->global_params+            voices*self->priv->voice_params;
      const gulong dst_count=internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params;
      const GValue *src=data;
      GValue *dst=self->priv->data;

      GST_DEBUG("keeping data count=%d, src_ct=%d, dst_ct=%d",count,src_count,dst_count);

      // copy old values over
      for(i=0;i<self->priv->length;i++) {
        memcpy(dst,src,count);
        src=&src[src_count];
        dst=&dst[dst_count];
      }
      // free old data
      g_free(data);
    }
    GST_DEBUG("extended pattern voices from %d to %d : data_count=%d = length=%d * ( ip=%d + gp=%d + voices=%d * vp=%d )",
      voices,self->priv->voices,
      new_data_count,self->priv->length,
      internal_params,self->priv->global_params,self->priv->voices,self->priv->voice_params);
  }
  else {
    GST_INFO("extending pattern voices from %d to %d failed : data_count=%d = length=%d * ( ip=%d + gp=%d + voices=%d * vp=%d )",
      voices,self->priv->voices,
      new_data_count,self->priv->length,
      internal_params,self->priv->global_params,self->priv->voices,self->priv->voice_params);
    //self->priv->data=data;
    //self->priv->voices=voices;
  }
}

/*
 * bt_pattern_get_internal_event_data:
 * @self: the pattern to search for the internal param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the internal parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
static GValue *bt_pattern_get_internal_event_data(const BtPattern * const self, const gulong tick, const gulong param) {
  gulong index;

  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick beyond length");return(NULL); }
  if(!(param<internal_params)) { GST_ERROR("param beyond internal_params");return(NULL); }

  index=(tick*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params))
       + param;
  return(&self->priv->data[index]);
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
  g_value_init(event,bt_machine_get_global_param_type(self->priv->machine,param));
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
  g_value_init(event,bt_machine_get_voice_param_type(self->priv->machine,param));
  g_assert(G_IS_VALUE(event));
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
  BtPattern *self;
  gulong voices;

  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);
  g_return_val_if_fail(BT_IS_STRING(name),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(machine),NULL);

  g_object_get(G_OBJECT(machine),"voices",&voices,NULL);
  if(!(self=BT_PATTERN(g_object_new(BT_TYPE_PATTERN,"song",song,"id",id,"name",name,"length",length,"voices",voices,"machine",machine,NULL)))) {
    goto Error;
  }
  if(!bt_pattern_init_data(self)) {
    goto Error;
  }
  // add the pattern to the machine
  bt_machine_add_pattern(machine,self);
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

/**
 * bt_pattern_new_with_event:
 * @song: the song the new instance belongs to
 * @machine: the machine the pattern belongs to
 * @cmd: a #BtPatternCmd
 *
 * Create a new default pattern instance containg the given @cmd event.
 * It will be automatically added to the machines pattern list.
 * If @cmd is %BT_PATTERN_CMD_NORMAL use bt_pattern_new() instead.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPattern *bt_pattern_new_with_event(const BtSong * const song, const BtMachine * const machine, const BtPatternCmd cmd) {
  BtPattern *self;
  gchar *mid,*id,*name;
  GValue *event;
  const gchar * const cmd_names[]={ N_("normal"),N_("break"),N_("mute"),N_("solo"),N_("bypass") };

  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(machine),NULL);

  g_object_get(G_OBJECT(machine),"id",&mid,NULL);
  id=g_strdup_printf("%s___%s",mid,cmd_names[cmd]);
  name=g_strdup_printf("   %s",_(cmd_names[cmd]));
  if(!(self=BT_PATTERN(g_object_new(BT_TYPE_PATTERN,"song",song,"id",id,"name",name,"length",1,"machine",machine,"is-internal",TRUE,NULL)))) {
    goto Error;
  }
  if(!bt_pattern_init_data(self)) {
    goto Error;
  }
  event=bt_pattern_get_internal_event_data(self,0,0);
  //bt_pattern_init_internal_event(self,event,0);
  g_value_init(event,BT_TYPE_PATTERN_CMD);
  g_value_set_enum(event,cmd);

  // add the pattern to the machine
  bt_machine_add_pattern(machine,self);
  g_free(mid);
  g_free(id);
  g_free(name);
  return(self);
Error:
  g_object_try_unref(self);
  g_free(mid);
  g_free(id);
  g_free(name);
  return(NULL);
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

  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);

  GST_INFO("copying pattern");

  g_object_get(G_OBJECT(self->priv->machine),"id",&mid,NULL);

  name=bt_machine_get_unique_pattern_name(self->priv->machine);
  id=g_strdup_printf("%s %s",mid,name);

  if((pattern=bt_pattern_new(self->priv->song,id,name,self->priv->length,self->priv->machine))) {
    const gulong data_count=self->priv->length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params);

    // copy data
    memcpy(pattern->priv->data,self->priv->data,data_count*sizeof(GValue));
    GST_INFO("  data copied");
    // add the pattern to the machine
    bt_machine_add_pattern(self->priv->machine,self);
  }

  g_free(mid);
  g_free(id);
  g_free(name);
  return(pattern);
}

//-- methods

/**
 * bt_pattern_get_global_param_index:
 * @self: the pattern to search for the global dparam
 * @name: the name of the global dparam
 * @error: pointer to an error variable
 *
 * Searches the list of registered dparam of the machine the pattern belongs to
 * for a global dparam of the given name and returns the index if found.
 *
 * Returns: the index. If an error occurs the function returns 0 and sets the
 * error variable. You should always check for error if you use this function.
 */
gulong bt_pattern_get_global_param_index(const BtPattern * const self, const gchar * const name, GError **error) {
  gulong ret=0;
  GError *tmp_error=NULL;

  g_return_val_if_fail(BT_IS_PATTERN(self),0);
  g_return_val_if_fail(BT_IS_STRING(name),0);
  g_return_val_if_fail(error == NULL || *error == NULL, 0);

  ret=bt_machine_get_global_param_index(self->priv->machine,name,&tmp_error);

  if (tmp_error!=NULL) {
    //g_set_error (error, error_domain, /* errorcode= */0,
    //             "global dparam for name %s not found", name);
    g_propagate_error(error, tmp_error);
  }
  return(ret);
}

/**
 * bt_pattern_get_voice_param_index:
 * @self: the pattern to search for the voice dparam
 * @name: the name of the voice dparam
 * @error: pointer to an error variable
 *
 * Searches the list of registered dparam of the machine the pattern belongs to
 * for a voice dparam of the given name and returns the index if found.
 *
 * Returns: the index. If an error occurs the function returns 0 and sets the
 * error variable. You should always check for error if you use this function.
 */
gulong bt_pattern_get_voice_param_index(const BtPattern * const self, const gchar * const name, GError **error) {
  gulong ret=0;
  GError *tmp_error=NULL;

  g_return_val_if_fail(BT_IS_PATTERN(self),0);
  g_return_val_if_fail(BT_IS_STRING(name),0);
  g_return_val_if_fail(error == NULL || *error == NULL, 0);

  ret=bt_machine_get_voice_param_index(self->priv->machine,name,&tmp_error);

  if (tmp_error!=NULL) {
    //g_set_error (error, error_domain, /* errorcode= */0,
    //            "voice dparam for name %s not found", name);
    g_propagate_error(error, tmp_error);
  }
  return(ret);
}

/**
 * bt_pattern_get_global_event_data:
 * @self: the pattern to search for the global param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with G_IS_VALUE(event).
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_global_event_data(const BtPattern * const self, const gulong tick, const gulong param) {

  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);
  g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick=%d beyond length=%d in pattern '%s'",tick,self->priv->length,self->priv->id);return(NULL); }
  if(!(param<self->priv->global_params)) { GST_ERROR("param=%d beyond global_params=%d in pattern '%s'",param,self->priv->global_params,self->priv->id);return(NULL); }

  const gulong index=(tick*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params))
       +       internal_params+param;

  g_assert(index<(self->priv->length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params)));
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
 * there, then the %GValue is uninitialized. Test with G_IS_VALUE(event).
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_voice_event_data(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param) {

  g_return_val_if_fail(BT_IS_PATTERN(self),NULL);
  g_return_val_if_fail(tick<self->priv->length,NULL);
  g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick=%d beyond length=%d in pattern '%s'",tick,self->priv->length,self->priv->id);return(NULL); }
  if(!(voice<self->priv->voices)) { GST_ERROR("voice=%d beyond voices=%d in pattern '%s'",voice,self->priv->voices,self->priv->id);return(NULL); }
  if(!(param<self->priv->voice_params)) { GST_ERROR("param=%d  beyond voice_ params=%d in pattern '%s'",param,self->priv->voice_params,self->priv->id);return(NULL); }

  const gulong index=(tick*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params))
       +       internal_params+self->priv->global_params+(voice*self->priv->voice_params)
       + param;

  g_assert(index<(self->priv->length*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params)));
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
  GValue *event;

  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  if((event=bt_pattern_get_global_event_data(self,tick,param))) {
    if(!G_IS_VALUE(event)) {
      bt_pattern_init_global_event(self,event,param);
    }
    if(bt_persistence_set_value(event,value)) {
      if(bt_machine_is_global_param_no_value(self->priv->machine,param,event)) {
        g_value_unset(event);
      }
      else {
        // notify others that the data has been changed
        g_signal_emit(G_OBJECT(self),signals[GLOBAL_PARAM_CHANGED_EVENT],0,tick,param);
        return(TRUE);
      }
    }
    else {
      GST_INFO("failed to set GValue for cell at tick=%lu, param=%lu",tick,param);
    }
  }
  else {
    GST_DEBUG("no GValue found for cell at tick=%lu, param=%lu",tick,param);
  }
  return(FALSE);
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

  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  GValue * const event=bt_pattern_get_voice_event_data(self,tick,voice,param);

  if(event) {
    if(!G_IS_VALUE(event)) {
      bt_pattern_init_voice_event(self,event,param);
    }
    if(bt_persistence_set_value(event,value)) {
      if(bt_machine_is_voice_param_no_value(self->priv->machine,param,event)) {
        g_value_unset(event);
      }
      else {
        // notify others that the data has been changed
        g_signal_emit(G_OBJECT(self),signals[VOICE_PARAM_CHANGED_EVENT],0,tick,voice,param);
        return(TRUE);
      }
    }
    else {
      GST_INFO("failed to set GValue for cell at tick=%lu, voice=%lu, param=%lu",tick,voice,param);
    }
  }
  else {
    GST_DEBUG("no GValue found for cell at tick=%lu, voice=%lu, param=%lu",tick,voice,param);
  }
  return(FALSE);
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

  if(event && G_IS_VALUE(event)) {
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

  if(event && G_IS_VALUE(event)) {
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

  if(event && G_IS_VALUE(event)) {
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

  if(event && G_IS_VALUE(event)) {
    return(TRUE);
  }
  return(FALSE);
}

/**
 * bt_pattern_get_cmd:
 * @self: the pattern to query the command from
 * @tick: the tick (time) position starting with 0
 *
 * Returns the command id in the specified tick row.
 *
 * Returns: the command id
 */
BtPatternCmd bt_pattern_get_cmd(const BtPattern * const self, const gulong tick) {

  g_return_val_if_fail(BT_IS_PATTERN(self),BT_PATTERN_CMD_NORMAL);
  g_return_val_if_fail(tick<self->priv->length,BT_PATTERN_CMD_NORMAL);

  const GValue * const event=bt_pattern_get_internal_event_data(self,tick,0);

  if(event && G_IS_VALUE(event)) {
    return(g_value_get_enum(event));
  }
  return(BT_PATTERN_CMD_NORMAL);
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
  gulong j,k;
  GValue *data;

  g_return_val_if_fail(BT_IS_PATTERN(self),FALSE);
  g_return_val_if_fail(tick<self->priv->length,FALSE);

  data=&self->priv->data[internal_params+tick*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params)];
  for(k=0;k<self->priv->global_params;k++) {
    if(G_IS_VALUE(data)) {
      return(TRUE);
    }
    data++;
  }
  for(j=0;j<self->priv->voices;j++) {
    for(k=0;k<self->priv->voice_params;k++) {
      if(G_IS_VALUE(data)) {
        return(TRUE);
      }
      data++;
    }
  }
  return(FALSE);
}

/**
 * bt_pattern_play_tick:
 * @self: the pattern a tick should be played from
 * @tick: the tick index in the pattern
 *
 * Pushes all control changes from this pattern-row into the #BtMachine params.
 */
void bt_pattern_play_tick(const BtPattern * const self, const gulong tick) {
  gulong j,k;
  GValue *data;

  g_return_if_fail(BT_IS_PATTERN(self));
  g_return_if_fail(tick<self->priv->length);

  data=&self->priv->data[tick*(internal_params+self->priv->global_params+self->priv->voices*self->priv->voice_params)];
  for(k=0;k<self->priv->global_params;k++) {
    if(G_IS_VALUE(data)) {
      bt_machine_set_global_param_value(self->priv->machine,k,data);
    }
    else {
      bt_machine_set_global_param_no_value(self->priv->machine,k);
    }
    data++;
  }
  for(j=0;j<self->priv->voices;j++) {
    for(k=0;k<self->priv->voice_params;k++) {
      if(G_IS_VALUE(data)) {
        bt_machine_set_voice_param_value(self->priv->machine,j,k,data);
      }
      else {
        bt_machine_set_voice_param_no_value(self->priv->machine,j,k);
      }
      data++;
    }
  }
}

//-- io interface

static xmlNodePtr bt_pattern_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtPattern * const self = BT_PATTERN(persistence);
  gchar *id;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node,child_node2;

  GST_DEBUG("PERSISTENCE::pattern");

  // @todo: hack, command-patterns start with "   "
  if(self->priv->name[0]==' ') return((xmlNodePtr)1);

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("pattern"),NULL))) {
    gulong i,j,k;
    gchar *value;

    //id=g_alloca(strlen(machine_id)+strlen(name)+2);
    //g_sprintf(id,"%s_%s",machine_id,name);
    //g_object_set(G_OBJECT(pattern),"id",id,NULL);

    xmlNewProp(node,XML_CHAR_PTR("id"),XML_CHAR_PTR(self->priv->id));
    xmlNewProp(node,XML_CHAR_PTR("name"),XML_CHAR_PTR(self->priv->name));
    xmlNewProp(node,XML_CHAR_PTR("length"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->length)));

    g_object_get(G_OBJECT(self->priv->machine),"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("machine"),XML_CHAR_PTR(id));
    g_free(id);

    // save pattern data
    for(i=0;i<self->priv->length;i++) {
      // check if there are any GValues stored ?
      if(bt_pattern_tick_has_data(self,i)) {
        child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("tick"),NULL);
        xmlNewProp(child_node,XML_CHAR_PTR("time"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(i)));
        // save tick data
        for(k=0;k<self->priv->global_params;k++) {
          if((value=bt_pattern_get_global_event(self,i,k))) {
            child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("globaldata"),NULL);
            xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(bt_machine_get_global_param_name(self->priv->machine,k)));
            xmlNewProp(child_node2,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));g_free(value);
          }
        }
        for(j=0;j<self->priv->voices;j++) {
          const gchar * const voice_str=bt_persistence_strfmt_ulong(j);
          for(k=0;k<self->priv->voice_params;k++) {
            if((value=bt_pattern_get_voice_event(self,i,j,k))) {
              child_node2=xmlNewChild(child_node,NULL,XML_CHAR_PTR("voicedata"),NULL);
              xmlNewProp(child_node2,XML_CHAR_PTR("voice"),XML_CHAR_PTR(voice_str));
              xmlNewProp(child_node2,XML_CHAR_PTR("name"),XML_CHAR_PTR(bt_machine_get_voice_param_name(self->priv->machine,k)));
              xmlNewProp(child_node2,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));g_free(value);
            }
          }
        }
      }
    }
  }
  return(node);
}

static gboolean bt_pattern_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  const BtPattern * const self = BT_PATTERN(persistence);
  gboolean res=FALSE;
  xmlChar *id,*name,*length_str,*tick_str,*value,*voice_str;
  glong tick,voice,param;
  gulong length;
  xmlNodePtr child_node;
  GError *error=NULL;

  GST_DEBUG("PERSISTENCE::pattern");
  g_assert(node);

  id=xmlGetProp(node,XML_CHAR_PTR("id"));
  name=xmlGetProp(node,XML_CHAR_PTR("name"));
  length_str=xmlGetProp(node,XML_CHAR_PTR("length"));
  length=length_str?atol((char *)length_str):0;
  g_object_set(G_OBJECT(self),"id",id,"name",name,"length",length,NULL);
  xmlFree(id);xmlFree(name);xmlFree(length_str);

  if(!bt_pattern_init_data(self)) {
    GST_WARNING("Can't init pattern data");
    goto Error;
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
          if(!strncmp((char *)child_node->name,"globaldata\0",11)) {
            param=bt_pattern_get_global_param_index(self,(gchar *)name,&error);
            if(!error) {
              bt_pattern_set_global_event(self,tick,param,(gchar *)value);
            }
            else {
              GST_WARNING("error while loading global pattern data at tick %d, param %d: %s",tick,param,error->message);
              g_error_free(error);
              BT_PERSISTENCE_ERROR(Error);
            }
          }
          else if(!strncmp((char *)child_node->name,"voicedata\0",10)) {
            voice_str=xmlGetProp(child_node,XML_CHAR_PTR("voice"));
            voice=atol((char *)voice_str);
            param=bt_pattern_get_voice_param_index(self,(gchar *)name,&error);
            if(!error) {
              bt_pattern_set_voice_event(self,tick,voice,param,(gchar *)value);
            }
            else {
              GST_WARNING("error while loading voice pattern data at tick %d, param %d, voice %d: %s",tick,param,voice,error->message);
              g_error_free(error);
              BT_PERSISTENCE_ERROR(Error);
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

  res=TRUE;
Error:
  return(res);
}

static void bt_pattern_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_pattern_persistence_load;
  iface->save = bt_pattern_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_pattern_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtPattern * const self = BT_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case PATTERN_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case PATTERN_ID: {
      g_value_set_string(value, self->priv->id);
    } break;
    case PATTERN_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case PATTERN_LENGTH: {
      g_value_set_ulong(value, self->priv->length);
    } break;
    case PATTERN_VOICES: {
      g_value_set_ulong(value, self->priv->voices);
    } break;
    case PATTERN_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    case PATTERN_IS_INTERNAL: {
      g_value_set_boolean(value, self->priv->is_internal);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_pattern_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtPattern * const self = BT_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case PATTERN_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for pattern: %p",self->priv->song);
    } break;
    case PATTERN_ID: {
      g_free(self->priv->id);
      self->priv->id = g_value_dup_string(value);
      GST_DEBUG("set the id for pattern: %s",self->priv->id);
    } break;
    case PATTERN_NAME: {
      g_free(self->priv->name);
      self->priv->name = g_value_dup_string(value);
      GST_DEBUG("set the display name for the pattern: %s",self->priv->name);
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case PATTERN_LENGTH: {
      const gulong length=self->priv->length;
      self->priv->length = g_value_get_ulong(value);
      if(length!=self->priv->length) {
        GST_DEBUG("set the length for pattern: %d",self->priv->length);
        bt_pattern_resize_data_length(self,length);
        bt_song_set_unsaved(self->priv->song,TRUE);
      }
    } break;
    case PATTERN_VOICES: {
      const gulong voices=self->priv->voices;
      self->priv->voices = g_value_get_ulong(value);
      if(voices!=self->priv->voices) {
        GST_DEBUG("set the voices for pattern: %d",self->priv->voices);
        bt_pattern_resize_data_voices(self,voices);
        bt_song_set_unsaved(self->priv->song,TRUE);
      }
    } break;
    case PATTERN_MACHINE: {
      g_object_try_weak_unref(self->priv->machine);
      if((self->priv->machine = BT_MACHINE(g_value_get_object(value)))) {
        g_object_try_weak_ref(self->priv->machine);
        // @todo shouldn't we just listen to notify::voices too and resize patterns automatically
        g_object_get(G_OBJECT(self->priv->machine),"global-params",&self->priv->global_params,"voice-params",&self->priv->voice_params,NULL);
        GST_DEBUG("set the machine for pattern: %p (machine-refs: %d)",self->priv->machine,(G_OBJECT(self->priv->machine))->ref_count);
      }
      else {
        GST_DEBUG("unset the machine for pattern");
      }
    } break;
    case PATTERN_IS_INTERNAL: {
      self->priv->is_internal = g_value_get_boolean(value);
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

  g_object_try_weak_unref(self->priv->song);
  g_object_try_weak_unref(self->priv->machine);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_pattern_finalize(GObject * const object) {
  const BtPattern * const self = BT_PATTERN(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->id);
  g_free(self->priv->name);
  g_free(self->priv->data);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_pattern_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtPattern * const self = BT_PATTERN(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PATTERN, BtPatternPrivate);
}

static void bt_pattern_class_init(BtPatternClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  error_domain=g_quark_from_static_string("BtPattern");

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtPatternPrivate));

  gobject_class->set_property = bt_pattern_set_property;
  gobject_class->get_property = bt_pattern_get_property;
  gobject_class->dispose      = bt_pattern_dispose;
  gobject_class->finalize     = bt_pattern_finalize;

  klass->global_param_changed_event = NULL;
  klass->voice_param_changed_event = NULL;

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
                                        G_ABS_STRUCT_OFFSET(BtPatternClass,global_param_changed_event),
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
                                        G_ABS_STRUCT_OFFSET(BtPatternClass,voice_param_changed_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        bt_marshal_VOID__ULONG_ULONG_ULONG,
                                        G_TYPE_NONE, // return type
                                        3, // n_params
                                        G_TYPE_ULONG,G_TYPE_ULONG,G_TYPE_ULONG // param data
                                        );

  g_object_class_install_property(gobject_class,PATTERN_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Song object, the pattern belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,PATTERN_ID,
                                  g_param_spec_string("id",
                                     "id contruct prop",
                                     "pattern identifier",
                                     "unamed pattern", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,PATTERN_NAME,
                                  g_param_spec_string("name",
                                     "name contruct prop",
                                     "the display-name of the pattern",
                                     "unamed", /* default value */
                                     G_PARAM_READWRITE));

   g_object_class_install_property(gobject_class,PATTERN_LENGTH,
                                  g_param_spec_ulong("length",
                                     "length prop",
                                     "length of the pattern in ticks",
                                     1,
                                     G_MAXULONG,
                                     1,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,PATTERN_VOICES,
                                  g_param_spec_ulong("voices",
                                     "voices prop",
                                     "number of voices in the pattern",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,PATTERN_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Machine object, the pattern belongs to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,PATTERN_IS_INTERNAL,
                                  g_param_spec_boolean("is-internal",
                                     "is-internal construct prop",
                                     "internal (cmd-pattern) indicator",
                                     FALSE,
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_pattern_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtPatternClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_pattern_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtPattern),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_pattern_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_pattern_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtPattern",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
