/* $Id: machine.c,v 1.88 2005-02-12 12:56:50 ensonic Exp $
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
  MACHINE_INPUT_GAIN,
  MACHINE_OUTPUT_LEVEL,
  MACHINE_OUTPUT_GAIN,
	MACHINE_PATTERNS,
	MACHINE_STATE
};

typedef enum {
	/* utillity elements to allow multiple inputs */
	PART_ADDER=0,
	/* helper to make adder link to next element */
	PART_ADDER_CONVERT,
	/* the elements to control and analyse the current input signal */
  PART_INPUT_LEVEL,
  PART_INPUT_GAIN,
	/* the buffer frames convert element is needed for machines that require fixed with buffers */
	//PART_BUFFER_FRAMES_CONVERT,
	/* the gstreamer element that produces/processes the signal */
  PART_MACHINE,
	/* the elements to control and analyse the current output signal */
  PART_OUTPUT_LEVEL,
  PART_OUTPUT_GAIN,
	/* utillity elements to allow multiple outputs */
	PART_SPREADER,
	/* how many elements are used */
	PART_COUNT
} BtMachinePart;

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
  
  /* the gstreamer elements that is used */
  GstElement *machines[PART_COUNT];
  
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

static gboolean bt_machine_set_mute(BtMachine *self,BtSetup *setup) {
	gboolean res=TRUE;
	GList *wires,*node;
	BtWire *wire;
	BtMachine *dst_machine;
	BtMachineState state;
	
	// we need to pause all elements downstream until we hit a loop-based element :(
	// @todo we need to do the same upstream too
	if((wires=bt_setup_get_wires_by_src_machine(setup,self))) {
		for(node=wires;node;node=g_list_next(node)) {
			wire=BT_WIRE(node->data);
    	g_object_get(G_OBJECT(wire),"dst",&dst_machine,NULL);
	    if(bt_machine_has_active_adder(dst_machine)) {	// or is paused
				GST_INFO("  setting element '%s' to paused",self->priv->id);
				if(gst_element_set_state(self->priv->machines[PART_MACHINE],GST_STATE_PAUSED)==GST_STATE_FAILURE) {
					GST_WARNING("    setting element '%s' to paused state failed",self->priv->id);
					res=FALSE;
				}
	    }
	    else {
	      bt_machine_set_mute(dst_machine,setup);
	    }
			g_object_unref(dst_machine);
			g_object_unref(wire);
	  }
		g_list_free(wires);
	}
	else {
		if(gst_element_set_state(self->priv->machines[PART_MACHINE],GST_STATE_PAUSED)==GST_STATE_FAILURE) {
			GST_WARNING("    setting element '%s' to paused state failed",self->priv->id);
			res=FALSE;
		}
	}
	return(res);
}

static gboolean bt_machine_unset_mute(BtMachine *self,BtSetup *setup) {
	gboolean res=TRUE;
	GList *wires,*node;
	BtWire *wire;
	BtMachine *dst_machine;
	BtMachineState state;
	
	// we need to unpause all elements downstream until we hit a loop-based element :(
	// @todo we need to do the same upstream too
	if((wires=bt_setup_get_wires_by_src_machine(setup,self))) {
		for(node=wires;node;node=g_list_next(node)) {
			wire=BT_WIRE(node->data);
    	g_object_get(G_OBJECT(wire),"dst",&dst_machine,NULL);
	    if(bt_machine_has_active_adder(dst_machine)) {	// or is paused
				GST_INFO("  setting element '%s' to playing",self->priv->id);
				if(gst_element_set_state(self->priv->machines[PART_MACHINE],GST_STATE_PLAYING)==GST_STATE_FAILURE) {
					GST_WARNING("    setting element '%s' to playing state failed",self->priv->id);
					res=FALSE;
				}
	    }
	    else {
	      bt_machine_unset_mute(dst_machine,setup);
	    }
			g_object_unref(dst_machine);
			g_object_unref(wire);
	  }
		g_list_free(wires);
	}
	else {
		if(gst_element_set_state(self->priv->machines[PART_MACHINE],GST_STATE_PLAYING)==GST_STATE_FAILURE) {
			GST_WARNING("    setting element '%s' to playing state failed",self->priv->id);
			res=FALSE;
		}
	}
	return(res);
}

static gboolean bt_machine_change_state(BtMachine *self, BtMachineState new_state) {
	gboolean res=TRUE;
	BtSetup *setup;

	// reject a few nonsense changes
	if((new_state==BT_MACHINE_STATE_BYPASS) && (!BT_IS_PROCESSOR_MACHINE(self))) return(FALSE);
	if((new_state==BT_MACHINE_STATE_SOLO) && (BT_IS_SINK_MACHINE(self))) return(FALSE);

	g_object_get(self->priv->song,"setup",&setup,NULL);
	
	GST_INFO("state change for element '%s'",self->priv->id);
	
	// return to normal state
	switch(self->priv->state) {
		case BT_MACHINE_STATE_MUTE:
			if(!bt_machine_unset_mute(self,setup)) res=FALSE;
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
			if(!bt_machine_set_mute(self,setup)) res=FALSE;
			break;
		case BT_MACHINE_STATE_SOLO:
			// @todo set all but this machine to paused
			break;
		case BT_MACHINE_STATE_BYPASS:
			// @todo set this machine to paused + connect its source and sink
			break;
	}
	self->priv->state=new_state;

	g_object_try_unref(setup);
	return(res);
}

/*
 * bt_machine_get_sink_peer:
 * @elem: the element to locate the sink peer for
 *
 * Locates the #GstElement that is connected to the sink pad of the given
 * #GstElement.
 *
 * Returns: the sink peer #GstElement or NULL
 */
static GstElement *bt_machine_get_sink_peer(GstElement *elem) {
	GstElement *peer;
	GstPad *pad,*peer_pad;
		
	// add before machine (sink peer of machine)
	if((pad=gst_element_get_pad(elem,"sink"))
		&& (peer_pad=gst_pad_get_peer(pad))
		&& (peer=GST_ELEMENT(gst_object_get_parent(GST_OBJECT(peer_pad))))
	) {
		return(peer);
	}
	return(NULL);
}

/*
 * bt_machine_insert_element:
 * @self: the machine for which the element should be inserted
 * @part_position: the element at this position should be inserted (activated)
 *
 * Searches surrounding elements of the new element for active peers
 * and connects them. The new elemnt needs to be created before calling this method.
 *
 * Returns: %TRUE for sucess
 */
static gboolean bt_machine_insert_element(BtMachine *self,GstElement *peer,BtMachinePart part_position) {
	gboolean res=FALSE;
	gint i,pre,post;
		
	//seek elements before and after part_position
	pre=post=-1;
	for(i=part_position-1;i>-1;i--) {
		if(self->priv->machines[i]) {
			pre=i;break;
		}
	}
	for(i=part_position+1;i<PART_COUNT;i++) {
		if(self->priv->machines[i]) {
			post=i;break;
		}
	}
	GST_INFO("positions: %d ... %d(%s) ... %d",pre,part_position,GST_OBJECT_NAME(self->priv->machines[part_position]),post);
	// get pads
	if((pre!=-1) && (post!=-1)) {
		// unlink old connection
		gst_element_unlink(self->priv->machines[pre],self->priv->machines[post]);
		// link new connection
  	res=gst_element_link_many(self->priv->machines[pre],self->priv->machines[part_position],self->priv->machines[post],NULL);
	}
	else if(pre==-1) {
		// unlink old connection
		gst_element_unlink(peer,self->priv->machines[post]);
		// link new connection
  	res=gst_element_link_many(peer,self->priv->machines[part_position],self->priv->machines[post],NULL);
	}
	else if(post==-1) {
		// unlink old connection
		gst_element_unlink(self->priv->machines[pre],peer);
		// link new connection
  	res=gst_element_link_many(self->priv->machines[pre],self->priv->machines[part_position],peer,NULL);
	}
	return(res);
}

/*
 * bt_machine_make_name:
 * @self: the machine to generate the unique name for
 *
 * Generates a system wide unique name by adding the song instance address as a
 * postfix to the name.
 *
 * Return: the name in newly allocated memory
 */
static gchar *bt_machine_make_name(BtMachine *self) {
	gchar *name;
	
	name=g_strdup_printf("%s_%p",self->priv->id,self->priv->song);
	return(name);
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
	gchar *name; 

  g_assert(BT_IS_MACHINE(self));
  g_assert(self->priv->machines[PART_MACHINE]==NULL);
  g_assert(self->priv->id);
  g_assert(self->priv->plugin_name);
	g_assert(self->priv->song);
  GST_INFO("initializing machine");

	name=bt_machine_make_name(self);
  self->priv->machines[PART_MACHINE]=gst_element_factory_make(self->priv->plugin_name,name);
	g_free(name);

  if(!self->priv->machines[PART_MACHINE]) {
    GST_ERROR("  failed to instantiate machine \"%s\"",self->priv->plugin_name);
    return(FALSE);
  }
  // we need to make sure the machine is out of the right class
  {
    GstElementFactory *element_factory=gst_element_get_factory(self->priv->machines[PART_MACHINE]);
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
  self->dst_elem=self->src_elem=self->priv->machines[PART_MACHINE];
  GST_INFO("  instantiated machine \"%s\", obj->ref_count=%d",self->priv->plugin_name,G_OBJECT(self->priv->machines[PART_MACHINE])->ref_count);
  if((self->priv->dparam_manager=gst_dpman_get_manager(self->priv->machines[PART_MACHINE]))) {
    GParamSpec **specs;
    GstDParam **dparam;
    guint i;

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
  gst_bin_add(self->priv->bin,self->priv->machines[PART_MACHINE]);
  GST_INFO("  added machine to bin, obj->ref_count=%d",G_OBJECT(self->priv->machines[PART_MACHINE])->ref_count);
  g_assert(self->priv->machines[PART_MACHINE]!=NULL);
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
 * bt_machine_enable_input_level:
 * @self: the machine to enable the input-level analyser in
 *
 * Creates the input-level analyser of the machine and activates it.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 */
gboolean bt_machine_enable_input_level(BtMachine *self) {
  gboolean res=FALSE;
	GstElement *peer;
	
	g_assert(BT_IS_MACHINE(self));
	g_assert(!BT_IS_SOURCE_MACHINE(self));
	
	GST_INFO(" for machine '%s'",self->priv->id);
  
  // add input-level analyser
  if(!(self->priv->machines[PART_INPUT_LEVEL]=gst_element_factory_make("level",g_strdup_printf("input_level_%p",self)))) {
    GST_ERROR("failed to create input level analyser for '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
  }
  g_object_set(G_OBJECT(self->priv->machines[PART_INPUT_LEVEL]),
		"interval",0.1,"signal",TRUE,
		"peak-ttl",0.2,"peak-falloff", 20.0,
		NULL);
  gst_bin_add(self->priv->bin,self->priv->machines[PART_INPUT_LEVEL]);
	// is the machine unconnected towards the input side (its sink)?
	if(!(peer=bt_machine_get_sink_peer(self->priv->machines[PART_MACHINE]))) {
		GST_DEBUG("machine '%s' is not yet connected",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
  	if(!gst_element_link(self->priv->machines[PART_INPUT_LEVEL],self->priv->machines[PART_MACHINE])) {
			GST_ERROR("failed to link the machines input level analyser");goto Error;
		}
  	self->dst_elem=self->priv->machines[PART_INPUT_LEVEL];
  	GST_INFO("sucessfully added input level analyser %p",self->priv->machines[PART_INPUT_LEVEL]);
	}
	else {
		GST_DEBUG("machine '%s' is connected to '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]),GST_OBJECT_NAME(peer));
		if(!bt_machine_insert_element(self,peer,PART_INPUT_LEVEL)) {
			GST_ERROR("failed to link the input level analyser for '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
		}	
 		GST_INFO("sucessfully added input level analyser %p",self->priv->machines[PART_INPUT_LEVEL]);
	}
  res=TRUE;
Error:
  return(res);
}

/**
 * bt_machine_enable_input_gain:
 * @self: the machine to enable the input-gain element in
 *
 * Creates the input-gain element of the machine and activates it.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 */
gboolean bt_machine_enable_input_gain(BtMachine *self) {
  gboolean res=FALSE;
	GstElement *peer;
	
	g_assert(BT_IS_MACHINE(self));
	g_assert(!BT_IS_SOURCE_MACHINE(self));

	GST_INFO(" for machine '%s'",self->priv->id);

  // add input-gain element
  if(!(self->priv->machines[PART_INPUT_GAIN]=gst_element_factory_make("volume",g_strdup_printf("input_gain_%p",self)))) {
    GST_ERROR("failed to create machines input gain element");goto Error;
  }
  gst_bin_add(self->priv->bin,self->priv->machines[PART_INPUT_GAIN]);
	// is the machine unconnected towards the input side (its sink)?
	if(!(peer=bt_machine_get_sink_peer(self->priv->machines[PART_MACHINE]))) {
		GST_DEBUG("machine '%s' is not yet connected",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
  	if(!gst_element_link(self->priv->machines[PART_INPUT_GAIN],self->priv->machines[PART_MACHINE])) {
			GST_ERROR("failed to link the input gain element for '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
		}
  	self->dst_elem=self->priv->machines[PART_INPUT_GAIN];
  	GST_INFO("sucessfully added input gain element %p",self->priv->machines[PART_INPUT_GAIN]);
	}
	else {
		GST_DEBUG("machine '%s' is connected to '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]),GST_OBJECT_NAME(peer));
		if(!bt_machine_insert_element(self,peer,PART_INPUT_GAIN)) {
			GST_ERROR("failed to link the input gain element for '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
		}	
 		GST_INFO("sucessfully added input gain element %p",self->priv->machines[PART_INPUT_GAIN]);
	}
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
  
  if(!self->priv->machines[PART_ADDER]) {
		// create the adder
    self->priv->machines[PART_ADDER]=gst_element_factory_make("adder",g_strdup_printf("adder_%p",self));
    g_assert(self->priv->machines[PART_ADDER]!=NULL);
    gst_bin_add(self->priv->bin, self->priv->machines[PART_ADDER]);
    // adder not links directly to some elements
    self->priv->machines[PART_ADDER_CONVERT]=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",self));
    g_assert(self->priv->machines[PART_ADDER_CONVERT]!=NULL);
    gst_bin_add(self->priv->bin, self->priv->machines[PART_ADDER_CONVERT]);
    GST_DEBUG("  about to link adder -> convert -> dst_elem");
    if(!gst_element_link_many(self->priv->machines[PART_ADDER], self->priv->machines[PART_ADDER_CONVERT], self->dst_elem, NULL)) {
      GST_ERROR("failed to link the machines internal adder");res=FALSE;
    }
    else {
      GST_DEBUG("  adder activated for \"%s\"",gst_element_get_name(self->priv->machines[PART_MACHINE]));
      self->dst_elem=self->priv->machines[PART_ADDER];
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
  return(self->dst_elem==self->priv->machines[PART_ADDER]);
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
  
  if(!self->priv->machines[PART_SPREADER]) {
    self->priv->machines[PART_SPREADER]=gst_element_factory_make("tee",g_strdup_printf("tee%p",self));
    g_assert(self->priv->machines[PART_SPREADER]!=NULL);
    gst_bin_add(self->priv->bin, self->priv->machines[PART_SPREADER]);
    if(!gst_element_link(self->src_elem, self->priv->machines[PART_SPREADER])) {
      GST_ERROR("failed to link the machines internal spreader");res=FALSE;
    }
    else {
      GST_DEBUG("  spreader activated for \"%s\"",gst_element_get_name(self->priv->machines[PART_MACHINE]));
      self->src_elem=self->priv->machines[PART_SPREADER];
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
  return(self->src_elem==self->priv->machines[PART_SPREADER]);
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
	
	if(index<g_list_length(self->priv->patterns)) {
		return(g_object_ref(BT_PATTERN(g_list_nth_data(self->priv->patterns,(guint)index))));
	}
	return(NULL);
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
			// error reason
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
      g_object_set_property(G_OBJECT(dparam),"value-double",event);
			//g_object_set(G_OBJECT(dparam),"value-double",g_value_get_double(event),NULL);
    } break;
    case G_TYPE_INT: {
			g_object_set_property(G_OBJECT(dparam),"value-int",event);
		} break;
    default:
			GST_ERROR("unsupported GType=%d:'%s'",G_VALUE_TYPE(event),G_VALUE_TYPE_NAME(event));
  }
}

/**
 * bt_machine_set_voice_dparam_value:
 * @self: the machine to set the global dparam value
 * @voice: the voice to change
 * @index: the offset in the list of global dparams
 * @event: the new value
 *
 * Sets a the specified voice dparam to the give data value.
 */
void bt_machine_set_voice_dparam_value(const BtMachine *self, gulong voice, gulong index, GValue *event) {
  GstDParam *dparam;
  
  g_assert(BT_IS_MACHINE(self));
  g_assert(G_IS_VALUE(event));
	g_assert(voice<self->priv->voices);
  g_assert(index<self->priv->voice_params);

	// @todo set voice events
	// dparam=self->priv->voice_dparams[voice][index];
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
      g_value_set_object(value, self->priv->machines[PART_MACHINE]);
    } break;
    case MACHINE_INPUT_LEVEL: {
      g_value_set_object(value, self->priv->machines[PART_INPUT_LEVEL]);
    } break;
    case MACHINE_INPUT_GAIN: {
      g_value_set_object(value, self->priv->machines[PART_INPUT_GAIN]);
    } break;
    case MACHINE_OUTPUT_LEVEL: {
      g_value_set_object(value, self->priv->machines[PART_OUTPUT_LEVEL]);
    } break;
    case MACHINE_OUTPUT_GAIN: {
      g_value_set_object(value, self->priv->machines[PART_OUTPUT_GAIN]);
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
			if(self->priv->machines[PART_MACHINE]) {
				gchar *name=bt_machine_make_name(self);
				gst_element_set_name(self,name);
				g_free(name);
			}
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
	gint i;

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // remove the GstElements from the bin
  // gstreamer uses floating references, therefore elements are destroyed, when removed from the bin
  if(self->priv->bin) {
		for(i=0;i<PART_COUNT;i++) {
			if(self->priv->machines[i]) {
				g_assert(GST_IS_BIN(self->priv->bin));
				g_assert(GST_IS_ELEMENT(self->priv->machines[i]));
				GST_DEBUG("  removing machine \"%s\" from bin, obj->ref_count=%d",gst_element_get_name(self->priv->machines[i]),(G_OBJECT(self->priv->machines[i]))->ref_count);
				gst_bin_remove(self->priv->bin,self->priv->machines[i]);
				GST_DEBUG("  bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
			}
    }
    // release the bin (that is ref'ed in bt_machine_new() )
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

	GST_DEBUG("  chaining up");

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
	GST_DEBUG("  done");
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
  
  GST_DEBUG("!!!! self=%p, self->priv->machine=%p",self,self->priv->machines[PART_MACHINE]);
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

  g_object_class_install_property(gobject_class,MACHINE_INPUT_GAIN,
                                  g_param_spec_object("input-gain",
                                     "input-gain prop",
                                     "the input-gain element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MACHINE_OUTPUT_LEVEL,
                                  g_param_spec_object("output-level",
                                     "output-level prop",
                                     "the output-level element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MACHINE_OUTPUT_GAIN,
                                  g_param_spec_object("output-gain",
                                     "output-gain prop",
                                     "the output-gain element, if any",
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
