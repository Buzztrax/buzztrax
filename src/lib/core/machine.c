// $Id: machine.c,v 1.148 2005-08-24 13:25:34 ensonic Exp $
/**
 * SECTION:btmachine
 * @short_description: base class for signal processing machines
 *
 * The machine class cares about inserting additional low-level elemnts to do 
 * signal conversion etc.. Further it provides general facillities like 
 * input/output level monitoring.
 *
 * A machine can have several #GstElements:
 * <variablelist>
 *  <varlistentry>
 *    <term>adder:</term>
 *    <listitem><simpara>mixes all incomming signals</simpara></listitem>
 *  </varlistentry>
 *  <varlistentry>
 *    <term>input volume:</term>
 *    <listitem><simpara>gain for incomming signals</simpara></listitem>
 *  </varlistentry>
 *  <varlistentry>
 *    <term>input level:</term>
 *    <listitem><simpara>level meter for incomming signal</simpara></listitem>
 *  </varlistentry>
 *  <varlistentry>
 *    <term>machine:</term>
 *    <listitem><simpara>the real machine</simpara></listitem>
 *  </varlistentry>
 *  <varlistentry>
 *    <term>output volume:</term>
 *    <listitem><simpara>gain for outgoing signal</simpara></listitem>
 *  </varlistentry>
 *  <varlistentry>
 *    <term>output level:</term>
 *    <listitem><simpara>level meter for outgoing signal</simpara></listitem>
 *  </varlistentry>
 *  <varlistentry>
 *    <term>spreader:</term>
 *    <listitem><simpara>distibutes signal to outgoing connections</simpara></listitem>
 *  </varlistentry>
 * </variablelist>
 * The adder and spreader elements are activated on demand.
 * The volume controls and level meters are activated as requested via the API.
 * It is recommended to only activate them, when needed. The instances are cached
 * after deactivation (so that they can be easily reactivated) and destroyed with
 * the #BtMachine object.
 *
 * Furthermore the machine handles a list of #BtPattern instances. These contain
 * event patterns that form a #BtSequence.
 */ 
/*
 * @todo try to derive this from GstBin!
 *  then put the machines into itself (and not into the songs bin, but insert the machine directly into the song->bin
 *  when adding internal machines we need to fix the ghost pads (this may require relinking)
 *    gst_element_add_ghost_pad and gst_element_remove_ghost_pad
 */
 
#define BT_CORE
#define BT_MACHINE_C

#include <libbtcore/core.h>

//-- signal ids

enum {
  PATTERN_ADDED_EVENT,
  PATTERN_REMOVED_EVENT,
  LAST_SIGNAL
};

//-- property ids

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

  /* dynamic parameter control */
  GstController *global_controller;
  GstController **voice_controllers;
  gchar **global_names,**voice_names;
  GType *global_types,*voice_types; 
  guint *global_flags,*voice_flags;
  GValue *global_no_val,*voice_no_val;

  GList *patterns;  // each entry points to BtPattern
  
  /* the gstreamer elements that is used */
  GstElement *machines[PART_COUNT];
  
  /* public fields are
  GstElement *dst_elem,*src_elem;
  */
};

static GQuark error_domain=0;

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- enums

GType bt_machine_state_get_type(void) {
  static GType type = 0;
  if(type==0) {
    static GEnumValue values[] = {
      { BT_MACHINE_STATE_NORMAL,"BT_MACHINE_STATE_NORMAL","just working" },
      { BT_MACHINE_STATE_MUTE,  "BT_MACHINE_STATE_MUTE",   "be quiet" },
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
  
  // we need to pause all elements downstream until we hit a loop-based element  (has an adder) :(
  // @todo we need to do the same upstream too until we hit one with a spreader
  if((wires=bt_setup_get_wires_by_src_machine(setup,self))) {
    for(node=wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      g_object_get(G_OBJECT(wire),"dst",&dst_machine,NULL);
      GST_INFO("  setting element '%s' to paused",self->priv->id);
      if(gst_element_set_state(self->priv->machines[PART_MACHINE],GST_STATE_PAUSED)==GST_STATE_FAILURE) {
        GST_WARNING("    setting element '%s' to paused state failed",self->priv->id);
        res=FALSE;
      }
      if(!bt_machine_has_active_adder(dst_machine)) {
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
  
  // we need to unpause all elements downstream until we hit a loop-based element (has an adder) :(
  // @todo we need to do the same upstream too until we hit one with a spreader
  if((wires=bt_setup_get_wires_by_src_machine(setup,self))) {
    for(node=wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      g_object_get(G_OBJECT(wire),"dst",&dst_machine,NULL);
      GST_INFO("  setting element '%s' to playing",self->priv->id);
      if(gst_element_set_state(self->priv->machines[PART_MACHINE],GST_STATE_PLAYING)==GST_STATE_FAILURE) {
        GST_WARNING("    setting element '%s' to playing state failed",self->priv->id);
        res=FALSE;
      }
      if(!bt_machine_has_active_adder(dst_machine)) {
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

/*
 * bt_machine_change_state:
 *
 * Reset old state and go to new state. Do sanity checking of allowed states for
 * given machine.
 *
 * Returns: %TRUE for success
 */
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
    case BT_MACHINE_STATE_MUTE:  { // source, processor, sink
      if(!bt_machine_unset_mute(self,setup)) res=FALSE;
    } break;
    case BT_MACHINE_STATE_SOLO:  { // source
      GList *node,*machines=bt_setup_get_machines_by_type(setup,BT_TYPE_SOURCE_MACHINE);
      BtMachine *machine;
      // set all but this machine to playing again
      for(node=machines;node;node=g_list_next(node)) {
        machine=BT_MACHINE(node->data);
        if(machine!=self) {
          if(!bt_machine_unset_mute(machine,setup)) res=FALSE;
        }
        g_object_unref(machine);
      }
      g_list_free(machines);
    } break;
    case BT_MACHINE_STATE_BYPASS:  { // processor
      // @todo disconnect its source and sink + set this machine to playing
    } break;
    case BT_MACHINE_STATE_NORMAL:
      g_return_val_if_reached(FALSE);
      break;
  }
  // set to new state
  switch(new_state) {
    case BT_MACHINE_STATE_MUTE:  { // source, processor, sink
      if(!bt_machine_set_mute(self,setup)) res=FALSE;
      // alternatively just disconnect
    } break;
    case BT_MACHINE_STATE_SOLO:  { // source
      GList *node,*machines=bt_setup_get_machines_by_type(setup,BT_TYPE_SOURCE_MACHINE);
      BtMachine *machine;
      // set all but this machine to paused
      for(node=machines;node;node=g_list_next(node)) {
        machine=BT_MACHINE(node->data);
        if(machine!=self) {
          // if a different machine is solo, set it to normal and unmute the current source
          // @todo graphics need to listen to notify::state
          if(machine->priv->state==BT_MACHINE_STATE_SOLO) {
            machine->priv->state=BT_MACHINE_STATE_NORMAL;
            g_object_notify(G_OBJECT(machine),"state");
            if(!bt_machine_unset_mute(self,setup)) res=FALSE;
          }
          if(!bt_machine_set_mute(machine,setup)) res=FALSE;
        }
        g_object_unref(machine);
      }
      g_list_free(machines);
    } break;
    case BT_MACHINE_STATE_BYPASS:  { // processor
      // @todo set this machine to paused + connect its source and sink
    } break;
    case BT_MACHINE_STATE_NORMAL:
      g_return_val_if_reached(FALSE);
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
  BtSetup *setup;
  BtWire *wire;
    
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
    if(!res) {
      gst_element_unlink_many(self->priv->machines[pre],self->priv->machines[part_position],self->priv->machines[post],NULL);
      GST_WARNING("failed to link part '%s' inbetween '%s' and '%s'",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[pre]),GST_OBJECT_NAME(self->priv->machines[post]));
    }
  }
  else if(pre==-1) {
    self->dst_elem=self->priv->machines[part_position];
    // unlink old connection
    gst_element_unlink(peer,self->priv->machines[post]);
    // link new connection
    res=gst_element_link_many(peer,self->priv->machines[part_position],self->priv->machines[post],NULL);
    if(!res) {
      gst_element_unlink_many(peer,self->priv->machines[part_position],self->priv->machines[post],NULL);
      GST_WARNING("failed to link part '%s' before '%s'",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[post]));
      // try to re-wire
      if((res=gst_element_link(self->priv->machines[part_position],self->priv->machines[post]))) {
        g_object_get(G_OBJECT(self->priv->song),"setup",&setup,NULL);
        if((wire=bt_setup_get_wire_by_dst_machine(setup,self))) {
          if(!(res=bt_wire_reconnect(wire))) {
            GST_WARNING("failed to reconnect wire after linking '%s' before '%s'",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[post]));
          }
          g_object_unref(wire);
        }
        g_object_try_unref(setup);
      }
      else {
        GST_WARNING("failed to link part '%s' before '%s' again",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[post]));
      }
    }
  }
  else if(post==-1) {
    self->src_elem=self->priv->machines[part_position];
    // unlink old connection
    gst_element_unlink(self->priv->machines[pre],peer);
    // link new connection
    res=gst_element_link_many(self->priv->machines[pre],self->priv->machines[part_position],peer,NULL);
    if(!res) {
      gst_element_unlink_many(self->priv->machines[pre],self->priv->machines[part_position],peer,NULL);
      GST_WARNING("failed to link part '%s' after '%s'",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[pre]));
      // try to re-wire
      if((res=gst_element_link(self->priv->machines[pre],self->priv->machines[part_position]))) {
        g_object_get(G_OBJECT(self->priv->song),"setup",&setup,NULL);
        if((wire=bt_setup_get_wire_by_src_machine(setup,self))) {
          if(!(res=bt_wire_reconnect(wire))) {
            GST_WARNING("failed to reconnect wire after linking '%s' after '%s'",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[pre]));
          }
          g_object_unref(wire);
        }
        g_object_try_unref(setup);
      }
      else {
        GST_WARNING("failed to link part '%s' after '%s' again",GST_OBJECT_NAME(self->priv->machines[part_position]),GST_OBJECT_NAME(self->priv->machines[pre]));
      }
    }
  }
  else {
    GST_ERROR("failed to link part '%s' in broken machine",GST_OBJECT_NAME(self->priv->machines[part_position]));
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
static gchar *bt_machine_make_name(const BtMachine *self) {
  gchar *name;
  
  name=g_strdup_printf("%s_%p",self->priv->id,self->priv->song);
  return(name);
}

/*
 * bt_machine_resize_pattern_voices:
 * @self: the machine which has changed its number of voices
 *
 * Iterates over the machines patterns and adjust their voices too.
 */
static void bt_machine_resize_pattern_voices(const BtMachine *self) {
  GList* node;

  // reallocate self->priv->patterns->priv->data
  for(node=self->priv->patterns;node;node=g_list_next(node)) {
    g_object_set(BT_PATTERN(node->data),"voices",self->priv->voices,NULL);
  }
}

/*
 * bt_machine_resize_voices:
 * @self: the machine which has changed its number of voices
 *
 * Adjust the private data structure after a change in the number of voices.
 */
static void bt_machine_resize_voices(const BtMachine *self,gulong voices) {
  GST_INFO("changing machine voices from %d to %d",voices,self->priv->voices);

  if(!GST_IS_CHILD_PROXY(self->priv->machines[PART_MACHINE])) return;

  g_object_set(self->priv->machines[PART_MACHINE],"voices",self->priv->voices,NULL);

  // @todo make it use g_renew0()
  // this is not as easy as it sounds (realloc does not know how big the old mem was)
  self->priv->voice_controllers=(GstController **)g_renew(gpointer, self->priv->voice_controllers ,self->priv->voices);
  if(voices<self->priv->voices) {
    GstObject *voice_child;
    GParamSpec **properties,*property;
    guint number_of_properties;
    gulong i,j,k;

    // bind params for new voices
    for(j=voices;j<self->priv->voices;j++) {
      self->priv->voice_controllers[j]=NULL;
      if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),j))) {
        if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(voice_child)),&number_of_properties))) {
          for(i=k=0;i<number_of_properties;i++) {
            property=properties[i];
            if(property->flags&GST_PARAM_CONTROLLABLE) {
              // bind params to the voice controller
              if(!(self->priv->voice_controllers[j]=gst_controller_new(G_OBJECT(voice_child), property->name, NULL))) {
                GST_WARNING("failed to add property \"%s\" to the %d voice controller",property->name,j);
              }
              // set interpolation mode depending on param type
              if(bt_machine_is_voice_param_trigger(self,k)) {
                gst_controller_set_interpolation_mode(self->priv->voice_controllers[j],self->priv->voice_names[k],GST_INTERPOLATE_TRIGGER);
              }
              else { // one of GST_INTERPOLATE_NONE/LINEAR/...
                gst_controller_set_interpolation_mode(self->priv->voice_controllers[j],self->priv->voice_names[k],GST_INTERPOLATE_NONE);
              }
              k++;
            }
          }
					g_free(properties);
        }
      }
    }
  }
}


/*
 * bt_machine_get_property_meta_value:
 * @value: the value that will hold the result
 * @property: the paramspec object to get the meta data from
 * @key: the meta data key
 *
 * Fetches the meta data from the given paramspec object and sets the value.
 * The values needs to be initialized to the correct type.
 */
static void bt_machine_get_property_meta_value(GValue *value,GParamSpec *property,GQuark key) {
  g_value_init(value,property->value_type);
  switch(property->value_type) {
    case G_TYPE_BOOLEAN:
      g_value_set_boolean(value,GPOINTER_TO_INT(g_param_spec_get_qdata(property,key)));
      break;
    case G_TYPE_INT:
      g_value_set_int(value,GPOINTER_TO_INT(g_param_spec_get_qdata(property,key)));
      break;
    case G_TYPE_UINT:
      g_value_set_uint(value,GPOINTER_TO_UINT(g_param_spec_get_qdata(property,key)));
      break;
    default:
      GST_ERROR("unsupported GType=%d:'%s'",property->value_type,G_VALUE_TYPE_NAME(property->value_type));
  }
}

//-- signal handler

void bt_machine_on_bpm_changed(BtSongInfo *song_info, GParamSpec *arg, gpointer user_data) {
  BtMachine *self=BT_MACHINE(user_data);
  gulong bpm;
  
  g_object_get(song_info,"bpm",&bpm,NULL);
  gst_tempo_change_tempo(GST_TEMPO(self->priv->machines[PART_MACHINE]),(glong)bpm,-1,-1);
}

void bt_machine_on_tpb_changed(BtSongInfo *song_info, GParamSpec *arg, gpointer user_data) {
  BtMachine *self=BT_MACHINE(user_data);
  gulong tpb;
  
  g_object_get(song_info,"tpb",&tpb,NULL);
  gst_tempo_change_tempo(GST_TEMPO(self->priv->machines[PART_MACHINE]),-1,(glong)tpb,-1);
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
  BtSetup *setup;
  GParamSpec **properties;
  guint number_of_properties;

  g_assert(BT_IS_MACHINE(self));
  g_assert(self->priv->machines[PART_MACHINE]==NULL);
  g_assert(self->priv->id);
  g_assert(self->priv->plugin_name);
  g_assert(self->priv->song);

  GST_INFO("initializing machine");
  // name the machine and try to instantiate it
  {
    gchar *name=bt_machine_make_name(self);
    self->priv->machines[PART_MACHINE]=gst_element_factory_make(self->priv->plugin_name,name);
    g_free(name);
  }
  if(!self->priv->machines[PART_MACHINE]) {
    GST_ERROR("  failed to instantiate machine \"%s\"",self->priv->plugin_name);
    return(FALSE);
  }
  GST_INFO("machine element instantiated");
  // we need to make sure the machine is from the right class
  {
    GstElementFactory *element_factory=gst_element_get_factory(self->priv->machines[PART_MACHINE]);
    const gchar *element_class=gst_element_factory_get_klass(element_factory);
    GST_INFO("checking machine class \"%s\"",element_class);
    if(BT_IS_SINK_MACHINE(self)) {
      if(g_ascii_strncasecmp(element_class,"Sink/",5) && g_ascii_strncasecmp(element_class,"Sink\0",5)) {
        GST_ERROR("  plugin \"%s\" is in \"%s\" class instead of \"Sink/...\"",self->priv->plugin_name,element_class);
        return(FALSE);
      }
    }
    else if(BT_IS_SOURCE_MACHINE(self)) {
      if(g_ascii_strncasecmp(element_class,"Source/",7) && g_ascii_strncasecmp(element_class,"Source\0",7)) {
        GST_ERROR("  plugin \"%s\" is in \"%s\" class instead of \"Source/...\"",self->priv->plugin_name,element_class);
        return(FALSE);
      }
    }
    else if(BT_IS_PROCESSOR_MACHINE(self)) {
      if(g_ascii_strncasecmp(element_class,"Filter/",7) && g_ascii_strncasecmp(element_class,"Filter\0",7)) {
        GST_ERROR("  plugin \"%s\" is in \"%s\" class instead of \"Filter/...\"",self->priv->plugin_name,element_class);
        return(FALSE);
      }
    }
  }
  // there is no adder or spreader in use by default
  self->dst_elem=self->src_elem=self->priv->machines[PART_MACHINE];
  GST_INFO("  instantiated machine \"%s\", obj->ref_count=%d",self->priv->plugin_name,G_OBJECT(self->priv->machines[PART_MACHINE])->ref_count);
  
  // register global params
  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self->priv->machines[PART_MACHINE])),&number_of_properties))) {
    GParamSpec *property;
    guint i,j;
    
    // count number of controlable params
    for(i=0;i<number_of_properties;i++) {
      if(properties[i]->flags&GST_PARAM_CONTROLLABLE) self->priv->global_params++;
    }
    self->priv->global_types =(GType *     )g_new0(GType   ,self->priv->global_params);
    self->priv->global_names =(gchar **    )g_new0(gpointer,self->priv->global_params);
    self->priv->global_flags =(guint *     )g_new0(guint   ,self->priv->global_params);
    self->priv->global_no_val=(GValue *    )g_new0(GValue  ,self->priv->global_params);
    for(i=j=0;i<number_of_properties;i++) {
      property=properties[i];
      if(property->flags&GST_PARAM_CONTROLLABLE) {
        GST_DEBUG("    adding global_param [%d/%d] \"%s\"",j,self->priv->global_params,property->name);
        // add global param
        self->priv->global_names[j]=property->name;
        self->priv->global_types[j]=property->value_type;
        if(GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE])) {
          self->priv->global_flags[j]=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
          bt_machine_get_property_meta_value(&self->priv->global_no_val[j],property,gst_property_meta_quark_no_val);
        }
        // bind param to machines global controller
        if(!(self->priv->global_controller=gst_controller_new(G_OBJECT(self->priv->machines[PART_MACHINE]), property->name, NULL))) {
          GST_WARNING("failed to add property \"%s\" to the global controller",property->name);
        }
        // set interpolation mode depending on param type
        if(bt_machine_is_global_param_trigger(self,j)) {
          gst_controller_set_interpolation_mode(self->priv->global_controller,self->priv->global_names[j],GST_INTERPOLATE_TRIGGER);
        }
        else { // one of GST_INTERPOLATE_NONE/LINEAR/...
          gst_controller_set_interpolation_mode(self->priv->global_controller,self->priv->global_names[j],GST_INTERPOLATE_NONE);
        }
        GST_DEBUG("    added global_param [%d/%d] \"%s\"",j,self->priv->global_params,property->name);
        j++;
      }
    }
    g_free(properties);
  }
  // check if the elemnt implements the GstChildProxy interface
  if(GST_IS_CHILD_PROXY(self->priv->machines[PART_MACHINE])) {
    GstObject *voice_child;

    GST_INFO("  instance is polyphonic!, initial voices=%d",self->priv->voices);
    
    g_object_set(self->priv->machines[PART_MACHINE],"voices",self->priv->voices,NULL);
    
    // register voice params
    // get child for voice 0
    if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),0))) {
      if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(voice_child)),&number_of_properties))) {
        GParamSpec *property;
        guint i,j;
        
        // count number of controlable params
        for(i=0;i<number_of_properties;i++) {
          if(properties[i]->flags&GST_PARAM_CONTROLLABLE) self->priv->voice_params++;
        }
        self->priv->voice_types      =(GType *        )g_new0(GType   ,self->priv->voice_params);
        self->priv->voice_names      =(gchar **       )g_new0(gpointer,self->priv->voice_params);
        self->priv->voice_flags      =(guint *        )g_new0(guint   ,self->priv->voice_params);
        self->priv->voice_no_val     =(GValue *       )g_new0(GValue  ,self->priv->voice_params);

        for(i=j=0;i<number_of_properties;i++) {
          property=properties[i];
          if(property->flags&GST_PARAM_CONTROLLABLE) {
            GST_DEBUG("    adding voice_param [%d/%d] \"%s\"",j,self->priv->voice_params,property->name);
            // add voice param
            self->priv->voice_names[j]=property->name;
            self->priv->voice_types[j]=property->value_type;
            if(GST_IS_PROPERTY_META(voice_child)) {
              self->priv->voice_flags[j]=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
              bt_machine_get_property_meta_value(&self->priv->voice_no_val[j],property,gst_property_meta_quark_no_val);
            }
            GST_DEBUG("    added voice_param [%d/%d] \"%s\"",j,self->priv->voice_params,property->name);
            j++;
          }
        }
      }
      g_free(properties);

      // bind params to machines voice controller
      bt_machine_resize_voices(self,0);
    }
    else {
      GST_WARNING("  can't get first voice child!");
    }
  }
  else {
    GST_INFO("  instance is monophonic!");
  }
  // check if the elemnt implements the GstTempo interface
  if(GST_IS_TEMPO(self->priv->machines[PART_MACHINE])) {
    BtSongInfo *song_info;
    gulong bpm,tpb;
    
    g_object_get(G_OBJECT(self->priv->song),"song-info",&song_info,NULL);
    // @todo handle stpb later (subtick per beat)
    g_object_get(song_info,"bpm",&bpm,"tpb",&tpb,NULL);
    gst_tempo_change_tempo(GST_TEMPO(self->priv->machines[PART_MACHINE]),(glong)bpm,(glong)tpb,-1);
    
    g_signal_connect(G_OBJECT(song_info),"notify::bpm",G_CALLBACK(bt_machine_on_bpm_changed),(gpointer)self);
    g_signal_connect(G_OBJECT(song_info),"notify::tpb",G_CALLBACK(bt_machine_on_tpb_changed),(gpointer)self);
    g_object_unref(song_info);
  }

  g_object_get(G_OBJECT(self->priv->song),"bin",&self->priv->bin,NULL);
  gst_bin_add(self->priv->bin,self->priv->machines[PART_MACHINE]);
  GST_INFO("  added machine to bin, obj->ref_count=%d",G_OBJECT(self->priv->machines[PART_MACHINE])->ref_count);
  g_assert(self->priv->machines[PART_MACHINE]!=NULL);
  g_assert(self->src_elem!=NULL);
  g_assert(self->dst_elem!=NULL);
  if(!(self->priv->global_params+self->priv->voice_params)) {
    GST_WARNING("  machine has no params");
  }

  if(BT_IS_SINK_MACHINE(self)) {
    GST_DEBUG("  this will be the master for the song");
    g_object_set(G_OBJECT(self->priv->song),"master",G_OBJECT(self),NULL);
  }
  // prepare internal patterns for the machine
  bt_pattern_new_with_event(self->priv->song,self,BT_PATTERN_CMD_BREAK);
  bt_pattern_new_with_event(self->priv->song,self,BT_PATTERN_CMD_MUTE);
  if(BT_IS_SOURCE_MACHINE(self)) {
    bt_pattern_new_with_event(self->priv->song,self,BT_PATTERN_CMD_SOLO);
  }
  else if(BT_IS_PROCESSOR_MACHINE(self)) {
    bt_pattern_new_with_event(self->priv->song,self,BT_PATTERN_CMD_BYPASS);
  }
  // add the machine to the setup of the song
  // @todo the method should get the setup as an parameter (faster when bulk adding) (store it in the class?)
  g_object_get(G_OBJECT(self->priv->song),"setup",&setup,NULL);
  g_assert(setup!=NULL);
  bt_setup_add_machine(setup,self);
  g_object_unref(setup);

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
  gchar *name;
  
  g_assert(BT_IS_MACHINE(self));
  g_assert(!BT_IS_SOURCE_MACHINE(self));
  
  GST_INFO(" for machine '%s'",self->priv->id);
  
  // add input-level analyser
  name=g_strdup_printf("input_level_%p",self);
  if(!(self->priv->machines[PART_INPUT_LEVEL]=gst_element_factory_make("level",name))) {
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
  g_free(name);
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
  gchar *name;
  
  g_assert(BT_IS_MACHINE(self));
  g_assert(!BT_IS_SOURCE_MACHINE(self));

  GST_INFO(" for machine '%s'",self->priv->id);

  // add input-gain element
  name=g_strdup_printf("input_gain_%p",self);
  if(!(self->priv->machines[PART_INPUT_GAIN]=gst_element_factory_make("volume",name))) {
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
  g_free(name);
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
    gchar *name;
    // create the adder
    name=g_strdup_printf("adder_%p",self);
    self->priv->machines[PART_ADDER]=gst_element_factory_make("adder",name);
    g_assert(self->priv->machines[PART_ADDER]!=NULL);
    g_free(name);
    gst_bin_add(self->priv->bin, self->priv->machines[PART_ADDER]);
    // adder not links directly to some elements
    name=g_strdup_printf("audioconvert_%p",self);
    self->priv->machines[PART_ADDER_CONVERT]=gst_element_factory_make("audioconvert",name);
    g_assert(self->priv->machines[PART_ADDER_CONVERT]!=NULL);
    g_free(name);
    gst_bin_add(self->priv->bin, self->priv->machines[PART_ADDER_CONVERT]);
    GST_DEBUG("  about to link adder -> convert -> dst_elem");
    if(!gst_element_link_many(self->priv->machines[PART_ADDER], self->priv->machines[PART_ADDER_CONVERT], self->dst_elem, NULL)) {
      gst_element_unlink_many(self->priv->machines[PART_ADDER], self->priv->machines[PART_ADDER_CONVERT], self->dst_elem, NULL);
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
    gchar *name;
    // create th spreader (tee)
    name=g_strdup_printf("tee_%p",self);
    self->priv->machines[PART_SPREADER]=gst_element_factory_make("tee",name);
    g_assert(self->priv->machines[PART_SPREADER]!=NULL);
    g_free(name);
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

// pattern handling

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
    // @todo do we need to manually invoke bt_machine_on_pattern_changed() once?
    g_signal_emit(G_OBJECT(self),signals[PATTERN_ADDED_EVENT], 0, pattern);
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  else {
    GST_WARNING("trying to add pattern again"); 
  }
}

/**
 * bt_machine_remove_pattern:
 * @self: the machine to remove the pattern from
 * @pattern: the existing pattern instance
 *
 * Remove the given pattern from the machine.
 */
void bt_machine_remove_pattern(const BtMachine *self, const BtPattern *pattern) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(BT_IS_PATTERN(pattern));

  if(g_list_find(self->priv->patterns,pattern)) {
    self->priv->patterns=g_list_remove(self->priv->patterns,pattern);
    g_signal_emit(G_OBJECT(self),signals[PATTERN_REMOVED_EVENT], 0, pattern);
    // @todo update the controller
    g_object_unref(G_OBJECT(pattern));
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  else {
    GST_WARNING("trying to remove pattern that is not in machine");
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
  
  //GST_DEBUG("pattern-list.length=%d",g_list_length(self->priv->patterns));
  
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
 * bt_machine_get_unique_pattern_name:
 * @self: the machine for which the name should be unique
 *
 * The function generates a unique pattern name for this machine by eventually
 * adding a number postfix. This method should be used when adding new patterns.
 *
 * Returns: the newly allocated unique name
 */
gchar *bt_machine_get_unique_pattern_name(const BtMachine *self) {
  BtPattern *pattern=NULL;
  gchar *id,*ptr;
  guint8 i=0;
  
  id=g_strdup_printf("%s 00",self->priv->id);
  ptr=&id[strlen(self->priv->id)+1];
  do {
    (void)g_sprintf(ptr,"%02u",i++);
    g_object_try_unref(pattern);
  } while((pattern=bt_machine_get_pattern_by_id(self,id)) && (i<100));
  g_object_try_unref(pattern);
  g_free(id);
  i--;
  
  return(g_strdup_printf("%02u",i));
}

// global and voice param handling

/**
 * bt_machine_is_polyphonic:
 * @self: the machine to check
 *
 * Tells if the machine can produce (multiple) voices. Monophonic machines have
 * their (one) voice params as part of the global params.
 *
 * Returns: %TRUE for polyphic machines, %FALSE for monophonic ones
 */
gboolean bt_machine_is_polyphonic(const BtMachine *self) {
  g_assert(BT_IS_MACHINE(self));
  
#ifdef HAVE_GST_POLYVOICE_POLY_VOICE_H
  GST_DEBUG(" is machine \"%s\" poly ? %d",self->priv->id,GST_IS_PARENT(self->priv->machines[PART_MACHINE]));
  
  return(GST_IS_PARENT(self->priv->machines[PART_MACHINE]));
#else
  return(FALSE);
#endif
}

/**
 * bt_machine_get_global_param_index:
 * @self: the machine to search for the global param
 * @name: the name of the global param
 * @error: the location of an error instance to fill with a message, if an error occures
 *
 * Searches the list of registered param of a machine for a global param of
 * the given name and returns the index if found.
 *
 * Returns: the index or sets error if it is not found and returns -1.
 */
glong bt_machine_get_global_param_index(const BtMachine *self, const gchar *name, GError **error) {
  glong ret=-1,i;
  gboolean found=FALSE;

  g_assert(BT_IS_MACHINE(self));
  g_assert(name);
  g_return_val_if_fail(error == NULL || *error == NULL, -1);
  
  for(i=0;i<self->priv->global_params;i++) {
    if(!strcmp(self->priv->global_names[i],name)) {
      ret=i;
      found=TRUE;
      break;
    }
  }  
  if(!found && error) {
    g_set_error (error, error_domain, /* errorcode= */0,
                "global param for name %s not found", name);
  }
  g_assert((found || (error && *error)));
  return(ret);
}

/**
 * bt_machine_get_voice_param_index:
 * @self: the machine to search for the voice param
 * @name: the name of the voice param
 * @error: the location of an error instance to fill with a message, if an error occures
 *
 * Searches the list of registered param of a machine for a voice param of
 * the given name and returns the index if found.
 *
 * Returns: the index or sets error if it is not found and returns -1.
 */
glong bt_machine_get_voice_param_index(const BtMachine *self, const gchar *name, GError **error) {
  gulong ret=-1,i;
  gboolean found=FALSE;

  g_assert(BT_IS_MACHINE(self));
  g_assert(name);
  g_return_val_if_fail(error == NULL || *error == NULL, -1);
 
  for(i=0;i<self->priv->voice_params;i++) {
    if(!strcmp(self->priv->voice_names[i],name)) {
      ret=i;
      found=TRUE;
      break;
    }
  }  
  if(!found && error) {
    g_set_error (error, error_domain, /* errorcode= */0,
                "voice param for name %s not found", name);
  }
  g_assert((found || (error && *error)));
  return(ret);
}


/**
 * bt_machine_get_voice_dparam:
 * @self: the machine to search for the voice param
 * @voice: the voice index
 * @index: the offset in the list of voice params
 *
 * Retrieves the voice GstDParam
 *
 * Returns: the requested voice GstDParam
 */

/**
 * bt_machine_get_global_param_spec:
 * @self: the machine to search for the global param
 * @index: the offset in the list of global params
 *
 * Retrieves the parameter specification for the global param
 *
 * Returns: the #GParamSpec for the requested global param
 */
GParamSpec *bt_machine_get_global_param_spec(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->global_params);
  
  return(g_object_class_find_property(
    G_OBJECT_CLASS(BT_MACHINE_GET_CLASS(self->priv->machines[PART_MACHINE])),
    self->priv->global_names[index])
  );
}

/**
 * bt_machine_get_voice_param_spec:
 * @self: the machine to search for the voice param
 * @index: the offset in the list of voice params
 *
 * Retrieves the parameter specification for the voice param
 *
 * Returns: the #GParamSpec for the requested voice param
 */
GParamSpec *bt_machine_get_voice_param_spec(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->voice_params);
  
  return(g_object_class_find_property(
    G_OBJECT_CLASS(BT_MACHINE_GET_CLASS(self->priv->machines[PART_MACHINE])),
    self->priv->voice_names[index])
  );
}

/**
 * bt_machine_get_global_param_type:
 * @self: the machine to search for the global param type
 * @index: the offset in the list of global params
 *
 * Retrieves the GType of a global param 
 *
 * Returns: the requested GType
 */
GType bt_machine_get_global_param_type(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->global_params);
  g_assert(self->priv->global_types);
  
  return(self->priv->global_types[index]);
}

/**
 * bt_machine_get_voice_param_type:
 * @self: the machine to search for the voice param type
 * @index: the offset in the list of voice params
 *
 * Retrieves the GType of a voice param 
 *
 * Returns: the requested GType
 */
GType bt_machine_get_voice_param_type(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->voice_params);

  return(self->priv->voice_types[index]);
}

/**
 * bt_machine_set_global_param_value:
 * @self: the machine to set the global param value
 * @index: the offset in the list of global params
 * @event: the new value
 *
 * Sets a the specified global param to the give data value.
 */
void bt_machine_set_global_param_value(const BtMachine *self, gulong index, GValue *event) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(G_IS_VALUE(event));
  g_assert(index<self->priv->global_params);
  
  GST_DEBUG("set value for %s.%s",self->priv->id,self->priv->global_names[index]);
  g_object_set_property(G_OBJECT(self->priv->machines[PART_MACHINE]),self->priv->global_names[index],event);
}

/**
 * bt_machine_set_voice_param_value:
 * @self: the machine to set the voice param value
 * @voice: the voice to change
 * @index: the offset in the list of voice params
 * @event: the new value
 *
 * Sets a the specified voice param to the give data value.
 */
void bt_machine_set_voice_param_value(const BtMachine *self, gulong voice, gulong index, GValue *event) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(G_IS_VALUE(event));
  g_assert(voice<self->priv->voices);
  g_assert(index<self->priv->voice_params);

  g_object_set_property(G_OBJECT(gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),voice)),self->priv->voice_names[index],event);
}

/**
 * bt_machine_set_global_param_no_value:
 * @self: the machine to set the global param value to the no-value
 * @index: the offset in the list of global params
 *
 * Sets a the specified global param to the neutral no-value.
 */
void bt_machine_set_global_param_no_value(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->global_params);

  if(!(self->priv->global_flags[index]&0x02)) {  /* MPF_STATE */
    bt_machine_set_global_param_value(self,index,&self->priv->global_no_val[index]);
  }
}

/**
 * bt_machine_set_voice_param_no_value:
 * @self: the machine to set the voice param value to the no-value
 * @voice: the voice to change
 * @index: the offset in the list of voice params
 *
 * Sets a the specified voice param to the neutral no-value.
 */
void bt_machine_set_voice_param_no_value(const BtMachine *self, gulong voice, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(voice<self->priv->voices);
  g_assert(index<self->priv->global_params);

  if(!(self->priv->voice_flags[index]&0x02)) {  /* MPF_STATE */
    bt_machine_set_voice_param_value(self,voice,index,&self->priv->voice_no_val[index]);
  }
}

/**
 * bt_machine_get_global_param_name:
 * @self: the machine to get the param name from 
 * @index: the offset in the list of global params
 *
 * Gets the global param name. Do not modify returned content.
 *
 * Returns: the requested name
 */
const gchar *bt_machine_get_global_param_name(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->global_params);
  
  return(self->priv->global_names[index]);
}

/**
 * bt_machine_get_voice_param_name:
 * @self: the machine to get the param name from 
 * @index: the offset in the list of voice params
 *
 * Gets the voice param name. Do not modify returned content.
 *
 * Returns: the requested name
 */
const gchar *bt_machine_get_voice_param_name(const BtMachine *self, gulong index) {
  g_assert(BT_IS_MACHINE(self));
  g_assert(index<self->priv->voice_params);
  
  return(self->priv->voice_names[index]);
}

static GValue *bt_machine_get_param_min_value(const BtMachine *self, GParamSpec *property) {
  GValue *value=g_new0(GValue,1);

  if(GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE])) {
    bt_machine_get_property_meta_value(value,property,gst_property_meta_quark_min_val);
  }
  else {
    g_value_init(value,property->value_type);
    switch(property->value_type) {
      case G_TYPE_BOOLEAN:
        g_value_set_boolean(value,0);
      break;
      case G_TYPE_INT: {
        GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
        g_value_set_int(value,int_property->minimum);
      }  break;
      case G_TYPE_UINT: {
        GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
        g_value_set_uint(value,uint_property->minimum);
      }  break;
      default:
        GST_ERROR("unsupported GType=%d:'%s'",property->value_type,G_VALUE_TYPE_NAME(property->value_type));
    }
  }
  return(value);
}

/**
 * bt_machine_get_global_param_min_value:
 * @self: the machine to get the min param value from 
 * @index: the offset in the list of global params
 *
 * Gets the minimum value of a global param.
 *
 * Returns: the the minimum value as a new GValue
 */
GValue *bt_machine_get_global_param_min_value(const BtMachine *self, gulong index) {
  GParamSpec *property=bt_machine_get_global_param_spec(self,index);
  
  return(bt_machine_get_param_min_value(self,property));
}

/**
 * bt_machine_get_voice_param_min_value:
 * @self: the machine to get the min param value from 
 * @index: the offset in the list of voice params
 *
 * Gets the minimum value of a voice param.
 *
 * Returns: the the minimum value as a new GValue
 */
GValue *bt_machine_get_voice_param_min_value(const BtMachine *self, gulong index) {
  GParamSpec *property=bt_machine_get_voice_param_spec(self,index);

  return(bt_machine_get_param_min_value(self,property));
}

static GValue *bt_machine_get_param_max_value(const BtMachine *self, GParamSpec *property) {
  GValue *value=g_new0(GValue,1);

  if(GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE])) {
    bt_machine_get_property_meta_value(value,property,gst_property_meta_quark_max_val);
  }
  else {
    g_value_init(value,property->value_type);
    switch(property->value_type) {
      case G_TYPE_BOOLEAN:
        g_value_set_boolean(value,1);
      break;
      case G_TYPE_INT: {
        GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
        g_value_set_int(value,int_property->maximum);
      }  break;
      case G_TYPE_UINT: {
        GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
        g_value_set_uint(value,uint_property->maximum);
      }  break;
      default:
        GST_ERROR("unsupported GType=%d:'%s'",property->value_type,G_VALUE_TYPE_NAME(property->value_type));
    }
  }
  return(value);
}

/**
 * bt_machine_get_global_param_max_value:
 * @self: the machine to get the max param value from 
 * @index: the offset in the list of global params
 *
 * Gets the maximum value of a global param.
 *
 * Returns: the the maximum value as a new GValue
 */
GValue *bt_machine_get_global_param_max_value(const BtMachine *self, gulong index) {
  GParamSpec *property=bt_machine_get_global_param_spec(self,index);
  
  return(bt_machine_get_param_max_value(self,property));
}

/**
 * bt_machine_get_voice_param_max_value:
 * @self: the machine to get the max param value from 
 * @index: the offset in the list of voice params
 *
 * Gets the maximum value of a voice param.
 *
 * Returns: the the maximum value as a new GValue
 */
GValue *bt_machine_get_voice_param_max_value(const BtMachine *self, gulong index) {
  GParamSpec *property=bt_machine_get_voice_param_spec(self,index);

  return(bt_machine_get_param_max_value(self,property));
}

/**
 * bt_machine_is_global_param_trigger:
 * @self: the machine to check params from
 * @index: the offset in the list of global params
 *
 * Tests if the global param is a trigger param
 * (like a key-note or a drum trigger).
 *
 * Returns: %TRUE if it is a trigger
 */
gboolean bt_machine_is_global_param_trigger(const BtMachine *self, gulong index) {
  if(!(self->priv->global_flags[index]&0x02)) return(TRUE);
  return(FALSE);
}

/**
 * bt_machine_is_voice_param_trigger:
 * @self: the machine to check params from
 * @index: the offset in the list of voice params
 *
 * Tests if the voice param is a trigger param
 * (like a key-note or a drum trigger).
 *
 * Returns: %TRUE if it is a trigger
 */
gboolean bt_machine_is_voice_param_trigger(const BtMachine *self, gulong index) {
  if(!(self->priv->voice_flags[index]&0x02)) return(TRUE);
  return(FALSE);
}

/**
 * bt_machine_describe_global_param_value:
 * @self: the machine to get a param description from
 * @index: the offset in the list of global params
 * @event: the value to describe
 *
 * Described a param value in human readable form. The type of the given @value
 * must match the type of the paramspec of the param referenced by @index.
 *
 * Returns: the description as newly allocated string
 */
gchar *bt_machine_describe_global_param_value(const BtMachine *self, gulong index, GValue *event) {
  gchar *str=NULL;
  GstElement *machine=self->priv->machines[PART_MACHINE];

  if(GST_IS_PROPERTY_META(machine)) {
    str=gst_property_meta_describe_property(GST_PROPERTY_META(machine),index,event);
  }
  return(str);
}

/**
 * bt_machine_describe_voice_param_value:
 * @self: the machine to get a param description from
 * @index: the offset in the list of voice params
 * @event: the value to describe
 *
 * Described a param value in human readable form. The type of the given @value
 * must match the type of the paramspec of the param referenced by @index.
 *
 * Returns: the description as newly allocated string
 */
gchar *bt_machine_describe_voice_param_value(const BtMachine *self, gulong index, GValue *event) {
  gchar *str=NULL;
  GstElement *machine=self->priv->machines[PART_MACHINE];
  
  if(GST_IS_CHILD_PROXY(machine)) {
    GstObject *voice_child;

    // get child for voice 0
    if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),0))) {
      if(GST_IS_PROPERTY_META(voice_child)) {
        str=gst_property_meta_describe_property(GST_PROPERTY_META(voice_child),index,event);
      }
    }
  }
  return(str);
}

//-- controller handling

/**
 * bt_machine_global_controller_change_value:
 * @self: the machine to change the param for
 * @param: the global parameter index
 * @timestamp: the time stamp of the change
 * @value: the new value or %NULL to unset a previous one
 *
 * Depending on wheter the given value is NULL, sets or unsets the controller
 * value for the specified param and at the given time.
 */
void bt_machine_global_controller_change_value(const BtMachine *self,gulong param,GstClockTime timestamp,GValue *value) {
  if(value) {
    gst_controller_set(self->priv->global_controller,self->priv->global_names[param],timestamp,value);
  }
  else {
    gst_controller_unset(self->priv->global_controller,self->priv->global_names[param],timestamp);
  }
}

/**
 * bt_machine_voice_controller_change_value:
 * @self: the machine to change the param for
 * @param: the voice parameter index
 * @voice: the voice number
 * @timestamp: the time stamp of the change
 * @value: the new value or %NULL to unset a previous one
 *
 * Depending on wheter the given value is NULL, sets or unsets the controller
 * value for the specified param and at the given time.
 */
void bt_machine_voice_controller_change_value(const BtMachine *self,gulong param,gulong voice,GstClockTime timestamp,GValue *value) {
  if(value) {
    gst_controller_set(self->priv->voice_controllers[voice],self->priv->voice_names[param],timestamp,value);
  }
  else {
    gst_controller_unset(self->priv->voice_controllers[voice],self->priv->voice_names[param],timestamp);
  }
}

//-- debug helper

void bt_machine_dbg_print_parts(const BtMachine *self) {
  /* [A AC IL IG M OL OG S] */
  GST_DEBUG("%s [%c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c %c%s%c]",
    self->priv->id,
  
    self->priv->machines[PART_ADDER]==self->dst_elem?'<':' ',
    self->priv->machines[PART_ADDER]?"A":"a",
    self->priv->machines[PART_ADDER]==self->src_elem?'>':' ',
  
    self->priv->machines[PART_ADDER_CONVERT]==self->dst_elem?'<':' ',
    self->priv->machines[PART_ADDER_CONVERT]?"AC":"ac",
    self->priv->machines[PART_ADDER_CONVERT]==self->src_elem?'>':' ',
  
    self->priv->machines[PART_INPUT_LEVEL]==self->dst_elem?'<':' ',
    self->priv->machines[PART_INPUT_LEVEL]?"IL":"il",
    self->priv->machines[PART_INPUT_LEVEL]==self->src_elem?'>':' ',
    
    self->priv->machines[PART_INPUT_GAIN]==self->dst_elem?'<':' ',
    self->priv->machines[PART_INPUT_GAIN]?"IG":"ig",
    self->priv->machines[PART_INPUT_GAIN]==self->src_elem?'>':' ',
  
    self->priv->machines[PART_MACHINE]==self->dst_elem?'<':' ',
    self->priv->machines[PART_MACHINE]?"M":"m",
    self->priv->machines[PART_MACHINE]==self->src_elem?'>':' ',
  
    self->priv->machines[PART_OUTPUT_LEVEL]==self->dst_elem?'<':' ',
    self->priv->machines[PART_OUTPUT_LEVEL]?"OL":"ol",
    self->priv->machines[PART_OUTPUT_LEVEL]==self->src_elem?'>':' ',
  
    self->priv->machines[PART_OUTPUT_GAIN]==self->dst_elem?'<':' ',
    self->priv->machines[PART_OUTPUT_GAIN]?"OG":"og",
    self->priv->machines[PART_OUTPUT_GAIN]==self->src_elem?'>':' ',
  
    self->priv->machines[PART_SPREADER]==self->dst_elem?'<':' ',
    self->priv->machines[PART_SPREADER]?"S":"s",
    self->priv->machines[PART_SPREADER]==self->src_elem?'>':' '
  );
}

void bt_machine_dbg_dump_global_controller_queue(const BtMachine *self) {
  gulong i;
  FILE *file;
  gchar *name;
  GList *list,*node;
  GstTimedValue *tv;
  
  for(i=0;i<self->priv->global_params;i++) {
    name=g_strdup_printf("/tmp/buzztard-%s_g%02lu.dat",self->priv->id,i);
    if((file=fopen(name,"wb"))) {
      fprintf(file,"# global param \"%s\" for machine \"%s\"\n",self->priv->global_names[i],self->priv->id);
      if((list=(GList *)gst_controller_get_all(self->priv->global_controller,self->priv->global_names[i]))) {
        for(node=list;node;node=g_list_next(node)) {
          tv=(GstTimedValue *)node->data;
          fprintf(file,"%"GST_TIME_FORMAT" %"G_GUINT64_FORMAT" ",GST_TIME_ARGS(tv->timestamp),tv->timestamp);
          switch(G_VALUE_TYPE(&tv->value)) {
            case G_TYPE_INT: fprintf(file,"%d\n",g_value_get_int(&tv->value));break;
            case G_TYPE_UINT: fprintf(file,"%u\n",g_value_get_uint(&tv->value));break;
            case G_TYPE_LONG: fprintf(file,"%ld\n",g_value_get_long(&tv->value));break;
            case G_TYPE_ULONG: fprintf(file,"%lu\n",g_value_get_ulong(&tv->value));break;
            case G_TYPE_FLOAT: fprintf(file,"%f\n",g_value_get_float(&tv->value));break;
            case G_TYPE_DOUBLE: fprintf(file,"%lf\n",g_value_get_double(&tv->value));break;
            default: fprintf(file,"0\n");break;
          }
        }
        g_list_free(list);
      }
      fclose(file);
    }
    g_free(name);
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
  gulong voices;
  
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
        gst_element_set_name(self->priv->machines[PART_MACHINE],name);
        g_free(name);
      }
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_free(self->priv->plugin_name);
      self->priv->plugin_name = g_value_dup_string(value);
      GST_DEBUG("set the plugin_name for machine: %s",self->priv->plugin_name);
    } break;
    case MACHINE_VOICES: {
      voices=self->priv->voices;
      self->priv->voices = g_value_get_ulong(value);
      if(voices!=self->priv->voices) {
        GST_DEBUG("set the voices for machine: %d",self->priv->voices);
        bt_machine_resize_voices(self,voices);
        bt_machine_resize_pattern_voices(self);
        bt_song_set_unsaved(self->priv->song,TRUE);
      }
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
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_dispose(GObject *object) {
  BtMachine *self = BT_MACHINE(object);
  BtSongInfo *song_info;
  guint i;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // disconnect notify handlers
  g_object_get(G_OBJECT(self->priv->song),"song-info",&song_info,NULL);
  if(song_info) {
    g_signal_handlers_disconnect_matched(song_info,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_machine_on_bpm_changed,NULL);
    g_signal_handlers_disconnect_matched(song_info,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_machine_on_tpb_changed,NULL);
    g_object_unref(song_info);
  }
  
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

  g_free(self->priv->voice_no_val);
  g_free(self->priv->global_no_val);
  g_free(self->priv->voice_flags);
  g_free(self->priv->global_flags);
  g_free(self->priv->voice_types);
  g_free(self->priv->global_types);
  g_free(self->priv->global_names);
  g_free(self->priv->voice_names);
  g_free(self->priv->voice_controllers);
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
  // default is no voice, only global params
  //self->priv->voices=1;
  self->priv->properties=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
  
  GST_DEBUG("!!!! self=%p",self);
}

static void bt_machine_class_init(BtMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  error_domain=g_quark_from_static_string("BtMachine");
  
  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  
  gobject_class->set_property = bt_machine_set_property;
  gobject_class->get_property = bt_machine_get_property;
  gobject_class->dispose      = bt_machine_dispose;
  gobject_class->finalize     = bt_machine_finalize;

  klass->pattern_added_event = NULL;
  klass->pattern_removed_event = NULL;
  
  /** 
   * BtMachine::pattern-added:
   * @self: the machine object that emitted the signal
   * @pattern: the new pattern
   *
   * A new pattern item has been added to the machine
   */
  signals[PATTERN_ADDED_EVENT] = g_signal_new("pattern-added",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_ABS_STRUCT_OFFSET(BtMachineClass,pattern_added_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__POINTER,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_PATTERN // param data
                                        );
  
  /**
   * BtMachine::pattern-removed:
   * @self: the machine object that emitted the signal
   * @pattern: the old pattern
   *
   * A pattern item has been removed from the machine
   */
  signals[PATTERN_REMOVED_EVENT] = g_signal_new("pattern-removed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_ABS_STRUCT_OFFSET(BtMachineClass,pattern_removed_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__POINTER,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_PATTERN // param data
                                        );

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
                                     "a copy of the list of patterns",
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
    type = g_type_register_static(G_TYPE_OBJECT,"BtMachine",&info,G_TYPE_FLAG_ABSTRACT);

  }
  return type;
}
