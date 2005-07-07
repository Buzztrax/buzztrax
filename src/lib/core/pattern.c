/* $Id: pattern.c,v 1.51 2005-07-07 21:44:58 ensonic Exp $
 * class for an event pattern of a #BtMachine instance
 */
 
#define BT_CORE
#define BT_PATTERN_C

#include <libbtcore/core.h>

//-- signal ids

enum {
  CHANGED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  PATTERN_SONG=1,
  PATTERN_ID,
	PATTERN_NAME,
  PATTERN_LENGTH,
  PATTERN_VOICES,
  PATTERN_MACHINE
};

struct _BtPatternPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the pattern belongs to */
	BtSong *song;
	/* the id, we can use to lookup the pattern */
	gchar *id;
	/* the display name of the pattern */
	gchar *name;

  /* the number of ticks */
  gulong length;
  /* the number of voices */
  gulong voices;
  /* the number of dynamic params the machine provides per instance */
  gulong global_params;
  /* the number of dynamic params the machine provides per instance and voice */
  gulong voice_params;
  /* the machine the pattern belongs to */
  BtMachine *machine;

  /* an array of GValue
   * with the size of length*(global_params+voices*voice_params)
   */
	GValue *data;
};

static GQuark error_domain=0;

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- helper methods

static gboolean bt_pattern_init_data(const BtPattern *self) {
  gboolean ret=FALSE;
  gulong data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
  //GValue *data;

  if(self->priv->machine==NULL) return(TRUE);
  if(data_count==0) return(TRUE);
		
  if(self->priv->data) {
    GST_INFO("data has already been initialized");
    return(TRUE);
  }

  GST_DEBUG("sizes : %d*(%d+%d*%d)=%d",self->priv->length,self->priv->global_params,self->priv->voices,self->priv->voice_params,data_count);
  if((self->priv->data=g_try_new0(GValue,data_count))) {
    // initialize the GValues (can we use memcpy for the tick-lines)
    /*
    data=self->priv->data;
    for(i=0;i<self->priv->length;i++) {
      for(k=0;k<self->priv->global_params;k++) {
        g_value_init(data,bt_machine_get_global_param_type(self->priv->machine,k));
        data++;
      }
      for(j=0;j<self->priv->voices;j++) {
        for(k=0;k<self->priv->voice_params;k++) {
          g_value_init(data,bt_machine_get_voice_param_type(self->priv->machine,k));
          data++;
        }
      }
    }
    */
    ret=TRUE;
  }
  return(ret);
}

static void bt_pattern_resize_data_length(const BtPattern *self, gulong length) {
	gulong old_data_count=length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
	gulong new_data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
	GValue *data=self->priv->data;
	
  // allocate new space
	if((self->priv->data=g_try_new0(GValue,new_data_count))) {
		if(data) {
			// copy old values over
			memcpy(self->priv->data,data,MIN(old_data_count,new_data_count*sizeof(GValue)));
			// free old data
			g_free(data);
		}
  }
	else {
		GST_WARNING("extending pattern length from %d to %d failed : data_count=%d = length=%d * ( gp=%d + voices=%d * vp=%d )",
			length,self->priv->length,
			new_data_count,self->priv->length,self->priv->global_params,self->priv->voices,self->priv->voice_params);
		//self->priv->data=data;
		//self->priv->length=length;
	}
}

static void bt_pattern_resize_data_voices(const BtPattern *self, gulong voices) {
	//gulong old_data_count=self->priv->length*(self->priv->global_params+voices*self->priv->voice_params);
	gulong new_data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
	GValue *data=self->priv->data;
	
  // allocate new space
	if((self->priv->data=g_try_new0(GValue,new_data_count))) {
		if(data) {
			gulong i;
			gulong count=self->priv->voice_params*MIN(voices,self->priv->voices);
			GValue *src,*dst;
		
			// copy old values over
			src=&data[self->priv->global_params];
			dst=&self->priv->data[self->priv->global_params];
			for(i=0;i<self->priv->length;i++) {
				memcpy(dst,src,count*sizeof(GValue));
				src=&src[self->priv->global_params+voices*self->priv->voice_params];
				dst=&dst[self->priv->global_params+self->priv->voices*self->priv->voice_params];			
			}
			// free old data
			g_free(data);
		}
  }
	else {
		GST_WARNING("extending pattern voices from %d to %d failed : data_count=%d = length=%d * ( gp=%d + voices=%d * vp=%d )",
			voices,self->priv->voices,
			new_data_count,self->priv->length,self->priv->global_params,self->priv->voices,self->priv->voice_params);
		//self->priv->data=data;
		//self->priv->voices=voices;
	}
}

/*
 * bt_pattern_get_global_event_data:
 * @self: the pattern to search for the global dparam
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
static GValue *bt_pattern_get_global_event_data(const BtPattern *self, gulong tick, gulong param) {
  gulong index;

  g_assert(BT_IS_PATTERN(self));
	g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick beyond length");return(NULL); }
  if(!(param<self->priv->global_params)) { GST_ERROR("param beyond global_params");return(NULL); }

  index=(tick*(self->priv->global_params+self->priv->voices*self->priv->voice_params))
       + param;
  return(&self->priv->data[index]);
}

/*
 * bt_pattern_get_voice_event_data:
 * @self: the pattern to search for the voice dparam
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the voice parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
static GValue *bt_pattern_get_voice_event_data(const BtPattern *self, gulong tick, gulong voice, gulong param) {
  gulong index;

  g_assert(BT_IS_PATTERN(self));
	g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick beyond length");return(NULL); }
  if(!(voice<self->priv->voices)) { GST_ERROR("voice beyond voices");return(NULL); }
  if(!(param<self->priv->voice_params)) { GST_ERROR("param beyond voice_ params");return(NULL); }

  index=(tick*(self->priv->global_params+self->priv->voices*self->priv->voice_params))
       +       self->priv->global_params+(voice*self->priv->voice_params)
       +param;
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
static void bt_pattern_init_global_event(const BtPattern *self, GValue *event, gulong param) {
  g_assert(BT_IS_PATTERN(self));

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
static void bt_pattern_init_voice_event(const BtPattern *self, GValue *event, gulong param) {
  g_assert(BT_IS_PATTERN(self));

  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_machine_get_voice_param_type(self->priv->machine,param));
	g_assert(G_IS_VALUE(event));
}


/*
 * bt_pattern_set_event:
 * @self: the pattern the cell belongs to
 * @event: the event cell
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the given pattern cell.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_pattern_set_event(const BtPattern *self, GValue *event, const gchar *value) {
  g_assert(BT_IS_PATTERN(self));
  g_assert(G_IS_VALUE(event));
  g_assert(value);

  // depending on the type, set the GValue
  switch(G_VALUE_TYPE(event)) {
    case G_TYPE_DOUBLE: {
      //gdouble val=atof(value); // this is dependend on the locale
			gdouble val=g_ascii_strtod(value,NULL);
      g_value_set_double(event,val);
      GST_DEBUG("store double event %s",value);
    } break;
    case G_TYPE_BOOLEAN: {
			gint val=atoi(value);
      g_value_set_boolean(event,val);
      GST_DEBUG("store boolean event %s",value);
    } break;
    case G_TYPE_INT: {
			gint val=atoi(value);
      g_value_set_int(event,val);
      GST_DEBUG("store int event %s",value);
    } break;
    case G_TYPE_UINT: {
			gint val=atoi(value);
      g_value_set_uint(event,val);
      GST_DEBUG("store uint event %s",value);
    } break;
    default:
      GST_ERROR("unsupported GType=%d:'%s' for value=\"%s\"",G_VALUE_TYPE(event),G_VALUE_TYPE_NAME(event),value);
      return(FALSE);
  }
  // notify other that the data has been changed
  g_signal_emit(G_OBJECT(self), signals[CHANGED_EVENT], 0);
  return(TRUE);
}

/*
 * bt_pattern_get_event:
 * @self: the pattern the cell belongs to
 * @event: the event cell
 *
 * Returns the string representation of the given cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
static gchar *bt_pattern_get_event(const BtPattern *self, GValue *event) {
	gchar *res=NULL;
  g_assert(BT_IS_PATTERN(self));
  g_assert(G_IS_VALUE(event));
	
  // depending on the type, set the result
  switch(G_VALUE_TYPE(event)) {
    case G_TYPE_DOUBLE:
      res=g_strdup_printf("%lf",g_value_get_double(event));
			break;
    case G_TYPE_BOOLEAN:
			res=g_strdup_printf("%d",g_value_get_boolean(event));
			break;
    case G_TYPE_INT:
			res=g_strdup_printf("%d",g_value_get_int(event));
			break;
    case G_TYPE_UINT:
			res=g_strdup_printf("%u",g_value_get_uint(event));
			break;
    default:
      GST_ERROR("unsupported GType=%d:'%s'",G_VALUE_TYPE(event),G_VALUE_TYPE_NAME(event));
      return(NULL);
  }
	return(res);
}

//-- constructor methods

/**
 * bt_pattern_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the pattern
 * @name: the display name of the pattern
 * @length: the number of ticks
 * @voices: the number of voices
 * @machine: the machine the pattern belongs to
 *
 * Create a new instance. It will be automatically added to the machines pattern
 * list.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPattern *bt_pattern_new(const BtSong *song, const gchar *id, const gchar *name, gulong length, gulong voices,const BtMachine *machine) {
  BtPattern *self;
  
  g_assert(BT_IS_SONG(song));
  g_assert(id);
  g_assert(name);
  g_assert(BT_IS_MACHINE(machine));
	
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
 * bt_pattern_copy:
 * @self: the pattern to create a copy from
 *
 * Create a new instance as a copy of the given instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPattern *bt_pattern_copy(const BtPattern *self) {
	BtPattern *pattern;
	BtSong *song;
	BtMachine *machine;
	gchar *id,*name,*mid;
	glong length, voices;
	
	g_assert(BT_IS_PATTERN(self));
	
	GST_INFO("copying pattern");
	
	g_object_get(G_OBJECT(self),"song",&song,"length",&length,"voices",&voices,"machine",&machine,NULL);
	g_object_get(G_OBJECT(machine),"id",&mid,NULL);
	
	name=bt_machine_get_unique_pattern_name(machine);
	id=g_strdup_printf("%s %s",mid,name);

	if((pattern=bt_pattern_new(song,id,name,length,voices,machine))) {
		gulong data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);

		// copy data
		memcpy(pattern->priv->data,self->priv->data,data_count*sizeof(GValue));
		GST_INFO("  data copied");
	}

	g_free(mid);
	g_free(id);
	g_free(name);
	g_object_unref(machine);

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
gulong bt_pattern_get_global_param_index(const BtPattern *self, const gchar *name, GError **error) {
	gulong ret=0;
	GError *tmp_error=NULL;
	
  g_assert(BT_IS_PATTERN(self));
  g_assert(name);
	g_return_val_if_fail(error == NULL || *error == NULL, 0);
	
	ret=bt_machine_get_global_param_index(self->priv->machine,name,&tmp_error);
	
	if (tmp_error!=NULL) {
		//g_set_error (error, error_domain, /* errorcode= */0,
		//					 	"global dparam for name %s not found", name);
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
gulong bt_pattern_get_voice_param_index(const BtPattern *self, const gchar *name, GError **error) {
	gulong ret=0;
	GError *tmp_error=NULL;
	
  g_assert(BT_IS_PATTERN(self));
  g_assert(name);
	g_return_val_if_fail(error == NULL || *error == NULL, 0);
	
	ret=bt_machine_get_voice_param_index(self->priv->machine,name,&tmp_error);
	
	if (tmp_error!=NULL) {
		//g_set_error (error, error_domain, /* errorcode= */0,
		//						"voice dparam for name %s not found", name);
		g_propagate_error(error, tmp_error);
	}
  return(ret);
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
gboolean bt_pattern_set_global_event(const BtPattern *self, gulong tick, gulong param, const gchar *value) {
	GValue *event;

	if((event=bt_pattern_get_global_event_data(self,tick,param))) {
		if(!G_IS_VALUE(event)) {
			bt_pattern_init_global_event(self,event,param);
		}
		bt_pattern_set_event(self,event,value);
		return(TRUE);
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
gboolean bt_pattern_set_voice_event(const BtPattern *self, gulong tick, gulong voice, gulong param, const gchar *value) {
	GValue *event;

	if((event=bt_pattern_get_voice_event_data(self,tick,voice, param))) {
		if(!G_IS_VALUE(event)) {
			bt_pattern_init_voice_event(self,event,param);
		}
		bt_pattern_set_event(self,event,value);
		return(TRUE);
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
gchar *bt_pattern_get_global_event(const BtPattern *self, gulong tick, gulong param) {
	GValue *event;

	if((event=bt_pattern_get_global_event_data(self,tick,param)) && G_IS_VALUE(event)) {
		return(bt_pattern_get_event(self,event));
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
gchar *bt_pattern_get_voice_event(const BtPattern *self, gulong tick, gulong voice, gulong param) {
	GValue *event;

	if((event=bt_pattern_get_voice_event_data(self,tick,voice,param)) && G_IS_VALUE(event)) {
		return(bt_pattern_get_event(self,event));
	}
	return(NULL);
}

/**
 * bt_pattern_tick_has_data:
 * @self: the pattern to check
 * @index: the tick index in the pattern
 *
 * Check if there are any event in the given pattern-row.
 *
 * Returns: %TRUE is there are events, %FALSE otherwise
 */
gboolean bt_pattern_tick_has_data(const BtPattern *self, gulong index) {
  gulong j,k;
  GValue *data;

  g_assert(BT_IS_PATTERN(self));

  data=&self->priv->data[index*(self->priv->global_params+self->priv->voices*self->priv->voice_params)];
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
 * @index: the tick index in the pattern
 *
 * Pushes all control changes from this pattern-row into the #BtMachine params.
 */
void bt_pattern_play_tick(const BtPattern *self, gulong index) {
  gulong j,k;
  GValue *data;

  g_assert(BT_IS_PATTERN(self));

  data=&self->priv->data[index*(self->priv->global_params+self->priv->voices*self->priv->voice_params)];
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

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_pattern_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPattern *self = BT_PATTERN(object);

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
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_pattern_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPattern *self = BT_PATTERN(object);
  gulong length,voices;
  
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
      length=self->priv->length;
      self->priv->length = g_value_get_ulong(value);
      if(length!=self->priv->length) {
				GST_DEBUG("set the length for pattern: %d",self->priv->length);
				bt_pattern_resize_data_length(self,length);
				bt_song_set_unsaved(self->priv->song,TRUE);
			}
    } break;
    case PATTERN_VOICES: {
      voices=self->priv->voices;
      self->priv->voices = g_value_get_ulong(value);
			if(voices!=self->priv->voices) {
				GST_DEBUG("set the voices for pattern: %d",self->priv->voices);
				bt_pattern_resize_data_voices(self,voices);
				bt_song_set_unsaved(self->priv->song,TRUE);
			}
    } break;
    case PATTERN_MACHINE: {
      g_object_try_weak_unref(self->priv->machine);
      self->priv->machine = BT_MACHINE(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->machine);
      g_object_get(G_OBJECT(self->priv->machine),"global-params",&self->priv->global_params,"voice-params",&self->priv->voice_params,NULL);
      GST_DEBUG("set the machine for pattern: %p",self->priv->machine);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_dispose(GObject *object) {
  BtPattern *self = BT_PATTERN(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

	g_object_try_weak_unref(self->priv->song);
  g_object_try_weak_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_pattern_finalize(GObject *object) {
  BtPattern *self = BT_PATTERN(object);

  GST_DEBUG("!!!! self=%p",self);
  
	g_free(self->priv->id);
	g_free(self->priv->name);
  g_free(self->priv->data);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_pattern_init(GTypeInstance *instance, gpointer g_class) {
  BtPattern *self = BT_PATTERN(instance);

  self->priv = g_new0(BtPatternPrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->voices=-1;
}

static void bt_pattern_class_init(BtPatternClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	error_domain=g_quark_from_static_string("BtPattern");
	
  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_pattern_set_property;
  gobject_class->get_property = bt_pattern_get_property;
  gobject_class->dispose      = bt_pattern_dispose;
  gobject_class->finalize     = bt_pattern_finalize;
  
  klass->changed_event = NULL;
  
  /** 
	 * BtPattern::changed:
   * @self: the pattern object that emitted the signal
	 *
	 * signals that the data of this pattern has been changed
	 */
  signals[CHANGED_EVENT] = g_signal_new("changed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_ABS_STRUCT_OFFSET(BtPatternClass,changed_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );

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
                                     "machine contruct prop",
                                     "Machine object, the pattern belongs to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_pattern_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
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
		type = g_type_register_static(G_TYPE_OBJECT,"BtPattern",&info,0);
  }
  return type;
}
