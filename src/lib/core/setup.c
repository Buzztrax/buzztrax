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
 * SECTION:btsetup
 * @short_description: class with all machines and wires (#BtMachine and #BtWire)
 * for a #BtSong instance
 *
 * The setup manages virtual gear. That is used #BtMachines and the #BtWires
 * that connect them.
 *
 * It also manages the GStreamer #GstPipleine content.
 */
/* @todo: support dynamic (un)linking (while playing)
 *
 * right now we update the pipeline before going to playing (see
 * bt_setup_update_pipeline()). This always starts with master right now,
 * we need to make that dynamic.
 *
 * When we add a machine, we do nothing else
 * When we remove a machine, we remove all connected wires
 *
 * When we add a wire, we run check_connected(self,master,NULL,NULL).
 * When we remove a wire, we run check_connected(self,master,NULL,NULL).
 *
 * We don't need to handle the not_visited_* lists. Disconnected things are
 * never added (see update_bin_in_pipeline()). Instead we need a list of
 * blocked_pads.
 *
 * When adding/removing a bin while playing, block all src-pads that are
 * connected to existing elements when linking.
 * Once the deep scan of the graph is finished, we can unblock all remeberred
 * pads. We should probably also send a seek and a tag een to newly added
 * sources.
 *
 * When dynamically adding/removing we ideally do not make any change to the
 * existing pipeline, but collect a list of machines and wires we want to
 * add/remove (subgraph). We also need to know if the new subgraph makes any
 * practical change (does the new wire connect to the master in the end or is
 * the wire we remove connected). 
 *
 * Adding wires ----------------------------------------------------------------
 * - if the target of new wire is not connecting to the master or the src of the
 *     wire is not reaching a source or conneted machine, just add to setup
 *   else -> build subgraph (we want all wires+machines that become fully
 *     connected as the result of adding this wire)
 * - for the subgraph we would study all wires that are not connected
 *   not_connected = get_list_of_unconencted_wires();
 *   sub_graph = g_list_append(NULL,this_wire);
 *   do {
 *     found_more=FALSE;
 *     foreach(not_connected) {
 *       if(linked_to_wire_sub_graph(wire,sub_graph) {
 *         sub_graph = g_list_append(sub_graph,wire);
 *         not_connected = g_list_remove(not_connected,wire);
 *         found_more=TRUE;
 *       }
 *     }
 *   } while(found_more);
 *   // now we would need to eliminate disconnected paths again
 *   do {
 *     found_more=FALSE;
 *     foreach(sub_graph) {
 *       // check if wire is connected in real song or subgraph
 *       if(wire_is_not_connected(wire) {
 *         sub_graph = g_list_remove(sub_graph,wire);
 *         found_more=TRUE;
 *       }
 *     }
 *   } while(found_more);
 *   if(!is_empty(sub_graph)) {
 *     src_wires=NULL;
 *     // get all wires in subgraph that are connected to real song in src side
 *     foreach(sub_graph) {
 *       is_src=TRUE;
 *       foreach(sub_graph) {
 *         if(this_wire.src==that_wire.dst) {
 *           is_src=FALSE;
 *           break;
 *         }
 *       }
 *       if(is_src) {
 *         src_wires = g_list_append(src_wires,this_wire);
 *       }
 *     }
 *     foreach(sub_graph) {
 *       // do we need gst_element_set_locked_state(...,TRUE);
 *       update_bin_in_pipeline(...);
 *     }
 *     foreach(src_wires) {
 *       link_wire_and_block(wire,src,dst);
 *     }
 *     foreach(sub_graph) {
 *       if(wire not in src_wires) {
 *         link_wire(wire,src,dst);
 *         gst_element_set_state(PLAYING);
 *       }
 *     }
 *     foreach(src_wires) {
 *       un_block(wire,src);
 *     }
 *   }
 *
 * Graph before:
 *   A    B
 * Graph matrix before:
 *   + A B
 *   A
 *   B
 * Add: A => B
 * Block: A
 * Link: A => B
 * Parent: A, B, A => B
 * Unblock: A?
 * Graph after:
 *   A => B
 * Graph matrix after:
 *   + A B
 *   A   =
 *   B
 *
 * Graph brefore:
 *   A    B -> C
 *     => D =>
 * Graph matrix before:
 *   + A B C D
 *   A       =
 *   B     -
 *   C
 *   D     =
 * Add: A => B
 * Block: A
 * Link: A => B, B => C
 * Parent: B, A => B, B => C
 * Unblock: A
 * Graph after:
 *   A => B => C
 *     => D =>
 * Graph matrix after:
 *   + A B C D
 *   A   =   =
 *   B     =
 *   C
 *   D     =
 *
 * Graph brefore:
 *   A -> B    C
 *     => D =>
 * Graph matrix before:
 *   + A B C D
 *   A   -   =
 *   B      
 *   C
 *   D     =
 * Add: B => C
 * Block: A
 * Link: A => B, B => C
 * Parent: B, A => B, B => C
 * Unblock: A
 * Graph after:
 *   A => B => C
 *     => D =>
 * Graph matrix after:
 *   + A B C D
 *   A   =   =
 *   B     = 
 *   C
 *   D     =
 *
 * Removing wires --------------------------------------------------------------
 * - if the wire is not in the pipeline (not connecting things to the master)
 *     just remove from setup
 *   else -> build subgraph (we want all wires+machines that become disconned
 *     as the result of removing this wire)
 * - for the subgraph we would study all wires that are connected
 *   - we can do this simillar as for adding
 *
 * Graph before:
 *   A => B
 * Remove: A => B
 * Block: A
 * Unlink: A => B
 * Unparent: A, B, A => B
 * Unblock: A?
 * Graph after:
 *   A    B
 *
 * Graph brefore:
 *   A => B => C
 *     => D =>
 * Remove: A => B
 * Block: A
 * Unlink: A => B, B => C
 * Unparent: B, A => B, B => C
 * Unblock: A
 * Graph after:
 *   A    B -> C
 *     => D =>
 *
 * --
 *
 * We could have an alternative data-structure in setup:
 * struct Connection[num_src][num_dst]
 * but its not giving much benefit actually
 * We could print the song-graph as a matrix for debuging purposes
 *
 * --
 *
 * We could have a GHashMap for wires and machines in setup to track if they are
 * added or not. Then check_connected() can be made two pass.
 * - GEnum BtSetupConnectionState={DISCONNECTED,DISCONNECTING,CONNECTING,CONNECTED}
 *   [done]
 * - when we add an element its state is Disconnected
 *   [done]
 * - when we remove an element we set the state to DISCONNECTING if it was Connected ?
 * 1. check_connected
 *   - when this is called all elements are either disconnected or connected
 *     - exception is when we remove things
 *   - disconnected elements that should be connected will be set to CONNECTING
 *   - connected elements that should be disconnected will be set to DISCONNCTING
 *   - we run this regardless if the song is playing or not, whenever we update/
 *     remove things (also in the future we will always play)
 * 2. update_pipeline
 *   - the elements which are in CONNECTING or DISCONNECTING are our subgraph
 *   - add all machines that are in CONNECTING and set to CONNECTED
 *   - add & link all wires that are in CONNECTING and set to CONNECTED
 *   - unlink and remove all wires that are in DISCONNECTING and set to DISCONNECTED
 *   - remove all machines that are in DISCONNECTING and set to DISCONNECTED
 *   - after this has run all elements are either DISCONNECTED or CONNECTED
 *   - when to run
 *     - we actually don't need to run this if we are not playing
 *      (but in the future we will always play)
 *     - we can run it before playing
 *
 * --
 *
 * When adding bins in a playing pipeline we need to sync the bin state with the
 * pipeline
 *
 * --
 *
 * If we play always
 * - we need a silente source connected to master (inside master?)
 * - we could remove bt_song_is_playable() and make bt_setup_update_pipeline() static
 */

#define BT_CORE
#define BT_SETUP_C

#include "core_private.h"

// new pipeline updating, that uses two phases
#define NEW_CODE 1
// allow live updates
#define LIVE_CONNECT 1

//-- signal ids

enum {
  MACHINE_ADDED_EVENT,
  MACHINE_REMOVED_EVENT,
  WIRE_ADDED_EVENT,
  WIRE_REMOVED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  SETUP_PROPERTIES=1,
  SETUP_SONG,
  SETUP_MACHINES,
  SETUP_WIRES,
  SETUP_MISSING_MACHINES
};

typedef enum {
  /* start with 1, so that we can differentiate between NULL and DISCONNECTED
   * when doing g_hash_table_lookup */ 
  CS_DISCONNECTED=1,
  CS_DISCONNECTING,
  CS_CONNECTING,
  CS_CONNECTED
} BtSetupConnectionState;

typedef enum {
  SCO_BEFORE=1,
  SCO_NORMAL,
  SCO_AFTER
} BtSetupStateChangeOrder;


#define GET_CONNECTION_STATE(self,bin) \
  GPOINTER_TO_INT(g_hash_table_lookup(self->priv->connection_state,(gpointer)bin))

#define GET_STATE_CHANGE_ORDER(self,bin) \
  GPOINTER_TO_INT(g_hash_table_lookup(self->priv->state_change_order,(gpointer)bin))

  
struct _BtSetupPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the setup belongs to */
  G_POINTER_ALIAS(BtSong *,song);
  
  /* the top-level gstreamer container element */
  GstBin *bin;

  GList *machines;         // each entry points to BtMachine
  GList *wires;            // each entry points to BtWire
  GList *missing_machines; // each entry points to a gchar*

  /* (ui) properties accociated with this song
   * zoom. scroll-position */
  GHashTable *properties;
  
  /* state of elements (wires and machines) as BtSetupConnectionState */
  GHashTable *connection_state;
  /* activation order of elements (wires and machines) as BtSetupStateChangeOrder */
  GHashTable *state_change_order;
  /* list of blocked pads, to unnlock after updates */
  GSList *blocked_pads;
  
  /* seek event for dynamically added elements */
  GstEvent *play_seek_event;
};

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- constructor methods

/**
 * bt_setup_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSetup *bt_setup_new(const BtSong * const song) {
  return(BT_SETUP(g_object_new(BT_TYPE_SETUP,"song",song,NULL)));
}

//-- private methods

static void set_disconnected(const BtSetup * const self, GstBin *bin) {
  BtSetupConnectionState state=GET_CONNECTION_STATE(self,bin);
  
  if(state==0 || state==CS_DISCONNECTING) {
    g_hash_table_insert(self->priv->connection_state,(gpointer)bin,GINT_TO_POINTER(CS_DISCONNECTED));
  }
  else if(state==CS_CONNECTING) {
    GST_WARNING_OBJECT(bin,"disallowed state-change from connecting to disconnected");
  }
  else if(state==CS_CONNECTED) {
    GST_WARNING_OBJECT(bin,"disallowed state-change from connected to disconnected");
  }
}

static void set_disconnecting(const BtSetup * const self, GstBin *bin) {
  BtSetupConnectionState state=GET_CONNECTION_STATE(self,bin);
  
  if(state==CS_CONNECTED) {
    g_hash_table_insert(self->priv->connection_state,(gpointer)bin,GINT_TO_POINTER(CS_DISCONNECTING));
  }
  else if(state==CS_CONNECTING) {
    GST_WARNING_OBJECT(bin,"disallowed state-change from connecting to disconnecteding");
  }
}

static void set_connecting(const BtSetup * const self, GstBin *bin) {
  BtSetupConnectionState state=GET_CONNECTION_STATE(self,bin);
  
  if(state==CS_DISCONNECTED) {
    g_hash_table_insert(self->priv->connection_state,(gpointer)bin,GINT_TO_POINTER(CS_CONNECTING));
  }
  else if(state==CS_DISCONNECTING) {
    GST_WARNING_OBJECT(bin,"disallowed state-change from disconnecting to connecting");
  }
}

static void set_connected(const BtSetup * const self, GstBin *bin) {
  BtSetupConnectionState state=GET_CONNECTION_STATE(self,bin);
  
  if(state==CS_CONNECTING) {
    g_hash_table_insert(self->priv->connection_state,(gpointer)bin,GINT_TO_POINTER(CS_CONNECTED));
  }
  else if(state==CS_DISCONNECTING) {
    GST_WARNING_OBJECT(bin,"disallowed state-change from disconnecting to connected");
  }
  else if(state==CS_DISCONNECTED) {
    GST_WARNING_OBJECT(bin,"disallowed state-change from disconnected to connected");
  }
}

/*
 * bt_setup_get_wire_by_machine_type:
 * @self: the setup to search for the wire
 * @machine: the machine that is at the src or dst of the wire
 * @type: the type name (src or dst) that the given machine should have in the wire
 *
 * Searches for the first wire in setup that uses the given #BtMachine as the given
 * type.
 * In other words - it returns the first wire that has the the given #BtMachine as
 * the given type.
 *
 * Returns: the #BtWire or %NULL
 */
static BtWire *bt_setup_get_wire_by_machine_type(const BtSetup * const self, const BtMachine * const machine, const gchar * const type) {
  gboolean found=FALSE;
  BtMachine * const search_machine;
  GList *node;

  for(node=self->priv->wires;node;node=g_list_next(node)) {
    BtWire * const wire=BT_WIRE(node->data);
    g_object_get(G_OBJECT(wire),type,&search_machine,NULL);
    if(search_machine==machine) found=TRUE;
    g_object_unref(search_machine);
    if(found) return(g_object_ref(wire));
  }
  GST_DEBUG("no wire found for %s-machine %p",type,machine);
  return(NULL);
}

/*
 * bt_setup_get_wires_by_machine_type:
 * @self: the setup to search for the wires
 * @machine: the machine that is at the src or dst of the wire
 * @type: the type name (src or dst) that the given machine should have in the wire
 *
 * Searches for all wires in setup that uses the given #BtMachine as the given
 * type.
 *
 * Returns: a #GList with the #BtWires or %NULL
 */
static GList *bt_setup_get_wires_by_machine_type(const BtSetup * const self,const BtMachine * const machine, const gchar * const type) {
  GList *wires=NULL,*node;
  BtMachine * const search_machine;

  for(node=self->priv->wires;node;node=g_list_next(node)) {
    BtWire * const wire=BT_WIRE(node->data);
    g_object_get(G_OBJECT(wire),type,&search_machine,NULL);
    if(search_machine==machine) {
      wires=g_list_prepend(wires,g_object_ref(wire));
    }
    g_object_unref(search_machine);
  }
  return(wires);
}

static void link_wire(const BtSetup * const self,GstElement *wire,GstElement *src_machine,GstElement *dst_machine) {
  GstPadLinkReturn link_res;
  GstPad *src_pad,*dst_pad,*peer;

  // link start of wire
  GST_INFO("linking start of wire");      
  dst_pad=gst_element_get_static_pad(GST_ELEMENT(wire),"sink");
  if(!(peer=gst_pad_get_peer(dst_pad))) {
    src_pad=gst_element_get_request_pad(GST_ELEMENT(src_machine),"src%d");
    if(/*(BT_IS_SOURCE_MACHINE(src_machine) && (GST_STATE(self->priv->bin)==GST_STATE_PLAYING)) ||*/ 
      (GST_STATE(src_machine)==GST_STATE_PLAYING)) {
      gst_pad_set_blocked(src_pad,TRUE);
      self->priv->blocked_pads=g_slist_prepend(self->priv->blocked_pads,src_pad);
    }
    if(GST_PAD_LINK_FAILED(link_res=gst_pad_link(src_pad,dst_pad))) {
      GST_WARNING("Can't link start of wire : %d : %s:%s -> %s:%s",
        link_res,GST_DEBUG_PAD_NAME(src_pad),GST_DEBUG_PAD_NAME(dst_pad));
    }
  }
  else {
    GST_INFO("start of wire is already linked");
    gst_object_unref(peer);
  }
  gst_object_unref(dst_pad);
  // link end of wire
  GST_INFO("linking end of wire");      
  src_pad=gst_element_get_static_pad(GST_ELEMENT(wire),"src");
  if(!(peer=gst_pad_get_peer(src_pad))) {
    dst_pad=gst_element_get_request_pad(GST_ELEMENT(dst_machine),"sink%d");
    if(GST_PAD_LINK_FAILED(link_res=gst_pad_link(src_pad,dst_pad))) {
      GST_WARNING("Can't link end of wire : %d : %s:%s -> %s:%s",
        link_res,GST_DEBUG_PAD_NAME(src_pad),GST_DEBUG_PAD_NAME(dst_pad));
    }
  }
  else {
    GST_INFO("end of wire is already linked");
    gst_object_unref(peer);
  }
  gst_object_unref(src_pad);
}

static void unlink_wire(const BtSetup * const self,GstElement *wire,GstElement *src_machine,GstElement *dst_machine) {
  GstPad *src_pad,*dst_pad;

  // unlink start of wire
  GST_INFO("unlinking start of wire");      
  dst_pad=gst_element_get_static_pad(wire,"sink");
  if((src_pad=gst_pad_get_peer(dst_pad))) {
    if(/*(BT_IS_SOURCE_MACHINE(src_machine) && (GST_STATE(self->priv->bin)==GST_STATE_PLAYING)) ||*/ 
      (GST_STATE(src_machine)==GST_STATE_PLAYING)) {
      //gst_pad_set_blocked_async(src_pad,TRUE,NULL,NULL);
      gst_pad_set_blocked(src_pad,TRUE);
      self->priv->blocked_pads=g_slist_prepend(self->priv->blocked_pads,src_pad);
    }
    gst_pad_unlink(src_pad,dst_pad);
    gst_element_release_request_pad(src_machine,src_pad);
    // unref twice: one for gst_pad_get_peer() and once for the request_pad
    gst_object_unref(src_pad);gst_object_unref(src_pad);
  }
  gst_object_unref(dst_pad);

  GST_INFO("unlinking end of wire");    
  src_pad=gst_element_get_static_pad(wire,"src");
  if((dst_pad=gst_pad_get_peer(src_pad))) {
    gst_pad_unlink(src_pad,dst_pad);
    gst_element_release_request_pad(dst_machine,dst_pad);
    // unref twice: one for gst_pad_get_peer() and once for the request_pad
    gst_object_unref(dst_pad);gst_object_unref(dst_pad);
  }
  gst_object_unref(src_pad); 
}

/*
 * update_bin_in_pipeline:
 * @self: the setup object
 *
 * Add or remove machine or wires to/from the main pipeline.
 */
static gboolean update_bin_in_pipeline(const BtSetup * const self,GstBin *bin,gboolean is_connected,GList **not_visited) {
  gboolean is_added=(GST_OBJECT_PARENT(bin)!=NULL);
  
  if(not_visited) {
    *not_visited=g_list_remove(*not_visited,(gconstpointer)bin);
  }
  
  GST_INFO_OBJECT(bin,"update object : connected=%d, added=%d",
    is_connected,is_added);  

  if(is_connected) {
    if(!is_added) {
      gst_bin_add(self->priv->bin,GST_ELEMENT(bin));
    }
  }
  else {
    if(is_added) {
      gst_object_ref(GST_OBJECT(bin));
      GST_OBJECT_FLAG_SET(bin,GST_OBJECT_FLOATING);
      gst_bin_remove(self->priv->bin,GST_ELEMENT(bin));
    }
  }
  
  return(TRUE);
}

/*
 * check_connected:
 * @self: the setup object
 * @dst_machine: the machine to start with, usually the master
 *
 * Check if a machine is connected. It recurses up on the machines input side.
 * Its adds connected machines and wires to the song and removed unconnected
 * ones. It links connected bins and unlinks unconnected ones.
 *
 * Returns: %TRUE if there is a conenction to a src.
 */
static gboolean check_connected(const BtSetup * const self,BtMachine *dst_machine,GList **not_visited_machines,GList **not_visited_wires) {
  gboolean is_connected=FALSE,wire_is_connected;
  BtWire *wire;
  BtMachine *src_machine;
  GList *node,*list;
  
  list = bt_setup_get_wires_by_dst_machine(self,dst_machine);
  GST_INFO_OBJECT(dst_machine,"check %d incomming wires",g_list_length(list));
  for(node=list;node;node=g_list_next(node)) {
    wire=BT_WIRE(node->data);
    
    // check if wire is marked for removal
#if NEW_CODE
    if(GET_CONNECTION_STATE(self,wire)!=CS_DISCONNECTING) {
#endif
      wire_is_connected=FALSE;
      g_object_get(wire,"src",&src_machine,NULL);
      if(BT_IS_SOURCE_MACHINE(src_machine)) {
        /* for source machine we can stop the recurssion */
        wire_is_connected=TRUE;
#if NEW_CODE
        *not_visited_machines=g_list_remove(*not_visited_machines,(gconstpointer)src_machine);
#endif
      }
      else {
        /* for processor machine we might need to look further,
         * but if machine is not in not_visited_machines anymore,
         * check if it is added or not and return */
        BtSetupConnectionState state=GET_CONNECTION_STATE(self,src_machine);
  
        if((!g_list_find(*not_visited_machines,src_machine)) && (state==CS_CONNECTING || state==CS_CONNECTED)) {
          wire_is_connected=TRUE;
        }
        else {
          wire_is_connected|=check_connected(self,src_machine,not_visited_machines,not_visited_wires);
        }
      }
      GST_INFO("wire target checked, connected=%d?",wire_is_connected);
      if(!wire_is_connected) {
#if NEW_CODE
        set_disconnecting(self,GST_BIN(wire));
        set_disconnecting(self,GST_BIN(src_machine));
#else
        unlink_wire(self,GST_ELEMENT(wire),GST_ELEMENT(src_machine),GST_ELEMENT(dst_machine));
        update_bin_in_pipeline(self,GST_BIN(src_machine),FALSE,not_visited_machines);
#endif      
      }
#if ! NEW_CODE
      update_bin_in_pipeline(self,GST_BIN(wire),wire_is_connected,not_visited_wires);
#endif
      if(wire_is_connected) {
#if NEW_CODE
        if(!is_connected) {
          set_connecting(self,GST_BIN(dst_machine));
        }
        set_connecting(self,GST_BIN(src_machine));
        set_connecting(self,GST_BIN(wire));
#else
        if(!is_connected) {
          update_bin_in_pipeline(self,GST_BIN(dst_machine),TRUE,not_visited_machines);
        }
        update_bin_in_pipeline(self,GST_BIN(src_machine),TRUE,not_visited_machines);
        link_wire(self,GST_ELEMENT(wire),GST_ELEMENT(src_machine),GST_ELEMENT(dst_machine));
#endif
      }
      else {
        GST_INFO("skip disconnecting wire");
      }
      is_connected|=wire_is_connected;
      g_object_unref(src_machine);
#if NEW_CODE
    }
#endif

#if NEW_CODE
  *not_visited_wires=g_list_remove(*not_visited_wires,(gconstpointer)wire);
#endif

    g_object_unref(wire);
  }
  g_list_free(list);
  if(!is_connected) {
    update_bin_in_pipeline(self,GST_BIN(dst_machine),FALSE,not_visited_machines);
    
    set_disconnecting(self,GST_BIN(dst_machine));
  }
#if NEW_CODE
  *not_visited_machines=g_list_remove(*not_visited_machines,(gconstpointer)dst_machine);
#endif
  GST_INFO("all wire targets checked, connected=%d?",is_connected);
  return(is_connected);
}

#if NEW_CODE

static void add_machine_in_pipeline(gpointer key,gpointer value,gpointer user_data) {
  if((GPOINTER_TO_INT(value)==CS_CONNECTING) && BT_IS_MACHINE(key)) {
    const BtSetup * const self=BT_SETUP(user_data);
    
    GST_INFO_OBJECT(key,"add machine");
    update_bin_in_pipeline(self,GST_BIN(key),TRUE,NULL);
  }
}

static void add_wire_in_pipeline(gpointer key,gpointer value,gpointer user_data) {
  if((GPOINTER_TO_INT(value)==CS_CONNECTING) && BT_IS_WIRE(key)) {
    const BtSetup * const self=BT_SETUP(user_data);
    GstElement *src,*dst;
    
    GST_INFO_OBJECT(key,"add & link wire");
    update_bin_in_pipeline(self,GST_BIN(key),TRUE,NULL);

    g_object_get(G_OBJECT(key),"src",&src,"dst",&dst,NULL);
    link_wire(self,GST_ELEMENT(key),src,dst);
    bt_machine_renegotiate_adder_format(BT_MACHINE(dst));
    g_object_unref(src);
    g_object_unref(dst);
  }
}

static void determine_state_change_order(gpointer key,gpointer value,gpointer user_data) {
  const BtSetup * const self=BT_SETUP(user_data);
  BtSetupStateChangeOrder sco=SCO_NORMAL;

  if(GPOINTER_TO_INT(value)==CS_CONNECTING) {
    /* adding/->PLAYING: need to flag:
     * - all source machines if they are connecting
     * - all wires that are connecting and where the src_machine is connected
     */
    if(BT_IS_SOURCE_MACHINE(key)) {
      sco=SCO_AFTER;
    }
    if(BT_IS_WIRE(key)) {
      BtMachine *src;
      g_object_get(G_OBJECT(key),"src",&src,NULL);
      if(GET_CONNECTION_STATE(self,src)==CS_CONNECTED) {
        sco=SCO_AFTER;
      }
      g_object_unref(src);
    }
  }
  else if(GPOINTER_TO_INT(value)==CS_DISCONNECTING) {
    /* removing/->NULL: need to flag:
     * - all source machines if they are disconncting
     * - all wires that are disconnecting and where the src_machine is connected
     */
    if(BT_IS_SOURCE_MACHINE(key)) {
      sco=SCO_BEFORE;
    }
    if(BT_IS_WIRE(key)) {
      BtMachine *src;
      g_object_get(G_OBJECT(key),"src",&src,NULL);
      if(GET_CONNECTION_STATE(self,src)==CS_CONNECTED) {
        sco=SCO_BEFORE;
      }
      g_object_unref(src);
    }
  }
  g_hash_table_insert(self->priv->state_change_order,(gpointer)key,GINT_TO_POINTER(sco));
}

static void activate_element(const BtSetup * const self,gpointer *key) {
  GST_INFO_OBJECT(GST_OBJECT(key),"set from %s to %s",
    gst_element_state_get_name(GST_STATE(key)),
    gst_element_state_get_name(GST_STATE(GST_OBJECT_PARENT(GST_OBJECT(key)))));

  if(GST_STATE(GST_OBJECT_PARENT(GST_OBJECT(key)))==GST_STATE_PLAYING) {
    gst_element_set_state(GST_ELEMENT(key),GST_STATE_READY);
    if(!(gst_element_send_event(GST_ELEMENT(key),gst_event_ref(self->priv->play_seek_event)))) {
      GST_WARNING_OBJECT(key,"failed to handle seek event");
    }
    gst_element_set_state(GST_ELEMENT(key),GST_STATE_PLAYING);
  }
  //gst_element_sync_state_with_parent(GST_ELEMENT(key));
}

static void deactivate_element(const BtSetup * const self,gpointer *key) {
  gst_element_set_locked_state(GST_ELEMENT(key),TRUE);
  gst_element_set_state(GST_ELEMENT(key),GST_STATE_NULL);
}

static void sync_states_for_state_change_before(gpointer key,gpointer value,gpointer user_data) {
  const BtSetup * const self=BT_SETUP(user_data);
  if(GPOINTER_TO_INT(value)==CS_CONNECTING) {
    if(GET_STATE_CHANGE_ORDER(self,key)==SCO_BEFORE) {
      activate_element(self,key);
    }
  }
  else if(GPOINTER_TO_INT(value)==CS_DISCONNECTING) {
    if(GET_STATE_CHANGE_ORDER(self,key)==SCO_BEFORE) {
      deactivate_element(self,key);
    }
  }
}

static void sync_states_for_state_change_normal(gpointer key,gpointer value,gpointer user_data) {
  const BtSetup * const self=BT_SETUP(user_data);
  if(GPOINTER_TO_INT(value)==CS_CONNECTING) {
    if(GET_STATE_CHANGE_ORDER(self,key)==SCO_NORMAL) {
      activate_element(self,key);
    }
  }
  else if(GPOINTER_TO_INT(value)==CS_DISCONNECTING) {
    if(GET_STATE_CHANGE_ORDER(self,key)==SCO_NORMAL) {
      deactivate_element(self,key);
    }
  }
}

static void sync_states_for_state_change_after(gpointer key,gpointer value,gpointer user_data) {
  const BtSetup * const self=BT_SETUP(user_data);
  if(GPOINTER_TO_INT(value)==CS_CONNECTING) {
    if(GET_STATE_CHANGE_ORDER(self,key)==SCO_AFTER) {
      activate_element(self,key);
    }
  }
  else if(GPOINTER_TO_INT(value)==CS_DISCONNECTING) {
    if(GET_STATE_CHANGE_ORDER(self,key)==SCO_AFTER) {
      deactivate_element(self,key);
    }
  }
}

static void del_wire_in_pipeline(gpointer key,gpointer value,gpointer user_data) {
  if((GPOINTER_TO_INT(value)==CS_DISCONNECTING) && BT_IS_WIRE(key)) {
    const BtSetup * const self=BT_SETUP(user_data);
    GstElement *src,*dst;
    
    GST_INFO_OBJECT(key,"remove & unlink wire");
    g_object_get(G_OBJECT(key),"src",&src,"dst",&dst,NULL);
    unlink_wire(self,GST_ELEMENT(key),src,dst);
    bt_machine_renegotiate_adder_format(BT_MACHINE(dst));
    g_object_unref(src);
    g_object_unref(dst);
    
    update_bin_in_pipeline(self,GST_BIN(key),FALSE,NULL);
  }
}

static void del_machine_in_pipeline(gpointer key,gpointer value,gpointer user_data) {
  if((GPOINTER_TO_INT(value)==CS_DISCONNECTING) && BT_IS_MACHINE(key)) {
    const BtSetup * const self=BT_SETUP(user_data);
    
    GST_INFO_OBJECT(key,"remove machine");
    update_bin_in_pipeline(self,GST_BIN(key),FALSE,NULL);
  }
}

static void update_connection_states(gpointer key,gpointer value,gpointer user_data) {
  const BtSetup * const self=BT_SETUP(user_data);
  
  // debug
  gchar *states[]={"-","disconnected","disconnecting","connecting","connected"};
  GST_INFO_OBJECT(key,"%s",states[GPOINTER_TO_INT(value)]);
  // debug

  if(GPOINTER_TO_INT(value)==CS_CONNECTING) {
    set_connected(self,GST_BIN(key));
  }
  else if(GPOINTER_TO_INT(value)==CS_DISCONNECTING) {
    gst_element_set_locked_state(GST_ELEMENT(key), FALSE);
    set_disconnected(self,GST_BIN(key));
  }
}

static void update_pipeline(const BtSetup * const self) {
  GSList *node;
  BtSequence *sequence;
  gboolean loop;
  glong loop_end,length;
  gulong play_pos;
  GstClockTime bar_time;
 
  GST_WARNING("updating pipeline ----------------------------------------");

  // query seqment and position
  g_object_get(self->priv->song,"sequence",&sequence,"play-pos",&play_pos,NULL);
  g_object_get(sequence,"loop",&loop,"loop-end",&loop_end,"length",&length,NULL);
  bar_time=bt_sequence_get_bar_time(sequence);
  // configure self->priv->play_seek_event
  if (loop) {
    self->priv->play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, play_pos*bar_time,
        GST_SEEK_TYPE_SET, (loop_end+0)*bar_time);
  }
  else {
    self->priv->play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, play_pos*bar_time,
        GST_SEEK_TYPE_SET, (length+1)*bar_time);
  }
  g_object_unref(sequence);
  
  GST_WARNING("add machines");
  g_hash_table_foreach(self->priv->connection_state,add_machine_in_pipeline,(gpointer)self);
  GST_WARNING("add and link wires");
  g_hash_table_foreach(self->priv->connection_state,add_wire_in_pipeline,(gpointer)self);
  GST_WARNING("determine activation order");
  g_hash_table_foreach(self->priv->connection_state,determine_state_change_order,(gpointer)self);
  GST_WARNING("sync states");
  /* we need to do this in the right order
   *   adding/->PLAYING: from sinks to sources, or source last
   *   removing/->NULL: from sources to sinks, or source first
   */
  g_hash_table_foreach(self->priv->connection_state,sync_states_for_state_change_before,(gpointer)self);
  g_hash_table_foreach(self->priv->connection_state,sync_states_for_state_change_normal,(gpointer)self);
  g_hash_table_foreach(self->priv->connection_state,sync_states_for_state_change_after,(gpointer)self);
  GST_WARNING("unlink and remove wires");
  g_hash_table_foreach(self->priv->connection_state,del_wire_in_pipeline,(gpointer)self);
  GST_WARNING("remove machines");
  g_hash_table_foreach(self->priv->connection_state,del_machine_in_pipeline,(gpointer)self);
  // unblock src pads
  GST_WARNING("unblocking %d pads",g_slist_length(node=self->priv->blocked_pads));
  for(node=self->priv->blocked_pads;node;node=g_slist_next(node)) {
    gst_pad_set_blocked(GST_PAD(node->data),FALSE);
  }
  g_slist_free(self->priv->blocked_pads);
  self->priv->blocked_pads=NULL;
  GST_WARNING("update connection states");
  g_hash_table_foreach(self->priv->connection_state,update_connection_states,(gpointer)self);
  GST_WARNING("pipeline updated ----------------------------------------");
  gst_event_unref(self->priv->play_seek_event);
  self->priv->play_seek_event=NULL;
}
#endif

//-- public methods

/**
 * bt_setup_add_machine:
 * @self: the setup to add the machine to
 * @machine: the new machine instance
 *
 * Let the setup know that the suplied machine is now part of the song.
 *
 * Returns: return true, if the machine can be added. Returns false if the
 * machine is currently added to the setup.
 */
gboolean bt_setup_add_machine(const BtSetup * const self, const BtMachine * const machine) {
  gboolean ret=FALSE;

  // @todo add g_error stuff, to give the programmer more information, whats going wrong.

  g_return_val_if_fail(BT_IS_SETUP(self),FALSE);
  g_return_val_if_fail(BT_IS_MACHINE(machine),FALSE);

  if(!g_list_find(self->priv->machines,machine)) {
    ret=TRUE;
    self->priv->machines=g_list_append(self->priv->machines,g_object_ref(G_OBJECT(machine)));
    set_disconnected(self,GST_BIN(machine));

    g_signal_emit(G_OBJECT(self),signals[MACHINE_ADDED_EVENT], 0, machine);
    bt_song_set_unsaved(self->priv->song,TRUE);
    GST_DEBUG("added machine: %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);
  }
  else {
    GST_WARNING("trying to add machine %p again",machine);
  }
  return ret;
}

/**
 * bt_setup_add_wire:
 * @self: the setup to add the wire to
 * @wire: the new wire instance
 *
 * Let the setup know that the suplied wire is now part of the song.
 *
 * Returns: returns true, if the wire is added. Returns false, if the setup
 * contains a wire witch is link between the same src and dst machines (cycle
 * check).

 */
gboolean bt_setup_add_wire(const BtSetup * const self, const BtWire * const wire) {
  gboolean ret=FALSE;

  // @todo: add g_error stuff, to give the programmer more information, whats going wrong.

  g_return_val_if_fail(BT_IS_SETUP(self),FALSE);
  g_return_val_if_fail(BT_IS_WIRE(wire),FALSE);

  if(!g_list_find(self->priv->wires,wire)) {
    BtMachine *src,*dst;
    BtWire *other_wire1,*other_wire2;

    // check for wires with equal src and dst machines
    g_object_get(G_OBJECT(wire),"src",&src,"dst",&dst,NULL);
    other_wire1=bt_setup_get_wire_by_machines(self,src,dst);
    other_wire2=bt_setup_get_wire_by_machines(self,dst,src);
    if((!other_wire1) && (!other_wire2)) {
      ret=TRUE;

      self->priv->wires=g_list_append(self->priv->wires,g_object_ref(G_OBJECT(wire)));
      set_disconnected(self,GST_BIN(wire));
#if LIVE_CONNECT
      bt_setup_update_pipeline(self);
#endif
      bt_machine_renegotiate_adder_format(dst);
      g_signal_emit(G_OBJECT(self),signals[WIRE_ADDED_EVENT], 0, wire);
      bt_song_set_unsaved(self->priv->song,TRUE);
      GST_DEBUG("added wire: %p,ref_count=%d",wire,G_OBJECT(wire)->ref_count);
    }
    g_object_try_unref(other_wire1);
    g_object_try_unref(other_wire2);
    g_object_unref(src);
    g_object_unref(dst);
  }
  else {
    GST_WARNING("trying to add wire %p again",wire);
  }
  return ret;
}

/**
 * bt_setup_remove_machine:
 * @self: the setup to remove the machine from
 * @machine: the machine instance to remove
 *
 * Let the setup know that the suplied machine is removed from the song.
 */
void bt_setup_remove_machine(const BtSetup * const self, const BtMachine * const machine) {
  g_return_if_fail(BT_IS_SETUP(self));
  g_return_if_fail(BT_IS_MACHINE(machine));

  GST_DEBUG("trying to remove machine: %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);

  if(g_list_find(self->priv->machines,machine)) {
    self->priv->machines=g_list_remove(self->priv->machines,machine);
    g_hash_table_remove(self->priv->connection_state,(gpointer)machine);
    g_hash_table_remove(self->priv->state_change_order,(gpointer)machine);

    GST_DEBUG("removing machine: %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);
    g_signal_emit(G_OBJECT(self),signals[MACHINE_REMOVED_EVENT], 0, machine);

    // this triggers finalize if we don't have a ref
    if(GST_OBJECT_FLAG_IS_SET(machine,GST_OBJECT_FLOATING)) {
      gst_element_set_state(GST_ELEMENT(machine),GST_STATE_NULL);
      gst_object_unref(GST_OBJECT(machine));
    }
    else {
      gst_bin_remove(self->priv->bin,GST_ELEMENT(machine));
    }
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  else {
    GST_WARNING("trying to remove machine that is not in setup");
  }
}

/**
 * bt_setup_remove_wire:
 * @self: the setup to remove the wire from
 * @wire: the wire instance to remove
 *
 * Let the setup know that the suplied wire is removed from the song.
 */
void bt_setup_remove_wire(const BtSetup * const self, const BtWire * const wire) {
  g_return_if_fail(BT_IS_SETUP(self));
  g_return_if_fail(BT_IS_WIRE(wire));

  GST_DEBUG("trying to remove wire: %p,ref_count=%d",wire,G_OBJECT(wire)->ref_count);

  if(g_list_find(self->priv->wires,wire)) {
    self->priv->wires=g_list_remove(self->priv->wires,wire);

    GST_DEBUG("removing wire: %p,ref_count=%d",wire,G_OBJECT(wire)->ref_count);
    g_signal_emit(G_OBJECT(self),signals[WIRE_REMOVED_EVENT], 0, wire);

#if LIVE_CONNECT
    set_disconnecting(self,GST_BIN(wire));
    bt_setup_update_pipeline(self);
#else
    BtMachine *dst,*src;

    g_hash_table_remove(self->priv->connection_state,(gpointer)wire);
    g_hash_table_remove(self->priv->state_change_order,(gpointer)wire);
    g_object_get(G_OBJECT(wire),"dst",&dst,"src",&src,NULL);
    if(dst) {
      bt_machine_renegotiate_adder_format(dst);
      unlink_wire(self,GST_ELEMENT(wire),GST_ELEMENT(src),GST_ELEMENT(dst));
      gst_object_unref(src);
      gst_object_unref(dst);
    }

    // this triggers finalize if we don't have a ref
    if(GST_OBJECT_FLAG_IS_SET(wire,GST_OBJECT_FLOATING)) {
      gst_element_set_state(GST_ELEMENT(wire),GST_STATE_NULL);
      gst_object_unref(GST_OBJECT(wire));
    }
    else {
      gst_bin_remove(self->priv->bin,GST_ELEMENT(wire));
    }
#endif
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  else {
    GST_WARNING("trying to remove wire that is not in setup");
  }
}

/**
 * bt_setup_get_machine_by_id:
 * @self: the setup to search for the machine
 * @id: the identifier of the machine
 *
 * Search the setup for a machine by the supplied id.
 * The machine must have been added previously to this setup with bt_setup_add_machine().
 * Unref the machine, when done with it.
 *
 * Returns: #BtMachine instance or %NULL if not found
 */
BtMachine *bt_setup_get_machine_by_id(const BtSetup * const self, const gchar * const id) {
  gboolean found=FALSE;
  gchar * const machine_id;
  const GList* node;

  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);

  /*
    find_by_property(gpointer item,gpointer data) {
      GValue value;
      g_object_get_property(item,data.key,&value);
      //a) compare via strcmp(find_by_property(value),find_by_property(data.compare_value));
      //b) switch(g_value_get_type(value)) {
      //     G_TYPE_STRING: strcmp(value,data.compare_value);
      //     default: value==data.compare_value;
      //   }
      //-> what about: gst_value_compare()
      // @todo method puts key into a gvalue and gets the param-spec by name, then calls generalized search
      //-> what about: g_param_values_cmp()
    }

    struct {
     gchar *key;
     GValue compare_value;
    } data;
    g_value_init(data.compare_value, G_TYPE_STRING);
    g_value_set_string(data.compare_value,id);
    data.key="id";

    node = g_list_find_custom(self->priv->machines, data, find_by_property);
    if(node) return(BT_MACHINE(node->data);
  */

  for(node=self->priv->machines;node;node=g_list_next(node)) {
    BtMachine * const machine=BT_MACHINE(node->data);
    g_object_get(G_OBJECT(machine),"id",&machine_id,NULL);
    if(!strcmp(machine_id,id)) found=TRUE;
    g_free(machine_id);
    if(found) {
      GST_DEBUG("  getting machine: %p,ref_count %d",machine,(G_OBJECT(machine))->ref_count);
      return(g_object_ref(machine));
    }
  }
  GST_DEBUG("no machine found for id \"%s\"",id);
  return(NULL);
}

/**
 * bt_setup_get_machine_by_index:
 * @self: the setup to search for the machine
 * @index: the list-position of the machine
 *
 * search the setup for a machine by the supplied index position.
 * The machine must have been added previously to this setup with bt_setup_add_machine().
 * Unref the machine, when done with it.
 *
 * Returns: #BtMachine instance or %NULL if not found
 */
BtMachine *bt_setup_get_machine_by_index(const BtSetup * const self, const gulong index) {
  BtMachine *machine;

  g_return_val_if_fail(BT_IS_SETUP(self),NULL);

  if((machine=g_list_nth_data(self->priv->machines,(guint)index))) {
    return(g_object_ref(machine));
  }
  return(NULL);
}

/**
 * bt_setup_get_machine_by_type:
 * @self: the setup to search for the machine
 * @type: the gobject type (sink,processor,source)
 *
 * Search the setup for the first machine with the given type.
 * The machine must have been added previously to this setup with bt_setup_add_machine().
 * Unref the machine, when done with it.
 *
 * Returns: #BtMachine instance or %NULL if not found
 */
BtMachine *bt_setup_get_machine_by_type(const BtSetup * const self, const GType type) {
  const GList *node;

  g_return_val_if_fail(BT_IS_SETUP(self),NULL);

  for(node=self->priv->machines;node;node=g_list_next(node)) {
    BtMachine * const machine=BT_MACHINE(node->data);
    if(G_OBJECT_TYPE(machine)==type) {
      return(g_object_ref(machine));
    }
  }
  GST_DEBUG("no machine found for this type");
  return(NULL);
}

/**
 * bt_setup_get_machines_by_type:
 * @self: the setup to search for the machine
 * @type: the gobject type (sink,processor,source)
 *
 * Gathers all machines of the given type from the setup.
 * Free the list (and unref the machines), when done with it.
 *
 * Returns: the list instance or %NULL if not found
 */
GList *bt_setup_get_machines_by_type(const BtSetup * const self, const GType type) {
  GList *machines=NULL;
  const GList * node;

  g_return_val_if_fail(BT_IS_SETUP(self),NULL);

  for(node=self->priv->machines;node;node=g_list_next(node)) {
    BtMachine * const machine=BT_MACHINE(node->data);
    if(G_OBJECT_TYPE(machine)==type) {
      machines=g_list_prepend(machines,g_object_ref(machine));
    }
  }
  return(machines);
}

/**
 * bt_setup_get_wire_by_src_machine:
 * @self: the setup to search for the wire
 * @src: the machine that is at the src end of the wire
 *
 * Searches for the first wire in setup that uses the given #BtMachine as a source.
 * In other words - it returns the first wire that starts at the given #BtMachine.
 * Unref the wire, when done with it.
 *
 * Returns: the #BtWire or %NULL
 */
BtWire *bt_setup_get_wire_by_src_machine(const BtSetup * const self, const BtMachine * const src) {
  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(src),NULL);
  return(bt_setup_get_wire_by_machine_type(self,src,"src"));
}

/**
 * bt_setup_get_wire_by_dst_machine:
 * @self: the setup to search for the wire
 * @dst: the machine that is at the dst end of the wire
 *
 * Searches for the first wire in setup that uses the given #BtMachine as a target.
 * In other words - it returns the first wire that ends at the given #BtMachine.
 * Unref the wire, when done with it.
 *
 * Returns: the #BtWire or %NULL
 */
BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup *const self, const BtMachine * const dst) {
  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(dst),NULL);
  return(bt_setup_get_wire_by_machine_type(self,dst,"dst"));
}

/**
 * bt_setup_get_wire_by_machines:
 * @self: the setup to search for the wire
 * @src: the machine that is at the src end of the wire
 * @dst: the machine that is at the dst end of the wire
 *
 * Searches for a wire in setup that uses the given #BtMachine instances as a
 * source and dest.
 * Unref the wire, when done with it.
 *
 * Returns: the #BtWire or NULL
 */
BtWire *bt_setup_get_wire_by_machines(const BtSetup * const self, const BtMachine * const src, const BtMachine * const dst) {
  gboolean found=FALSE;
  BtMachine * const src_machine, * const dst_machine;
  const GList *node;

  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(src),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(dst),NULL);

  for(node=self->priv->wires;node;node=g_list_next(node)) {
    BtWire * const wire=BT_WIRE(node->data);
    g_object_get(G_OBJECT(wire),"src",&src_machine,"dst",&dst_machine,NULL);
    if((src_machine==src) && (dst_machine==dst)) found=TRUE;
    g_object_unref(src_machine);
    g_object_unref(dst_machine);
    if(found) return(g_object_ref(wire));
  }
  GST_DEBUG("no wire found for machines %p %p",src,dst);
  return(NULL);
}

/**
 * bt_setup_get_wires_by_src_machine:
 * @self: the setup to search for the wire
 * @src: the machine that is at the src end of the wire
 *
 * Searches for all wires in setup that use the given #BtMachine as a source.
 * Free the list (and unref the wires), when done with it.
 *
 * Returns: a #GList with the #BtWires or %NULL
 */
GList *bt_setup_get_wires_by_src_machine(const BtSetup * const self, const BtMachine * const src) {
  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(src),NULL);
  return(bt_setup_get_wires_by_machine_type(self,src,"src"));
}

/**
 * bt_setup_get_wires_by_dst_machine:
 * @self: the setup to search for the wire
 * @dst: the machine that is at the dst end of the wire
 *
 * Searches for all wires in setup that use the given #BtMachine as a target.
 * Free the list (and unref the wires), when done with it.
 *
 * Returns: a #GList with the #BtWires or %NULL
 */
GList *bt_setup_get_wires_by_dst_machine(const BtSetup * const self, const BtMachine * const dst) {
  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(dst),NULL);
  return(bt_setup_get_wires_by_machine_type(self,dst,"dst"));
}

/**
 * bt_setup_get_unique_machine_id:
 * @self: the setup for which the name should be unique
 * @base_name: the leading name part
 *
 * The function makes the supplied base_name unique in this setup by eventually
 * adding a number postfix. This method should be used when adding new machines.
 *
 * Returns: the newly allocated unique name
 */
gchar *bt_setup_get_unique_machine_id(const BtSetup * const self, gchar * const base_name) {
  BtMachine *machine;
  guint8 i=0;

  g_return_val_if_fail(BT_IS_SETUP(self),NULL);
  g_return_val_if_fail(BT_IS_STRING(base_name),NULL);

  if(!(machine=bt_setup_get_machine_by_id(self,base_name))) {
    return(g_strdup(base_name));
  }
  else {
    g_object_unref(machine);
    machine=NULL;
  }

  gchar * const id=g_strdup_printf("%s 00",base_name);
  gchar * const ptr=&id[strlen(base_name)+1];
  do {
    (void)g_sprintf(ptr,"%02u",i++);
    g_object_try_unref(machine);
  } while((machine=bt_setup_get_machine_by_id(self,id)) && (i<100));
  g_object_try_unref(machine);
  return(id);
}

/**
 * bt_setup_remember_missing_machine:
 * @self: the setup
 * @str: human readable description of the missing machine
 *
 * Loaders can use this function to collect information about machines that
 * failed to load.
 * The front-end can access this later by reading BtSetup::missing-machines
 * property.
 */
void bt_setup_remember_missing_machine(const BtSetup * const self, const gchar * const str) {
  gboolean skip = FALSE;

  GST_INFO("missing machine %s",str);
  if(self->priv->missing_machines) {
    GList *node;
    for(node=self->priv->missing_machines;node;node=g_list_next(node)) {
      if(!strcmp(node->data,str)) {
        skip=TRUE;break;
      }
    }
  }
  if(!skip) {
    self->priv->missing_machines=g_list_prepend(self->priv->missing_machines,(gpointer)str);
  }
  else {
    g_free((gchar *)str);
  }
}

/*
 * bt_setup_update_pipeline:
 *
 * Rebuilds the whole pipeline, after changing the setup (adding/removing and
 * linking/unlinking machines).
 */
gboolean bt_setup_update_pipeline(const BtSetup * const self) {
  gboolean res=FALSE;
  BtMachine *master;
  
  // get master
  if((master=bt_setup_get_machine_by_type(self,BT_TYPE_SINK_MACHINE))) {
    GList *not_visited_machines,*not_visited_wires,*node;
    BtWire *wire;
    BtMachine *machine;
    
    // make a copy of lists and remove all visited items
    not_visited_machines=g_list_copy(self->priv->machines);
    not_visited_wires=g_list_copy(self->priv->wires);
    GST_INFO("checking connections for %d machines and %d wires",
      g_list_length(not_visited_machines),
      g_list_length(not_visited_wires));
    // ... and start checking connections (recursively)
    res=check_connected(self,master,&not_visited_machines,&not_visited_wires);
    g_object_unref(master);
    
    // remove all items that we have not visited and set them to disconnected
    GST_INFO("remove %d unconnected wires", g_list_length(not_visited_wires));
    for(node=not_visited_wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
#if NEW_CODE
      set_disconnecting(self,GST_BIN(wire));
#else
      update_bin_in_pipeline(self,GST_BIN(wire),FALSE,NULL);
#endif
    }
    g_list_free(not_visited_wires);
    GST_INFO("remove %d unconnected machines", g_list_length(not_visited_machines));
    for(node=not_visited_machines;node;node=g_list_next(node)) {
      machine=BT_MACHINE(node->data);
#if NEW_CODE
      set_disconnecting(self,GST_BIN(machine));
#else
      update_bin_in_pipeline(self,GST_BIN(machine),FALSE,NULL);
#endif
    }
    g_list_free(not_visited_machines);   

#if NEW_CODE
    update_pipeline(self);
#endif
  }
  GST_INFO("result of graph update = %d",res);
  return(res);
}

//-- io interface

static xmlNodePtr bt_setup_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  BtSetup * const self = BT_SETUP(persistence);
  xmlNodePtr node=NULL;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::setup");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("setup"),NULL))) {
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("properties"),NULL))) {
      if(!bt_persistence_save_hashtable(self->priv->properties, child_node)) goto Error;
    }
    else goto Error;
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("machines"),NULL))) {
      bt_persistence_save_list(self->priv->machines,child_node);
    }
    else goto Error;
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("wires"),NULL))) {
      bt_persistence_save_list(self->priv->wires,child_node);
    }
    else goto Error;
  }
Error:
  return(node);
}

static BtPersistence *bt_setup_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location, GError **err, va_list var_args) {
  BtSetup * const self = BT_SETUP(persistence);
  xmlNodePtr child_node;
  gboolean failed_parts=FALSE;

  GST_DEBUG("PERSISTENCE::setup");
  g_assert(node);

  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      if(!strncmp((gchar *)node->name,"machines\0",9)) {
        GType type=0;

        //bt_song_io_native_load_setup_machines(self,song,node->children);
        for(child_node=node->children;child_node;child_node=child_node->next) {
          if(!xmlNodeIsText(child_node)) {
            /* @todo: it would be smarter to have nodes name like "sink-machine"
             * and do type=g_type_from_name((gchar *)child_node->name);
             */
            if(!strncmp((gchar *)child_node->name,"machine\0",8)) {
              xmlChar * const type_str=xmlGetProp(child_node,XML_CHAR_PTR("type"));
              if(!strncmp((gchar *)type_str,"processor\0",10)) {
                type=BT_TYPE_PROCESSOR_MACHINE;
              }
              else if(!strncmp((gchar *)type_str,"sink\0",5)) {
                type=BT_TYPE_SINK_MACHINE;
              }
              else if(!strncmp((gchar *)type_str,"source\0",7)) {
                type=BT_TYPE_SOURCE_MACHINE;
              }
              else {
                GST_WARNING("machine node has no type");
              }
              if(type) {
                GError *err=NULL;
                BtMachine *machine=BT_MACHINE(bt_persistence_load(type,NULL,child_node,NULL,&err,"song",self->priv->song,NULL));
                if(err!=NULL) {
                  // collect failed machines
                  gchar * const plugin_name;

                  g_object_get(machine,"plugin-name",&plugin_name,NULL);
                  // takes ownership of the name
                  bt_setup_remember_missing_machine(self,plugin_name);
                  failed_parts=TRUE;

                  GST_WARNING("Can't create machine: %s",err->message);
                  g_error_free(err);
                }
                g_object_unref(machine);
              }
              xmlFree(type_str);
            }
          }
        }
      }
      else if(!strncmp((gchar *)node->name,"wires\0",6)) {
        for(child_node=node->children;child_node;child_node=child_node->next) {
          if(!xmlNodeIsText(child_node)) {
            GError *err=NULL;
            // @todo: rework construction
            BtWire * const wire=BT_WIRE(bt_persistence_load(BT_TYPE_WIRE,NULL,child_node,NULL,&err,"song",self->priv->song,NULL));
            if(err!=NULL) {
              failed_parts=TRUE;
              
              GST_WARNING("Can't create wire: %s",err->message);
              g_error_free(err);
            }
            g_object_unref(wire);
          }
        }
      }
      else if(!strncmp((gchar *)node->name,"properties\0",11)) {
        bt_persistence_load_hashtable(self->priv->properties,node);
      }
    }
  }
  if(failed_parts) {
    bt_song_write_to_lowlevel_dot_file(self->priv->song);
  }
  return(BT_PERSISTENCE(persistence));
}

static void bt_setup_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_setup_persistence_load;
  iface->save = bt_setup_persistence_save;
}

//-- wrapper

//-- g_object overrides

#if 0
static void bt_setup_constructed(GObject *object) {
  BtSetup *self=BT_SETUP(object);
  
  if(G_OBJECT_CLASS(parent_class)->constructed)
    G_OBJECT_CLASS(parent_class)->constructed(object);

  g_return_if_fail(BT_IS_SONG(self->priv->song));
}
#endif

/* returns a property for the given property_id for this object */
static void bt_setup_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtSetup * const self = BT_SETUP(object);
  return_if_disposed();
  switch (property_id) {
    case SETUP_PROPERTIES: {
      g_value_set_pointer(value, self->priv->properties);
    } break;
    case SETUP_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case SETUP_MACHINES: {
      /* @todo: this is not good, lists returned by bt_setup_get_xxx_by_yyy
       * returns a new list where one has to unref the elements
       */
      g_value_set_pointer(value,g_list_copy(self->priv->machines));
    } break;
    case SETUP_WIRES: {
      /* @todo: this is not good, lists returned by bt_setup_get_xxx_by_yyy
       * returns a new list where one has to unref the elements
       */
      g_value_set_pointer(value,g_list_copy(self->priv->wires));
    } break;
    case SETUP_MISSING_MACHINES: {
      g_value_set_pointer(value,self->priv->missing_machines);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_setup_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtSetup * const self = BT_SETUP(object);
  return_if_disposed();
  switch (property_id) {
    case SETUP_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      g_object_get(G_OBJECT(self->priv->song),"bin",&self->priv->bin,NULL);
      GST_INFO("set the song for setup: %p and get the bin: %p",self->priv->song,self->priv->bin);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_setup_dispose(GObject * const object) {
  BtSetup * const self = BT_SETUP(object);
  GList* node;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // unref list of wires
  if(self->priv->wires) {
    for(node=self->priv->wires;node;node=g_list_next(node)) {
      if(node->data) {
        GObject *obj=node->data;
        GST_DEBUG("  free wire : %p (%d)",obj,obj->ref_count);

        if(GST_OBJECT_FLAG_IS_SET(obj,GST_OBJECT_FLOATING)) {
          gst_element_set_state(GST_ELEMENT(obj),GST_STATE_NULL);
          gst_object_unref(obj);
        }
        else if(self->priv->bin) {
          gst_bin_remove(self->priv->bin,GST_ELEMENT(obj));
        }
        node->data=NULL;
      }
    }
  }
  // unref list of machines
  if(self->priv->machines) {
    for(node=self->priv->machines;node;node=g_list_next(node)) {
      if(node->data) {
        GObject *obj=node->data;
        GST_DEBUG("  free machine : %p (%d)",obj,obj->ref_count);

        if(GST_OBJECT_FLAG_IS_SET(obj,GST_OBJECT_FLOATING)) {
          gst_element_set_state(GST_ELEMENT(obj),GST_STATE_NULL);
          gst_object_unref(obj);
        }
        else if(self->priv->bin) {
          gst_bin_remove(self->priv->bin,GST_ELEMENT(obj));
        }
        node->data=NULL;
      }
    }
  }

  if(self->priv->bin) {
    gst_object_unref(self->priv->bin);
    self->priv->bin=NULL;
  }
  g_object_try_weak_unref(self->priv->song);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_setup_finalize(GObject * const object) {
  BtSetup * const self = BT_SETUP(object);

  GST_DEBUG("!!!! self=%p",self);

  // free list of wires
  if(self->priv->wires) {
    g_list_free(self->priv->wires);
    self->priv->wires=NULL;
  }
  // free list of machines
  if(self->priv->machines) {
    g_list_free(self->priv->machines);
    self->priv->machines=NULL;
  }
  // free list of missing_machines
  if(self->priv->missing_machines) {
    const GList* node;
    for(node=self->priv->missing_machines;node;node=g_list_next(node)) {
      g_free(node->data);
    }
    g_list_free(self->priv->missing_machines);
    self->priv->missing_machines=NULL;
  }

  g_hash_table_destroy(self->priv->properties);
  g_hash_table_destroy(self->priv->connection_state);
  g_hash_table_destroy(self->priv->state_change_order);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

//-- class internals

static void bt_setup_init(const GTypeInstance * const instance, gconstpointer g_class) {
  BtSetup * const self = BT_SETUP(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SETUP, BtSetupPrivate);

  self->priv->properties=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
  self->priv->connection_state=g_hash_table_new(NULL,NULL);
  self->priv->state_change_order=g_hash_table_new(NULL,NULL);
}

static void bt_setup_class_init(BtSetupClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSetupPrivate));

  gobject_class->set_property = bt_setup_set_property;
  gobject_class->get_property = bt_setup_get_property;
  gobject_class->dispose      = bt_setup_dispose;
  gobject_class->finalize     = bt_setup_finalize;

  klass->machine_added_event = NULL;
  klass->wire_added_event = NULL;
  klass->machine_removed_event = NULL;
  klass->wire_removed_event = NULL;

  /**
   * BtSetup::machine-added:
   * @self: the setup object that emitted the signal
   * @machine: the new machine
   *
   * A new machine item has been added to the setup
   */
  signals[MACHINE_ADDED_EVENT] = g_signal_new("machine-added",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        (guint)G_STRUCT_OFFSET(BtSetupClass,machine_added_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_MACHINE // param data
                                        );

  /**
   * BtSetup::wire-added:
   * @self: the setup object that emitted the signal
   * @wire: the new wire
   *
   * A new wire item has been added to the setup
   */
  signals[WIRE_ADDED_EVENT] = g_signal_new("wire-added",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        (guint)G_STRUCT_OFFSET(BtSetupClass,wire_added_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_WIRE // param data
                                        );

  /**
   * BtSetup::machine-removed:
   * @self: the setup object that emitted the signal
   * @machine: the old machine
   *
   * A machine item has been removed from the setup
   */
  signals[MACHINE_REMOVED_EVENT] = g_signal_new("machine-removed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        (guint)G_STRUCT_OFFSET(BtSetupClass,machine_removed_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_MACHINE // param data
                                        );

  /**
   * BtSetup::wire-removed:
   * @self: the setup object that emitted the signal
   * @wire: the old wire
   *
   * A wire item has been removed from the setup
   */
  signals[WIRE_REMOVED_EVENT] = g_signal_new("wire-removed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        (guint)G_STRUCT_OFFSET(BtSetupClass,wire_removed_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_WIRE // param data
                                        );

  g_object_class_install_property(gobject_class,SETUP_PROPERTIES,
                                  g_param_spec_pointer("properties",
                                     "properties prop",
                                     "list of setup properties",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SETUP_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the setup belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SETUP_MACHINES,
                                  g_param_spec_pointer("machines",
                                     "machine list prop",
                                     "A copy of the list of machines",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SETUP_WIRES,
                                  g_param_spec_pointer("wires",
                                     "wire list prop",
                                     "A copy of the list of wires",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SETUP_MISSING_MACHINES,
                                  g_param_spec_pointer("missing-machines",
                                     "missing-machines list prop",
                                     "The list of missing machines, don't change",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_setup_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSetupClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_setup_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSetup),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_setup_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_setup_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSetup",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
