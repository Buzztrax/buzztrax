/* $Id: machine.c,v 1.260 2007-07-01 12:51:36 ensonic Exp $
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
 * @todo: try to derive this from GstBin!
 *  then put the machines into itself (and not into the songs bin, but insert the machine directly into the song->bin
 *  when adding internal machines we need to fix the ghost pads (this may require relinking)
 *    gst_element_add_ghost_pad() and gst_element_remove_ghost_pad()
 *
 * @todo: when using the tee, we need queue elements after
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
  /* helper to enforce common format for adder inputs */
  PART_CAPS_FILTER,
  /* helper to make adder link to next element */
  PART_ADDER_CONVERT,
  /* the elements to control and analyse the current input signal */
  PART_INPUT_LEVEL,
  PART_INPUT_GAIN,
  /* the gstreamer element that produces/processes the signal */
  PART_MACHINE,
  /* the elements to control and analyse the current output signal */
  PART_OUTPUT_LEVEL,
  PART_OUTPUT_GAIN,
  /* if next element has more channels, spread signal
  PART_OUTPUT_PANORAMA,
  */
  /* utillity elements to allow multiple outputs */
  PART_SPREADER,
  /* how many elements are used */
  PART_COUNT
} BtMachinePart;

struct _BtMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* (ui) properties accociated with this machine */
  GHashTable *properties;

  /* the song the machine belongs to */
  G_POINTER_ALIAS(BtSong *,song);
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
  guint private_patterns;

  /* the gstreamer elements that are used */
  GstElement *machines[PART_COUNT];

  /* caps filter format */
  gint format; /* 0=int/1=float */
  gint channels;
  gint width;
  gint depth;

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
  if(G_UNLIKELY(type == 0)) {
    static const GEnumValue values[] = {
      { BT_MACHINE_STATE_NORMAL,"BT_MACHINE_STATE_NORMAL","just working" },
      { BT_MACHINE_STATE_MUTE,  "BT_MACHINE_STATE_MUTE",   "be quiet" },
      { BT_MACHINE_STATE_SOLO,  "BT_MACHINE_STATE_SOLO",  "be the only one playing" },
      { BT_MACHINE_STATE_BYPASS,"BT_MACHINE_STATE_BYPASS","be uneffective (pass through)" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtMachineState", values);
  }
  return type;
}

//-- signal handler

void bt_machine_on_bpm_changed(BtSongInfo * const song_info, const GParamSpec * const arg, gconstpointer const user_data) {
  const BtMachine * const self=BT_MACHINE(user_data);
  gulong bpm;

  g_object_get(song_info,"bpm",&bpm,NULL);
  gst_tempo_change_tempo(GST_TEMPO(self->priv->machines[PART_MACHINE]),(glong)bpm,-1,-1);
}

void bt_machine_on_tpb_changed(BtSongInfo * const song_info, const GParamSpec * const arg, gconstpointer const user_data) {
  const BtMachine * const self=BT_MACHINE(user_data);
  gulong tpb;

  g_object_get(song_info,"tpb",&tpb,NULL);
  gst_tempo_change_tempo(GST_TEMPO(self->priv->machines[PART_MACHINE]),-1,(glong)tpb,-1);
}

//-- helper methods

static GstElement *bt_machine_get_peer(GstElement * const elem, GstIterator *it) {
  GstElement *peer=NULL;
  gboolean done=FALSE;
  gpointer item;
  GstPad *pad,*peer_pad;

  if(!it) return(NULL);

  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        pad=GST_PAD(item);
        if((peer_pad=gst_pad_get_peer(pad))) {
          peer=GST_ELEMENT(gst_object_get_parent(GST_OBJECT(peer_pad)));
          gst_object_unref(peer_pad);
          done=TRUE;
        }
        gst_object_unref(pad);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync(it);
        break;
      case GST_ITERATOR_ERROR:
        done=TRUE;
        break;
      case GST_ITERATOR_DONE:
        done=TRUE;
        break;
    }
  }
  gst_iterator_free(it);
  return(peer);
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
  return(bt_machine_get_peer(elem,gst_element_iterate_sink_pads(elem)));
}

/*
 * bt_machine_get_source_peer:
 * @elem: the element to locate the source peer for
 *
 * Locates the #GstElement that is connected to the source pad of the given
 * #GstElement.
 *
 * Returns: the source peer #GstElement or NULL
 */
static GstElement *bt_machine_get_source_peer(GstElement * const elem) {
  return(bt_machine_get_peer(elem,gst_element_iterate_src_pads(elem)));
}

/*
 * mute the machine output
 */
static gboolean bt_machine_set_mute(const BtMachine * const self,const BtSetup * const setup) {
  const BtMachinePart part=BT_IS_SINK_MACHINE(self)?PART_INPUT_GAIN:PART_OUTPUT_GAIN;

  if(self->priv->state==BT_MACHINE_STATE_MUTE) return(TRUE);

  if(self->priv->machines[part]) {
    g_object_set(self->priv->machines[part],"mute",TRUE,NULL);
    return(TRUE);
  }
  return(FALSE);
}

/*
 * unmute the machine output
 */
static gboolean bt_machine_unset_mute(const BtMachine *const self, const BtSetup * const setup) {
  const BtMachinePart part=BT_IS_SINK_MACHINE(self)?PART_INPUT_GAIN:PART_OUTPUT_GAIN;

  if(self->priv->state!=BT_MACHINE_STATE_MUTE) return(TRUE);

  if(self->priv->machines[part]) {
    g_object_set(self->priv->machines[part],"mute",FALSE,NULL);
    return(TRUE);
  }
  return(FALSE);
}

/*
 * bt_machine_change_state:
 *
 * Reset old state and go to new state. Do sanity checking of allowed states for
 * given machine.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_machine_change_state(const BtMachine * const self, const BtMachineState new_state) {
  gboolean res=TRUE;
  BtSetup *setup;

  // reject a few nonsense changes
  if((new_state==BT_MACHINE_STATE_BYPASS) && (!BT_IS_PROCESSOR_MACHINE(self))) return(FALSE);
  if((new_state==BT_MACHINE_STATE_SOLO) && (BT_IS_SINK_MACHINE(self))) return(FALSE);
  if(self->priv->state==new_state) return(TRUE);

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
      GST_INFO("unmuted %d elements with result = %d",g_list_length(machines),res);
      g_list_free(machines);
    } break;
    case BT_MACHINE_STATE_BYPASS:  { // processor
      const GstElement * const element=self->priv->machines[PART_MACHINE];
      if(GST_IS_BASE_TRANSFORM(element)) {
        gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element),FALSE);
      }
      else {
        // @todo: disconnect its source and sink + set this machine to playing
        GST_INFO("element does not support passthrough");
      }
    } break;
    case BT_MACHINE_STATE_NORMAL:
      //g_return_val_if_reached(FALSE);
      break;
  }
  // set to new state
  switch(new_state) {
    case BT_MACHINE_STATE_MUTE:  { // source, processor, sink
      if(!bt_machine_set_mute(self,setup)) res=FALSE;
    } break;
    case BT_MACHINE_STATE_SOLO:  { // source
      GList *node,*machines=bt_setup_get_machines_by_type(setup,BT_TYPE_SOURCE_MACHINE);
      BtMachine *machine;
      // set all but this machine to paused
      for(node=machines;node;node=g_list_next(node)) {
        machine=BT_MACHINE(node->data);
        if(machine!=self) {
          // if a different machine is solo, set it to normal and unmute the current source
          if(machine->priv->state==BT_MACHINE_STATE_SOLO) {
            machine->priv->state=BT_MACHINE_STATE_NORMAL;
            g_object_notify(G_OBJECT(machine),"state");
            if(!bt_machine_unset_mute(self,setup)) res=FALSE;
          }
          if(!bt_machine_set_mute(machine,setup)) res=FALSE;
        }
        g_object_unref(machine);
      }
      GST_INFO("muted %d elements with result = %d",g_list_length(machines),res);
      g_list_free(machines);
    } break;
    case BT_MACHINE_STATE_BYPASS:  { // processor
      const GstElement *element=self->priv->machines[PART_MACHINE];
      if(GST_IS_BASE_TRANSFORM(element)) {
        gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element),TRUE);
      }
      else {
        // @todo set this machine to paused + connect its source and sink
      }
    } break;
    case BT_MACHINE_STATE_NORMAL:
      //g_return_val_if_reached(FALSE);
      break;
  }
  self->priv->state=new_state;

  g_object_try_unref(setup);
  return(res);
}

/*
 * bt_machine_insert_element:
 * @self: the machine for which the element should be inserted
 * @peer: the peer element
 * @part_position: the element at this position should be inserted (activated)
 *
 * Searches surrounding elements of the new element for active peers
 * and connects them. The new elemnt needs to be created before calling this method.
 *
 * Returns: %TRUE for sucess
 */
static gboolean bt_machine_insert_element(BtMachine *const self, GstElement * const peer, const BtMachinePart part_position) {
  gboolean res=FALSE;
  gint i,pre,post;
  BtSetup *setup;
  BtWire *wire;

  // look for elements before and after part_position
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
      // relink previous connection
      gst_element_link(self->priv->machines[pre],self->priv->machines[post]);
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
 * bt_machine_resize_pattern_voices:
 * @self: the machine which has changed its number of voices
 *
 * Iterates over the machines patterns and adjust their voices too.
 */
static void bt_machine_resize_pattern_voices(const BtMachine * const self) {
  GList* node;

  // reallocate self->priv->patterns->priv->data
  for(node=self->priv->patterns;node;node=g_list_next(node)) {
    g_object_set(BT_PATTERN(node->data),"voices",self->priv->voices,NULL);
  }
}

/*
 * bt_machine_activate_controller:
 * @param_parent: the object to attach the controller to
 * @param_name: which parameter should be controlled
 * @is_trigger: is it a trigger parameter
 *
 * Create the controller object and set interpolation mode.
 *
 * Returns: the controller object if the parameter could be attached.
 */
static GstController *bt_machine_activate_controller(GObject *param_parent,gchar *param_name,gboolean is_trigger) {
  GstController *ctrl;

  GST_INFO("activate controller for %s:%s", GST_IS_OBJECT(param_parent)?GST_OBJECT_NAME(param_parent):"-",param_name);

  if((ctrl=gst_object_control_properties(param_parent, param_name, NULL))) {
#ifdef HAVE_GST_CONTROLLER_NEW
    // @TODO: use new API for the control-source
/*
    GstInterpolationControlSource *cs;

    cs = gst_interpolation_control_source_new ();
    gst_controller_set_control_source (ctrl, param_name, GST_CONTROL_SOURCE (cs));
    gst_interpolation_control_source_set_interpolation_mode (cs,is_trigger?GST_INTERPOLATE_TRIGGER:GST_INTERPOLATE_NONE);
*/
#endif
    // set interpolation mode depending on param type
    gst_controller_set_interpolation_mode(ctrl,param_name,is_trigger?GST_INTERPOLATE_TRIGGER:GST_INTERPOLATE_NONE);
  }
  return(ctrl);
}

/*
 * bt_machine_deactivate_controller:
 * @param_parent: the object to dettach the controller from
 * @param_name: which parameter should be uncontrolled
 *
 * Create the controller object and set interpolation mode.
 */
static void bt_machine_deactivate_controller(GObject *param_parent,gchar *param_name) {
  GST_INFO("deactivate controller for %s:%s", GST_IS_OBJECT(param_parent)?GST_OBJECT_NAME(param_parent):"-",param_name);

  gst_object_uncontrol_properties(param_parent, param_name, NULL);
}


/*
 * bt_machine_resize_voices:
 * @self: the machine which has changed its number of voices
 *
 * Adjust the private data structure after a change in the number of voices.
 */
static void bt_machine_resize_voices(const BtMachine * const self, const gulong voices) {
  GST_INFO("changing machine %s:%p voices from %d to %d",self->priv->id,self->priv->machines[PART_MACHINE],voices,self->priv->voices);


  // @todo GST_IS_CHILD_BIN <-> GST_IS_CHILD_PROXY (sink-bin is a CHILD_PROXY but not a CHILD_BIN)
  if((!self->priv->machines[PART_MACHINE]) || (!GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE]))) {
    GST_WARNING("machine %s:%p is NULL or not polyphonic!",self->priv->id,self->priv->machines[PART_MACHINE]);
    return;
  }

  g_object_set(self->priv->machines[PART_MACHINE],"children",self->priv->voices,NULL);

  if(voices>self->priv->voices) {
    gulong j;

    // release params for old voices
    for(j=self->priv->voices;j<voices;j++) {
      g_object_try_unref(self->priv->voice_controllers[j]);
    }
  }

  // @todo make it use g_renew0()
  // this is not as easy as it sounds (realloc does not know how big the old mem was)
  self->priv->voice_controllers=(GstController **)g_renew(gpointer, self->priv->voice_controllers ,self->priv->voices);
  if(voices<self->priv->voices) {
    guint j;

    for(j=voices;j<self->priv->voices;j++) {
      self->priv->voice_controllers[j]=NULL;
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
 *
 * Returns: %TRUE if it could get the value
 */
static gboolean bt_machine_get_property_meta_value(GValue * const value, GParamSpec * const property, const GQuark key) {
  gboolean res=TRUE;
  gconstpointer const qdata=g_param_spec_get_qdata(property,key);
  GType base_type=bt_g_type_get_base_type(property->value_type);

  // can it be that qdata is NULL if the value is NULL
  //if(!qdata) {
  //  GST_WARNING("no property metadata for '%s'",property->name);
  //  return(FALSE);
  //}

  g_value_init(value,property->value_type);
  switch(base_type) {
    case G_TYPE_BOOLEAN:
      g_value_set_boolean(value,GPOINTER_TO_INT(qdata));
      break;
    case G_TYPE_INT:
      g_value_set_int(value,GPOINTER_TO_INT(qdata));
      break;
    case G_TYPE_UINT:
      g_value_set_uint(value,GPOINTER_TO_UINT(qdata));
      break;
    case G_TYPE_ENUM:
      g_value_set_enum(value,GPOINTER_TO_INT(qdata));
      break;
    case G_TYPE_STRING:
      if(qdata) {
        g_value_set_string(value,qdata);
      }
      else {
        g_value_set_string(value,"");
      }
      break;
    default:
      GST_WARNING("unsupported GType=%d:'%s'",property->value_type,G_VALUE_TYPE_NAME(property->value_type));
      res=FALSE;
  }
  return(res);
}

/*
 * bt_machine_make_internal_element:
 * @self: the machine
 * @part: which internal element to create
 * @factory_name: the element-factories name
 * @element_name: the name of the new #GstElement instance
 *
 * This helper is used by the family of bt_machine_enable_xxx() functions.
 */
static gboolean bt_machine_make_internal_element(const BtMachine * const self,const BtMachinePart part,const gchar * const factory_name,const gchar * const element_name) {
  gboolean res=FALSE;
  gchar *name;

  // add internal element
  name=g_alloca(strlen(element_name)+16);g_sprintf(name,"%s_%p",element_name,self);
  if(!(self->priv->machines[part]=gst_element_factory_make(factory_name,name))) {
    GST_WARNING("failed to create %s",element_name);goto Error;
  }
  gst_bin_add(self->priv->bin,self->priv->machines[part]);
  res=TRUE;
Error:
  return(res);
}

/*
 * bt_machine_add_input_element:
 * @self: the machine
 * @part: which internal element to link
 *
 * This helper is used by the family of bt_machine_enable_input_xxx() functions.
 */
static gboolean bt_machine_add_input_element(BtMachine * const self,const BtMachinePart part) {
  gboolean res=FALSE;
  GstElement *peer;

  // is the machine unconnected towards the input side (its sink)?
  if(!(peer=bt_machine_get_sink_peer(self->priv->machines[PART_MACHINE]))) {
    GST_DEBUG("machine '%s' is not yet connected on the input side",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
    if(!gst_element_link(self->priv->machines[part],self->priv->machines[PART_MACHINE])) {
      // DEBUG
      // bt_machine_dbg_print_parts(self);
      // bt_gst_element_dbg_pads(self->priv->machines[ part]);
      // bt_gst_element_dbg_pads(self->priv->machines[PART_MACHINE]);
      // DEBUG
      GST_ERROR("failed to link the element '%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
    }
    self->dst_elem=self->priv->machines[part];
    GST_INFO("sucessfully prepended element '%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
  }
  else {
    GST_DEBUG("machine '%s' is connected to '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]),GST_OBJECT_NAME(peer));
    if(!bt_machine_insert_element(self,peer,part)) {
      gst_object_unref(peer);
      GST_ERROR("failed to link the element '%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
    }
    gst_object_unref(peer);
    GST_INFO("sucessfully inserted element'%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
  }
  res=TRUE;
Error:
  return(res);
}

/*
 * bt_machine_add_output_element:
 * @self: the machine
 * @part: which internal element to link
 *
 * This helper is used by the family of bt_machine_enable_output_xxx() functions.
 */
static gboolean bt_machine_add_output_element(BtMachine * const self,const BtMachinePart part) {
  gboolean res=FALSE;
  GstElement *peer;

  // is the machine unconnected towards the output side (its source)?
  if(!(peer=bt_machine_get_source_peer(self->priv->machines[PART_MACHINE]))) {
    GST_DEBUG("machine '%s' is not yet connected on the output side",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
    if(!gst_element_link(self->priv->machines[PART_MACHINE],self->priv->machines[part])) {
      // DEBUG
      // bt_machine_dbg_print_parts(self);
      // bt_gst_element_dbg_pads(self->priv->machines[PART_MACHINE]);
      // bt_gst_element_dbg_pads(self->priv->machines[ part]);
      // DEBUG
      GST_ERROR("failed to link the element '%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
    }
    self->src_elem=self->priv->machines[part];
    GST_INFO("sucessfully appended element '%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
  }
  else {
    GST_DEBUG("machine '%s' is connected to '%s'",GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]),GST_OBJECT_NAME(peer));
    if(!bt_machine_insert_element(self,peer,part)) {
      gst_object_unref(peer);
      GST_ERROR("failed to link the element '%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));goto Error;
    }
    gst_object_unref(peer);
    GST_INFO("sucessfully inserted element'%s' for '%s'",GST_OBJECT_NAME(self->priv->machines[part]),GST_OBJECT_NAME(self->priv->machines[PART_MACHINE]));
  }
  res=TRUE;
Error:
  return(res);
}

//-- init helpers

static gboolean bt_machine_init_core_machine(BtMachine * const self) {
  gboolean res=FALSE;

  if(!bt_machine_make_internal_element(self,PART_MACHINE,self->priv->plugin_name,self->priv->id)) goto Error;

  // there is no adder or spreader in use by default
  self->dst_elem=self->src_elem=self->priv->machines[PART_MACHINE];
  GST_INFO("  instantiated machine %p, \"%s\", machine->ref_count=%d",self->priv->machines[PART_MACHINE],self->priv->plugin_name,G_OBJECT(self->priv->machines[PART_MACHINE])->ref_count);

  res=TRUE;
Error:
  return(res);
}

static void bt_machine_init_interfaces(const BtMachine * const self) {
  // initialize child-proxy iface properties
  if(GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE])) {
    if(!self->priv->voices) {
      GST_WARNING("voices==0");
      //g_object_get(self->priv->machines[PART_MACHINE],"children",&self->priv->voices,NULL);
    }
    else {
      g_object_set(self->priv->machines[PART_MACHINE],"children",self->priv->voices,NULL);
    }
    GST_INFO("  child proxy iface initialized");
  }
  // initialize tempo iface properties
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
    GST_INFO("  tempo iface initialized");
  }
  GST_INFO("machine element instantiated and interfaces initialized");
}

/*
 * bt_machine_check_type:
 *
 * Sanity check if the machine is of the right type. It counts the source,
 * sink pads and check with the machine class-type.
 *
 * Returns: %TRUE if type and pads match
 */
static gboolean bt_machine_check_type(const BtMachine * const self) {
  BtMachineClass *klass=BT_MACHINE_GET_CLASS(self);
  GstIterator *it;
  GstPad *pad;
  gulong pad_src_ct=0,pad_sink_ct=0;
  gboolean done;

  if(!klass->check_type) {
    GST_WARNING("no BtMachine::check_type() implemented");
    return(TRUE);
  }

  // get pad counts per type
  it=gst_element_iterate_pads(self->priv->machines[PART_MACHINE]);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (it, (gpointer)&pad)) {
      case GST_ITERATOR_OK:
        switch(gst_pad_get_direction(pad)) {
          case GST_PAD_SRC: pad_src_ct++;break;
          case GST_PAD_SINK: pad_sink_ct++;break;
          default:
            GST_INFO("unhandled pad type discovered");
            break;
        }
        gst_object_unref(pad);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free(it);

  // test pad counts and element type
  if(!((klass->check_type)(self,pad_src_ct,pad_sink_ct))) {
    return(FALSE);
  }
  return(TRUE);
}

static void bt_machine_init_global_params(const BtMachine * const self) {
  GParamSpec **properties;
  guint number_of_properties;

  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self->priv->machines[PART_MACHINE])),&number_of_properties))) {
    GParamSpec *property;
    GParamSpec **child_properties=NULL;
    //GstController *ctrl;
    guint number_of_child_properties;
    guint i,j;
    gboolean skip;

    // check if the elemnt implements the GstChildBin interface
    if(GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE])) {
      GstObject *voice_child;

      g_assert(gst_child_proxy_get_children_count(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE])));
      // get child for voice 0
      if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),0))) {
        child_properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_OBJECT_GET_CLASS(voice_child)),&number_of_child_properties);
        g_object_unref(voice_child);
      }
    }

    // count number of controlable params
    for(i=0;i<number_of_properties;i++) {
      if(properties[i]->flags&GST_PARAM_CONTROLLABLE) {
        // check if this param is also registered as child param, if so skip
        skip=FALSE;
        if(child_properties) {
          for(j=0;j<number_of_child_properties;j++) {
            if(!strcmp(properties[i]->name,child_properties[j]->name)) {
              GST_DEBUG("    skipping global_param [%d] \"%s\"",i,properties[i]->name);
              skip=TRUE;
              properties[i]=NULL;
              break;
            }
          }
        }
        if(!skip) self->priv->global_params++;
      }
    }
    GST_INFO("found %d global params",self->priv->global_params);
    self->priv->global_types =(GType * )g_new0(GType   ,self->priv->global_params);
    self->priv->global_names =(gchar **)g_new0(gpointer,self->priv->global_params);
    self->priv->global_flags =(guint * )g_new0(guint   ,self->priv->global_params);
    self->priv->global_no_val=(GValue *)g_new0(GValue  ,self->priv->global_params);
    for(i=j=0;i<number_of_properties;i++) {
      property=properties[i];
      if(property && property->flags&GST_PARAM_CONTROLLABLE) {
        GST_DEBUG("    adding global_param [%d/%d] \"%s\"",j,self->priv->global_params,property->name);
        // add global param
        self->priv->global_names[j]=property->name;
        self->priv->global_types[j]=property->value_type;
        self->priv->global_flags[j]=GST_PROPERTY_META_STATE;
        if(GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE])) {
          self->priv->global_flags[j]=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
          if(!(bt_machine_get_property_meta_value(&self->priv->global_no_val[j],property,gst_property_meta_quark_no_val))) {
            GST_WARNING("    can't get no-val property-meta for global_param [%d/%d] \"%s\"",j,self->priv->global_params,property->name);
          }
        }
        // bind param to machines global controller (possibly returns ref to existing)
        GST_DEBUG("    added global_param [%d/%d] \"%s\"",j,self->priv->global_params,property->name);
        j++;
      }
    }
    g_free(properties);
    g_free(child_properties);
  }
}

static void bt_machine_init_voice_params(const BtMachine * const self) {
  GParamSpec **properties;
  guint number_of_properties;

  // check if the elemnt implements the GstChildProxy interface
  if(GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE])) {
    GstObject *voice_child;

    // register voice params
    // get child for voice 0
    if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),0))) {
      if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_OBJECT_GET_CLASS(voice_child)),&number_of_properties))) {
        GParamSpec *property;
        guint i,j;

        // count number of controlable params
        for(i=0;i<number_of_properties;i++) {
          if(properties[i]->flags&GST_PARAM_CONTROLLABLE) self->priv->voice_params++;
        }
        GST_INFO("found %d voice params",self->priv->voice_params);
        self->priv->voice_types =(GType * )g_new0(GType   ,self->priv->voice_params);
        self->priv->voice_names =(gchar **)g_new0(gpointer,self->priv->voice_params);
        self->priv->voice_flags =(guint * )g_new0(guint   ,self->priv->voice_params);
        self->priv->voice_no_val=(GValue *)g_new0(GValue  ,self->priv->voice_params);

        for(i=j=0;i<number_of_properties;i++) {
          property=properties[i];
          if(property->flags&GST_PARAM_CONTROLLABLE) {
            GST_DEBUG("    adding voice_param %p, [%d/%d] \"%s\"",property, j,self->priv->voice_params,property->name);
            // add voice param
            self->priv->voice_names[j]=property->name;
            self->priv->voice_types[j]=property->value_type;
            self->priv->voice_flags[j]=GST_PROPERTY_META_STATE;
            if(GST_IS_PROPERTY_META(voice_child)) {
              self->priv->voice_flags[j]=GPOINTER_TO_INT(g_param_spec_get_qdata(property,gst_property_meta_quark_flags));
              if(!(bt_machine_get_property_meta_value(&self->priv->voice_no_val[j],property,gst_property_meta_quark_no_val))) {
                GST_WARNING("    can't get no-val property-meta for voice_param [%d/%d] \"%s\"",j,self->priv->voice_params,property->name);
              }
            }
            GST_DEBUG("    added voice_param [%d/%d] \"%s\"",j,self->priv->voice_params,property->name);
            j++;
          }
        }
      }
      g_free(properties);

      // bind params to machines voice controller
      bt_machine_resize_voices(self,0);
      g_object_unref(voice_child);
    }
    else {
      GST_WARNING("  can't get first voice child!");
    }
  }
  else {
    GST_INFO("  instance is monophonic!");
    self->priv->voices=0;
  }
}

static gboolean bt_machine_setup(BtMachine * const self) {
  BtMachineClass *klass=BT_MACHINE_GET_CLASS(self);
  BtPattern *pattern;

  g_assert(self->priv->song);
  // get the bin from the song, we are in
  g_object_get(G_OBJECT(self->priv->song),"bin",&self->priv->bin,NULL);
  g_assert(self->priv->bin);

  // name the machine and try to instantiate it
  if(!bt_machine_init_core_machine(self)) return(FALSE);

  // initialize iface properties
  bt_machine_init_interfaces(self);
  // we need to make sure the machine is from the right class
  if(!bt_machine_check_type(self)) return(FALSE);

  GST_DEBUG("machine-refs: %d",(G_OBJECT(self))->ref_count);

  // register global params
  bt_machine_init_global_params(self);
  // register voice params
  bt_machine_init_voice_params(self);

  GST_DEBUG("machine-refs: %d",(G_OBJECT(self))->ref_count);

  // post sanity checks
  GST_INFO("  added machine %p to bin, machine->ref_count=%d  bin->ref_count=%d",self->priv->machines[PART_MACHINE],G_OBJECT(self->priv->machines[PART_MACHINE])->ref_count,G_OBJECT(self->priv->bin)->ref_count);
  g_assert(self->priv->machines[PART_MACHINE]!=NULL);
  g_assert(self->src_elem!=NULL);
  g_assert(self->dst_elem!=NULL);
  if(!BT_IS_SINK_MACHINE(self) && !(self->priv->global_params+self->priv->voice_params)) {
    GST_WARNING("  machine %s has no params",self->priv->id);
  }

  // prepare common internal patterns for the machine
  if((pattern=bt_pattern_new_with_event(self->priv->song,self,BT_PATTERN_CMD_BREAK))) {
    g_object_unref(pattern);
  }
  if((pattern=bt_pattern_new_with_event(self->priv->song,self,BT_PATTERN_CMD_MUTE))) {
    g_object_unref(pattern);
  }

  // prepare additional internal patterns for the machine and setup elements
  if(klass->setup) {
    (klass->setup)(self);
    // store number of patterns
    self->priv->private_patterns=g_list_length(self->priv->patterns);
  }


  GST_DEBUG("machine-refs: %d",(G_OBJECT(self))->ref_count);

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
gboolean bt_machine_new(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!self->priv->machines[PART_MACHINE],FALSE);
  g_return_val_if_fail(BT_IS_STRING(self->priv->id),FALSE);
  g_return_val_if_fail(BT_IS_STRING(self->priv->plugin_name),FALSE);
  g_return_val_if_fail(BT_IS_SONG(self->priv->song),FALSE);

  GST_INFO("initializing machine");

  if(bt_machine_setup(self)) {
    BtSetup *setup;

    // add the machine to the setup of the song
    g_object_get(G_OBJECT(self->priv->song),"setup",&setup,NULL);
    g_assert(setup!=NULL);
    bt_setup_add_machine(setup,self);
    g_object_unref(setup);
    res=TRUE;
  }
  return(res);
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
gboolean bt_machine_enable_input_level(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!BT_IS_SOURCE_MACHINE(self),FALSE);

  if(!self->priv->machines[PART_INPUT_LEVEL]) {
    GST_INFO(" for machine '%s'",self->priv->id);

    // add input-level analyser
    if(!bt_machine_make_internal_element(self,PART_INPUT_LEVEL,"level","input_level")) goto Error;
    g_object_set(G_OBJECT(self->priv->machines[PART_INPUT_LEVEL]),
      "interval",(GstClockTime)(0.10*GST_SECOND),"message",TRUE,
      "peak-ttl",(GstClockTime)(0.25*GST_SECOND),"peak-falloff", 36.0,
      NULL);
    if(!bt_machine_add_input_element(self,PART_INPUT_LEVEL)) goto Error;
  }
  res=TRUE;
Error:
  return(res);
}

/**
 * bt_machine_enable_output_level:
 * @self: the machine to enable the output-level analyser in
 *
 * Creates the output-level analyser of the machine and activates it.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 */
gboolean bt_machine_enable_output_level(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!BT_IS_SINK_MACHINE(self),FALSE);

  if(!self->priv->machines[PART_OUTPUT_LEVEL]) {
    GST_INFO(" for machine '%s'",self->priv->id);

    // add output-level analyser
    if(!bt_machine_make_internal_element(self,PART_OUTPUT_LEVEL,"level","output_level")) goto Error;
    g_object_set(G_OBJECT(self->priv->machines[PART_OUTPUT_LEVEL]),
      "interval",(GstClockTime)(0.25*GST_SECOND),"message",TRUE,
      "peak-ttl",(GstClockTime)(0.50*GST_SECOND),"peak-falloff", 20.0,
      NULL);
    if(!bt_machine_add_output_element(self,PART_OUTPUT_LEVEL)) goto Error;
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
gboolean bt_machine_enable_input_gain(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!BT_IS_SOURCE_MACHINE(self),FALSE);

  if(!self->priv->machines[PART_INPUT_GAIN]) {
    GST_INFO(" for machine '%s'",self->priv->id);

    // add input-gain element
    if(!bt_machine_make_internal_element(self,PART_INPUT_GAIN,"volume","input_gain")) goto Error;
    if(!bt_machine_add_input_element(self,PART_INPUT_GAIN)) goto Error;
  }
  res=TRUE;
Error:
  return(res);
}

/**
 * bt_machine_enable_output_gain:
 * @self: the machine to enable the output-gain element in
 *
 * Creates the output-gain element of the machine and activates it.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 */
gboolean bt_machine_enable_output_gain(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!BT_IS_SINK_MACHINE(self),FALSE);

  if(!self->priv->machines[PART_OUTPUT_GAIN]) {
    GST_INFO(" for machine '%s'",self->priv->id);

    // add input-gain element
    if(!bt_machine_make_internal_element(self,PART_OUTPUT_GAIN,"volume","output_gain")) goto Error;
    if(!bt_machine_add_output_element(self,PART_OUTPUT_GAIN)) goto Error;
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
gboolean bt_machine_activate_adder(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!BT_IS_SOURCE_MACHINE(self),FALSE);

  if(!self->priv->machines[PART_ADDER]) {
    GST_INFO(" for machine '%s'",self->priv->id);

    // create the adder
    if(!(bt_machine_make_internal_element(self,PART_ADDER,"adder","adder"))) goto Error;
    // adder does not link directly to some elements
    if(!(bt_machine_make_internal_element(self,PART_CAPS_FILTER,"capsfilter","capsfilter"))) goto Error;
    if(!(bt_machine_make_internal_element(self,PART_ADDER_CONVERT,"audioconvert","audioconvert"))) goto Error;
    GST_DEBUG("  about to link adder -> convert -> dst_elem");
    if(!gst_element_link_many(self->priv->machines[PART_ADDER], self->priv->machines[PART_CAPS_FILTER], self->priv->machines[PART_ADDER_CONVERT], self->dst_elem, NULL)) {
      gst_element_unlink_many(self->priv->machines[PART_ADDER], self->priv->machines[PART_CAPS_FILTER], self->priv->machines[PART_ADDER_CONVERT], self->dst_elem, NULL);
      GST_ERROR("failed to link the internal adder of machines '%s'",gst_element_get_name(self->priv->machines[PART_MACHINE]));goto Error;
    }
    else {
      GST_DEBUG("  adder activated for '%s'",gst_element_get_name(self->priv->machines[PART_MACHINE]));
      self->dst_elem=self->priv->machines[PART_ADDER];
    }
  }
  res=TRUE;
Error:
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
gboolean bt_machine_has_active_adder(const BtMachine * const self) {
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);

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
gboolean bt_machine_activate_spreader(BtMachine * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(!BT_IS_SINK_MACHINE(self),FALSE);

  if(!self->priv->machines[PART_SPREADER]) {
    GST_INFO(" for machine '%s'",self->priv->id);

    // create the spreader (tee)
    if(!(bt_machine_make_internal_element(self,PART_SPREADER,"tee","tee"))) goto Error;
    if(!gst_element_link(self->src_elem, self->priv->machines[PART_SPREADER])) {
      GST_ERROR("failed to link the internal spreader of machines '%s'",gst_element_get_name(self->priv->machines[PART_MACHINE]));res=FALSE;
    }
    else {
      GST_DEBUG("  spreader activated for '%s'",gst_element_get_name(self->priv->machines[PART_MACHINE]));
      self->src_elem=self->priv->machines[PART_SPREADER];
    }
  }
  res=TRUE;
Error:
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
gboolean bt_machine_has_active_spreader(const BtMachine * const self) {
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);

  return(self->src_elem==self->priv->machines[PART_SPREADER]);
}

static guint get_int_value(GstStructure *str,gchar *name) {
  const GValue *v;
  guint res=0;

  if((v=gst_structure_get_value(str,name))) {
    if(G_VALUE_HOLDS_INT(v))
      res = g_value_get_int(v);
    else if(GST_VALUE_HOLDS_INT_RANGE(v))
      res = gst_value_get_int_range_min(v);
  }
  return(res);
}

/**
 * bt_machine_renegotiate_adder_format:
 * @self: the machine
 *
 * Analyze the format on all machines linking to this one and determine a common
 * format for mixing.
 */
void bt_machine_renegotiate_adder_format(const BtMachine * const self) {
  BtSetup *setup;
  BtWire *wire;
  BtMachine *src;
  GList *wires,*node;

  /* do nothing if we don't have and adder & capsfilter or not caps */
  if(!self->priv->machines[PART_CAPS_FILTER]) return;

  g_object_get(self->priv->song,"setup",&setup,NULL);
  if(!setup) return;

  if((wires=bt_setup_get_wires_by_dst_machine(setup,self))) {
    GstPad *pad;
    GstCaps *pad_caps,*new_caps;
    const GstCaps *pad_tmpl_caps=NULL,*caps;
    GstStructure *ps,*ns;
    guint size,i;
    gint p_format,n_format=0;
    gint p_width,n_width=8;
    gint p_depth,n_depth=8;
    gint p_channels,n_channels=1;
    gint singnednes_signed_ct=0,signedness_unsigned_ct=0,p_signedness;
    gboolean adjust_channels=FALSE;
    const gchar *p_name;
    const gchar *fmt_names[]={
      "audio/x-raw-int",
      "audio/x-raw-float"
    };

    for(node=wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      g_object_get(wire,"src",&src,NULL);

      if((pad=gst_element_get_pad(src->priv->machines[PART_MACHINE],"src"))) {
        // @todo: only check template caps?
        if((pad_caps=gst_pad_get_negotiated_caps(pad)) ||
          (pad_tmpl_caps=gst_pad_get_pad_template_caps(pad))) {

          caps=pad_caps?pad_caps:pad_tmpl_caps;
          GST_INFO("checking caps %" GST_PTR_FORMAT, caps);

          size=gst_caps_get_size(caps);
          for(i=0;i<size;i++) {
            ps=gst_caps_get_structure(caps,i);
            p_name=gst_structure_get_name(ps);
            if(!strcmp(p_name,fmt_names[0])) p_format=0;
            else if(!strcmp(p_name,fmt_names[1])) p_format=1;
            else {
              GST_WARNING("unsupported format: %s",p_name);
              continue;
            }

            if(p_format>=n_format) {
              n_format=p_format;
              // check width/depth
              p_width=get_int_value(ps,"width");
              if(p_width>n_width) n_width=p_width;
              if(n_format==0) {
                p_depth=get_int_value(ps,"depth");
                if(p_depth>n_depth) n_width=p_depth;
              }
            }
            // check channels
            p_channels=get_int_value(ps,"channels");
            if(p_channels>n_channels) n_channels=p_channels;
            else adjust_channels=TRUE;
            if(p_format==0) {
              // check signedness
              if(gst_structure_get_int(ps,"signedness",&p_signedness)) {
                if(p_signedness) singnednes_signed_ct++;
                else signedness_unsigned_ct++;
              }
            }
          }
          if(pad_caps) gst_caps_unref(pad_caps);
        }
        else {
          GST_WARNING("No caps on pad?");
        }
        gst_object_unref(pad);
      }
      g_object_unref(src);
      g_object_unref(wire);
    }
    /* @todo add panorama elements
    if(adjust_channels) {
      gint m_channels;
      const GValue *v;

      for(node=wires;node;node=g_list_next(node)) {
        wire=BT_WIRE(node->data);
        g_object_get(wire,"src",&src,NULL);

        // check max-channels
        m_channels=0;
        if((pad=gst_element_get_pad(src->priv->machines[PART_MACHINE],"src"))) {
          // @todo: only check template caps?
          if((pad_caps=gst_pad_get_negotiated_caps(pad)) ||
            (pad_tmpl_caps=gst_pad_get_pad_template_caps(pad))) {

            caps=pad_caps?pad_caps:pad_tmpl_caps;
            GST_INFO("checking caps %" GST_PTR_FORMAT, caps);

            size=gst_caps_get_size(caps);
            for(i=0;i<size;i++) {
              ps=gst_caps_get_structure(caps,i);
              if((v=gst_structure_get_value(ps,"channels"))) {
                if(G_VALUE_HOLDS_INT(v))
                  p_channels = g_value_get_int(v);
                else if(GST_VALUE_HOLDS_INT_RANGE(v))
                  p_channels = gst_value_get_int_range_max(v);
              }
              if(p_channels>m_channels) m_channels=p_channels;
            }
            if(pad_caps) gst_caps_unref(pad_caps);
          }
          gst_object_unref(pad);
        }
        if(m_channels<n_channels) {
          bt_machine_activate_panorama(src);
        }
        else {
          bt_machine_deactivate_panorama(src);
        }

        g_object_unref(src);
        g_object_unref(wire);
      }
    }
    */
    g_list_free(wires);


    // what about rate, endianness and signed
    ns=gst_structure_new(fmt_names[n_format],
      "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT,
      NULL);
    gst_structure_set(ns,
      "channels",GST_TYPE_INT_RANGE,n_channels,8,
      "width",GST_TYPE_INT_RANGE,n_width,32,
      "signedness",G_TYPE_INT,G_BYTE_ORDER,
      NULL);
    if(n_format==0) {
      gst_structure_set(ns,
        "depth",GST_TYPE_INT_RANGE,n_depth,32,
        "signedness",G_TYPE_INT,(singnednes_signed_ct>=signedness_unsigned_ct),
        NULL);
    }
    new_caps=gst_caps_new_full(ns,NULL);

    GST_INFO("set new caps %" GST_PTR_FORMAT, new_caps);

    g_object_set(self->priv->machines[PART_CAPS_FILTER],"caps",new_caps,NULL);
  }
  g_object_unref(setup);
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
void bt_machine_add_pattern(const BtMachine * const self, const BtPattern * const pattern) {
  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(BT_IS_PATTERN(pattern));

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
void bt_machine_remove_pattern(const BtMachine * const self, const BtPattern * const pattern) {
  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(BT_IS_PATTERN(pattern));

  if(g_list_find(self->priv->patterns,pattern)) {
    self->priv->patterns=g_list_remove(self->priv->patterns,pattern);
    g_signal_emit(G_OBJECT(self),signals[PATTERN_REMOVED_EVENT], 0, pattern);
    GST_DEBUG("removing pattern: ref_count=%d",G_OBJECT(pattern)->ref_count);
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
BtPattern *bt_machine_get_pattern_by_id(const BtMachine * const self,const gchar * const id) {
  gboolean found=FALSE;
  BtPattern *pattern;
  gchar *pattern_id;
  GList* node;

  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);

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
BtPattern *bt_machine_get_pattern_by_index(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<g_list_length(self->priv->patterns),NULL);

  return(g_object_ref(BT_PATTERN(g_list_nth_data(self->priv->patterns,(guint)index))));
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
gchar *bt_machine_get_unique_pattern_name(const BtMachine * const self) {
  BtPattern *pattern=NULL;
  gchar *id,*ptr;
  guint8 i=0;

  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);

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

/**
 * bt_machine_has_patterns:
 * @self: the machine for which to check the patterns
 *
 * Check if the machine has #BtPattern entries appart from the standart private
 * ones.
 *
 * Returns: %TRUE if it has patterns
 */
gboolean bt_machine_has_patterns(const BtMachine * const self) {
  return(g_list_length(self->priv->patterns)>self->priv->private_patterns);
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
gboolean bt_machine_is_polyphonic(const BtMachine * const self) {
  gboolean res;
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);

  res=GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE]);
  GST_INFO(" is machine \"%s\" polyphonic ? %d",self->priv->id,res);
  return(res);
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
gboolean bt_machine_is_global_param_trigger(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(index<self->priv->global_params,FALSE);

  if(!(self->priv->global_flags[index]&GST_PROPERTY_META_STATE)) return(TRUE);
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
gboolean bt_machine_is_voice_param_trigger(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(index<self->priv->voice_params,FALSE);

  if(!(self->priv->voice_flags[index]&GST_PROPERTY_META_STATE)) return(TRUE);
  return(FALSE);
}

/**
 * bt_machine_is_global_param_no_value:
 * @self: the machine to check params from
 * @index: the offset in the list of global params
 * @value: the value to compare against the no-value
 *
 * Tests if the given value is the no-value of the global param
 *
 * Returns: %TRUE if it is the no-value
 */
gboolean bt_machine_is_global_param_no_value(const BtMachine * const self, const gulong index, GValue * const value) {
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(index<self->priv->global_params,FALSE);
  g_return_val_if_fail(G_IS_VALUE(value),FALSE);

  if(!G_IS_VALUE(&self->priv->global_no_val[index])) return(FALSE);

  if(gst_value_compare(&self->priv->global_no_val[index],value)==GST_VALUE_EQUAL) return(TRUE);
  return(FALSE);
}

/**
 * bt_machine_is_voice_param_no_value:
 * @self: the machine to check params from
 * @index: the offset in the list of voice params
 * @value: the value to compare against the no-value
 *
 * Tests if the given value is the no-value of the voice param
 *
 * Returns: %TRUE if it is the no-value
 */
gboolean bt_machine_is_voice_param_no_value(const BtMachine * const self, const gulong index, GValue * const value) {
  g_return_val_if_fail(BT_IS_MACHINE(self),FALSE);
  g_return_val_if_fail(index<self->priv->voice_params,FALSE);
  g_return_val_if_fail(G_IS_VALUE(value),FALSE);

  if(!G_IS_VALUE(&self->priv->voice_no_val[index])) return(FALSE);

  if(gst_value_compare(&self->priv->voice_no_val[index],value)==GST_VALUE_EQUAL) return(TRUE);
  return(FALSE);
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
glong bt_machine_get_global_param_index(const BtMachine *const self, const gchar * const name, GError **error) {
  glong ret=-1,i;
  gboolean found=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),-1);
  g_return_val_if_fail(BT_IS_STRING(name),-1);
  g_return_val_if_fail(error == NULL || *error == NULL, -1);

  for(i=0;i<self->priv->global_params;i++) {
    if(!strcmp(self->priv->global_names[i],name)) {
      ret=i;
      found=TRUE;
      break;
    }
  }
  if(!found && error) {
    GST_WARNING("global param for name %s not found", name);
    g_set_error (error, error_domain, /* errorcode= */0,
                "global param for name %s not found", name);
  }
  //g_assert((found || (error && *error)));
  g_assert(((found && (ret>=0)) || ((ret==-1) && ((error && *error) || !error))));
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
glong bt_machine_get_voice_param_index(const BtMachine * const self, const gchar * const name, GError **error) {
  gulong ret=-1,i;
  gboolean found=FALSE;

  g_return_val_if_fail(BT_IS_MACHINE(self),-1);
  g_return_val_if_fail(BT_IS_STRING(name),-1);
  g_return_val_if_fail(error == NULL || *error == NULL, -1);

  for(i=0;i<self->priv->voice_params;i++) {
    if(!strcmp(self->priv->voice_names[i],name)) {
      ret=i;
      found=TRUE;
      break;
    }
  }
  if(!found && error) {
    GST_WARNING("voice param for name %s not found", name);
    g_set_error (error, error_domain, /* errorcode= */0,
                "voice param for name %s not found", name);
  }
  g_assert(((found && (ret>=0)) || ((ret==-1) && ((error && *error) || !error))));
  return(ret);
}

/**
 * bt_machine_get_global_param_spec:
 * @self: the machine to search for the global param
 * @index: the offset in the list of global params
 *
 * Retrieves the parameter specification for the global param
 *
 * Returns: the #GParamSpec for the requested global param
 */
GParamSpec *bt_machine_get_global_param_spec(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->global_params,NULL);

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
GParamSpec *bt_machine_get_voice_param_spec(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->voice_params,NULL);

  GST_INFO("    voice-param %d '%s'",index,self->priv->voice_names[index]);
  return(g_object_class_find_property(
    G_OBJECT_CLASS(BT_MACHINE_GET_CLASS(self->priv->machines[PART_MACHINE])),
    self->priv->voice_names[index])
  );

#if 0
  GstObject *voice_child;
  GParamSpec *pspec=NULL;

  GST_INFO("    voice-param %d '%s'",index,self->priv->voice_names[index]);

  if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),0))) {
    pspec=g_object_class_find_property(
      G_OBJECT_CLASS(GST_OBJECT_GET_CLASS(voice_child)),
      self->priv->voice_names[index]);
    g_object_unref(voice_child);
  }
  return(pspec);
#endif
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
GType bt_machine_get_global_param_type(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),0);
  g_return_val_if_fail(index<self->priv->global_params,0);
  g_return_val_if_fail(self->priv->global_types,0);

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
GType bt_machine_get_voice_param_type(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),0);
  g_return_val_if_fail(index<self->priv->voice_params,0);

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
void bt_machine_set_global_param_value(const BtMachine * const self, const gulong index, GValue * const event) {
  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(G_IS_VALUE(event));
  g_return_if_fail(index<self->priv->global_params);

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
void bt_machine_set_voice_param_value(const BtMachine * const self, const gulong voice, const gulong index, GValue * const event) {
  GstObject *voice_child;

  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(G_IS_VALUE(event));
  g_return_if_fail(voice<self->priv->voices);
  g_return_if_fail(index<self->priv->voice_params);

  if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),voice))) {
    g_object_set_property(G_OBJECT(voice_child),self->priv->voice_names[index],event);
    g_object_unref(voice_child);
  }
}

/**
 * bt_machine_set_global_param_no_value:
 * @self: the machine to set the global param value to the no-value
 * @index: the offset in the list of global params
 *
 * Sets a the specified global param to the neutral no-value.
 */
void bt_machine_set_global_param_no_value(const BtMachine * const self, const gulong index) {
  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(index<self->priv->global_params);

  if(bt_machine_is_global_param_trigger(self,index)) {
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
void bt_machine_set_voice_param_no_value(const BtMachine * const self, const gulong voice, const gulong index) {
  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(voice<self->priv->voices);
  g_return_if_fail(index<self->priv->global_params);

  if(bt_machine_is_voice_param_trigger(self,index)) {
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
const gchar *bt_machine_get_global_param_name(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->global_params,NULL);

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
const gchar *bt_machine_get_voice_param_name(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->voice_params,NULL);

  return(self->priv->voice_names[index]);
}

static GValue *bt_machine_get_param_min_value(const BtMachine * const self, GParamSpec * const property) {
  GValue *value=g_new0(GValue,1);

  // @todo: this should ev. check voice_child
  if(!GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE]) ||
    !bt_machine_get_property_meta_value(value,property,gst_property_meta_quark_min_val)
  ) {
    GType base_type=bt_g_type_get_base_type(property->value_type);

    GST_INFO("    no property-meta min-val for param \"%s\"",property->name);
    g_value_init(value,property->value_type);
    switch(base_type) {
      case G_TYPE_BOOLEAN:
        g_value_set_boolean(value,0);
      break;
      case G_TYPE_INT: {
        const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
        g_value_set_int(value,int_property->minimum);
      } break;
      case G_TYPE_UINT: {
        const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
        g_value_set_uint(value,uint_property->minimum);
      }  break;
      case G_TYPE_FLOAT: {
        const GParamSpecFloat *float_property=G_PARAM_SPEC_FLOAT(property);
        g_value_set_float(value,float_property->minimum);
      } break;
      case G_TYPE_DOUBLE: {
        const GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);
        g_value_set_double(value,double_property->minimum);
      } break;
      case G_TYPE_ENUM: {
        const GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
        const GEnumClass *enum_class=enum_property->enum_class;
        g_value_set_enum(value,enum_class->minimum);
      } break;
      default:
        GST_ERROR("unsupported GType=%d:'%s' ('%s')",property->value_type,g_type_name(property->value_type),g_type_name(base_type));
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
GValue *bt_machine_get_global_param_min_value(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->global_params,NULL);

  return(bt_machine_get_param_min_value(self,bt_machine_get_global_param_spec(self,index)));
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
GValue *bt_machine_get_voice_param_min_value(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->voice_params,NULL);

  return(bt_machine_get_param_min_value(self,bt_machine_get_voice_param_spec(self,index)));
}

static GValue *bt_machine_get_param_max_value(const BtMachine * const self, GParamSpec * const property) {
  GValue *value=g_new0(GValue,1);

  // @todo: this should ev. check voice_child
  if(!GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE]) ||
    !bt_machine_get_property_meta_value(value,property,gst_property_meta_quark_max_val)
  ) {
    GType base_type=bt_g_type_get_base_type(property->value_type);

    GST_INFO("    no property-meta max-val for param \"%s\"",property->name);
    g_value_init(value,property->value_type);
    switch(base_type) {
      case G_TYPE_BOOLEAN:
        g_value_set_boolean(value,1);
      break;
      case G_TYPE_INT: {
        const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
        g_value_set_int(value,int_property->maximum);
      } break;
      case G_TYPE_UINT: {
        const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
        g_value_set_uint(value,uint_property->maximum);
      } break;
      case G_TYPE_FLOAT: {
        const GParamSpecFloat *float_property=G_PARAM_SPEC_FLOAT(property);
        g_value_set_float(value,float_property->maximum);
      } break;
      case G_TYPE_DOUBLE: {
        const GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);
        g_value_set_double(value,double_property->maximum);
      } break;
      case G_TYPE_ENUM: {
        const GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
        const GEnumClass *enum_class=enum_property->enum_class;
        g_value_set_enum(value,enum_class->maximum);
      } break;
      default:
        GST_ERROR("unsupported GType=%d:'%s' ('%s')",property->value_type,g_type_name(property->value_type),g_type_name(base_type));
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
GValue *bt_machine_get_global_param_max_value(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->global_params,NULL);

  return(bt_machine_get_param_max_value(self,bt_machine_get_global_param_spec(self,index)));
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
GValue *bt_machine_get_voice_param_max_value(const BtMachine * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->voice_params,NULL);

  return(bt_machine_get_param_max_value(self,bt_machine_get_voice_param_spec(self,index)));
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
gchar *bt_machine_describe_global_param_value(const BtMachine * const self, const gulong index, GValue * const event) {
  gchar *str=NULL;

  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->global_params,NULL);
  g_return_val_if_fail(G_IS_VALUE(event),NULL);

  if(GST_IS_PROPERTY_META(self->priv->machines[PART_MACHINE])) {
    str=gst_property_meta_describe_property(GST_PROPERTY_META(self->priv->machines[PART_MACHINE]),index,event);
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
gchar *bt_machine_describe_voice_param_value(const BtMachine * const self, const gulong index, GValue * const event) {
  gchar *str=NULL;

  GST_INFO("%p voice value %d %p",self,index,event);

  g_return_val_if_fail(BT_IS_MACHINE(self),NULL);
  g_return_val_if_fail(index<self->priv->voice_params,NULL);
  g_return_val_if_fail(G_IS_VALUE(event),NULL);

  if(GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE])) {
    GstObject *voice_child;

    // get child for voice 0
    if((voice_child=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),0))) {
      if(GST_IS_PROPERTY_META(voice_child)) {
        str=gst_property_meta_describe_property(GST_PROPERTY_META(voice_child),index,event);
      }
      //else {
        //GST_WARNING("%s is not PROPERTY_META",self->priv->id);
      //}
      g_object_unref(voice_child);
    }
    //else {
      //GST_WARNING("%s has no voice child",self->priv->id);
    //}
  }
  //else {
    //GST_WARNING("%s is not CHILD_BIN",self->priv->id);
  //}
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
void bt_machine_global_controller_change_value(const BtMachine * const self, const gulong param, const GstClockTime timestamp, GValue * const value) {
  GstObject *param_parent;

  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(param<self->priv->global_params);

  // @TODO: use new API for the control-source

  param_parent=GST_OBJECT(self->priv->machines[PART_MACHINE]);

  if(value) {
    // @TODO: this needs to check if the property is controlled
    if(!self->priv->global_controller) {
      GstController *ctrl=bt_machine_activate_controller(G_OBJECT(param_parent), self->priv->global_names[param], bt_machine_is_global_param_trigger(self,param));

      g_object_try_unref(self->priv->global_controller);
      self->priv->global_controller=ctrl;
    }
    //GST_DEBUG("%s global controller change: %"GST_TIME_FORMAT" param %d:%s",self->priv->id,GST_TIME_ARGS(timestamp),param,self->priv->global_names[param]);

#ifdef HAVE_GST_CONTROLLER_NEW
/*
    gst_interpolation_control_source_set(self->priv->global_control_source[param],timestamp,value);
*/
#else
#endif
    gst_controller_set(self->priv->global_controller,self->priv->global_names[param],timestamp,value);

  }
  else {
    GList *values;
    gboolean remove=TRUE;

    gst_controller_unset(self->priv->global_controller,self->priv->global_names[param],timestamp);

    if((values=(GList *)gst_controller_get_all(self->priv->global_controller,self->priv->global_names[param]))) {
      if(g_list_length(values)>0) {
        remove=FALSE;
      }
      g_list_free(values);
    }
    if(remove) {
      bt_machine_deactivate_controller(G_OBJECT(param_parent), self->priv->global_names[param]);
    }
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
void bt_machine_voice_controller_change_value(const BtMachine * const self, const gulong param, const gulong voice, const GstClockTime timestamp, GValue * const value) {
  GstObject *param_parent;

  g_return_if_fail(BT_IS_MACHINE(self));
  g_return_if_fail(param<self->priv->voice_params);
  g_return_if_fail(voice<self->priv->voices);
  g_return_if_fail(GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE]));

  // @TODO: use new API for the control-source

  param_parent=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(self->priv->machines[PART_MACHINE]),voice);

  if(value) {
    // @TODO: this needs to check if the property is controlled
    if(!self->priv->voice_controllers[voice]) {
      GstController *ctrl=bt_machine_activate_controller(G_OBJECT(param_parent), self->priv->voice_names[param], bt_machine_is_voice_param_trigger(self,param));

      g_object_try_unref(self->priv->voice_controllers[voice]);
      self->priv->voice_controllers[voice]=ctrl;
    }
    //GST_DEBUG("%s voice controller change: %"GST_TIME_FORMAT" voice %d, param %d:%s",self->priv->id,GST_TIME_ARGS(timestamp),voice,param,self->priv->voice_names[param]);
    gst_controller_set(self->priv->voice_controllers[voice],self->priv->voice_names[param],timestamp,value);
  }
  else {
    GList *values;
    gboolean remove=TRUE;

    gst_controller_unset(self->priv->voice_controllers[voice],self->priv->voice_names[param],timestamp);

    if((values=(GList *)gst_controller_get_all(self->priv->voice_controllers[voice],self->priv->voice_names[param]))) {
      if(g_list_length(values)>0) {
        remove=FALSE;
      }
      g_list_free(values);
    }
    if(remove) {
      bt_machine_deactivate_controller(G_OBJECT(param_parent), self->priv->voice_names[param]);
    }
  }
}

//-- debug helper

GList *bt_machine_get_element_list(const BtMachine * const self) {
  GList *list=NULL;
  gulong i;

  for(i=0;i<PART_COUNT;i++) {
    if(self->priv->machines[i]) {
      list=g_list_append(list,self->priv->machines[i]);
    }
  }

  return(list);
}

void bt_machine_dbg_print_parts(const BtMachine * const self) {
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

void bt_machine_dbg_dump_global_controller_queue(const BtMachine * const self) {
  gulong i;
  FILE *file;
  gchar *name,*str;
  GList *list,*node;
  GstTimedValue *tv;

  for(i=0;i<self->priv->global_params;i++) {
    name=g_strdup_printf("/tmp/buzztard-%s_g%02lu.dat",self->priv->id,i);
    if((file=fopen(name,"wb"))) {
      fprintf(file,"# global param \"%s\" for machine \"%s\"\n",self->priv->global_names[i],self->priv->id);
      if((list=(GList *)gst_controller_get_all(self->priv->global_controller,self->priv->global_names[i]))) {
        for(node=list;node;node=g_list_next(node)) {
          tv=(GstTimedValue *)node->data;
          str=g_strdup_value_contents(&tv->value);
          fprintf(file,"%"GST_TIME_FORMAT" %"G_GUINT64_FORMAT" %s\n",GST_TIME_ARGS(tv->timestamp),tv->timestamp,str);
          g_free(str);
        }
        g_list_free(list);
      }
      fclose(file);
    }
    g_free(name);
  }
}

//-- io interface

static xmlNodePtr bt_machine_persistence_save(const BtPersistence * const persistence, const xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtMachine * const self = BT_MACHINE(persistence);
  GstObject *machine,*machine_voice;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node;
  gulong i,j;
  GValue value={0,};

  GST_DEBUG("PERSISTENCE::machine");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("machine"),NULL))) {
    xmlNewProp(node,XML_CHAR_PTR("id"),XML_CHAR_PTR(self->priv->id));

    // @todo: also store non-controllable parameters (preferences) <prefsdata name="" value="">
    // @todo: skip parameters which are default values (is that really a good idea?)
    machine=GST_OBJECT(self->priv->machines[PART_MACHINE]);
    for(i=0;i<self->priv->global_params;i++) {
      // skip trigger parameters and parameters that are also used as voice params
      if(bt_machine_is_global_param_trigger(self,i)) continue;
      if(self->priv->voice_params && bt_machine_get_voice_param_index(self,self->priv->global_names[i],NULL)>-1) continue;

      if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("globaldata"),NULL))) {
        g_value_init(&value,self->priv->global_types[i]);
        g_object_get_property(G_OBJECT(machine),self->priv->global_names[i],&value);
        gchar * const str=bt_persistence_get_value(&value);
        xmlNewProp(child_node,XML_CHAR_PTR("name"),XML_CHAR_PTR(self->priv->global_names[i]));
        xmlNewProp(child_node,XML_CHAR_PTR("value"),XML_CHAR_PTR(str));
        g_free(str);
        g_value_unset(&value);
      }
    }
    for(j=0;j<self->priv->voices;j++) {
      machine_voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),j);

      for(i=0;i<self->priv->voice_params;i++) {
        if(bt_machine_is_voice_param_trigger(self,i)) continue;
        if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("voicedata"),NULL))) {
          g_value_init(&value,self->priv->voice_types[i]);
          g_object_get_property(G_OBJECT(machine_voice),self->priv->voice_names[i],&value);
          gchar * const str=bt_persistence_get_value(&value);
          xmlNewProp(child_node,XML_CHAR_PTR("voice"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(j)));
          xmlNewProp(child_node,XML_CHAR_PTR("name"),XML_CHAR_PTR(self->priv->voice_names[i]));
          xmlNewProp(child_node,XML_CHAR_PTR("value"),XML_CHAR_PTR(str));
          g_free(str);
          g_value_unset(&value);
        }
      }
      g_object_unref(machine_voice);
    }
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("properties"),NULL))) {
      if(!bt_persistence_save_hashtable(self->priv->properties,child_node)) goto Error;
    }
    else goto Error;
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("patterns"),NULL))) {
      bt_persistence_save_list(self->priv->patterns,child_node);
    }
    else goto Error;
  }
Error:
  return(node);
}

static gboolean bt_machine_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  BtMachine * const self = BT_MACHINE(persistence);
  gboolean res=FALSE;
  xmlChar *id,*name,*voice_str,*value_str;
  xmlNodePtr child_node;
  GValue value={0,};
  glong param,voice;
  GstObject *machine,*machine_voice;
  GError *error=NULL;

  GST_DEBUG("PERSISTENCE::machine");
  g_assert(node);

  id=xmlGetProp(node,XML_CHAR_PTR("id"));
  g_object_set(G_OBJECT(self),"id",id,NULL);
  xmlFree(id);

  if(!bt_machine_setup(self)) {
    GST_WARNING("Can't init machine");
    goto Error;
  }

  machine=GST_OBJECT(self->priv->machines[PART_MACHINE]);
  g_assert(machine);

  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      // @todo: load prefsdata
      if(!strncmp((gchar *)node->name,"globaldata\0",11)) {
        name=xmlGetProp(node,XML_CHAR_PTR("name"));
        value_str=xmlGetProp(node,XML_CHAR_PTR("value"));
        param=bt_machine_get_global_param_index(self,(gchar *)name,&error);
        if(!error) {
          g_value_init(&value,self->priv->global_types[param]);
          bt_persistence_set_value(&value,(gchar *)value_str);
          g_object_set_property(G_OBJECT(machine),(gchar *)name,&value);
          g_value_unset(&value);
          GST_INFO("initialized global machine data for param %d: %s",param, name);
          xmlFree(name);xmlFree(value_str);
        }
        else {
          GST_WARNING("error while loading global machine data for param %d: %s",param,error->message);
          g_error_free(error);error=NULL;
          xmlFree(name);xmlFree(value_str);
          BT_PERSISTENCE_ERROR(Error);
        }
      }
      else if(!strncmp((gchar *)node->name,"voicedata\0",10)) {
        voice_str=xmlGetProp(node,XML_CHAR_PTR("voice"));
        voice=atol((char *)voice_str);
        name=xmlGetProp(node,XML_CHAR_PTR("name"));
        value_str=xmlGetProp(node,XML_CHAR_PTR("value"));
        param=bt_machine_get_voice_param_index(self,(gchar *)name,&error);
        if(!error) {
          machine_voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),voice);
          g_assert(machine_voice);

          g_value_init(&value,self->priv->voice_types[param]);
          bt_persistence_set_value(&value,(gchar *)value_str);
          g_object_set_property(G_OBJECT(machine_voice),(gchar *)name,&value);
          g_value_unset(&value);
          GST_INFO("initialized voice machine data for param %d: %s",param, name);
          xmlFree(name);xmlFree(value_str);xmlFree(voice_str);
          g_object_unref(machine_voice);
        }
        else {
          GST_WARNING("error while loading voice machine data for param %d, voice %d: %s",param,voice,error->message);
          g_error_free(error);error=NULL;
          xmlFree(name);xmlFree(value_str);xmlFree(voice_str);
          BT_PERSISTENCE_ERROR(Error);
        }
      }
      else if(!strncmp((gchar *)node->name,"properties\0",11)) {
        bt_persistence_load_hashtable(self->priv->properties,node);
      }
      else if(!strncmp((gchar *)node->name,"patterns\0",9)) {
        BtPattern *pattern;

        for(child_node=node->children;child_node;child_node=child_node->next) {
          if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"pattern\0",8))) {
            pattern=BT_PATTERN(g_object_new(BT_TYPE_PATTERN,"song",self->priv->song,"machine",self,"voices",self->priv->voices,NULL));
            if(bt_persistence_load(BT_PERSISTENCE(pattern),child_node,NULL)) {
              bt_machine_add_pattern(self,pattern);
            }
            g_object_unref(pattern);
          }
        }
      }
    }
  }

  res=TRUE;
Error:
  return(res);
}

static void bt_machine_persistence_interface_init(gpointer const g_iface, gconstpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_machine_persistence_load;
  iface->save = bt_machine_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  const BtMachine * const self = BT_MACHINE(object);
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
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->machines[PART_MACHINE]);
      #endif
    } break;
    case MACHINE_INPUT_LEVEL: {
      g_value_set_object(value, self->priv->machines[PART_INPUT_LEVEL]);
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->machines[PART_INPUT_LEVEL]);
      #endif
    } break;
    case MACHINE_INPUT_GAIN: {
      g_value_set_object(value, self->priv->machines[PART_INPUT_GAIN]);
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->machines[PART_INPUT_GAIN]);
      #endif
    } break;
    case MACHINE_OUTPUT_LEVEL: {
      g_value_set_object(value, self->priv->machines[PART_OUTPUT_LEVEL]);
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->machines[PART_OUTPUT_LEVEL]);
      #endif
    } break;
    case MACHINE_OUTPUT_GAIN: {
      g_value_set_object(value, self->priv->machines[PART_OUTPUT_GAIN]);
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->machines[PART_OUTPUT_GAIN]);
      #endif
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
static void bt_machine_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec)
{
  const BtMachine * const self = BT_MACHINE(object);

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
        gchar *name=g_alloca(strlen(self->priv->id)+16);

        g_sprintf(name,"%s_%p",self->priv->id,self);
        gst_element_set_name(self->priv->machines[PART_MACHINE],name);
      }
      bt_song_set_unsaved(self->priv->song,TRUE);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_free(self->priv->plugin_name);
      self->priv->plugin_name = g_value_dup_string(value);
      GST_DEBUG("set the plugin_name for machine: %s",self->priv->plugin_name);
    } break;
    case MACHINE_VOICES: {
      const gulong voices=self->priv->voices;
      self->priv->voices = g_value_get_ulong(value);
      if(GST_IS_CHILD_BIN(self->priv->machines[PART_MACHINE])) {
        if(voices!=self->priv->voices) {
          GST_DEBUG("set the voices for machine: %d",self->priv->voices);
          bt_machine_resize_voices(self,voices);
          bt_machine_resize_pattern_voices(self);
          bt_song_set_unsaved(self->priv->song,TRUE);
        }
      }
    } break;
    case MACHINE_GLOBAL_PARAMS: {
      self->priv->global_params = g_value_get_ulong(value);
    } break;
    case MACHINE_VOICE_PARAMS: {
      self->priv->voice_params = g_value_get_ulong(value);
    } break;
    case MACHINE_STATE: {
      if(bt_machine_change_state(self,g_value_get_enum(value))) {
        GST_DEBUG("set the state for machine: %d",self->priv->state);
        bt_song_set_unsaved(self->priv->song,TRUE);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_dispose(GObject * const object) {
  const BtMachine * const self = BT_MACHINE(object);
  guint i,j;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p,%s, song=%p",self,self->priv->id,self->priv->song);

  // disconnect notify handlers
  if(self->priv->song) {
    BtSongInfo *song_info;
    g_object_get(G_OBJECT(self->priv->song),"song-info",&song_info,NULL);
    if(song_info) {
      GST_DEBUG("  disconnecting song-info handlers");
      g_signal_handlers_disconnect_matched(song_info,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_machine_on_bpm_changed,NULL);
      g_signal_handlers_disconnect_matched(song_info,G_SIGNAL_MATCH_FUNC,0,0,NULL,bt_machine_on_tpb_changed,NULL);
      g_object_unref(song_info);
    }
  }

  // remove the GstElements from the bin
  // gstreamer uses floating references, therefore elements are destroyed, when removed from the bin
  if(self->priv->bin) {
    GstStateChangeReturn res;

    GST_DEBUG("  bin->ref_count=%d, bin->num_children=%d",
      (G_OBJECT(self->priv->bin))->ref_count,
      GST_BIN_NUMCHILDREN(self->priv->bin)
    );
    for(i=0;i<PART_COUNT;i++) {
      if(self->priv->machines[i]) {
        g_assert(GST_IS_BIN(self->priv->bin));
        g_assert(GST_IS_ELEMENT(self->priv->machines[i]));
        for(j=i+1;j<PART_COUNT;j++) {
          if(self->priv->machines[j]) {
            GST_DEBUG("  unlinking machine \"%s\", \"%s\"",
              gst_element_get_name(self->priv->machines[i]),
              gst_element_get_name(self->priv->machines[j]));
            gst_element_unlink(self->priv->machines[i],self->priv->machines[j]);
          }
        }
        GST_DEBUG("  removing machine \"%s\" from bin, obj->ref_count=%d",gst_element_get_name(self->priv->machines[i]),(G_OBJECT(self->priv->machines[i]))->ref_count);
        if((res=gst_element_set_state(self->priv->machines[i],GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
          GST_WARNING("can't go to null state");
        else
          GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));
        gst_bin_remove(self->priv->bin,self->priv->machines[i]);
        GST_DEBUG("  bin->ref_count=%d, bin->num_children=%d",
          (self->priv->bin?(G_OBJECT(self->priv->bin))->ref_count:-1),
          (self->priv->bin?GST_BIN_NUMCHILDREN(self->priv->bin):-1)
        );
      }
    }
    // release the bin (that is ref'ed in bt_machine_setup() )
    GST_INFO("  releasing the bin, bin->ref_count=%d, bin->num_children=%d",
      (self->priv->bin?(G_OBJECT(self->priv->bin))->ref_count:-1),
      (self->priv->bin?GST_BIN_NUMCHILDREN(self->priv->bin):-1)
    );
    gst_object_unref(self->priv->bin);
  }

  GST_DEBUG("  releasing song: %p",self->priv->song);
  g_object_try_weak_unref(self->priv->song);

  GST_DEBUG("  releasing controllers, global.ref_ct=%d, voices=%d",
    (self->priv->global_controller?(G_OBJECT(self->priv->global_controller))->ref_count:-1),
    self->priv->voices);
  // unref controllers
  g_object_try_unref(self->priv->global_controller);
  if(self->priv->voice_controllers) {
    for(i=0;i<self->priv->voices;i++) {
      g_object_try_unref(self->priv->voice_controllers[i]);
    }
  }

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
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void bt_machine_finalize(GObject * const object) {
  const BtMachine * const self = BT_MACHINE(object);

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

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void bt_machine_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtMachine * const self = BT_MACHINE(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE, BtMachinePrivate);
  // default is no voice, only global params
  //self->priv->voices=1;
  self->priv->properties=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);

  GST_DEBUG("!!!! self=%p",self);
}

static void bt_machine_class_init(BtMachineClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  error_domain=g_quark_from_static_string("BtMachine");
  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMachinePrivate));

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
                                        g_cclosure_marshal_VOID__OBJECT,
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
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_PATTERN // param data
                                        );

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES,
                                  g_param_spec_pointer("properties",
                                     "properties prop",
                                     "list of machine properties",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "song object, the machine belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_ID,
                                  g_param_spec_string("id",
                                     "id contruct prop",
                                     "machine identifier",
                                     "unamed machine", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_PLUGIN_NAME,
                                  g_param_spec_string("plugin-name",
                                     "plugin-name construct prop",
                                     "the name of the gst plugin for the machine",
                                     "unamed machine", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_VOICES,
                                  g_param_spec_ulong("voices",
                                     "voices prop",
                                     "number of voices in the machine",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_GLOBAL_PARAMS,
                                  g_param_spec_ulong("global-params",
                                     "global-params prop",
                                     "number of params for the machine",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_VOICE_PARAMS,
                                  g_param_spec_ulong("voice-params",
                                     "voice-params prop",
                                     "number of params for each machine voice",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine element prop",
                                     "the machine element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_INPUT_LEVEL,
                                  g_param_spec_object("input-level",
                                     "input-level prop",
                                     "the input-level element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_INPUT_GAIN,
                                  g_param_spec_object("input-gain",
                                     "input-gain prop",
                                     "the input-gain element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_OUTPUT_LEVEL,
                                  g_param_spec_object("output-level",
                                     "output-level prop",
                                     "the output-level element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_OUTPUT_GAIN,
                                  g_param_spec_object("output-gain",
                                     "output-gain prop",
                                     "the output-gain element, if any",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_PATTERNS,
                                  g_param_spec_pointer("patterns",
                                     "pattern list prop",
                                     "a copy of the list of patterns",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_STATE,
                                  g_param_spec_enum("state",
                                     "state prop",
                                     "the current state of this machine",
                                     BT_TYPE_MACHINE_STATE,  /* enum type */
                                     BT_MACHINE_STATE_NORMAL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_machine_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
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
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_machine_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtMachine",&info,G_TYPE_FLAG_ABSTRACT);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
