/* $Id: pattern.c,v 1.11 2004-09-21 14:01:19 ensonic Exp $
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
  glong length;
  /* the number of voices */
  glong voices;
  /* the number of dynamic params the machine provides per instance */
  glong global_params;
  /* the number of dynamic params the machine provides per instance and voice */
  glong voice_params;
  /* the machine the pattern belongs to */
  BtMachine *machine;

  /* an array of GValue
   * with the size of length*(global_params+voices*voice_params)
   */
	GValue  *data;
};

//-- helper methods

static void bt_pattern_init_data(const BtPattern *self) {
  glong data_count=self->private->length*(self->private->global_params+self->private->voices*self->private->voice_params);
  glong i,j,k;
  GValue *data;

  if(self->private->machine==NULL) return;
  if(self->private->length==0) return;
  if(self->private->voices==-1) return;

  if(self->private->data) {
    GST_ERROR("that should not happen");
    return;
  }

  GST_DEBUG("bt_pattern_init_data : %d*(%d+%d*%d)=%d",self->private->length,self->private->global_params,self->private->voices,self->private->voice_params,data_count);
  self->private->data=g_new0(GValue,data_count);
  // initialize the GValues (can we use memcpy for the tick-lines)
  /*
  data=self->private->data;
  for(i=0;i<self->private->length;i++) {
    for(k=0;k<self->private->global_params;k++) {
      g_value_init(data,bt_machine_get_global_dparam_type(self->private->machine,k));
      data++;
    }
    for(j=0;j<self->private->voices;j++) {
      for(k=0;k<self->private->voice_params;k++) {
        g_value_init(data,bt_machine_get_voice_dparam_type(self->private->machine,k));
        data++;
      }
    }
  }
  */
}

static void bt_pattern_resize_data_length(const BtPattern *self, glong length) {
  // @todo implement pattern resizing
  // old_data=self->private->data;
  // allocate new space
  // copy old values over
  // if new space is bigger initialize new lines
  // free old data
  GST_ERROR("not yet implemented");
}

static void bt_pattern_resize_data_voices(const BtPattern *self, glong voices) {
  // @todo implement pattern resizing
  // old_data=self->private->data;
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
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtPattern *bt_pattern_new(const BtSong *song, const gchar *id, const gchar *name, glong length, glong voices,const BtMachine *machine) {
  BtPattern *self;
  self=BT_PATTERN(g_object_new(BT_TYPE_PATTERN,"song",song,"id",id,"name",name,"length",length,"voices",voices,"machine",machine,NULL));
  
  bt_pattern_init_data(self);
  return(self);
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
 * Returns: the GValue or NULL if out of the pattern range
 */
GValue *bt_pattern_get_global_event_data(const BtPattern *self, glong tick, glong param) {
  glong index;

  if((param==-1)) return(NULL);
  if(!(tick<self->private->length)) { GST_ERROR("tick beyond length");return(NULL); }
  if(!(param<self->private->global_params)) { GST_ERROR("param beyond global_params");return(NULL); }

  index=(tick*(self->private->global_params+self->private->voices*self->private->voice_params))
       + param;
  // @todo add assertion that there is a valid GValue at this index?
  return(&self->private->data[index]);
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
 * Returns: the GValue or NULL if out of the pattern range
 */
GValue *bt_pattern_get_voice_event_data(const BtPattern *self, glong tick, glong voice, glong param) {
  glong index;

  if((param==-1)) return(NULL);
  if(!(tick<self->private->length)) { GST_ERROR("tick beyond length");return(NULL); }
  if(!(voice<self->private->voices)) { GST_ERROR("voice beyond voices");return(NULL); }
  if(!(param<self->private->voice_params)) { GST_ERROR("param beyond voice_ params");return(NULL); }

  index=(tick*(self->private->global_params+self->private->voices*self->private->voice_params))
       +       self->private->global_params+(voice*self->private->voice_params)
       +param;
  // @todo add assertion that there is a valid GValue at this index?
  return(&self->private->data[index]);
}

/**
 * bt_pattern_get_global_dparam_index:
 * @self: the pattern to search for the global dparam
 * @name: the name of the global dparam
 *
 * Searches the list of registered dparam of the machine the pattern belongs to
 * for a global dparam of the given name and returns the index if found.
 *
 * Returns: the index or -1 when it has not be found
 */
glong bt_pattern_get_global_dparam_index(const BtPattern *self, const gchar *name) {
  return(bt_machine_get_global_dparam_index(self->private->machine,name));
}

/**
 * bt_pattern_get_voice_dparam_index:
 * @self: the pattern to search for the voice dparam
 * @name: the name of the voice dparam
 *
 * Searches the list of registered dparam of the machine the pattern belongs to
 * for a voice dparam of the given name and returns the index if found.
 *
 * Returns: the index or -1 when it has not be found
 */
glong bt_pattern_get_voice_dparam_index(const BtPattern *self, const gchar *name) {
  return(bt_machine_get_voice_dparam_index(self->private->machine,name));
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
void bt_pattern_init_global_event(const BtPattern *self, GValue *event, glong param) {
  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_machine_get_global_dparam_type(self->private->machine,param));
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
void bt_pattern_init_voice_event(const BtPattern *self, GValue *event, glong param) {
  //GST_DEBUG("at %d",param);
  g_value_init(event,bt_machine_get_voice_dparam_type(self->private->machine,param));
}


/**
 * bt_pattern_set_event:
 * @self: the pattern the cell belongs to
 * @event: the event cell
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the given pattern cell.
 *
 * Returns: TRUE for success
 */
gboolean bt_pattern_set_event(const BtPattern *self, GValue *event, const gchar *value) {
  g_assert(event);
  g_assert(value);

  // depending on the type, set the GValue
  switch(G_VALUE_TYPE(event)) {
    case G_TYPE_DOUBLE: {
      gdouble val=atof(value);
      g_value_set_double(event,val);
      GST_INFO("store double event %s",value);
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
void bt_pattern_play_tick(const BtPattern *self, glong index) {
  glong j,k;
  GValue *data;

  data=&self->private->data[index*(self->private->global_params+self->private->voices*self->private->voice_params)];
  for(k=0;k<self->private->global_params;k++) {
    if(G_IS_VALUE(data)) {
      bt_machine_set_global_dparam_value(self->private->machine,k,data);
    }
    data++;
  }
  for(j=0;j<self->private->voices;j++) {
    for(k=0;k<self->private->voice_params;k++) {
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
      g_value_set_object(value, self->private->song);
    } break;
    case PATTERN_ID: {
      g_value_set_string(value, self->private->id);
    } break;
    case PATTERN_NAME: {
      g_value_set_string(value, self->private->name);
    } break;
    case PATTERN_LENGTH: {
      g_value_set_long(value, self->private->length);
    } break;
    case PATTERN_VOICES: {
      g_value_set_long(value, self->private->voices);
    } break;
    case PATTERN_MACHINE: {
      g_value_set_object(value, self->private->machine);
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
  glong length,voices;
  
  return_if_disposed();
  switch (property_id) {
    case PATTERN_SONG: {
      g_object_try_weak_unref(self->private->song);
      self->private->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->private->song);
      //GST_DEBUG("set the song for pattern: %p",self->private->song);
    } break;
    case PATTERN_ID: {
      g_free(self->private->id);
      self->private->id = g_value_dup_string(value);
      GST_DEBUG("set the id for pattern: %s",self->private->id);
    } break;
    case PATTERN_NAME: {
      g_free(self->private->name);
      self->private->name = g_value_dup_string(value);
      GST_DEBUG("set the display name for the pattern: %s",self->private->name);
    } break;
    case PATTERN_LENGTH: {
      length=self->private->length;
      self->private->length = g_value_get_long(value);
      GST_DEBUG("set the length for pattern: %d",self->private->length);
      if(self->private->data) bt_pattern_resize_data_length(self,length);
    } break;
    case PATTERN_VOICES: {
      voices=self->private->voices;
      self->private->voices = g_value_get_long(value);
      GST_DEBUG("set the voices for pattern: %d",self->private->voices);
      if(self->private->data) bt_pattern_resize_data_voices(self,voices);
    } break;
    case PATTERN_MACHINE: {
      glong global_params,voice_params;
      g_object_try_unref(self->private->machine);
      self->private->machine = g_object_try_ref(G_OBJECT(g_value_get_object(value)));
      self->private->global_params=bt_g_object_get_long_property(G_OBJECT(self->private->machine),"global_params");
      self->private->voice_params=bt_g_object_get_long_property(G_OBJECT(self->private->machine),"voice_params");
      GST_DEBUG("set the machine for pattern: %p",self->private->machine);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_dispose(GObject *object) {
  BtPattern *self = BT_PATTERN(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

	g_object_try_weak_unref(self->private->song);
  g_object_try_unref(self->private->machine);
}

static void bt_pattern_finalize(GObject *object) {
  BtPattern *self = BT_PATTERN(object);

  GST_DEBUG("!!!! self=%p",self);
  
	g_free(self->private->id);
	g_free(self->private->name);
  g_free(self->private->data);
  g_free(self->private);
}

static void bt_pattern_init(GTypeInstance *instance, gpointer g_class) {
  BtPattern *self = BT_PATTERN(instance);

  self->private = g_new0(BtPatternPrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->voices=-1;
}

static void bt_pattern_class_init(BtPatternClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
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
																	g_param_spec_long("length",
                                     "length prop",
                                     "length of the pattern in ticks",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,PATTERN_VOICES,
																	g_param_spec_long("voices",
                                     "voices prop",
                                     "number of voices in the pattern",
                                     0,
                                     G_MAXLONG,
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
      sizeof (BtPatternClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_pattern_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtPattern),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_pattern_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtPattern",&info,0);
  }
  return type;
}

