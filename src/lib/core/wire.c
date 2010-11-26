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
 * SECTION:btwire
 * @short_description: class for a connection of two #BtMachines
 *
 * Abstracts connection between two #BtMachines. After creation, the elements
 * are connected. In contrast to directly wiring #GstElements this insert needed
 * conversion elements automatically.
 * Furthermore each wire has a volume and if possible panorama/balance element.
 */

/*
 * @todo try to derive this from GstBin!
 *  - then put the machines into itself (and not into the songs bin, 
 *    but insert the wire directly into the song->bin
 *  - when adding internal machines we need to fix the ghost pads using
 *    gst_ghost_pad_set_target()
 */

#define BT_CORE
#define BT_WIRE_C

#include "core_private.h"

//-- signal ids

enum {
  PATTERN_CREATED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  WIRE_CONSTRUCTION_ERROR=1,
  WIRE_PROPERTIES,
  WIRE_SONG,
  WIRE_SRC,
  WIRE_DST,
  WIRE_GAIN,
  WIRE_PAN,
  WIRE_NUM_PARAMS,
  WIRE_ANALYZERS
};

// capsfiter, convert, pan, volume are gap-aware
typedef enum {
  /* queue to avoid blocking when src has a spreader */
  PART_QUEUE,
  /* used to link analyzers */
  PART_TEE,
  /* volume-control element */
  PART_GAIN,
  /* wire format conversion elements */
  PART_CONVERT,
  /*PART_SCALE, unused right now */
  /* panorama / balance */
  PART_PAN,
  /* how many elements are used */
  PART_COUNT
} BtWirePart;

struct _BtWirePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  /* used to signal failed instance creation */
  GError **constrution_error;

  /* (ui) properties accociated with this machine */
  GHashTable *properties;

  /* the song the wire belongs to */
  G_POINTER_ALIAS(BtSong *,song);
  
  /* status in songs pipeline */
  gboolean is_added,is_connected;

  /* which machines are linked */
  BtMachine *src,*dst;
  
  /* src/sink ghost-pad of the wire */
  GstPad *src_pad,*sink_pad;

  /* the number of dynamic params the wire provides */
  gulong num_params;
  
  /* dynamic parameter control */
  GstController *wire_controller[BT_WIRE_MAX_NUM_PARAMS];
  GstInterpolationControlSource *wire_control_sources[BT_WIRE_MAX_NUM_PARAMS];
  GParamSpec *wire_props[BT_WIRE_MAX_NUM_PARAMS];

  /* event patterns in relation to patterns of the target machine */
  GHashTable *patterns;  // each entry points to BtWirePattern using BtPattern as a key

  /* the gstreamer elements that is used */
  GstElement *machines[PART_COUNT];
  GstPad *src_pads[PART_COUNT],*sink_pads[PART_COUNT];

  /* wire analyzers */
  GList *analyzers;
};

static GQuark error_domain=0;

static guint signals[LAST_SIGNAL]={0,};

static gchar *src_pn[]={
  "src",    /* queue */
  "\0src%d",/* tee */
  "src",    /* gain */
  "src",    /* convert */
  "src"     /* pan */
};
static gchar *sink_pn[]={
  "sink",   /* queue */
  "sink",   /* tee */
  "sink",   /* gain */
  "sink",   /* convert */
  "sink"    /* pan */
};

//-- the class

static void bt_wire_persistence_interface_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtWire, bt_wire, GST_TYPE_BIN,
  G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
    bt_wire_persistence_interface_init));

//-- pad templates
static GstStaticPadTemplate wire_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate wire_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

// macros

#define WIRE_PARAM_NAME(ix) self->priv->wire_props[ix]->name
#define WIRE_PARAM_TYPE(ix) self->priv->wire_props[ix]->value_type

//-- helper methods

/*
 * bt_wire_link_elements:
 * @self: the wire in which we link
 * @src,@sink: the pads
 *
 * Link two pads.
 *
 * Returns: %TRUE for sucess
 */
static gboolean bt_wire_link_elements(const BtWire * const self, GstPad *src, GstPad *sink) {
  GstPadLinkReturn plr;
  
  if((plr=gst_pad_link(src,sink))!=GST_PAD_LINK_OK) {
    GST_WARNING_OBJECT(self,"can't link %s:%s with %s:%s: %d",GST_DEBUG_PAD_NAME(src),GST_DEBUG_PAD_NAME(sink),plr);
    return(FALSE);
  }
  return(TRUE);
}


/*
 * bt_wire_make_internal_element:
 * @self: the wire
 * @part: which internal element to create
 * @factory_name: the element-factories name
 * @element_name: the name of the new #GstElement instance
 *
 * This helper is used by the bt_wire_link() function.
 */
static gboolean bt_wire_make_internal_element(const BtWire * const self, const BtWirePart part, const gchar * const factory_name, const gchar * const element_name) {
  gboolean res=FALSE;
  const gchar * const parent_name=GST_OBJECT_NAME(self);
  gchar * const name=g_alloca(strlen(parent_name)+2+strlen(element_name));

  // create internal element
  //strcat(name,parent_name);strcat(name,":");strcat(name,element_name);
  g_sprintf(name,"%s:%s",parent_name,element_name);
  if(!(self->priv->machines[part]=gst_element_factory_make(factory_name,name))) {
    GST_WARNING_OBJECT(self,"failed to create %s from factory %s",element_name,factory_name);goto Error;
  }
  
  // disable deep notify
  {
    GObjectClass *gobject_class=G_OBJECT_GET_CLASS(self->priv->machines[part]);
    GObjectClass *parent_class=g_type_class_peek_static(G_TYPE_OBJECT);
    gobject_class->dispatch_properties_changed=parent_class->dispatch_properties_changed;
  }

  // get the pads
  if(src_pn[part]) {
    if(src_pn[part][0]!='\0') {
      self->priv->src_pads[part]=gst_element_get_static_pad(self->priv->machines[part],src_pn[part]);
    } else {
      self->priv->src_pads[part]=gst_element_get_request_pad(self->priv->machines[part],&src_pn[part][1]);
    }
  }
  if(sink_pn[part])
    self->priv->sink_pads[part]=gst_element_get_static_pad(self->priv->machines[part],sink_pn[part]);

  gst_bin_add(GST_BIN(self),self->priv->machines[part]);
  res=TRUE;
Error:
  return(res);
}

//-- helper methods

static void bt_wire_init_params(const BtWire * const self) {
  self->priv->wire_props[0]=g_object_class_find_property(
    G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self->priv->machines[PART_GAIN])),
    "volume");
  if(self->priv->machines[PART_PAN]) {
    self->priv->wire_props[1]=g_object_class_find_property(
      G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self->priv->machines[PART_PAN])),"panorama");
    self->priv->num_params=2;
  }
}

/*
 * bt_wire_get_peer_pad:
 * @elem: a gstreamer element
 *
 * Get the peer pad connected to the first pad in the given iter.
 *
 * Returns: the pad or %NULL
 */
static GstPad *bt_wire_get_peer_pad(GstIterator *it) {
  gboolean done=FALSE;
  gpointer item;
  GstPad *pad,*peer_pad=NULL;

  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        pad=GST_PAD(item);
        peer_pad=gst_pad_get_peer(pad);
        done=TRUE;
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
  return(peer_pad);
}

#if 0
/*
 * bt_wire_get_src_peer_pad:
 * @elem: a gstreamer element
 *
 * Get the peer pad connected to the given elements first source pad.
 *
 * Returns: the pad or %NULL
 */
static GstPad *bt_wire_get_src_peer_pad(GstElement * const elem) {
  return(bt_wire_get_peer_pad(gst_element_iterate_src_pads(elem)));
}
#endif

/*
 * bt_wire_get_sink_peer_pad:
 * @elem: a gstreamer element
 *
 * Get the peer pad connected to the given elements first sink pad.
 *
 * Returns: the pad or %NULL
 */
static GstPad *bt_wire_get_sink_peer_pad(GstElement * const elem) {
  return(bt_wire_get_peer_pad(gst_element_iterate_sink_pads(elem)));
}

/*
 * bt_wire_activate_analyzers:
 * @self: the wire
 *
 * Add all analyzers to the bin and link them.
 */
static void bt_wire_activate_analyzers(const BtWire * const self) {
  gboolean res=TRUE;
  const GList* node;
  GstElement *prev=NULL,*next;
  GstPad *tee_src,*analyzer_sink;
  gboolean is_playing;

  if(!self->priv->analyzers) return;

  GST_INFO("add analyzers (%d elements)",g_list_length(self->priv->analyzers));
  g_object_get(self->priv->song,"is-playing",&is_playing,NULL);
  
  // do final link afterwards
  for(node=self->priv->analyzers;(node && res);node=g_list_next(node)) {
    next=GST_ELEMENT(node->data);

    if(!(res=gst_bin_add(GST_BIN(self),next))) {
      GST_INFO("cannot add element \"%s\" to bin",GST_OBJECT_NAME(next));
    }
    if(prev) {
      if(!(res=gst_element_link(prev,next))) {
        GST_INFO("cannot link elements \"%s\" -> \"%s\"",GST_OBJECT_NAME(prev),GST_OBJECT_NAME(next));
      }
    }
    prev=next;
  }
  GST_INFO("blocking tee.src");
  tee_src=gst_element_get_request_pad(self->priv->machines[PART_TEE],"src%d");
  analyzer_sink=gst_element_get_static_pad(GST_ELEMENT(self->priv->analyzers->data),"sink");
  if(is_playing)
    gst_pad_set_blocked(tee_src,TRUE);
  GST_INFO("sync state");
  for(node=self->priv->analyzers;node;node=g_list_next(node)) {
    next=GST_ELEMENT(node->data);
    if(!(gst_element_sync_state_with_parent(next))) {
      GST_INFO("cannot sync state for elements \"%s\"",GST_OBJECT_NAME(next));
    }
  }
  GST_INFO("linking to tee");
  if(GST_PAD_LINK_FAILED(gst_pad_link(tee_src,analyzer_sink))) {
    GST_INFO("cannot link analyzers to tee");
  }
  if(is_playing)
    gst_pad_set_blocked(tee_src,FALSE);
  gst_object_unref(analyzer_sink);
  GST_INFO("add analyzers done");
}

/*
 * bt_wire_deactivate_analyzers:
 * @self: the wire
 *
 * Remove all analyzers to the bin and unlink them.
 */
static void bt_wire_deactivate_analyzers(const BtWire * const self) {
  gboolean res=TRUE;
  const GList* node;
  GstElement *prev,*next;
  GstPad *src_pad=NULL;
  GstStateChangeReturn state_ret;
  gboolean is_playing;

  if(!self->priv->analyzers) return;

  GST_INFO("remove analyzers (%d elements)",g_list_length(self->priv->analyzers));
  g_object_get(self->priv->song,"is-playing",&is_playing,NULL);

  if((src_pad=bt_wire_get_sink_peer_pad(GST_ELEMENT(self->priv->analyzers->data)))) {
    // block src_pad (of tee)
    if(is_playing)
      gst_pad_set_blocked(src_pad,TRUE);
  }
  
  prev=self->priv->machines[PART_TEE];
  for(node=self->priv->analyzers;(node && res);node=g_list_next(node)) {
    next=GST_ELEMENT(node->data);

    if((state_ret=gst_element_set_state(next,GST_STATE_NULL))!=GST_STATE_CHANGE_SUCCESS) {
      GST_INFO("cannot set state to NULL for element '%s', ret='%s'",GST_OBJECT_NAME(next),gst_element_state_change_return_get_name(state_ret));
    }
    gst_element_unlink(prev,next);
    prev=next;
  }
  for(node=self->priv->analyzers;(node && res);node=g_list_next(node)) {
    next=GST_ELEMENT(node->data);
    if(!(res=gst_bin_remove(GST_BIN(self),next))) {
      GST_INFO("cannot remove element '%s' from bin",GST_OBJECT_NAME(next));
    }
  }
  if(src_pad) {
    if(is_playing)
      gst_pad_set_blocked(src_pad,FALSE);
    // remove request-pad
    GST_INFO("releasing request pad for tee");
    gst_element_release_request_pad(self->priv->machines[PART_TEE],src_pad);
    gst_object_unref(src_pad);
  }
}

/*
 * bt_wire_link_machines:
 * @self: the wire that should be used for this connection
 *
 * Links the gst-element of this wire and tries a couple for conversion elements
 * if necessary.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_wire_link_machines(const BtWire * const self) {
  gboolean res=TRUE;
  BtMachine * const src = self->priv->src;
  BtMachine * const dst = self->priv->dst;
  GstElement ** const machines=self->priv->machines;
  GstPad ** const src_pads=self->priv->src_pads;
  GstPad ** const sink_pads=self->priv->sink_pads;
  GstElement *dst_machine, *src_machine;
  GstCaps *caps;
  const GstCaps *dst_caps=NULL;
  GstPad *pad;
  gboolean skip_convert=FALSE;
  guint six,dix;

  g_assert(BT_IS_WIRE(self));

  if(!machines[PART_QUEUE]) {
    /* @todo: use the queue on demand.
     * if(bt_machine_activate_spreader(src)
     *    add the queue.
     * else
     *    remove the queue
     *
     * problem:
     *   we need to relink all outgoing wires for this src when adding/removing a wire
     */
    if(!bt_wire_make_internal_element(self,PART_QUEUE,"queue","queue")) return(FALSE);
    // configure the queue
    // @idea: if machine on source side is a live-source, should be keep the queue larger?
    // @todo: if we have/require gstreamer-0.10.31 ret rid of the check
    if(g_object_class_find_property(G_OBJECT_GET_CLASS(machines[PART_QUEUE]),"silent")) {
      g_object_set(G_OBJECT(machines[PART_QUEUE]),
        "max-size-buffers",2,
        "max-size-bytes",0,
        "max-size-time",G_GUINT64_CONSTANT(0),
        "silent",TRUE,
        NULL);
    } else {
      g_object_set(G_OBJECT(machines[PART_QUEUE]),
        "max-size-buffers",2,
        "max-size-bytes",0,
        "max-size-time",G_GUINT64_CONSTANT(0),
        NULL);
    }
    GST_DEBUG("created queue element for wire : %p '%s' -> %p '%s'",src,GST_OBJECT_NAME(src),dst,GST_OBJECT_NAME(dst));
  }
  if(!machines[PART_TEE]) {
    if(!bt_wire_make_internal_element(self,PART_TEE,"tee","tee")) return(FALSE);
    GST_DEBUG("created tee element for wire : %p '%s' -> %p '%s'",src,GST_OBJECT_NAME(src),dst,GST_OBJECT_NAME(dst));
  }
  if(!machines[PART_GAIN]) {
    if(!bt_wire_make_internal_element(self,PART_GAIN,"volume","gain")) return(FALSE);
    GST_DEBUG("created volume-gain element for wire : %p '%s' -> %p '%s'",src,GST_OBJECT_NAME(src),dst,GST_OBJECT_NAME(dst));
  }
  
  g_object_get(dst,"machine",&dst_machine,NULL);
  if((pad=gst_element_get_static_pad(dst_machine,"sink"))) {
    // this does not work for unlinked pads
    // @todo: if we link multiple machines to one, we could cache this
    // it seems to be responsible for ~14% of the loading time
    // - checking template caps it not enough - as the machine could have
    //   negotiated to mono
    // - would it make sense to quickly check template caps to exclude some cases?
    if((caps=gst_pad_get_allowed_caps(pad))) {
    //if((caps=gst_pad_get_caps(pad))) {
      GstStructure *structure;
      gint channels=1;
      
      if(GST_CAPS_IS_SIMPLE(caps)) {
        structure = gst_caps_get_structure(caps,0);
        
        if((gst_structure_get_int(structure,"channels",&channels))) {
          GST_DEBUG("channels on wire.dst=%d",channels);
        }
      }
      else {
        gint c,i,size=gst_caps_get_size(caps);
        
        for(i=0;i<size;i++) {
          if((structure=gst_caps_get_structure(caps,i))) {
            if(gst_structure_get_int(structure,"channels",&c)) {
              if(c>channels) channels=c;
            }
          }
        }
        GST_DEBUG("channels on wire.dst=%d, (checked %d caps)",channels, size);
      }
      if(channels==2) {
        /* insert panorama */
        GST_DEBUG_OBJECT(self,"adding panorama/balance");
        if(!machines[PART_PAN]) {
          if(!bt_wire_make_internal_element(self,PART_PAN,"audiopanorama","pan")) return(FALSE);
          g_object_set(G_OBJECT (machines[PART_PAN]),"method",1,NULL);
        }
      }
      else {
        GST_DEBUG("channels on wire.dst=1, no panorama/balance needed");
        if(machines[PART_PAN]) {
           GST_WARNING_OBJECT(self,"releasing panorama/balance");
           gst_bin_remove(GST_BIN(self),machines[PART_PAN]);
           machines[PART_PAN]=NULL;
        }
      }
      gst_caps_unref(caps);
    }
    else {
      GST_INFO("empty caps :(");
    }
    dst_caps=gst_pad_get_pad_template_caps(pad);
    gst_object_unref(pad);
  }
  gst_object_unref(dst_machine);

  g_object_get(src,"machine",&src_machine,NULL);
  if((pad=gst_element_get_static_pad(src_machine,"src"))) {
    const GstCaps *src_caps=gst_pad_get_pad_template_caps(pad);
#if GST_CHECK_VERSION(0,10,25)
    skip_convert=gst_caps_can_intersect(bt_default_caps, src_caps);
    if(skip_convert) {
      skip_convert&=gst_caps_can_intersect(dst_caps, src_caps);
    }
#else
    GstCaps *c=gst_caps_intersect(bt_default_caps, src_caps);
    skip_convert=!(c && gst_caps_is_empty(c));
    gst_caps_unref(c);
    if(skip_convert) {
      c=gst_caps_intersect(dst_caps, src_caps);
      skip_convert&=!(c && gst_caps_is_empty(c));
      gst_caps_unref(c);
    }
#endif
    gst_object_unref(pad);
  }
  gst_object_unref(src_machine);
  
  GST_DEBUG("trying to link machines : %p '%s' -> %p '%s'",src,GST_OBJECT_NAME(src),dst,GST_OBJECT_NAME(dst));
  if(!skip_convert) {
    GST_WARNING_OBJECT(self,"adding converter");
    if(!machines[PART_CONVERT]) {
      bt_wire_make_internal_element(self,PART_CONVERT,"audioconvert","audioconvert");
      // this is off by default anyway
      //g_assert(machines[PART_CONVERT]!=NULL);
      //g_object_set(machines[PART_CONVERT],"dithering",0,"noise-shaping",0,NULL);
    }
  } else {
    if(machines[PART_CONVERT]) {
      GST_WARNING_OBJECT(self,"releasing converter");
      gst_bin_remove(GST_BIN(self),machines[PART_CONVERT]);
      machines[PART_CONVERT]=NULL;
    }
  }
  res=bt_wire_link_elements(self,src_pads[PART_QUEUE],sink_pads[PART_TEE]);
  res&=bt_wire_link_elements(self,src_pads[PART_TEE],sink_pads[PART_GAIN]);
  if(res) {
    six=PART_QUEUE;
    if(machines[PART_PAN]) {
      GST_DEBUG("trying to link machines with pan");
      if(skip_convert) {
        res=bt_wire_link_elements(self,src_pads[PART_GAIN],sink_pads[PART_PAN]);
        if(!res) {
          gst_pad_unlink(src_pads[PART_GAIN],sink_pads[PART_PAN]);
          GST_WARNING("failed to link machines with pan and without convert");
        }
        else {
          dix=PART_PAN;
          GST_DEBUG("  wire okay with pan and without convert");
        }
      }
      else {
        res=bt_wire_link_elements(self,src_pads[PART_GAIN],sink_pads[PART_CONVERT]);
        res&=bt_wire_link_elements(self,src_pads[PART_CONVERT],sink_pads[PART_PAN]);
        if(!res) {
          gst_pad_unlink(src_pads[PART_GAIN],sink_pads[PART_CONVERT]);
          gst_pad_unlink(src_pads[PART_CONVERT],sink_pads[PART_PAN]);
          GST_WARNING("failed to link machines with pan and with convert");
        }
        else {
          dix=PART_PAN;
          GST_DEBUG("  wire okay with pan and with convert");
        }
      }
    }
    else {
      GST_DEBUG("trying to link machines without pan");
      if(skip_convert) {
        dix=PART_GAIN;
        GST_DEBUG("  wire okay without pan and with convert");
      }
      else {
        res=bt_wire_link_elements(self,src_pads[PART_GAIN],sink_pads[PART_CONVERT]);
        if(!res) {
          gst_pad_unlink(src_pads[PART_GAIN],sink_pads[PART_CONVERT]);
          GST_WARNING("failed to link machines without pan and without convert");
        }
        else {
          dix=PART_CONVERT;
          GST_DEBUG("  wire okay without pan and without convert");
        }
      }
    }
  }
  if(res) {
    // update ghostpads
    GST_INFO ("updating sink ghostpad : elem=%p (ref_ct=%d),'%s', pad=%p (ref_ct=%d)",
      machines[six],(G_OBJECT_REF_COUNT(machines[six])),GST_OBJECT_NAME(machines[six]),
      sink_pads[six],(G_OBJECT_REF_COUNT(sink_pads[six])));
    if(!gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->sink_pad),sink_pads[six])) {
      GST_WARNING("failed to link internal pads for sink ghostpad");
    }

    GST_INFO ("updating src ghostpad : elem=%p (ref_ct=%d),'%s', pad=%p (ref_ct=%d)",
      machines[dix],(G_OBJECT_REF_COUNT(machines[dix])),GST_OBJECT_NAME(machines[dix]),
      src_pads[dix],(G_OBJECT_REF_COUNT(src_pads[dix])));
    if(!gst_ghost_pad_set_target(GST_GHOST_PAD(self->priv->src_pad),src_pads[dix])) {
      GST_WARNING("failed to link internal pads for src ghostpad");
    }
    /* we link the wire to the machine in setup as needed */
  }
  else {
    GST_INFO("failed to link the machines");
    gst_pad_unlink(src_pads[PART_QUEUE],sink_pads[PART_TEE]);
    gst_pad_unlink(src_pads[PART_TEE],sink_pads[PART_GAIN]);
    // print out the content of both machines (using GST_DEBUG)
    bt_machine_dbg_print_parts(src);
    bt_wire_dbg_print_parts(self);
    bt_machine_dbg_print_parts(dst);
  }
  return(res);
}

/*
 * bt_wire_unlink_machines:
 * @self: the wire that should be used for this connection
 *
 * Unlinks gst-element of this wire and removes the conversion elements from the
 * song.
 */
static void bt_wire_unlink_machines(const BtWire * const self) {
  GstElement ** const machines=self->priv->machines;
  GstPad ** const src_pads=self->priv->src_pads;
  GstPad ** const sink_pads=self->priv->sink_pads;

  g_assert(BT_IS_WIRE(self));

  // check if wire has been properly initialized
  if(self->priv->src && self->priv->dst && machines[PART_QUEUE] && machines[PART_TEE] && machines[PART_GAIN]) {
    GST_DEBUG("unlink machines '%s' -> '%s'",GST_OBJECT_NAME(self->priv->src),GST_OBJECT_NAME(self->priv->dst));
    
    gst_pad_unlink(src_pads[PART_QUEUE],sink_pads[PART_TEE]);
    gst_pad_unlink(src_pads[PART_TEE],sink_pads[PART_GAIN]);
    if(machines[PART_CONVERT]) {
      if(machines[PART_PAN]) {
        gst_pad_unlink(src_pads[PART_GAIN],sink_pads[PART_CONVERT]);
        gst_pad_unlink(src_pads[PART_CONVERT],sink_pads[PART_PAN]);
      }
      else {
        gst_pad_unlink(src_pads[PART_GAIN],sink_pads[PART_CONVERT]);
      }
    }
    else {
      if(machines[PART_PAN]) {
        gst_pad_unlink(src_pads[PART_GAIN],sink_pads[PART_PAN]);
      }
    }
  }
}

/*
 * bt_wire_connect:
 * @self: the wire that should be used for this connection
 *
 * Connect two machines with a wire. The source and destination machine must be
 * passed upon construction.
 * Each machine can be involved in multiple connections. The method
 * transparently add spreader or adder elements in such cases. It further
 * inserts elements for data-type conversion if necessary.
 *
 * Same way the resulting wire should be added to the setup using #bt_setup_add_wire().
 *
 * Returns: true for success
 */
static gboolean bt_wire_connect(const BtWire * const self) {
  gboolean res=FALSE;
  BtWire *other_wire;
  BtMachine * const src=self->priv->src;
  BtMachine * const dst=self->priv->dst;
  gchar *name,*src_name,*dst_name;

  g_assert(BT_IS_WIRE(self));

  // move this to connect?
  if((!src) || (!dst)) {
    GST_WARNING("trying to add create wire with NULL endpoint(s), src=%p and dst=%p",src,dst);
    return(res);
  }
  
  GST_DEBUG("self=%p, src->refs=%d, dst->refs=%d",self,G_OBJECT_REF_COUNT(src),G_OBJECT_REF_COUNT(dst));
  GST_DEBUG("trying to link machines : %p '%s' [%p %p] -> %p '%s' [%p %p]",
    src,GST_OBJECT_NAME(src),src->src_wires, src->dst_wires,
    dst,GST_OBJECT_NAME(dst),dst->dst_wires, dst->src_wires);


  /* check that we don't have such a wire yet */
  if(src->src_wires && dst->dst_wires) {
    if((other_wire=bt_machine_get_wire_by_dst_machine(src,dst)) && (other_wire!=self)) {
      GST_WARNING("trying to add create already existing wire: %p!=%p",other_wire,self);
      g_object_unref(other_wire);
      goto Error;
    }
    g_object_try_unref(other_wire);
  }
  if(src->dst_wires && dst->src_wires) {
    if((other_wire=bt_machine_get_wire_by_dst_machine(dst,src)) && (other_wire!=self)) {
      GST_WARNING("trying to add create already existing wire (reversed): %p!=%p",other_wire,self);
      g_object_unref(other_wire);
      goto Error;
    }
    g_object_try_unref(other_wire);
  }

  // name the wire
  g_object_get(src,"id",&src_name,NULL);
  g_object_get(dst,"id",&dst_name,NULL);
  name=g_strdup_printf("%s_%s",src_name,dst_name);
  gst_object_set_name(GST_OBJECT(self),name);
  GST_INFO("naming wire : %s",name);
  g_free(name);
  g_free(src_name);
  g_free(dst_name);

  // if there is already a wire from src && src has not yet an spreader (in use)
  if((other_wire=(src->src_wires?(BtWire*)(src->src_wires->data):NULL))) {
    if(!bt_machine_has_active_spreader(src)) {
      GST_DEBUG("  other wire from src found");
      bt_wire_unlink_machines(other_wire);
      // create spreader (if needed)
      if(!bt_machine_activate_spreader(src)) {
        g_object_unref(other_wire);
        goto Error;
      }
      // correct the link for the other wire
      if(!bt_wire_link_machines(other_wire)) {
        GST_ERROR("failed to re-link the machines after inserting internal spreader");goto Error;
      }
    }
  }

  // if there is already a wire to dst && dst has not yet an adder (in use)
  if((other_wire=(dst->dst_wires?(BtWire*)(dst->dst_wires->data):NULL))) {
    if(!bt_machine_has_active_adder(dst)) {
      GST_DEBUG("  other wire to dst found");
      bt_wire_unlink_machines(other_wire);
      // create adder (if needed)
      if(!bt_machine_activate_adder(dst)) {
        g_object_unref(other_wire);
        goto Error;
      }
      // correct the link for the other wire
      if(!bt_wire_link_machines(other_wire)) {
        GST_ERROR("failed to re-link the machines after inserting internal adder");goto Error;
      }
    }
  }

  GST_DEBUG("link prepared, src->refs=%d, dst->refs=%d",G_OBJECT_REF_COUNT(src),G_OBJECT_REF_COUNT(dst));

  if(!bt_wire_link_machines(self)) {
    GST_ERROR("linking machines failed : %p '%s' -> %p '%s'",src,GST_OBJECT_NAME(src),dst,GST_OBJECT_NAME(dst));
    goto Error;
  }
  else {
    // register params
    bt_wire_init_params(self);
  }
  GST_DEBUG("linking machines succeeded, src->refs=%d, dst->refs=%d",G_OBJECT_REF_COUNT(src),G_OBJECT_REF_COUNT(dst));

  res=TRUE;

Error:
  return(res);
}

//-- constructor methods

/**
 * bt_wire_new:
 * @song: the song the new instance belongs to
 * @src_machine: the data source (#BtSourceMachine or #BtProcessorMachine)
 * @dst_machine: the data sink (#BtSinkMachine or #BtProcessorMachine)
 * @err: inform about failed instance creation
 *
 * Create a new instance.
 * The new wire is automaticall added to a songs setup. You don't need to call
 * <code>#bt_setup_add_wire(setup,wire);</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWire *bt_wire_new(const BtSong * const song, const BtMachine * const src_machine, const BtMachine * const dst_machine, GError **err) {
  return(BT_WIRE(g_object_new(BT_TYPE_WIRE,"construction-error",err,"song",song,"src",src_machine,"dst",dst_machine,NULL)));
}

//-- methods

/**
 * bt_wire_reconnect:
 * @self: the wire to re-link
 *
 * Call this method after internal elements in a #BtMachine have changed, but
 * failed to link.
 *
 * Returns: %TRUE for success and %FALSE otherwise
 */
gboolean bt_wire_reconnect(BtWire * const self) {
  g_return_val_if_fail(BT_IS_WIRE(self),FALSE);

  // @todo: should not be needed anymore as we have ghostpads!
  GST_DEBUG("relinking machines '%s' -> '%s'",GST_OBJECT_NAME(self->priv->src),GST_OBJECT_NAME(self->priv->dst));
  bt_wire_unlink_machines(self);
  return(bt_wire_link_machines(self));
}

/**
 * bt_wire_add_wire_pattern:
 * @self: the wire to add the wire-pattern to
 * @pattern: the pattern that the wire-pattern is associated with
 * @wire_pattern: the new wire-pattern instance
 *
 * Add the supplied wire-pattern to the wire. This is automatically done by
 * #bt_wire_pattern_new().
 */
void bt_wire_add_wire_pattern(const BtWire * const self, const BtPattern * const pattern, const BtWirePattern * const wire_pattern) {
  g_hash_table_insert(self->priv->patterns,(gpointer)pattern,(gpointer)g_object_ref((gpointer)wire_pattern));
  // notify others that the pattern has been created
  g_signal_emit((gpointer)self,signals[PATTERN_CREATED_EVENT],0,wire_pattern);
}

/**
 * bt_wire_get_pattern:
 * @self: the wire that has the pattern
 * @pattern: the pattern that the wire-pattern is associated with
 *
 * Gets the wire-pattern that hold the automation data for this @wire.
 *
 * Returns: a reference to the wire-pattern, unref when done.
 */
BtWirePattern *bt_wire_get_pattern(const BtWire * const self, const BtPattern * const pattern) {
  BtWirePattern *wire_pattern;

  if((wire_pattern=g_hash_table_lookup(self->priv->patterns,pattern))) {
    g_object_ref(wire_pattern);
  }
  return(wire_pattern);
}

/**
 * bt_wire_get_param_index:
 * @self: the wire to search for the param
 * @name: the name of the param
 * @error: the location of an error instance to fill with a message, if an error occures
 *
 * Searches the list of registered param of a wire for a param of
 * the given name and returns the index if found.
 *
 * Returns: the index or sets error if it is not found and returns -1.
 */
glong bt_wire_get_param_index(const BtWire *const self, const gchar * const name, GError **error) {
  const gulong num_params=self->priv->num_params;
  glong ret=-1,i;
  gboolean found=FALSE;

  g_return_val_if_fail(BT_IS_WIRE(self),-1);
  g_return_val_if_fail(BT_IS_STRING(name),-1);
  g_return_val_if_fail(error == NULL || *error == NULL, -1);

  for(i=0;i<num_params;i++) {
    if(!strcmp(WIRE_PARAM_NAME(i),name)) {
      ret=i;
      found=TRUE;
      break;
    }
  }
  if(!found) {
    GST_WARNING("wire param for name %s not found", name);
    if(error) {
      g_set_error (error, error_domain, /* errorcode= */0,
                  "wire param for name %s not found", name);
    }
  }
  //g_assert((found || (error && *error)));
  g_assert(((found && (ret>=0)) || ((ret==-1) && ((error && *error) || !error))));
  return(ret);
}

/**
 * bt_wire_get_param_spec:
 * @self: the wire to search for the param
 * @index: the offset in the list of params
 *
 * Retrieves the parameter specification for the param
 *
 * Returns: the #GParamSpec for the requested param
 */
GParamSpec *bt_wire_get_param_spec(const BtWire * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_WIRE(self),NULL);
  g_return_val_if_fail(index<self->priv->num_params,NULL);

  return(self->priv->wire_props[index]);
}

/**
 * bt_wire_get_param_type:
 * @self: the wire to search for the param type
 * @index: the offset in the list of params
 *
 * Retrieves the GType of a param
 *
 * Returns: the requested GType
 */
GType bt_wire_get_param_type(const BtWire * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_WIRE(self),G_TYPE_INVALID);
  g_return_val_if_fail(index<self->priv->num_params,G_TYPE_INVALID);

  return(WIRE_PARAM_TYPE(index));
}

/**
 * bt_wire_get_param_name:
 * @self: the wire to get the param name from
 * @index: the offset in the list of params
 *
 * Gets the param name. Do not modify returned content.
 *
 * Returns: the requested name
 */
const gchar *bt_wire_get_param_name(const BtWire * const self, const gulong index) {
  g_return_val_if_fail(BT_IS_WIRE(self),NULL);
  g_return_val_if_fail(index<self->priv->num_params,NULL);

  return(WIRE_PARAM_NAME(index));
}

/**
 * bt_wire_get_param_details:
 * @self: the wire to search for the param details
 * @index: the offset in the list of params
 * @pspec: place for the param spec
 * @min_val: place to hold new GValue with minimum
 * @max_val: place to hold new GValue with maximum 
 *
 * Retrieves the details of a voice param. Any detail can be %NULL if its not
 * wanted.
 */
void bt_wire_get_param_details(const BtWire * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val) {
  GParamSpec *property=bt_wire_get_param_spec(self,index);

  if(pspec) {
    *pspec=property;
  }
  if(min_val || max_val) {
    GType base_type=bt_g_type_get_base_type(property->value_type);

    if(min_val) {
      *min_val=g_new0(GValue,1);
      g_value_init(*min_val,property->value_type);
    }
    if(max_val) {
      *max_val=g_new0(GValue,1);
      g_value_init(*max_val,property->value_type);
    }
    switch(base_type) {
      case G_TYPE_BOOLEAN:
        if(min_val) g_value_set_boolean(*min_val,0);
        if(max_val) g_value_set_boolean(*max_val,0);
      break;
      case G_TYPE_INT: {
        const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
        if(min_val) g_value_set_int(*min_val,int_property->minimum);
        if(max_val) g_value_set_int(*max_val,int_property->maximum);
      } break;
      case G_TYPE_UINT: {
        const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
        if(min_val) g_value_set_uint(*min_val,uint_property->minimum);
        if(max_val) g_value_set_uint(*max_val,uint_property->maximum);
      } break;
      case G_TYPE_FLOAT: {
        const GParamSpecFloat *float_property=G_PARAM_SPEC_FLOAT(property);
        if(min_val) g_value_set_float(*min_val,float_property->minimum);
        if(max_val) g_value_set_float(*max_val,float_property->maximum);
      } break;
      case G_TYPE_DOUBLE: {
        const GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);
        if(min_val) g_value_set_double(*min_val,double_property->minimum);
        if(max_val) g_value_set_double(*max_val,double_property->maximum);
      } break;
      case G_TYPE_ENUM: {
        const GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
        const GEnumClass *enum_class=enum_property->enum_class;
        if(min_val) g_value_set_enum(*min_val,enum_class->minimum);
        if(max_val) g_value_set_enum(*max_val,enum_class->maximum);
      } break;
      default:
        GST_ERROR("unsupported GType=%lu:'%s' ('%s')",(gulong)property->value_type,g_type_name(property->value_type),g_type_name(base_type));
    }
  }
}

/**
 * bt_wire_controller_change_value:
 * @self: the wire to change the param for
 * @param: the parameter index
 * @timestamp: the time stamp of the change
 * @value: the new value or %NULL to unset a previous one
 *
 * Depending on wheter the given value is NULL, sets or unsets the controller
 * value for the specified param and at the given time.
 */
/* @todo: we have no default value handling here (see machine)
 */
void bt_wire_controller_change_value(const BtWire * const self, const gulong param, const GstClockTime timestamp, GValue * const value) {
  GObject *param_parent=NULL;
  gchar *param_name;
  GstInterpolationControlSource *cs;

  g_return_if_fail(BT_IS_WIRE(self));
  g_return_if_fail(param<self->priv->num_params);

  switch(param) {
    case 0:
      param_parent=G_OBJECT(self->priv->machines[PART_GAIN]);
      break;
    case 1:
      param_parent=G_OBJECT(self->priv->machines[PART_PAN]);
      break;
    default:
      GST_WARNING("unimplemented wire param");
      break;
  }
  param_name=WIRE_PARAM_NAME(param);
  cs=self->priv->wire_control_sources[param];

  if(value) {
    gboolean add=TRUE;

    // check if the property is alredy controlled
    if(cs && gst_interpolation_control_source_get_count(cs)) {
      add=FALSE;
    }
    if(G_UNLIKELY(add)) {
      GstController *ctrl;
      
      if((ctrl=gst_object_control_properties(param_parent, param_name, NULL))) {
        cs=gst_interpolation_control_source_new();
        gst_controller_set_control_source(ctrl,param_name,GST_CONTROL_SOURCE(cs));
        // set interpolation mode depending on param type
        gst_interpolation_control_source_set_interpolation_mode(cs,GST_INTERPOLATE_NONE);
        self->priv->wire_control_sources[param]=cs;
      }

      // @todo: is this needed, we're in add=TRUE after all
      g_object_try_unref(self->priv->wire_controller[param]);
      self->priv->wire_controller[param]=ctrl;
    }
    //GST_INFO("set wire controller: %"GST_TIME_FORMAT" param %d:%s",GST_TIME_ARGS(timestamp),param,name);
    gst_interpolation_control_source_set(cs,timestamp,value);
  }
  else {
    if(self->priv->wire_controller[param]) {
      gboolean remove=TRUE;

      //GST_INFO("%s unset global controller: %"GST_TIME_FORMAT" param %d:%s",self->priv->id,GST_TIME_ARGS(timestamp),param,self->priv->global_names[param]);
      if(cs) {
        gst_interpolation_control_source_unset(cs,timestamp);
        if(gst_interpolation_control_source_get_count(cs)) {
          remove=FALSE;
        }
      }
      if(remove) {
        gst_controller_set_control_source(self->priv->wire_controller[param],param_name,NULL);
        g_object_unref(cs);
        self->priv->wire_control_sources[param]=NULL;
        gst_object_uncontrol_properties(param_parent, param_name, NULL);
      }
    }
  }
}

//-- debug helper

GList *bt_wire_get_element_list(const BtWire * const self) {
  GList *list=NULL;
  gulong i;

  for(i=0;i<PART_COUNT;i++) {
    if(self->priv->machines[i]) {
      list=g_list_append(list,self->priv->machines[i]);
    }
  }

  return(list);
}

void bt_wire_dbg_print_parts(const BtWire * const self) {
  gchar *sid=NULL, *did=NULL;

  if(self->priv->src) g_object_get(self->priv->src,"id",&sid,NULL);
  if(self->priv->dst) g_object_get(self->priv->dst,"id",&did,NULL);

  /* [Q T G C P] */
  GST_INFO("%s->%s [%s %s %s %s %s %s %s]", 
    safe_string(sid), safe_string(did),
    self->priv->machines[PART_TEE]?"Q":"q",
    self->priv->machines[PART_TEE]?"T":"t",
    self->priv->machines[PART_GAIN]?"G":"g",
    self->priv->machines[PART_CONVERT]?"C":"c",
    self->priv->machines[PART_PAN]?"P":"p"
  );
  g_free(sid);
  g_free(did);
}

//-- io interface

static xmlNodePtr bt_wire_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  const BtWire * const self = BT_WIRE(persistence);
  gchar * const id;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wire");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("wire"),NULL))) {
    gdouble gain;
    gfloat pan;
  
    g_object_get(self->priv->src,"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("src"),XML_CHAR_PTR(id));
    g_free(id);

    g_object_get(self->priv->dst,"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("dst"),XML_CHAR_PTR(id));
    g_free(id);

    // serialize gain and panorama
    g_object_get(self->priv->machines[PART_GAIN],"volume",&gain,NULL);
    xmlNewProp(node,XML_CHAR_PTR("gain"),XML_CHAR_PTR(bt_persistence_strfmt_double(gain)));
    if(self->priv->machines[PART_PAN]) {
       g_object_get(self->priv->machines[PART_PAN],"panorama",&pan,NULL);
       xmlNewProp(node,XML_CHAR_PTR("panorama"),XML_CHAR_PTR(bt_persistence_strfmt_double((gdouble)pan)));
    }
    
    if(g_hash_table_size(self->priv->properties)) {
      if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("properties"),NULL))) {
        if(!bt_persistence_save_hashtable(self->priv->properties,child_node)) goto Error;
      }
      else goto Error;
    }
    if(self->priv->patterns) {
      if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("wire-patterns"),NULL))) {
        GList *list=NULL;
        
        g_hash_table_foreach(self->priv->patterns,bt_persistence_collect_hashtable_entries,(gpointer)&list);
        bt_persistence_save_list(list,child_node);
        g_list_free(list);
      }
      else goto Error;
    }
  }
Error:
  return(node);
}

static BtPersistence *bt_wire_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  BtWire *self;
  BtPersistence *result;
  BtSetup *setup = NULL;
  xmlChar *src_id, *dst_id, *gain_str, *pan_str;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wire");
  g_assert(node);

  src_id=xmlGetProp(node,XML_CHAR_PTR("src"));
  dst_id=xmlGetProp(node,XML_CHAR_PTR("dst"));
  
  if(!persistence) {
    BtSong *song=NULL;
    BtMachine *src_machine,*dst_machine;
    gchar *param_name;

    // we need to get parameters from var_args (need to handle all baseclass params
    param_name=va_arg(var_args,gchar*);
    while(param_name) {
      if(!strcmp(param_name,"song")) {
        song=va_arg(var_args, gpointer);
      }
      else {
        GST_WARNING("unhandled argument: %s",param_name);
        break;
      }
      param_name=va_arg(var_args,gchar*);
    }

    g_object_get(song,"setup",&setup,NULL);
    src_machine=bt_setup_get_machine_by_id(setup,(gchar *)src_id);
    dst_machine=bt_setup_get_machine_by_id(setup,(gchar *)dst_id);
    
    self=bt_wire_new(song,src_machine,dst_machine,err);
    result=BT_PERSISTENCE(self);
    g_object_try_unref(src_machine);
    g_object_try_unref(dst_machine);
    if(*err!=NULL) {
      goto Error;
    }
  }
  else {
    self=BT_WIRE(persistence);
    result=BT_PERSISTENCE(self);
  }

  xmlFree(src_id);
  xmlFree(dst_id);
  
  if((gain_str=xmlGetProp(node,XML_CHAR_PTR("gain")))) {
    gdouble gain=g_ascii_strtod((gchar *)gain_str,NULL);
    g_object_set(self->priv->machines[PART_GAIN],"volume",gain,NULL);
    xmlFree(gain_str);
  }
  if(self->priv->machines[PART_PAN] && (pan_str=xmlGetProp(node,XML_CHAR_PTR("panorama")))) {
    gfloat pan=g_ascii_strtod((gchar *)pan_str,NULL);
    g_object_set(self->priv->machines[PART_PAN],"panorama",pan,NULL);
    xmlFree(pan_str);
  }
  
  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      if(!strncmp((gchar *)node->name,"properties\0",11)) {
        bt_persistence_load_hashtable(self->priv->properties,node);
      }
      /* "patterns" is legacy, we need "wire-patterns" as a node name to be unique */
      else if(!strncmp((gchar *)node->name,"wire-patterns\0",14) || !strncmp((gchar *)node->name,"patterns\0",9)) {
        BtWirePattern *wire_pattern;
  
        for(child_node=node->children;child_node;child_node=child_node->next) {
          if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"wire-pattern\0",13) || (!strncmp((char *)child_node->name,"pattern\0",8)))) {
            GError *err=NULL;
            
            wire_pattern=BT_WIRE_PATTERN(bt_persistence_load(BT_TYPE_WIRE_PATTERN,NULL,child_node,&err,"song",self->priv->song,"wire",self,NULL));
            if(err!=NULL) {
              GST_WARNING("Can't create wire-pattern: %s",err->message);
              g_error_free(err);
            }
            g_object_unref(wire_pattern);
          }
        }
      }
    }
  }

Done:
  g_object_try_unref(setup);
  return(result);
Error:
  goto Done;
}

static void bt_wire_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wire_persistence_load;
  iface->save = bt_wire_persistence_save;
}

//-- wrapper

//-- class internals

static void bt_wire_constructed(GObject *object) {
  BtWire * const self=BT_WIRE(object);
  BtSetup * const setup;

  if(G_OBJECT_CLASS(bt_wire_parent_class)->constructed)
    G_OBJECT_CLASS(bt_wire_parent_class)->constructed(object);

  if(BT_IS_SINK_MACHINE(self->priv->src))
    goto SrcIsSinkMachineError;
  if(BT_IS_SOURCE_MACHINE(self->priv->dst))
    goto SinkIsSrcMachineError;
  if(self->priv->src==self->priv->dst)
    goto SrcIsSinkError;
  
  GST_INFO("wire constructor, self->priv=%p, between %p and %p",self->priv,self->priv->src,self->priv->dst);
  if(self->priv->src && self->priv->dst) {
    if(!bt_wire_connect(self)) {
      goto ConnectError;
    }

    // add the wire to the setup of the song
    g_object_get((gpointer)(self->priv->song),"setup",&setup,NULL);
    bt_setup_add_wire(setup,self);
    g_object_unref(setup);
  }
  else {
    goto NoMachinesError;
  }
  return;
ConnectError:
  GST_WARNING("failed to connect wire");
  if(self->priv->constrution_error) {
    g_set_error(self->priv->constrution_error, error_domain, /* errorcode= */0,
               "failed to connect wire.");
  }
  return;
SrcIsSinkMachineError:
  GST_WARNING("src is sink-machine");
  if(self->priv->constrution_error) {
    g_set_error(self->priv->constrution_error, error_domain, /* errorcode= */0,
               "src is sink-machine.");
  }
  return;
SinkIsSrcMachineError:
  GST_WARNING("sink is src-machine");
  if(self->priv->constrution_error) {
    g_set_error(self->priv->constrution_error, error_domain, /* errorcode= */0,
               "sink is src-machine.");
  }
  return;
SrcIsSinkError:
  GST_WARNING("src and sink are the same machine");
  if(self->priv->constrution_error) {
    g_set_error(self->priv->constrution_error, error_domain, /* errorcode= */0,
               "src and sink are the same machine.");
  }
  return;
NoMachinesError:
  GST_WARNING("src and/or sink are NULL");
  if(self->priv->constrution_error) {
    g_set_error(self->priv->constrution_error, error_domain, /* errorcode= */0,
               "src=%p and/or sink=%p are NULL.",self->priv->src,self->priv->dst);
  }
  return; 
}

/* returns a property for the given property_id for this object */
static void bt_wire_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtWire *const self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_CONSTRUCTION_ERROR: {
      g_value_set_pointer(value, self->priv->constrution_error);
    } break;
    case WIRE_PROPERTIES: {
      g_value_set_pointer(value, self->priv->properties);
    } break;
    case WIRE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case WIRE_SRC: {
      g_value_set_object(value, self->priv->src);
    } break;
    case WIRE_DST: {
      g_value_set_object(value, self->priv->dst);
    } break;
    case WIRE_GAIN: {
      g_value_set_object(value, self->priv->machines[PART_GAIN]);
    } break;
    case WIRE_PAN: {
      g_value_set_object(value, self->priv->machines[PART_PAN]);
    } break;
    case WIRE_NUM_PARAMS: {
      g_value_set_ulong(value, self->priv->num_params);
    } break;
    case WIRE_ANALYZERS: {
      g_value_set_pointer(value, self->priv->analyzers);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wire_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtWire *const self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_CONSTRUCTION_ERROR: {
      self->priv->constrution_error=(GError **)g_value_get_pointer(value);
    } break;
    case WIRE_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wire: %p",self->priv->song);
    } break;
    case WIRE_SRC: {
      self->priv->src=BT_MACHINE(g_value_dup_object(value));
      GST_DEBUG("set the source element for the wire: %p",self->priv->src);
    } break;
    case WIRE_DST: {
      self->priv->dst=BT_MACHINE(g_value_dup_object(value));
      GST_DEBUG("set the target element for the wire: %p",self->priv->dst);
    } break;
    case WIRE_NUM_PARAMS: {
      self->priv->num_params = g_value_get_ulong(value);
    } break;
    case WIRE_ANALYZERS: {
      bt_wire_deactivate_analyzers(self);
      self->priv->analyzers=g_value_get_pointer(value);
      bt_wire_activate_analyzers(self);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_dispose(GObject * const object) {
  const BtWire *const self = BT_WIRE(object);
  guint i;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG_OBJECT(self,"!!!! self=%p",self);

  // unref controllers
  GST_DEBUG("  releasing controllers");
  if(self->priv->machines[PART_GAIN] && self->priv->wire_props[0]) {
    g_object_try_unref(self->priv->wire_control_sources[0]);
    //bt_gst_object_deactivate_controller(G_OBJECT(self->priv->machines[PART_GAIN]), WIRE_PARAM_NAME(0));
    self->priv->wire_controller[0]=NULL;
  }
  if(self->priv->machines[PART_PAN] && self->priv->wire_props[1]) {
    g_object_try_unref(self->priv->wire_control_sources[1]);
    //bt_gst_object_deactivate_controller(G_OBJECT(self->priv->machines[PART_PAN]), WIRE_PARAM_NAME(1));
    self->priv->wire_controller[1]=NULL;
  }
  
  // remove the GstElements from the bin
  // @todo: is this actually needed?
  bt_wire_unlink_machines(self); // removes helper elements if in use
  bt_wire_deactivate_analyzers(self);

  // unref the pads
  for(i=0;i<PART_COUNT;i++) {
    if(self->priv->src_pads[i]) {
      if(src_pn[i][0]!='\0') {
        gst_object_unref(self->priv->src_pads[i]);
      } else {
        gst_element_release_request_pad(self->priv->machines[i],self->priv->src_pads[i]);
        gst_object_unref(self->priv->src_pads[i]);
      }
    }
    if(self->priv->sink_pads[i])
      gst_object_unref(self->priv->sink_pads[i]);
  }

  // remove ghost pads
  gst_element_remove_pad(GST_ELEMENT(self),self->priv->src_pad);
  gst_element_remove_pad(GST_ELEMENT(self),self->priv->sink_pad);

  g_object_try_weak_unref(self->priv->song);
  //gstreamer uses floating references, therefore elements are destroyed, when removed from the bin
  GST_DEBUG("  releasing the dst %p, dst->ref_count=%d",
    self->priv->dst,(self->priv->dst?G_OBJECT_REF_COUNT(self->priv->dst):-1));
  g_object_try_unref(self->priv->dst);
  GST_DEBUG("  releasing the src %p, src->ref_count=%d",
    self->priv->src,(self->priv->src?G_OBJECT_REF_COUNT(self->priv->src):-1));
  g_object_try_unref(self->priv->src);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(bt_wire_parent_class)->dispose(object);
  GST_DEBUG("  done");
  
}

static void bt_wire_finalize(GObject * const object) {
  BtWire * const self = BT_WIRE(object);

  GST_DEBUG_OBJECT(self,"!!!! self=%p",self);

  g_hash_table_destroy(self->priv->properties);
  g_hash_table_destroy(self->priv->patterns);

  G_OBJECT_CLASS(bt_wire_parent_class)->finalize(object);
}

static void bt_wire_init(BtWire *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE, BtWirePrivate);
  self->priv->num_params = 1;
  self->priv->properties=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
  self->priv->patterns=g_hash_table_new_full(NULL,NULL,NULL,(GDestroyNotify)g_object_unref);

  self->priv->src_pad=gst_ghost_pad_new_no_target("src",GST_PAD_SRC);
  gst_element_add_pad(GST_ELEMENT(self),self->priv->src_pad);
  self->priv->sink_pad=gst_ghost_pad_new_no_target("sink",GST_PAD_SINK);
  gst_element_add_pad(GST_ELEMENT(self),self->priv->sink_pad);

  GST_DEBUG("!!!! self=%p",self);
}

static void bt_wire_class_init(BtWireClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass * const gstelement_klass = GST_ELEMENT_CLASS(klass);

  // g_type_qname() is ((TypeNode *)type)->qname;
  // seems to be save to call in _class_init
  //error_domain=g_quark_from_static_string("BtWire");
  error_domain=g_type_qname(BT_TYPE_WIRE);
  g_type_class_add_private(klass,sizeof(BtWirePrivate));

  gobject_class->constructed  = bt_wire_constructed;
  gobject_class->set_property = bt_wire_set_property;
  gobject_class->get_property = bt_wire_get_property;
  gobject_class->dispose      = bt_wire_dispose;
  gobject_class->finalize     = bt_wire_finalize;
  
  // disable deep notify
  {
    GObjectClass *parent_class=g_type_class_peek_static(G_TYPE_OBJECT);
    gobject_class->dispatch_properties_changed=parent_class->dispatch_properties_changed;
  }

  gst_element_class_add_pad_template(gstelement_klass, gst_static_pad_template_get(&wire_src_template));
  gst_element_class_add_pad_template(gstelement_klass, gst_static_pad_template_get(&wire_sink_template));

  /**
   * BtWire::pattern-created:
   * @self: the wire-pattern object that emitted the signal
   * @tick: the tick position inside the pattern
   * @param: the parameter index
   *
   * signals that a param of this wire-pattern has been changed
   */
  signals[PATTERN_CREATED_EVENT] = g_signal_new("pattern-created",
                                   G_TYPE_FROM_CLASS(klass),
                                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                   0,
                                   NULL, // accumulator
                                   NULL, // acc data
                                   g_cclosure_marshal_VOID__OBJECT,
                                   G_TYPE_NONE, // return type
                                   1, // n_params
                                   BT_TYPE_WIRE_PATTERN // param data
                                   );

  g_object_class_install_property(gobject_class,WIRE_CONSTRUCTION_ERROR,
                                  g_param_spec_pointer("construction-error",
                                     "construction error prop",
                                     "signal failed instance creation",
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_PROPERTIES,
                                  g_param_spec_pointer("properties",
                                     "properties prop",
                                     "list of wire properties",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_SONG,
                                  g_param_spec_object("song",
                                     "song construct prop",
                                     "the song object, the wire belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_SRC,
                                  g_param_spec_object("src",
                                     "src ro prop",
                                     "src machine object, the wire links to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_DST,
                                  g_param_spec_object("dst",
                                     "dst ro prop",
                                     "dst machine object, the wire links to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_GAIN,
                                  g_param_spec_object("gain",
                                     "gain element prop",
                                     "the gain element for the connection",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_PAN,
                                  g_param_spec_object("pan",
                                     "panorama element prop",
                                     "the panorama element for the connection",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_NUM_PARAMS,
                                  g_param_spec_ulong("num-params",
                                     "num-params prop",
                                     "number of params for the wire",
                                     0,
                                     BT_WIRE_MAX_NUM_PARAMS,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_ANALYZERS,
                                  g_param_spec_pointer("analyzers",
                                     "analyzers prop",
                                     "list of wire analyzers",
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

