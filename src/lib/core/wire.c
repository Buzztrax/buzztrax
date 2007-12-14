/* $Id: wire.c,v 1.121 2007-11-26 15:13:26 ensonic Exp $
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
 * Furthermore each wire has a volume element.
 */

/*
 * @todo try to derive this from GstBin!
 *  then put the machines into itself (and not into the songs bin, but insert the machine directly into the song->bin
 *  when adding internal machines we need to fix the ghost pads (this may require relinking)
 *    gst_element_add_ghost_pad and gst_element_remove_ghost_pad
 *  What if we can directly connect src->dst, then the wire can't be an element (nothing there to use as ghost-pads).
 *  On the other hand this is only the case, if we do not need converters and have no volume and no monitors.
 *
 * @todo: when connecting to several wires to one src, we need queue elements at the begin of the wire
 */

#define BT_CORE
#define BT_WIRE_C

#include <libbtcore/core.h>

//-- property ids

enum {
  WIRE_PROPERTIES=1,
  WIRE_SONG,
  WIRE_SRC,
  WIRE_DST,
  WIRE_GAIN,
  WIRE_NUM_PARAMS,
  WIRE_ANALYZERS
};

typedef enum {
  /* source element in the wire for convinience */
  PART_SRC=0,
  /* @todo: what about having a queue here to avoid blocking when src has a spreader */
  /* used to link analyzers */
  PART_TEE,
  /* volume-control element */
  PART_GAIN,
  /* wire format conversion elements */
  PART_CONVERT,
  /*PART_SCALE, unused right now */
  /*
  @todo: need pan/balance here, if either src/dst has channesl>1
         can we remove PART_CONVERT, GAIN doen convert the most formats
         and pan does the channels
  PART_PAN,
   */
  /* target element in the wire for convinience */
  PART_DST,
  /* how many elements are used */
  PART_COUNT
} BtWirePart;

/* volume, panorama */
#define MAX_NUM_PARAMS 2

struct _BtWirePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* (ui) properties accociated with this machine */
  GHashTable *properties;

  /* the song the wire belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* the main gstreamer container element */
  GstBin *bin;

  /* which machines are linked */
  BtMachine *src,*dst;

  /* volume gain, set machines[PART_GAIN] to passthrough when gain=1.0 */
  gdouble gain;

  /* the number of dynamic params the wire provides */
  gulong num_params;
  
  /* dynamic parameter control */
  GstController *wire_controller[MAX_NUM_PARAMS];

  /* event patterns */
  GHashTable *patterns;  // each entry points to BtWirePattern using BtPattern as a key

  /* the gstreamer elements that is used */
  GstElement *machines[PART_COUNT];

  /* wire analyzers */
  GList *analyzers;
};

static GObjectClass *bt_wire_parent_class=NULL;

/* this actualy produces two more relocations
static void bt_wire_persistence_interface_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtWire, bt_wire, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
    bt_wire_persistence_interface_init));
*/

//-- signal handler

#if 0
static void on_format_negotiated(GstPad *pad, GParamSpec *arg, gpointer user_data) {
  /* this does not work yet, gstreamer seems to not yet provide a way to double
   * renegotiation
  bt_machine_renegotiate_adder_format(BT_MACHINE(user_data),pad);
  */
}
#endif

//-- helper methods

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

  // add internal element
  gchar * const name=g_alloca(strlen(element_name)+16);g_sprintf(name,"%s_%p",element_name,self);
  if(!(self->priv->machines[part]=gst_element_factory_make(factory_name,name))) {
    GST_ERROR("failed to create %s",element_name);goto Error;
  }
  gst_bin_add(self->priv->bin,self->priv->machines[part]);
  res=TRUE;
Error:
  return(res);
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

    if(!(res=gst_bin_add(self->priv->bin,next))) {
      GST_INFO("cannot add element \"%s\" to bin",gst_element_get_name(next));
    }
    if(prev) {
      if(!(res=gst_element_link(prev,next))) {
        GST_INFO("cannot link elements \"%s\" -> \"%s\"",gst_element_get_name(prev),gst_element_get_name(next));
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
      GST_INFO("cannot sync state for elements \"%s\"",gst_element_get_name(next));
    }
  }  
  GST_INFO("linking to tee");
  if(GST_PAD_LINK_FAILED(gst_pad_link(tee_src,analyzer_sink))) {
  //if(!(res=gst_element_link(self->priv->machines[PART_TEE],GST_ELEMENT(self->priv->analyzers->data)))) {
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
      GST_INFO("cannot set state to NULL for element '%s', ret='%s'",gst_element_get_name(next),gst_element_state_change_return_get_name(state_ret));
    }
    gst_element_unlink(prev,next);
    prev=next;
  }
  for(node=self->priv->analyzers;(node && res);node=g_list_next(node)) {
    next=GST_ELEMENT(node->data);
    if(!(res=gst_bin_remove(self->priv->bin,next))) {
      GST_INFO("cannot remove element '%s' from bin",gst_element_get_name(next));
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
 * bt_wire_change_gain:
 * @self: the wire for which the gain to change
 *
 * Updates the wire gain control. Bypasses gain control, if gain is very close
 * to 100%.
 */
static void bt_wire_change_gain(const BtWire * const self) {
  const gboolean passthrough=(fabs(self->priv->gain-1.0)<0.001);

  g_object_set(self->priv->machines[PART_GAIN],"volume",self->priv->gain,NULL);
  gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self->priv->machines[PART_GAIN]),passthrough);
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
  const BtMachine * const src = self->priv->src;
  const BtMachine * const dst = self->priv->dst;
  GstElement ** const machines=self->priv->machines;

  g_assert(BT_IS_WIRE(self));

  g_assert(GST_IS_OBJECT(src->src_elem));
  g_assert(GST_IS_OBJECT(dst->dst_elem));

  if(!self->priv->machines[PART_GAIN]) {
    if(!bt_wire_make_internal_element(self,PART_GAIN,"volume","gain")) return(FALSE);
    bt_wire_change_gain(self);
    GST_DEBUG("created volume-gain element for wire : %p '%s' -> %p '%s'",src->src_elem,GST_OBJECT_NAME(src->src_elem),dst->dst_elem,GST_OBJECT_NAME(dst->dst_elem));
  }

  if(!self->priv->machines[PART_TEE]) {
    if(!bt_wire_make_internal_element(self,PART_TEE,"tee","tee")) return(FALSE);
    GST_DEBUG("created tee element for wire : %p '%s' -> %p '%s'",src->src_elem,GST_OBJECT_NAME(src->src_elem),dst->dst_elem,GST_OBJECT_NAME(dst->dst_elem));
  }

  /* @todo: if dst.channels==2 add panorama
   * pad = gst_element_get_static_pad(dst.machines[PART_CAPS_FILTER],"sink");
   * caps = gst_pad_get_pad_template_caps(pad);
   * structure = gst_caps_get_structure(caps)
   * gst_structure_get_int (structure, "channels", &channels);
   */
  
  GST_DEBUG("trying to link machines directly : %p '%s' -> %p '%s'",src->src_elem,GST_OBJECT_NAME(src->src_elem),dst->dst_elem,GST_OBJECT_NAME(dst->dst_elem));
  // try link src to dst {directly, with convert, with scale, with ...}
  /* @todo: if we try linking without audioconvert and this links to an adder,
   * then the first link enforces the format (if first is mono and later stereo
   * sugnal is linked, this is downgraded).
   * Right now we need to do this, because of http://bugzilla.gnome.org/show_bug.cgi?id=418982
   */
  /*
  if(!gst_element_link_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], dst->dst_elem, NULL)) {
    gst_element_unlink_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], dst->dst_elem, NULL);
  */
    if(!machines[PART_CONVERT]) {
      bt_wire_make_internal_element(self,PART_CONVERT,"audioconvert","audioconvert");
      g_object_set(machines[PART_CONVERT],"dithering",0,"noise-shaping",0,NULL);
      g_assert(machines[PART_CONVERT]!=NULL);
    }
    GST_DEBUG("trying to link machines with convert");
    if(!gst_element_link_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_CONVERT], dst->dst_elem, NULL)) {
      gst_element_unlink_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_CONVERT], dst->dst_elem, NULL);
			/*
      if(!machines[PART_SCALE]) {
        bt_wire_make_internal_element(self,PART_SCALE,"audioresample","audioresample");
        g_assert(machines[PART_SCALE]!=NULL);
      }
      gst_bin_add(self->priv->bin, machines[PART_SCALE]);
      GST_DEBUG("trying to link machines with resample");
      if(!gst_element_link_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_SCALE], dst->dst_elem, NULL)) {
        gst_element_unlink_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_SCALE], dst->dst_elem, NULL);
        GST_DEBUG("trying to link machines with convert and resample");
        if(!gst_element_link_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_CONVERT], machines[PART_SCALE], dst->dst_elem, NULL)) {
          gst_element_unlink_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_CONVERT], machines[PART_SCALE], dst->dst_elem, NULL);
          GST_DEBUG("trying to link machines with scale and convert");
          if(!gst_element_link_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_SCALE], machines[PART_CONVERT], dst->dst_elem, NULL)) {
            gst_element_unlink_many(src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_SCALE], machines[PART_CONVERT], dst->dst_elem, NULL);
			*/
            GST_DEBUG("failed to link the machines");
            // print out the content of both machines (using GST_DEBUG)
            bt_machine_dbg_print_parts(src);bt_machine_dbg_print_parts(dst);
            res=FALSE;
			/*
          }
          else {
            machines[PART_SRC]=machines[PART_TEE];
            machines[PART_DST]=machines[PART_SCALE];
            GST_DEBUG("  wire okay with scale and convert");
          }
        }
        else {
          machines[PART_SRC]=machines[PART_TEE];
          machines[PART_DST]=machines[PART_CONVERT];
          GST_DEBUG("  wire okay with convert and scale");
        }
      }
      else {
        machines[PART_SRC]=machines[PART_TEE];
        machines[PART_DST]=machines[PART_SCALE];
        GST_DEBUG("  wire okay with scale");
      }
			*/
    }
    else {
      machines[PART_SRC]=machines[PART_TEE];
      machines[PART_DST]=machines[PART_CONVERT];
      GST_DEBUG("  wire okay with convert");
    }
  /*}
  else {
    machines[PART_SRC]=machines[PART_TEE];
    machines[PART_DST]=machines[PART_GAIN];
    GST_DEBUG("  wire okay");
  }
  */
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
  GstStateChangeReturn res;

  g_assert(BT_IS_WIRE(self));

  // check if wire has been properly initialized
  if(self->priv->src->src_elem && self->priv->dst->dst_elem && machines[PART_TEE] && machines[PART_GAIN]) {
    GstPad *dst_pad=NULL,*src_pad=NULL;

    src_pad=bt_wire_get_sink_peer_pad(machines[PART_TEE]);

    GST_DEBUG("unlink machines '%s' -> '%s'",GST_OBJECT_NAME(self->priv->src->src_elem),GST_OBJECT_NAME(self->priv->dst->dst_elem));
    /*if(machines[PART_CONVERT] && machines[PART_SCALE]) {
      gst_element_unlink_many(self->priv->src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_CONVERT], machines[PART_SCALE], self->priv->dst->dst_elem, NULL);
    }
    else*/
    if(machines[PART_CONVERT]) {
      dst_pad=bt_wire_get_src_peer_pad(machines[PART_CONVERT]);
      gst_element_unlink_many(self->priv->src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_CONVERT], self->priv->dst->dst_elem, NULL);
    }
    /*
    else if(machines[PART_SCALE]) {
      dst_pad=bt_wire_get_src_peer_pad(machines[PART_SCALE]);
      gst_element_unlink_many(self->priv->src->src_elem, machines[PART_TEE], machines[PART_GAIN], machines[PART_SCALE], self->priv->dst->dst_elem, NULL);
    }
    */
    else {
      dst_pad=bt_wire_get_src_peer_pad(machines[PART_GAIN]);
      gst_element_unlink_many(self->priv->src->src_elem, machines[PART_TEE], machines[PART_GAIN], self->priv->dst->dst_elem, NULL);
    }
    GST_DEBUG("request pads: adder %s,%s  tee %s,%s",
      GST_DEBUG_PAD_NAME(dst_pad),GST_DEBUG_PAD_NAME(src_pad));
    if(dst_pad && bt_machine_has_active_adder(self->priv->dst)) {
      // remove request-pad
      GST_INFO("releasing request pad for dst-adder");
      gst_element_release_request_pad(self->priv->dst->dst_elem,dst_pad);
      gst_object_unref(dst_pad);
    }
    if(src_pad && bt_machine_has_active_spreader(self->priv->src)) {
      // remove request-pad
      GST_INFO("releasing request pad for src-spreader");
      gst_element_release_request_pad(self->priv->src->src_elem,src_pad);
      gst_object_unref(src_pad);
    }
  }
  if(machines[PART_CONVERT]) {
    GST_DEBUG("  removing convert from bin, obj->ref_count=%d",G_OBJECT(machines[PART_CONVERT])->ref_count);
    if((res=gst_element_set_state(self->priv->machines[PART_CONVERT],GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
      GST_WARNING("can't go to null state");
    else
      GST_DEBUG("->NULL state change returned %d",res);
    gst_bin_remove(self->priv->bin, machines[PART_CONVERT]);
    machines[PART_CONVERT]=NULL;
  }
  /*
  if(self->priv->machines[PART_SCALE]) {
    GST_DEBUG("  removing scale from bin, obj->ref_count=%d",G_OBJECT(machines[PART_SCALE])->ref_count);
    if((res=gst_element_set_state(self->priv->machines[PART_SCALE],GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
      GST_WARNING("can't go to null state");
    else
      GST_DEBUG("->NULL state change returned %d",res);
    gst_bin_remove(self->priv->bin, machines[PART_SCALE]);
    machines[PART_SCALE]=NULL;
  }
  */
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
  const BtSong * const song=self->priv->song;
  BtSetup * const setup;
  BtWire *other_wire;
  BtMachine * const src=self->priv->src;
  BtMachine * const dst=self->priv->dst;
  const GstElement *old_peer;
  //GstPad *sink_pad;

  g_assert(BT_IS_WIRE(self));

  // move this to connect?
  if((!self->priv->src) || (!self->priv->dst)) {
    GST_WARNING("trying to add create wire with NULL endpoint, src=%p and dst=%p",self->priv->src,self->priv->dst);
    return(res);
  }

  g_object_get(G_OBJECT(song),"bin",&self->priv->bin,"setup",&setup,NULL);

  if((other_wire=bt_setup_get_wire_by_machines(setup,self->priv->src,self->priv->dst)) && (other_wire!=self)) {
    GST_WARNING("trying to add create already existing wire: %p!=%p",other_wire,self);
    g_object_unref(other_wire);
    goto Error;
  }
  g_object_try_unref(other_wire);
  if((other_wire=bt_setup_get_wire_by_machines(setup,self->priv->dst,self->priv->src)) && (other_wire!=self)) {
    GST_WARNING("trying to add create already existing wire (reversed): %p!=%p",other_wire,self);
    g_object_unref(other_wire);
    goto Error;
  }
  g_object_try_unref(other_wire);

  GST_DEBUG("bin->refs=%d, src->refs=%d, dst->refs=%d",G_OBJECT(self->priv->bin)->ref_count,G_OBJECT(src)->ref_count,G_OBJECT(dst)->ref_count);
  GST_DEBUG("trying to link machines : %p '%s' -> %p '%s'",src->src_elem,GST_OBJECT_NAME(src->src_elem),dst->dst_elem,GST_OBJECT_NAME(dst->dst_elem));

  // if there is already a connection from src && src has not yet an spreader (in use)
  if((other_wire=bt_setup_get_wire_by_src_machine(setup,src))
    && !bt_machine_has_active_spreader(src)) {
    GST_DEBUG("  other wire from src found");
    bt_wire_unlink_machines(other_wire);
    // create spreader (if needed)
    old_peer=src->src_elem;
    if(!bt_machine_activate_spreader(src)) {
      g_object_unref(other_wire);
      goto Error;
    }
    // if there is no conversion element on the wire ..
    if(other_wire->priv->machines[PART_SRC]==old_peer) {
      // we need to fix the src_elem of the other connect, as we have inserted the spreader
      other_wire->priv->machines[PART_SRC]=src->src_elem;
    }
    // correct the link for the other wire
    if(!bt_wire_link_machines(other_wire)) {
      GST_ERROR("failed to re-link the machines after inserting internal spreader");goto Error;
    }
    g_object_unref(other_wire);
  }

  // if there is already a wire to dst and dst has not yet an adder (in use)
  if((other_wire=bt_setup_get_wire_by_dst_machine(setup,dst))
    && !bt_machine_has_active_adder(dst)) {
    GST_DEBUG("  other wire to dst found");
    bt_wire_unlink_machines(other_wire);
    // create adder (if needed)
    old_peer=dst->dst_elem;
    if(!bt_machine_activate_adder(dst)) {
      g_object_unref(other_wire);
      goto Error;
    }
    // if there is no conversion element on the wire ..
    if(other_wire->priv->machines[PART_DST]==old_peer) {
      // we need to fix the dst_elem of the other connect, as we have inserted the adder
      other_wire->priv->machines[PART_DST]=dst->dst_elem;
    }
    // correct the link for the other wire
    if(!bt_wire_link_machines(other_wire)) {
      GST_ERROR("failed to re-link the machines after inserting internal adder");goto Error;
    }
    g_object_unref(other_wire);
  }

  GST_DEBUG("link prepared, bin->refs=%d, src->refs=%d, dst->refs=%d",G_OBJECT(self->priv->bin)->ref_count,G_OBJECT(src)->ref_count,G_OBJECT(dst)->ref_count);

  if(!bt_wire_link_machines(self)) {
    GST_ERROR("linking machines failed");goto Error;
  }
  GST_DEBUG("linking machines succeeded, bin->refs=%d, src->refs=%d, dst->refs=%d",G_OBJECT(self->priv->bin)->ref_count,G_OBJECT(src)->ref_count,G_OBJECT(dst)->ref_count);

  // needed for the adder format negotiation
#if 0
  if((sink_pad=gst_element_get_pad(self->priv->machines[PART_SRC],"sink"))) {
    g_signal_connect(sink_pad,"notify::caps",G_CALLBACK(on_format_negotiated),(gpointer)self->priv->dst);
    gst_object_unref(sink_pad);
  }
#endif
  //bt_machine_renegotiate_adder_format(self->priv->dst);

  res=TRUE;

Error:
  g_object_unref(setup);
  return(res);
}

//-- constructor methods

/**
 * bt_wire_new:
 * @song: the song the new instance belongs to
 * @src_machine: the data source (#BtSourceMachine or #BtProcessorMachine)
 * @dst_machine: the data sink (#BtSinkMachine or #BtProcessorMachine)
 *
 * Create a new instance.
 * The new wire is automaticall added to a songs setup. You don't need to call
 * <code>#bt_setup_add_wire(setup,wire);</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWire *bt_wire_new(const BtSong * const song, const BtMachine * const src_machine, const BtMachine * const dst_machine) {
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(src_machine),NULL);
  g_return_val_if_fail(!BT_IS_SINK_MACHINE(src_machine),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(dst_machine),NULL);
  g_return_val_if_fail(!BT_IS_SOURCE_MACHINE(dst_machine),NULL);
  g_return_val_if_fail(src_machine!=dst_machine,NULL);

  GST_INFO("create wire between %p and %p",src_machine,dst_machine);

  return(BT_WIRE(g_object_new(BT_TYPE_WIRE,"song",song,"src",src_machine,"dst",dst_machine,NULL)));
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

  GST_DEBUG("relinking machines '%s' -> '%s'",GST_OBJECT_NAME(self->priv->src->src_elem),GST_OBJECT_NAME(self->priv->dst->dst_elem));
  bt_wire_unlink_machines(self);
  return(bt_wire_link_machines(self));
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
BtWirePattern *bt_wire_get_pattern(const BtWire * const self, BtPattern *pattern) {
  BtWirePattern *wire_pattern;

  if(!(wire_pattern=g_hash_table_lookup(self->priv->patterns,pattern))) {
    GST_INFO("make new wire-pattern");
    wire_pattern=bt_wire_pattern_new(self->priv->song, self, pattern);
    g_hash_table_insert(self->priv->patterns,pattern,wire_pattern);
  }
  return(g_object_ref(wire_pattern));
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

  GST_DEBUG("    param 0 'volume'");
  return(g_object_class_find_property(
    G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(self->priv->machines[PART_GAIN])),
    "volume")
  );
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
  GParamSpec *pspec=bt_wire_get_param_spec(self,index);

  return(pspec->value_type);
}

#if 0
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
void bt_wire_controller_change_value(const BtWire * const self, const gulong param, const GstClockTime timestamp, GValue * const value) {
  GObject *param_parent;
  gchar *name;
#ifdef HAVE_GST_0_10_14
  GstControlSource *cs;
#endif

  g_return_if_fail(BT_IS_WIRE(self));
  g_return_if_fail(param<self->priv->num_params);

  switch(param) {
    case 0:
      param_parent=G_OBJECT(self->priv->machines[PART_GAIN]);
      name="volume";
      break;
    /*
    case 1:
      param_parent=G_OBJECT(self->priv->machines[PART_PAN]);
      name="panorama";
      break,
    */
  }

  if(value) {
    gboolean add=TRUE;

    // check if the property is alredy controlled
    if(self->priv->wire_controller[param]) {
#ifdef HAVE_GST_0_10_14
      if((cs=gst_controller_get_control_source(self->priv->wire_controller[param],name))) {
        if(gst_interpolation_control_source_get_count(GST_INTERPOLATION_CONTROL_SOURCE(cs))) {
          add=FALSE;
        }
        g_object_unref(cs);
      }
#else
      GList *values;
      if((values=(GList *)gst_controller_get_all(self->priv->global_controller,name))) {
        add=FALSE;
        g_list_free(values);
      }
#endif
    }
    if(add) {
      GstController *ctrl=bt_wire_activate_controller(param_parent, name, FALSE);

      g_object_try_unref(self->priv->wire_controller[param]);
      self->priv->wire_controller[param]=ctrl;
    }
    //GST_INFO("set wire controller: %"GST_TIME_FORMAT" param %d:%s",GST_TIME_ARGS(timestamp),param,name);
#ifdef HAVE_GST_0_10_14
    if((cs=gst_controller_get_control_source(self->priv->wire_controller[param],name))) {
      gst_interpolation_control_source_set(GST_INTERPOLATION_CONTROL_SOURCE(cs),timestamp,value);
      g_object_unref(cs);
    }
#else
    gst_controller_set(self->priv->wire_controller[param],name,timestamp,value);
#endif
  }
  else {
    gboolean remove=TRUE;

    //GST_INFO("%s unset global controller: %"GST_TIME_FORMAT" param %d:%s",self->priv->id,GST_TIME_ARGS(timestamp),param,self->priv->global_names[param]);
#ifdef HAVE_GST_0_10_14
    if((cs=gst_controller_get_control_source(self->priv->wire_controller[param],name))) {
      gst_interpolation_control_source_unset(GST_INTERPOLATION_CONTROL_SOURCE(cs),timestamp);
      g_object_unref(cs);
    }
#else
    gst_controller_unset(self->priv->wire_controller[param],name,timestamp);
#endif

    // check if the property is not having control points anymore
#ifdef HAVE_GST_0_10_14
    if((cs=gst_controller_get_control_source(self->priv->wire_controller[param],name))) {
      if(gst_interpolation_control_source_get_count(GST_INTERPOLATION_CONTROL_SOURCE(cs))) {
        remove=FALSE;
      }
      g_object_unref(cs);
    }
#else
    GList *values;
    if((values=(GList *)gst_controller_get_all(self->priv->wire_controller[param],name))) {
      //if(g_list_length(values)>0) {
        remove=FALSE;
      //}
      g_list_free(values);
    }
#endif
    if(remove) {
      bt_wire_deactivate_controller(param_parent, name);
    }
  }
}
#endif

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
  gchar * const sid, * const did;

  if(self->priv->src) g_object_get(self->priv->src,"id",&sid,NULL);
  if(self->priv->dst) g_object_get(self->priv->dst,"id",&did,NULL);

  /* [Src T G C S Dst] */
  GST_DEBUG("%s->%s [%s %s %s %s %s]", sid, did,
    self->priv->machines[PART_SRC]?"SRC":"src",
    self->priv->machines[PART_TEE]?"T":"t",
    self->priv->machines[PART_GAIN]?"G":"g",
    self->priv->machines[PART_CONVERT]?"C":"c",
    /*self->priv->machines[PART_SCALE]?"S":"s",*/
    self->priv->machines[PART_DST]?"DST":"dst"
  );
  g_free(sid);
  g_free(did);
}

//-- io interface

static xmlNodePtr bt_wire_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtWire * const self = BT_WIRE(persistence);
  gchar * const id;
  xmlNodePtr node=NULL;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wire");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("wire"),NULL))) {
    g_object_get(G_OBJECT(self->priv->src),"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("src"),XML_CHAR_PTR(id));
    g_free(id);

    g_object_get(G_OBJECT(self->priv->dst),"id",&id,NULL);
    xmlNewProp(node,XML_CHAR_PTR("dst"),XML_CHAR_PTR(id));
    g_free(id);

    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("properties"),NULL))) {
      if(!bt_persistence_save_hashtable(self->priv->properties,child_node)) goto Error;
    }
    else goto Error;
    
    xmlNewProp(node,XML_CHAR_PTR("gain"),XML_CHAR_PTR(bt_persistence_strfmt_double(self->priv->gain)));
  }
Error:
  return(node);
}

static gboolean bt_wire_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  const BtWire * const self = BT_WIRE(persistence);
  BtSetup * const setup;
  xmlChar *id, *gain;
  gboolean res=FALSE;

  GST_DEBUG("PERSISTENCE::wire");
  g_assert(node);

  g_object_get(G_OBJECT(self->priv->song),"setup",&setup,NULL);

  id=xmlGetProp(node,XML_CHAR_PTR("src"));
  self->priv->src=bt_setup_get_machine_by_id(setup,(gchar *)id);
  GST_DEBUG("src %s -> %p",id,self->priv->src);
  xmlFree(id);

  id=xmlGetProp(node,XML_CHAR_PTR("dst"));
  self->priv->dst=bt_setup_get_machine_by_id(setup,(gchar *)id);
  GST_DEBUG("dst %s -> %p",id,self->priv->dst);
  xmlFree(id);

  if((gain=xmlGetProp(node,XML_CHAR_PTR("gain")))) {
    self->priv->gain=g_ascii_strtod((gchar *)gain,NULL);
    xmlFree(gain);
  }
  // @todo: handle pan
  
  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      if(!strncmp((gchar *)node->name,"properties\0",11)) {
        bt_persistence_load_hashtable(self->priv->properties,node);
      }
    }
  }

  // this is simillar to the code in the constructor
  if(bt_wire_connect(self)) {
    bt_setup_add_wire(setup,self);
    res=TRUE;
  }
  g_object_unref(setup);
  return(res);
}

static void bt_wire_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wire_persistence_load;
  iface->save = bt_wire_persistence_save;
}

//-- wrapper

//-- class internals

static GObject* bt_wire_constructor(GType type, guint n_construct_properties, GObjectConstructParam *construct_properties) {
  BtWire * const self=BT_WIRE(G_OBJECT_CLASS(bt_wire_parent_class)->constructor(type,n_construct_properties,construct_properties));
  
  GST_INFO("wire constructor, self->priv=%p, between %p and %p",self->priv,self->priv->src,self->priv->dst);
  if(self->priv->src && self->priv->dst) {
    if(!bt_wire_connect(self)) {
      goto Error;
    }
    {
      BtSetup * const setup;
  
      // add the wire to the setup of the song
      g_object_get(G_OBJECT(self->priv->song),"setup",&setup,NULL);
      g_assert(setup!=NULL);
      bt_setup_add_wire(setup,self);
      g_object_unref(setup);
    }
  }
  return(G_OBJECT(self));
Error:
  g_object_try_unref(self);
  return(NULL);
}

/* returns a property for the given property_id for this object */
static void bt_wire_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtWire *const self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
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
      g_value_set_double(value, self->priv->gain);
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
static void bt_wire_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtWire *const self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wire: %p",self->priv->song);
    } break;
    case WIRE_SRC: {
      g_object_try_unref(self->priv->src);
      self->priv->src=BT_MACHINE(g_value_dup_object(value));
      GST_DEBUG("set the source element for the wire: %p",self->priv->src);
    } break;
    case WIRE_DST: {
      g_object_try_unref(self->priv->dst);
      self->priv->dst=BT_MACHINE(g_value_dup_object(value));
      GST_DEBUG("set the target element for the wire: %p",self->priv->dst);
    } break;
    case WIRE_GAIN: {
      self->priv->gain=g_value_get_double(value);
      if(self->priv->machines[PART_GAIN]) {
        bt_wire_change_gain(self);
      }
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

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  // remove the GstElements from the bin
  if(self->priv->bin) {
    GstStateChangeReturn res;

    GST_DEBUG("  bin->ref_count=%d, bin->num_children=%d",
      (G_OBJECT(self->priv->bin))->ref_count,
      GST_BIN_NUMCHILDREN(self->priv->bin)
    );

    bt_wire_unlink_machines(self); // removes helper elements if in use
    //bt_machine_renegotiate_adder_format(self->priv->dst);

    if(self->priv->machines[PART_TEE]) {
      GST_DEBUG("  removing machine \"%s\" from bin, obj->ref_count=%d",gst_element_get_name(self->priv->machines[PART_TEE]),(G_OBJECT(self->priv->machines[PART_TEE]))->ref_count);
      if((res=gst_element_set_state(self->priv->machines[PART_TEE],GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
        GST_WARNING("can't go to null state");
      else
        GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));
      gst_bin_remove(self->priv->bin, self->priv->machines[PART_TEE]);
    }
    if(self->priv->machines[PART_GAIN]) {
      GST_DEBUG("  removing machine \"%s\" from bin, obj->ref_count=%d",gst_element_get_name(self->priv->machines[PART_GAIN]),(G_OBJECT(self->priv->machines[PART_GAIN]))->ref_count);
      if((res=gst_element_set_state(self->priv->machines[PART_GAIN],GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE)
        GST_WARNING("can't go to null state");
      else
        GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));
      gst_bin_remove(self->priv->bin, self->priv->machines[PART_GAIN]);
    }
    bt_wire_deactivate_analyzers(self);

    GST_DEBUG("  releasing the bin, bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
    gst_object_unref(self->priv->bin);
  }

  g_object_try_weak_unref(self->priv->song);
  //gstreamer uses floating references, therefore elements are destroyed, when removed from the bin
  GST_DEBUG("  releasing the dst %p, dst->ref_count=%d",
    self->priv->dst,(self->priv->dst?(G_OBJECT(self->priv->dst))->ref_count:-1));
  g_object_try_unref(self->priv->dst);
  GST_DEBUG("  releasing the src %p, src->ref_count=%d",
    self->priv->src,(self->priv->src?(G_OBJECT(self->priv->src))->ref_count:-1));
  g_object_try_unref(self->priv->src);

  G_OBJECT_CLASS(bt_wire_parent_class)->dispose(object);
}

static void bt_wire_finalize(GObject * const object) {
  BtWire * const self = BT_WIRE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_hash_table_destroy(self->priv->properties);
  g_hash_table_destroy(self->priv->patterns);

  G_OBJECT_CLASS(bt_wire_parent_class)->finalize(object);
}

static void bt_wire_init(BtWire *self) {
  GST_INFO("wire init, no priv data yet");
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE, BtWirePrivate);
  self->priv->gain = 1.0;
  self->priv->num_params = 1;
  self->priv->properties=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
  self->priv->patterns=g_hash_table_new_full(g_direct_hash,g_direct_equal,NULL,g_object_unref);

  GST_DEBUG("!!!! self=%p",self);
}

static void bt_wire_class_init(BtWireClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  bt_wire_parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWirePrivate));

  gobject_class->constructor  = bt_wire_constructor;
  gobject_class->set_property = bt_wire_set_property;
  gobject_class->get_property = bt_wire_get_property;
  gobject_class->dispose      = bt_wire_dispose;
  gobject_class->finalize     = bt_wire_finalize;

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
                                  g_param_spec_double("gain",
                                     "gain prop",
                                     "gain for the connection",
                                     0.0,
                                     4.0,
                                     1.0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_NUM_PARAMS,
                                  g_param_spec_ulong("num-params",
                                     "num-params prop",
                                     "number of params for the wire",
                                     0,
                                     MAX_NUM_PARAMS,
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_ANALYZERS,
                                  g_param_spec_pointer("analyzers",
                                     "analyzers prop",
                                     "list of wire analyzers",
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_wire_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtWireClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtWire),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wire_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_wire_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtWire",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}

