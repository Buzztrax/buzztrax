/* $Id: machine.c,v 1.72 2005-01-21 18:25:40 ensonic Exp $
 * base class for a machine
 * @todo try to derive this from GstThread!
 *  then put the machines into itself (and not into the songs bin, but insert the machine directly into the song->bin
 *  when adding internal machines we need to fix the ghost pads (this may require relinking)
 */
 
#define BT_CORE
#define BT_MACHINE_C

#include <libbtcore/core.h>

enum {
  MACHINE_PROPERTIES=1,
  MACHINE_SONG,
	MACHINE_ID,
	MACHINE_PLUGIN_NAME,
  MACHINE_VOICES,
  MACHINE_GLOBAL_PARAMS,
  MACHINE_VOICE_PARAMS,
  MACHINE_MACHINE,
  MACHINE_INPUT_LEVEL,
	MACHINE_PATTERNS,
	MACHINE_STATE
};

struct _BtMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
  /* properties accociated with this machine */
  GHashTable *properties;
  
	/* the song the machine belongs to */
	BtSong *song;
	/* the main gstreamer container element */
	GstBin *bin;

	/* the id, we can use to lookup the machine */
	gchar *id;
	/* the name of the gst-plugin the machine is using */
	gchar *plugin_name;

  /* the number of voices the machine provides */
  gulong voices;
  /* the number of dynamic params the machine provides per instance */
  gulong global_params;
  /* the number of dynamic params the machine provides per instance and voice */
  gulong voice_params;
	
	/* the current state of the machine */
	BtMachineState state;

  /* gstreamer dparams */
  GstDParamManager *dparam_manager;
  GType *global_types;
  GType *voice_types; 
	GstDParam **global_dparams;
	GstDParam **voice_dparams;  // @todo we need these for every voice

  GList *patterns;	// each entry points to BtPattern
  
  /* the gstreamer element that is used */
  GstElement *machine;
	/* the gstreamer element pads */
	GstPad *dst,*src;
  
  /* utillity elements to allow multiple inputs/outputs */
  GstElement *adder,*spreader;
  
  /* the element to analyse the current input
   * could be placed right behind the internal adder
   * (see wire.c)
   */
  GstElement *input_level;
  /* the element to analyse the current output
   * could be placed right before the internal spreader
   * (see wire.c)
   */
  GstElement *output_level;
  
  /* public fields are
	GstElement *dst_elem,*src_elem;
  */
};

static GObjectClass *parent_class=NULL;

//-- enums

GType bt_machine_state_get_type(void) {
  static GType type = 0;
  if(type==0) {
    static GEnumValue values[] = {
      { BT_MACHINE_STATE_NORMAL,"BT_MACHINE_STATE_NORMAL","just working" },
      { BT_MACHINE_STATE_MUTE,	"BT_MACHINE_STATE_MUTE", 	"be quiet" },
      { BT_MACHINE_STATE_SOLO,  "BT_MACHINE_STATE_SOLO",  "be the only one playing" },
      { BT_MACHINE_STATE_BYPASS,"BT_MACHINE_STATE_BYPASS","be uneffective (pass through)" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtMachineStateType", values);
  }
  return type;
}

//-- helper methods

static gboolean bt_machine_change_state(BtMachine *self, BtMachineState new_state) {
	// reject a few nonsense changes
	if((new_state==BT_MACHINE_STATE_BYPASS) && (!BT_IS_PROCESSOR_MACHINE(self))) return(FALSE);
	if((new_state==BT_MACHINE_STATE_SOLO) && (BT_IS_SINK_MACHINE(self))) return(FALSE);
	
	// return to normal state
	switch(self->priv->state) {
		case BT_MACHINE_STATE_MUTE:
			gst_pad_set_active(self->priv->src,TRUE);
			/*
			gst_pad_link(self->priv->src,self->priv->src_peer);
			*/
			/*
			if(gst_element_set_state(self->priv->machine,GST_STATE_PLAYING)==GST_STATE_FAILURE) {
				GST_WARNING("setting element '%s' to playing state failed",self->priv->id);
			}
			*/
			break;
		case BT_MACHINE_STATE_SOLO:
			// @todo set all but this machine to playing again
			break;
		case BT_MACHINE_STATE_BYPASS:
			// @todo disconnect its source and sink + set this machine to playing
			break;
	}
	// set to new state
	switch(new_state) {
		case BT_MACHINE_STATE_MUTE:
			gst_pad_set_active(self->priv->src,FALSE);
			/*
			self->priv->src=gst_element_get_pad(self->priv->machine,"src");
	  	self->priv->src_peer=gst_pad_get_peer(self->priv->src);
	  	gst_pad_unlink(self->priv->src,self->priv->src_peer);
			*/
			/*
			if(gst_element_set_state(self->priv->machine,GST_STATE_PAUSED)==GST_STATE_FAILURE) {
				GST_WARNING("setting element '%s' to paused state failed",self->priv->id);
			}
			*/
			break;
		case BT_MACHINE_STATE_SOLO:
			// @todo set all but this machine to paused
			break;
		case BT_MACHINE_STATE_BYPASS:
			// @todo set this machine to paused + connect its source and sink
			break;
	}
	self->priv->state=new_state;
	return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_new:
 * @self: instance to finish construction of
 *
 * This is the common part of machine construction. It needs to be called from
 * within the sub-classes constructor methods.
 *
 * Returns: %TRUE for succes, %FALSE otherwise
 */
gboolean bt_machine_new(BtMachine *self) {

  g_assert(BT_IS_MACHINE(self));
  g_assert(self->priv->machine==NULL);
  g_assert(self->priv->id);
  g_assert(self->priv->plugin_name);
  GST_INFO("initializing machine");

  self->priv->machine=gst_element_factory_make(self->priv->plugin_name,self->priv->id);
  if(!self->priv->machine) {
    GST_ERROR("  failed to instantiate machine \"%s\"",self->priv->plugin_name);
    return(FALSE);
  }
  // we need to make sure the machine is out of the right class
  {
    GstElementFactory *element_factory=gst_element_get_factory(self->priv->machine);
    const gchar *element_class=gst_element_factory_get_klass(element_factory);
    GST_INFO("checking machine class \"%s\"",element_class);
    if(BT_IS_SINK_MACHINE(self)) {
      if(g_ascii_strncasecmp(element_class,"Sink/",5)) {
        GST_ERROR("  plugin \"%s\" is in \"%s\" class instead of \"Sink/...\"",self->priv->plugin_name,element_class);
        return(FALSE);
      }
    }
    else if(BT_IS_SOURCE_MACHINE(self)) {
      if(g_ascii_strncasecmp(element_class,"Source/",7)) {
        GST_ERROR("  plugin \"%s\" is in \"%s\" class instead of \"Source/...\"",self->priv->plugin_name,element_class);
        return(FALSE);
      }
    }
    else if(BT_IS_PROCESSOR_MACHINE(self)) {
      if(g_ascii_strncasecmp(element_class,"Filter/",7)) {
        GST_ERROR("  plugin \"%s\" is in \"%s\" class instead of \"Filter/...\"",self->priv->plugin_name,element_class);
        return(FALSE);
      }
    }
  }

  // there is no adder or spreader in use by default
  self->dst_elem=self->src_elem=self->priv->machine;
	self->priv->src=gst_element_get_pad(self->priv->machine,"src");
	self->priv->dst=gst_element_get_pad(self->priv->machine,"sink");
  GST_INFO("  instantiated machine \"%s\", obj->ref_count=%d",self->priv->plugin_name,G_OBJECT(self->priv->machine)->ref_count);
  if((self->priv->dparam_manager=gst_dpman_get_manager(self->priv->machine))) {
    GParamSpec **specs;
    GstDParam **dparam;
    guint i;
		gboolean setModeOK=FALSE;

    // setting param mode. Only synchronized is currently supported
    if(gst_dpman_set_mode(self->priv->dparam_manager, "synchronous")) {
    	GST_DEBUG("  machine \"%s\" supports synchronous dparams",self->priv->plugin_name);
    
	    // manage dparams
  	  specs=gst_dpman_list_dparam_specs(self->priv->dparam_manager);
    	// count the specs
    	for(i=0;specs[i];i++);
    	// right now, we have no voice params
    	self->priv->global_params=i;
    	self->priv->global_dparams=(GstDParam **)g_new0(gpointer,self->priv->global_params);
    	self->priv->global_types  =(GType *     )g_new0(GType   ,self->priv->global_params);
    	// iterate over all dparam
    	for(i=0,dparam=self->priv->global_dparams;specs[i];i++,dparam++) {
				gboolean attach_ret=FALSE;
			
      	self->priv->global_types[i]=G_PARAM_SPEC_VALUE_TYPE(specs[i]);
      	self->priv->global_dparams[i]=gst_dparam_new(self->priv->global_types[i]);
      	attach_ret=gst_dpman_attach_dparam(self->priv->dparam_manager,g_param_spec_get_name(specs[i]),self->priv->global_dparams[i]);
      	GST_DEBUG("    added global_param \"%s\"",g_param_spec_get_name(specs[i]));
				g_return_val_if_fail(attach_ret == TRUE,FALSE);
    	}
		}
		else {
			GST_DEBUG("  machine \"%s\" does not support syncronous mode",self->priv->plugin_name);
		}
  }
  g_object_get(G_OBJECT(self->priv->song),"bin",&self->priv->bin,NULL);
  gst_bin_add(self->priv->bin,self->priv->machine);
  GST_INFO("  added machine to bin, obj->ref_count=%d",G_OBJECT(self->priv->machine)->ref_count);
  g_assert(self->priv->machine!=NULL);
  g_assert(self->src_elem!=NULL);
  g_assert(self->dst_elem!=NULL);

  if(BT_IS_SINK_MACHINE(self)) {
    GST_DEBUG("  this will be the master for the song");
    g_object_set(G_OBJECT(self->priv->song),"master",G_OBJECT(self),NULL);
  }
  return(TRUE);
}

//-- methods

/**
 * bt_machine_add_input_level:
 * @self: the machine to add the input-level analyser to
 *
 * Add an input-level analyser to the machine and activate it.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 */
gboolean bt_machine_add_input_level(BtMachine *self) {
  gboolean res=FALSE;
  
  // add input-level meter
  if(!(self->priv->input_level=gst_element_factory_make("level",g_strdup_printf("input_level_%p",self)))) {
    GST_ERROR("failed to create machines input level analyser");goto Error;
  }
  g_object_set(G_OBJECT(self->priv->input_level),"interval",0.1, "signal",TRUE, NULL);
  gst_bin_add(self->priv->bin,self->priv->input_level);
  if(!gst_element_link(self->priv->input_level,self->priv->machine)) {
		GST_ERROR("failed to link the machines input level analyser");goto Error;
	}
  self->dst_elem=self->priv->input_level;
  GST_INFO("sucessfully added input level analyser %p",self->priv->input_level);
  
  res=TRUE;
Error:
  return(res);
}

/**
 * bt_machine_activate_adder:
 * @self: the machine to activate the adder in
 *
 * Machines use an adder to allow multiple incoming wires.
 * This method is used by the #BtWire class to activate the adder when needed.
 *
 * Returns: %TRUE for success
 */
gboolean bt_machine_activate_adder(BtMachine *self) {
  gboolean res=TRUE;
  
  if(!self->priv->adder) {
    GstElement *convert;
    
    self->priv->adder=gst_element_factory_make("adder",g_strdup_printf("adder_%p",self));
    g_assert(self->priv->adder!=NULL);
    gst_bin_add(self->priv->bin, self->priv->adder);
    // @todo this is a workaround for gst not making full caps-nego
    convert=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",self));
    g_assert(convert!=NULL);
    gst_bin_add(self->priv->bin, convert);
    GST_DEBUG("  about to link adder -> convert -> dst_elem");
    if(!gst_element_link_many(self->priv->adder, convert, self->dst_elem, NULL)) {
      GST_ERROR("failed to link the machines internal adder");res=FALSE;
    }
    else {
      GST_DEBUG("  adder activated for \"%s\"",gst_element_get_name(self->priv->machine));
      self->dst_elem=self->priv->adder;
    }
  }
  return(res);
}

/**
 * bt_machine_has_active_adder:
 * @self: the machine to check
 *
 * Checks if the machine currently uses an adder.
 * This method is used by the #BtWire class to activate the adder when needed.
 *
 * Returns: %TRUE for success
 */
gboolean bt_machine_has_active_adder(BtMachine *self) {
  return(self->dst_elem==self->priv->adder);
}

/**
 * bt_machine_activate_spreader:
 * @self: the machine to activate the spreader in
 *
 * Machines use a spreader to allow multiple outgoing wires.
 * This method is used by the #BtWire class to activate the spreader when needed.
 *
 * Returns: %TRUE for success
 */
gboolean bt_machine_activate_spreader(BtMachine *self) {
  gboolean res=TRUE;
  
  if(!self->priv->spreader) {
    self->priv->spreader=gst_element_factory_make("tee",g_strdup_printf("tee%p",self));
    g_assert(self->priv->spreader!=NULL);
    gst_bin_add(self->priv->bin, self->priv->spreader);
    if(!gst_element_link(self->src_elem, self->priv->spreader)) {
      GST_ERROR("failed to link the machines internal spreader");res=FALSE;
    }
    else {
      GST_DEBUG("  spreader activated for \"%s\"",gst_element_get_name(self->priv->machine));
      self->src_elem=self->priv->spreader;
    }
  }
  return(res);
}

/**
 * bt_machine_has_active_spreader:
 * @self: the machine to check
 *
 * Checks if the machine currently uses an spreader.
 * This method is used by the #BtWire class to activate the spreader when needed.
 *
 * Returns: %TRUE for success
 */
gboolean bt_machine_has_active_spreader(BtMachine *self) {
  return(self->src_elem==self->priv->spreader);
}

/**
 * bt_machine_add_pattern:
 * @self: the machine to add the pattern to
 * @pattern: the new pattern instance
 *
 * Add the supplied pattern to the machine. This is automatically done by 
 * #bt_pattern_new().
 */
void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(BT_IS_PATTERN(pattern));

  if(!g_list_find(self->priv->patterns,pattern)) {
    self->priv->patterns=g_list_append(self->priv->patterns,g_object_ref(G_OBJECT(pattern)));
  }
  else {
    GST_WARNING("trying to add pattern again"); 
  }
}

/**
 * bt_machine_get_pattern_by_id:
 * @self: the machine to search for the pattern
 * @id: the identifier of the pattern
 *
 * Search the machine for a pattern by the supplied id.
 * The pattern must have been added previously to this setup with #bt_machine_add_pattern().
 * Unref the pattern, when done with it.
 *
 * Returns: #BtPattern instance or %NULL if not found
 */
BtPattern *bt_machine_get_pattern_by_id(const BtMachine *self,const gchar *id) {
  gboolean found=FALSE;
	BtPattern *pattern;
  gchar *pattern_id;
	GList* node;
	
  g_assert(BT_IS_MACHINE(self));
  g_assert(id);
  
	for(node=self->priv->patterns;node;node=g_list_next(node)) {
		pattern=BT_PATTERN(node->data);
    g_object_get(G_OBJECT(pattern),"id",&pattern_id,NULL);
		if(!strcmp(pattern_id,id)) found=TRUE;
    g_free(pattern_id);
    if(found) return(g_object_ref(pattern));
	}
  GST_DEBUG("no pattern found for id \"%s\"",id);
  return(NULL);
}

/**
 * bt_machine_get_pattern_by_index:
 * @self: the machine to search for the pattern
 * @index: the index of the pattern in the machines pattern list
 *
 * Fetches the machine from the given position of the machines pattern list.
 * The pattern must have been added previously to this setup with #bt_machine_add_pattern().
 * Unref the pattern, when done with it.
 *
 * Returns: #BtPattern instance or %NULL if not found
 */
BtPattern *bt_machine_get_pattern_by_index(const BtMachine *self,gulong index) {
	g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
	g_return_val_if_fail(index<g_list_length(self->priv->patterns),NULL);

	return(g_object_ref(BT_PATTERN(g_list_nth_data(self->priv->patterns,(guint)index))));
}

/**
 * bt_machine_get_global_dparam_index:
 * @self: the machine to search for the global dparam
 * @name: the name of the global dparam
 * @error: the location of an error instance to fill with a message, if an error occures
 *
 * Searches the list of registered dparam of a machine for a global dparam of
 * the given name and returns the index if found.
 *
 * Returns: the index or sets error if it is not found and returns -1.
 */
glong bt_machine_get_global_dparam_index(const BtMachine *self, const gchar *name, GError **error) {
  GstDParam *dparam=NULL;
	glong ret=-1;
	gboolean found=FALSE;

  g_assert(BT_IS_MACHINE(self));
  g_assert(name);
	
	if(self->priv->dparam_manager) {
	  if((dparam=gst_dpman_get_dparam(self->priv->dparam_manager,name))) {
			gulong i;
	  	for(i=0;i<self->priv->global_params;i++) {
  	  	if(self->priv->global_dparams[i]==dparam) {
					ret=i;
					found=TRUE;
					break;
				}
  		}
		}
		if(!found && error) {
			// set error reason
			g_set_error (error,
								 	g_quark_from_static_string("BtMachine"), 	/* error domain */
									0,																				/* error code */
									"global dparam for name %s not found",		/* error message format string */
									name);
		}
	}
	else {
		if(error) {
			// set error reason
			g_set_error (error,
								 	g_quark_from_static_string("BtMachine"), 	/* error domain */
									0,																				/* error code */
									"machine does not support dparams");			/* error message format string */
		}
	}
  return(ret);
}

/**
 * bt_machine_get_voice_dparam_index:
 * @self: the machine to search for the voice dparam
 * @name: the name of the voice dparam
 * @error: the location of an error instance to fill with a message, if an error occures
 *
 * Searches the list of registered dparam of a machine for a voice dparam of
 * the given name and returns the index if found.
 *
 * Returns: the index or sets error if it is not found and returns -1.
 */
glong bt_machine_get_voice_dparam_index(const BtMachine *self, const gchar *name, GError **error) {
  GstDParam *dparam=NULL;
	gulong ret=-1;
	gboolean found=FALSE;

  g_assert(BT_IS_MACHINE(self));
  g_assert(name);
  
	if(self->priv->dparam_manager) {
	  if((dparam=gst_dpman_get_dparam(self->priv->dparam_manager,name))) {
			gulong i;
	  	// @todo we need to support multiple voices
  		for(i=0;i<self->priv->voice_params;i++) {
    		if(self->priv->voice_dparams[i]==dparam) {
					ret=i;
					found=TRUE;
					break;
				}
  		}
		}
		if(!found && error) {
			// set g_error
			g_set_error (error,
								 	g_quark_from_static_string("BtMachine"), 	/* error domain */
									0,																				/* error code */
									"voice dparam for name %s not found",		/* error message format string */
									name);
		}
	}
	else {
		if(error) {
			// set error reason
			g_set_error (error,
								 	g_quark_from_static_string("BtMachine"), 	/* error domain */
									0,																				/* error code */
									"machine does not support dparams");			/* error message format string */
		}
	}		
  return(ret);
}


/**
 * bt_machine_get_global_dparam:
 * @self: the machine to search for the global dparam
 * @index: the offset in the list of global dparams
 *
 * Retrieves the a global GstDParam
 *
 * Returns: the requested GstDParam
 */
GstDParam *bt_machine_get_global_dparam(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->global_params);
  g_assert(self->priv->global_dparams);
	
  return(self->priv->global_dparams[index]);
}

/**
 * bt_machine_get_global_dparam_type:
 * @self: the machine to search for the global dparam type
 * @index: the offset in the list of global dparams
 *
 * Retrieves the GType of a global dparam 
 *
 * Returns: the requested GType
 */
GType bt_machine_get_global_dparam_type(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->global_params);
	g_assert(self->priv->global_types);
  
  return(self->priv->global_types[index]);
}

/**
 * bt_machine_get_voice_dparam_type:
 * @self: the machine to search for the voice dparam type
 * @index: the offset in the list of voice dparams
 *
 * Retrieves the GType of a voice dparam 
 *
 * Returns: the requested GType
 */
GType bt_machine_get_voice_dparam_type(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->voice_params);

  return(self->priv->voice_types[index]);
}

/**
 * bt_machine_set_global_dparam_value:
 * @self: the machine to set the global dparam value
 * @index: the offset in the list of global dparams
 * @event: the new value
 *
 * Sets a the specified global dparam to the give data value.
 */
void bt_machine_set_global_dparam_value(const BtMachine *self, gulong index, GValue *event) {
  GstDParam *dparam;
  
  g_assert(BT_IS_MACHINE(self));
  g_assert(G_IS_VALUE(event));
  g_assert(index<self->priv->global_params);
  
  dparam=self->priv->global_dparams[index];
  // depending on the type, set the GValue
  switch(G_VALUE_TYPE(event)) {
    case G_TYPE_DOUBLE: {
      //gchar *str=g_strdup_value_contents(event);
      //GST_INFO("events for %s at track %d : \"%s\"",self->priv->id,index,str);
      //g_free(str);
      //g_object_set_property(G_OBJECT(dparam),"value_double",event);
			g_object_set(G_OBJECT(dparam),"value_double",g_value_get_double(event),NULL);
    }  break;
    default:
      GST_ERROR("unsupported GType=%d",G_VALUE_TYPE(event));
  }
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachine *self = BT_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PROPERTIES: {
      g_value_set_pointer(value, self->priv->properties);
    } break;
    case MACHINE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case MACHINE_ID: {
      g_value_set_string(value, self->priv->id);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_value_set_string(value, self->priv->plugin_name);
    } break;
    case MACHINE_VOICES: {
      g_value_set_ulong(value, self->priv->voices);
    } break;
    case MACHINE_GLOBAL_PARAMS: {
      g_value_set_ulong(value, self->priv->global_params);
    } break;
    case MACHINE_VOICE_PARAMS: {
      g_value_set_ulong(value, self->priv->voice_params);
    } break;
    case MACHINE_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    case MACHINE_INPUT_LEVEL: {
      g_value_set_object(value, self->priv->input_level);
    } break;
		case MACHINE_PATTERNS: {
			g_value_set_pointer(value,g_list_copy(self->priv->patterns));
		} break;
    case MACHINE_STATE: {
      g_value_set_enum(value, self->priv->state);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachine *self = BT_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      GST_DEBUG("set the song for machine: %p",self->priv->song);
    } break;
    case MACHINE_ID: {
      g_free(self->priv->id);
      self->priv->id = g_value_dup_string(value);
      GST_DEBUG("set the id for machine: %s",self->priv->id);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_free(self->priv->plugin_name);
      self->priv->plugin_name = g_value_dup_string(value);
      GST_DEBUG("set the plugin_name for machine: %s",self->priv->plugin_name);
    } break;
    case MACHINE_VOICES: {
      self->priv->voices = g_value_get_ulong(value);
      // @todo reallocate self->priv->patterns->priv->data
    } break;
    case MACHINE_GLOBAL_PARAMS: {
      self->priv->global_params = g_value_get_ulong(value);
    } break;
    case MACHINE_VOICE_PARAMS: {
      self->priv->voice_params = g_value_get_ulong(value);
    } break;
    case MACHINE_STATE: {
			bt_machine_change_state(self,g_value_get_enum(value));
      GST_DEBUG("set the state for machine: %d",self->priv->state);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_dispose(GObject *object) {
  BtMachine *self = BT_MACHINE(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // remove the GstElements from the bin
  // gstreamer uses floating references, therefore elements are destroyed, when removed from the bin
  if(self->priv->bin) {
    if(self->priv->machine) {
      g_assert(GST_IS_BIN(self->priv->bin));
      g_assert(GST_IS_ELEMENT(self->priv->machine));
      GST_DEBUG("  removing machine \"%s\" from bin, obj->ref_count=%d",gst_element_get_name(self->priv->machine),(G_OBJECT(self->priv->machine))->ref_count);
      gst_bin_remove(self->priv->bin,self->priv->machine);
      GST_DEBUG("  bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    }
    if(self->priv->adder) {
      g_assert(GST_IS_BIN(self->priv->bin));
      g_assert(GST_IS_ELEMENT(self->priv->adder));
      GST_DEBUG("  removing adder from bin, obj->ref_count=%d",(G_OBJECT(self->priv->adder))->ref_count);
      gst_bin_remove(self->priv->bin,self->priv->adder);
      GST_DEBUG("  bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    }
    if(self->priv->spreader) {
      g_assert(GST_IS_BIN(self->priv->bin));
      g_assert(GST_IS_ELEMENT(self->priv->spreader));
      GST_DEBUG("  removing spreader from bin, obj->ref_count=%d",(G_OBJECT(self->priv->spreader))->ref_count);
      gst_bin_remove(self->priv->bin,self->priv->spreader);
      GST_DEBUG("  bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    }
    if(self->priv->input_level) {
      g_assert(GST_IS_BIN(self->priv->bin));
      g_assert(GST_IS_ELEMENT(self->priv->input_level));
      GST_DEBUG("  removing input_level %p from bin, obj->ref_count=%d",self->priv->input_level,(G_OBJECT(self->priv->input_level))->ref_count);
      gst_bin_remove(self->priv->bin,self->priv->input_level);
      GST_DEBUG("  bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    }
    if(self->priv->output_level) {
      g_assert(GST_IS_BIN(self->priv->bin));
      g_assert(GST_IS_ELEMENT(self->priv->output_level));
      GST_DEBUG("  removing output_level from bin, obj->ref_count=%d",(G_OBJECT(self->priv->output_level))->ref_count);
      gst_bin_remove(self->priv->bin,self->priv->output_level);
      GST_DEBUG("  bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    }
    // release the bin (that is reffed in bt_machine_new() )
    GST_DEBUG("  releasing the bin, obj->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    g_object_unref(self->priv->bin);
  }

	//GST_DEBUG("  releasing song: %p",self->priv->song);
  g_object_try_weak_unref(self->priv->song);
	//GST_DEBUG("  releasing dparam manager: %p",self->priv->dparam_manager);
	// seems that gst_dpman_get_dparam() does not ref it, therefore we shouldn't unref it
  //g_object_try_unref(self->priv->dparam_manager);
	
	GST_DEBUG("  releasing patterns");

  // unref list of patterns
	if(self->priv->patterns) {
    GList* node;
		for(node=self->priv->patterns;node;node=g_list_next(node)) {
			g_object_try_unref(node->data);
      node->data=NULL;
		}
	}

	GST_DEBUG("  done");

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_finalize(GObject *object) {
  BtMachine *self = BT_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_hash_table_destroy(self->priv->properties);
	g_free(self->priv->id);
	g_free(self->priv->plugin_name);
  g_free(self->priv->voice_types);
  g_free(self->priv->voice_dparams);
  g_free(self->priv->global_types);
  g_free(self->priv->global_dparams);
  // free list of patterns
	if(self->priv->patterns) {
		g_list_free(self->priv->patterns);
		self->priv->patterns=NULL;
	}
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtMachine *self = BT_MACHINE(instance);
  self->priv = g_new0(BtMachinePrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->voices=-1;
  self->priv->properties=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
  
  GST_DEBUG("!!!! self=%p, self->priv->machine=%p",self,self->priv->machine);
}

static void bt_machine_class_init(BtMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  
  gobject_class->set_property = bt_machine_set_property;
  gobject_class->get_property = bt_machine_get_property;
  gobject_class->dispose      = bt_machine_dispose;
  gobject_class->finalize     = bt_machine_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES,
                                  g_param_spec_pointer("properties",
                                     "properties prop",
                                     "list of machine properties",
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MACHINE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "song object, the machine belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_ID,
                                  g_param_spec_string("id",
                                     "id contruct prop",
                                     "machine identifier",
                                     "unamed machine", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PLUGIN_NAME,
                                  g_param_spec_string("plugin-name",
                                     "plugin_name contruct prop",
                                     "the name of the gst plugin for the machine",
                                     "unamed machine", /* default value */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_VOICES,
																	g_param_spec_ulong("voices",
                                     "voices prop",
                                     "number of voices in the machine",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_GLOBAL_PARAMS,
																	g_param_spec_ulong("global-params",
                                     "global_params prop",
                                     "number of params for the machine",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_VOICE_PARAMS,
																	g_param_spec_ulong("voice-params",
                                     "voice_params prop",
                                     "number of params for each machine voice",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine element prop",
                                     "the machine element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE));
  
  g_object_class_install_property(gobject_class,MACHINE_INPUT_LEVEL,
                                  g_param_spec_object("input-level",
                                     "input-level prop",
                                     "the input-level element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MACHINE_PATTERNS,
                                  g_param_spec_pointer("patterns",
                                     "pattern list prop",
                                     "A copy of the list of patterns",
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MACHINE_STATE,
                                  g_param_spec_enum("state",
                                     "state prop",
                                     "the current state of this machine",
                                     BT_TYPE_MACHINE_STATE,  /* enum type */
                                     BT_MACHINE_STATE_NORMAL, /* default value */
                                     G_PARAM_READWRITE));
}

GType bt_machine_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMachine),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_machine_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtMachine",&info,0);
  }
  return type;
}
