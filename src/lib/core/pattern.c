/* $Id: pattern.c,v 1.29 2005-01-20 16:18:27 ensonic Exp $
 * class for an event pattern of a #BtMachine instance
 */
 
#define BT_CORE
#define BT_PATTERN_C

#include <libbtcore/core.h>

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
	GValue  *data;
};

static GObjectClass *parent_class=NULL;

//-- helper methods

static gboolean bt_pattern_init_data(const BtPattern *self) {
  gboolean ret=FALSE;
  gulong data_count=self->priv->length*(self->priv->global_params+self->priv->voices*self->priv->voice_params);
  //GValue *data;

  if(self->priv->machine==NULL) return(TRUE);
  if(self->priv->length==0) return(TRUE);
		
  if(self->priv->data) {
    GST_ERROR("data has already been initialized");
    return(TRUE);
  }

  GST_DEBUG("sizes : %d*(%d+%d*%d)=%d",self->priv->length,self->priv->global_params,self->priv->voices,self->priv->voice_params,data_count);
  if((self->priv->data=g_new0(GValue,data_count))) {
    // initialize the GValues (can we use memcpy for the tick-lines)
    /*
    data=self->priv->data;
    for(i=0;i<self->priv->length;i++) {
      for(k=0;k<self->priv->global_params;k++) {
        g_value_init(data,bt_machine_get_global_dparam_type(self->priv->machine,k));
        data++;
      }
      for(j=0;j<self->priv->voices;j++) {
        for(k=0;k<self->priv->voice_params;k++) {
          g_value_init(data,bt_machine_get_voice_dparam_type(self->priv->machine,k));
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
  // @todo implement pattern resizing
  // old_data=self->priv->data;
  // allocate new space
  // copy old values over
  // if new space is bigger initialize new lines
  // free old data
  GST_ERROR("not yet implemented");
}

static void bt_pattern_resize_data_voices(const BtPattern *self, gulong voices) {
  // @todo implement pattern resizing
  // old_data=self->priv->data;
  // allocate new space
  // copy old values over
  // if new space is bigger initialize new voices
  // free old data
  GST_ERROR("not yet implemented");
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
BtPattern *bt_pattern_new(const BtSong *song, const gchar *id, const gchar *name, glong length, glong voices,const BtMachine *machine) {
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
  bt_machine_add_pattern(machine,self);
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_pattern_get_global_event_data:
 * @self: the pattern to search for the global dparam
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *bt_pattern_get_global_event_data(const BtPattern *self, gulong tick, gulong param) {
  gulong index;

  g_assert(BT_IS_PATTERN(self));
	g_return_val_if_fail(self->priv->data,NULL);

  if(!(tick<self->priv->length)) { GST_ERROR("tick beyond length");return(NULL); }
  if(!(param<self->priv->global_params)) { GST_ERROR("param beyond global_params");return(NULL); }

  index=(tick*(self->priv->global_params+self->priv->voices*self->priv->voice_params))
       + param;
  return(&self->priv->data[index]);
}

/**
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
GValue *bt_pattern_get_voice_event_data(const BtPattern *self, gulong tick, gulong voice, gulong param) {
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

/**
 * bt_pattern_get_global_dparam_index:
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
gulong bt_pattern_get_global_dparam_index(const BtPattern *self, const gchar *name, GError **error) {
	gulong ret=0;
	GError *tmp_error=NULL;
	
  g_assert(BT_IS_PATTERN(self));
  g_assert(name);
	
	ret=bt_machine_get_global_dparam_index(self->priv->machine,name,&tmp_error);
	
	if (tmp_error!=NULL) {
		// set g_error
		g_set_error (error,
							 	g_quark_from_static_string("BtPattern"), 	/* error domain */
								0,																				/* error code */
								"global dparam for name %s not found",		/* error message format string */
								name);
		g_propagate_error(error, tmp_error);
	}
  return(ret);
}

/**
 * bt_pattern_get_voice_dparam_index:
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
gulong bt_pattern_get_voice_dparam_index(const BtPattern *self, const gchar *name, GError **error) {
	gulong ret=0;
	GError *tmp_error=NULL;
	
  g_assert(BT_IS_PATTERN(self));
  g_assert(name);
	
	ret=bt_machine_get_voice_dparam_index(self->priv->machine,name,&tmp_error);
	
	if (tmp_error!=NULL) {
		// set g_error
		g_set_error (error,
							 	g_quark_from_static_string("BtPattern"), 	/* error domain */
								0,																				/* error code */
								"voice dparam for name %s not found",			/* error message format string */
								name);
		g_propagate_error(error, tmp_error);
	}
  return(ret);
}


/**
 * bt_pattern_init_global_event:
 * @self: the pattern that holds the cell
 * @event: the pattern-cell to initialise
 * @param: the index of the global dparam
 *
 * Initialises a pattern cell with the type of the n-th global param of the
 * machine.
 *
 */
void bt_pattern_init_global_event(const BtPattern *self, GValue *event, gulong param) {
  g_assert(BT_IS_PATTERN(self));

  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_machine_get_global_dparam_type(self->priv->machine,param));
  g_assert(G_IS_VALUE(event));
}

/**
 * bt_pattern_init_voice_event:
 * @self: the pattern that holds the cell
 * @event: the pattern-cell to initialise
 * @param: the index of the voice dparam
 *
 * Initialises a pattern cell with the type of the n-th voice param of the
 * machine.
 *
 */
void bt_pattern_init_voice_event(const BtPattern *self, GValue *event, gulong param) {
  g_assert(BT_IS_PATTERN(self));
  g_assert(G_IS_VALUE(event));

  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_machine_get_voice_dparam_type(self->priv->machine,param));
}


/**
 * bt_pattern_set_event:
 * @self: the pattern the cell belongs to
 * @event: the event cell
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the given pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean bt_pattern_set_event(const BtPattern *self, GValue *event, const gchar *value) {
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
    default:
      GST_ERROR("unsupported GType=%d for value=\"%s\"",G_VALUE_TYPE(event),value);
      return(FALSE);
  }
  return(TRUE);
}

/**
 * bt_pattern_play_tick:
 * @self: the pattern the cell belongs to
 * @index: the tick index in the pattern
 *
 * Sets all dparams from this pattern-row into the #BtMachine.
 *
 */
void bt_pattern_play_tick(const BtPattern *self, gulong index) {
  gulong j,k;
  GValue *data;

  g_assert(BT_IS_PATTERN(self));

  data=&self->priv->data[index*(self->priv->global_params+self->priv->voices*self->priv->voice_params)];
  for(k=0;k<self->priv->global_params;k++) {
    if(G_IS_VALUE(data)) {
      bt_machine_set_global_dparam_value(self->priv->machine,k,data);
    }
    data++;
  }
  for(j=0;j<self->priv->voices;j++) {
    for(k=0;k<self->priv->voice_params;k++) {
      // @set voice events
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
    } break;
    case PATTERN_LENGTH: {
      length=self->priv->length;
      self->priv->length = g_value_get_ulong(value);
      GST_DEBUG("set the length for pattern: %d",self->priv->length);
      if(self->priv->data) bt_pattern_resize_data_length(self,length);
    } break;
    case PATTERN_VOICES: {
      voices=self->priv->voices;
      self->priv->voices = g_value_get_ulong(value);
      GST_DEBUG("set the voices for pattern: %d",self->priv->voices);
      if(self->priv->data) bt_pattern_resize_data_voices(self,voices);
    } break;
    case PATTERN_MACHINE: {
      g_object_try_weak_unref(self->priv->machine);
      self->priv->machine = BT_MACHINE(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->machine);
      g_object_get(G_OBJECT(self->priv->machine),"global_params",&self->priv->global_params,"voice_params",&self->priv->voice_params,NULL);
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

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_pattern_set_property;
  gobject_class->get_property = bt_pattern_get_property;
  gobject_class->dispose      = bt_pattern_dispose;
  gobject_class->finalize     = bt_pattern_finalize;

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
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

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
