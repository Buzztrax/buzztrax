/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btsetup
 * @short_description: class with all machines and wires (#BtMachine and #BtWire)
 * for a #BtSong instance
 *
 * The setup manages virtual gear in a #BtSong. This is a list of #BtMachines
 * that are used and the #BtWires that connect them.
 *
 * The setup manages the actual GStreamer #GstPipeline content. Freshly created
 * machines are not yet added to the pipeline. Only once a subgraph from a
 * source is fully connected to the sink, that subgraph is added o the pipeline.
 *
 * Applications can watch the GstObject:parent property to see whether a machine
 * is physically inserted into the processing pipeline.
 *
 * The setup takes ownership of the machines and wires. They are automatically
 * added when they are created and destroyed together with the setup.
 */
/* support dynamic (un)linking (while playing)
 *
 * When we add a machine, we do nothing else
 * When we remove a machine, we remove all connected wires
 *
 * When we add a wire, we run check_connected(self,master,NULL,NULL).
 * When we remove a wire, we run check_connected(self,master,NULL,NULL).
 *
 * We don't need to handle the not_visited_* lists. Disconnected things are
 * never added (see {add,rem}_bin_in_pipeline()). 
 *
 * When adding/removing a bin while playing, block all src-pads that are
 * connected to existing elements when linking.
 * Once the deep scan of the graph is finished, we can unblock all remembered
 * pads. Also send a seek and a tag event to newly added sources.
 *
 * When dynamically adding/removing we ideally do not make any change to the
 * existing pipeline, but collect a list of machines and wires we want to
 * add/remove (subgraph). We also need to know if the new subgraph makes any
 * practical change (does the new wire connect to the master in the end or is
 * the wire we remove connected).
 *
 * Adding wires ----------------------------------------------------------------
 * - if the target of new wire is not connecting to the master or the src of the
 *     wire is not reaching a source or connected machine, just add to setup
 *   else -> build subgraph (we want all wires+machines that become fully
 *     connected as the result of adding this wire)
 * - for the subgraph we would study all wires that are not connected
 *   not_connected = get_list_of_unconnected_wires();
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
 *       {add,rem}_bin_in_pipeline(...);
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
 * Graph before:
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
 * Graph before:
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
 *   else -> build subgraph (we want all wires+machines that become disconnected
 *     as the result of removing this wire)
 * - for the subgraph we would study all wires that are connected
 *   - we can do this similar as for adding
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
 * Graph before:
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
 * We could print the song-graph as a matrix for debugging purposes
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
 *   - connected elements that should be disconnected will be set to DISCONNECTING
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
 */
/* support dynamic (un)linking (while playing) - new attempt
 * - we don't care about adding removing machines
 * - we care about (un)linking wires (when removing machines we automatically
 *   unlink)
 * - tools has two helpers for savely liking/unlinking:
 *   bt_bin_activate_tee_chain/bt_bin_deactivate_tee_chain
 *   - instead of GList *analyzers we have GList *elements_to_{play,stop}
 * - we can do something along the lines, when (unlinking) wires, we need
 *   to detect the special case where the new wire would activate a subgraph
 *   or where the exising wire would deactivate a subgraph
 * - this is the case when elements_to_{play,stop} is not empty
 * - these lists are topologically sorted, in the play (=add) case the first
 *   element is closest to the sink, in the stop (=del) case the last one
 *   is closest to the sink
 */
/* TODO(ensonic):
 * - each time we/add remove a wire, we update the whole graph. Would be nice
 *   to avoid this when loading songs.
 */

// play safe when updating the song pipeline
#define STOP_PLAYBACK_FOR_UPDATES

#define BT_CORE
#define BT_SETUP_C

#include "core_private.h"

//-- signal ids

enum
{
  MACHINE_ADDED_EVENT,
  MACHINE_REMOVED_EVENT,
  WIRE_ADDED_EVENT,
  WIRE_REMOVED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum
{
  SETUP_PROPERTIES = 1,
  SETUP_SONG,
  SETUP_MACHINES,
  SETUP_WIRES,
  SETUP_MISSING_MACHINES
};

/* state of elements in graph
 *
 * CS_DISCONNECTED -> CS_CONNECTING
 *        ^                 |
 *        |                 v
 * CS_DISCONNECTING <- CS_CONNECTED
 */
typedef enum
{
  /* start with 1, so that we can differentiate between NULL and DISCONNECTED
   * when doing g_hash_table_lookup */
  CS_DISCONNECTED = 1,
  CS_DISCONNECTING,
  CS_CONNECTING,
  CS_CONNECTED
} BtSetupConnectionState;

#define GET_CONNECTION_STATE(self,bin) \
  GPOINTER_TO_INT(g_hash_table_lookup(self->priv->connection_state,(gpointer)bin))
#define SET_CONNECTION_STATE(self,bin,state) \
  g_hash_table_insert(self->priv->connection_state,(gpointer)bin,\
      GINT_TO_POINTER (state))

#define GET_GRAPH_DEPTH(self,bin) \
  GPOINTER_TO_INT(g_hash_table_lookup(self->priv->graph_depth,(gpointer)bin))
#define SET_GRAPH_DEPTH(self,bin,depth) \
  g_hash_table_insert(self->priv->graph_depth,(gpointer)bin,GINT_TO_POINTER(depth));

struct _BtSetupPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the setup belongs to */
  BtSong *song;

  /* the top-level gstreamer container element */
  GstBin *bin;

  GList *machines;              // each entry points to BtMachine
  GList *wires;                 // each entry points to BtWire
  GList *missing_machines;      // each entry points to a gchar*

  /* (ui) properties associated with this song
   * zoom. scroll-position */
  GHashTable *properties;

  /* state of elements (wires and machines) as BtSetupConnectionState */
  GHashTable *connection_state;
  /* depth in the graph, master=0, wires odd, machines even numbers */
  GHashTable *graph_depth;
  /* list of elements to add or remove, in topological order */
  GList *machines_to_add, *wires_to_add, *machines_to_del, *wires_to_del;
  /* list of elements that change state, in topological order */
  GList *elements_to_play, *elements_to_stop;

  /* see/newsegment event for dynamically added elements */
  GstEvent *play_seek_event;
  GstEvent *play_newsegment_event;
};

static guint signals[LAST_SIGNAL] = { 0, };

//-- the class

static void bt_setup_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSetup, bt_setup, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_setup_persistence_interface_init));


//-- constructor methods

/**
 * bt_setup_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSetup *
bt_setup_new (const BtSong * const song)
{
  return (BT_SETUP (g_object_new (BT_TYPE_SETUP, "song", song, NULL)));
}

//-- private methods

static void
set_disconnected (const BtSetup * const self, GstBin * bin)
{
  BtSetupConnectionState state = GET_CONNECTION_STATE (self, bin);

  if (state == 0 || state == CS_DISCONNECTING) {
    SET_CONNECTION_STATE (self, bin, CS_DISCONNECTED);
  } else if (state == CS_CONNECTING) {
    GST_WARNING_OBJECT (bin,
        "disallowed state-change from connecting to disconnected");
  } else if (state == CS_CONNECTED) {
    GST_WARNING_OBJECT (bin,
        "disallowed state-change from connected to disconnected");
  }
}

static void
set_disconnecting (const BtSetup * const self, GstBin * bin)
{
  BtSetupConnectionState state = GET_CONNECTION_STATE (self, bin);

  if (state == CS_CONNECTED) {
    SET_CONNECTION_STATE (self, bin, CS_DISCONNECTING);
  } else if (state == CS_CONNECTING) {
    GST_WARNING_OBJECT (bin,
        "disallowed state-change from connecting to disconnecting");
  }
}

static void
set_connecting (const BtSetup * const self, GstBin * bin)
{
  BtSetupConnectionState state = GET_CONNECTION_STATE (self, bin);

  if (state == CS_DISCONNECTED) {
    SET_CONNECTION_STATE (self, bin, CS_CONNECTING);
  } else if (state == CS_DISCONNECTING) {
    GST_WARNING_OBJECT (bin,
        "disallowed state-change from disconnecting to connecting");
  }
}

static void
set_connected (const BtSetup * const self, GstBin * bin)
{
  BtSetupConnectionState state = GET_CONNECTION_STATE (self, bin);

  if (state == CS_CONNECTING) {
    SET_CONNECTION_STATE (self, bin, CS_CONNECTED);
  } else if (state == CS_DISCONNECTING) {
    GST_WARNING_OBJECT (bin,
        "disallowed state-change from disconnecting to connected");
  } else if (state == CS_DISCONNECTED) {
    GST_WARNING_OBJECT (bin,
        "disallowed state-change from disconnected to connected");
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
static BtWire *
bt_setup_get_wire_by_machine_type (const BtSetup * const self,
    const BtMachine * const machine, const gchar * const type)
{
  gboolean found = FALSE;
  BtMachine *const search_machine;
  GList *node;

  for (node = self->priv->wires; node; node = g_list_next (node)) {
    BtWire *const wire = BT_WIRE (node->data);
    g_object_get (wire, type, &search_machine, NULL);
    if (search_machine == machine)
      found = TRUE;
    g_object_unref (search_machine);
    if (found)
      return g_object_ref (wire);
  }
  GST_DEBUG ("no wire found for %s-machine %p", type, machine);
  return (NULL);
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
static GList *
bt_setup_get_wires_by_machine_type (const BtSetup * const self,
    const BtMachine * const machine, const gchar * const type)
{
  GList *wires = NULL, *node;
  BtMachine *const search_machine;

  for (node = self->priv->wires; node; node = g_list_next (node)) {
    BtWire *const wire = BT_WIRE (node->data);
    g_object_get (wire, type, &search_machine, NULL);
    if (search_machine == machine) {
      wires = g_list_prepend (wires, g_object_ref (wire));
    }
    g_object_unref (search_machine);
  }
  return (wires);
}

static gboolean
link_wire (const BtSetup * const self, GstElement * wire)
{
  GstPadLinkReturn link_res;
  GstElement *src, *dst;
  GstPadTemplate *tmpl;
  GstPad *src_pad, *dst_pad, *peer;
  gboolean res = TRUE;

  g_object_get (wire, "src", &src, "dst", &dst, NULL);

  // link start of wire
  dst_pad = gst_element_get_static_pad (GST_ELEMENT (wire), "sink");
  GST_INFO_OBJECT (dst_pad, "linking start of wire");
  if (!(peer = gst_pad_get_peer (dst_pad))) {
    if ((tmpl =
            gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (src),
                "src_%u"))) {
      if ((src_pad =
              gst_element_request_pad (GST_ELEMENT (src), tmpl, NULL, NULL))) {
        if (GST_PAD_LINK_FAILED (link_res = gst_pad_link (src_pad, dst_pad))) {
          GST_WARNING ("Can't link start of wire : %s",
              bt_gst_debug_pad_link_return (link_res, src_pad, dst_pad));
          res = FALSE;
          gst_element_release_request_pad (src, src_pad);
          // TODO(ensonic): unblock the pad
        }
      } else {
        GST_WARNING_OBJECT (src,
            "Can't get request pad for src-peer of start of wire");
        res = FALSE;
      }
    } else {
      GST_WARNING_OBJECT (src,
          "Can't lookup pad-template 'src_%%u' for src-peer of start of wire");
      res = FALSE;
    }
  } else {
    GST_INFO ("start of wire is already linked");
    gst_object_unref (peer);
  }
  gst_object_unref (dst_pad);
  if (!res)
    goto Error;

  // link end of wire
  src_pad = gst_element_get_static_pad (GST_ELEMENT (wire), "src");
  GST_INFO_OBJECT (src_pad, "linking end of wire");
  if (!(peer = gst_pad_get_peer (src_pad))) {
    if ((tmpl =
            gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (dst),
                "sink_%u"))) {
      if ((dst_pad =
              gst_element_request_pad (GST_ELEMENT (dst), tmpl, NULL, NULL))) {
        if (GST_PAD_LINK_FAILED (link_res = gst_pad_link (src_pad, dst_pad))) {
          GST_WARNING ("Can't link end of wire : %s",
              bt_gst_debug_pad_link_return (link_res, src_pad, dst_pad));
          res = FALSE;
          gst_element_release_request_pad (dst, dst_pad);
        }
      } else {
        GST_WARNING_OBJECT (dst,
            "Can't get request pad for sink-peer of end of wire");
        res = FALSE;
      }
    } else {
      GST_WARNING_OBJECT (dst,
          "Can't lookup pad-template 'sink_%%u' for sink-peer of end of wire");
      res = FALSE;
    }
  } else {
    GST_INFO ("end of wire is already linked");
    gst_object_unref (peer);
  }
  gst_object_unref (src_pad);
Error:
  g_object_unref (src);
  g_object_unref (dst);
  return res;
}

static void
unlink_wire (const BtSetup * const self, GstElement * wire)
{
  GstElement *src, *dst;
  GstPad *src_pad, *dst_pad;

  g_object_get (wire, "src", &src, "dst", &dst, NULL);

  // unlink end of wire
  src_pad = gst_element_get_static_pad (wire, "src");
  GST_INFO_OBJECT (src_pad, "unlinking end of wire");
  if ((dst_pad = gst_pad_get_peer (src_pad))) {
    gst_pad_unlink (src_pad, dst_pad);
    gst_element_release_request_pad (dst, dst_pad);
    // unref twice: one for gst_pad_get_peer() and once for the request_pad
    gst_object_unref (dst_pad);
    gst_object_unref (dst_pad);
  } else {
    GST_WARNING_OBJECT (src_pad, "wire is not linked to dst %s",
        GST_OBJECT_NAME (dst));
  }
  gst_object_unref (src_pad);

  // unlink start of wire
  dst_pad = gst_element_get_static_pad (wire, "sink");
  GST_INFO_OBJECT (dst_pad, "unlinking start of wire");
  if ((src_pad = gst_pad_get_peer (dst_pad))) {
    gst_pad_unlink (src_pad, dst_pad);
    gst_element_release_request_pad (src, src_pad);
    // unref twice: one for gst_pad_get_peer() and once for the request_pad
    gst_object_unref (src_pad);
    gst_object_unref (src_pad);
  } else {
    GST_WARNING_OBJECT (dst_pad, "wire is not linked to src %s",
        GST_OBJECT_NAME (src));
  }
  gst_object_unref (dst_pad);

  g_object_unref (src);
  g_object_unref (dst);
}

/*
 * add_bin_in_pipeline:
 * @self: the setup object
 *
 * Add machine or wires to/from the main pipeline.
 */
static void
add_bin_in_pipeline (const BtSetup * const self, GstBin * bin)
{
  gboolean is_added = (GST_OBJECT_PARENT (bin) != NULL);

  GST_INFO_OBJECT (bin, "add object: added=%d,%" G_OBJECT_REF_COUNT_FMT,
      is_added, G_OBJECT_LOG_REF_COUNT (bin));

  if (!is_added) {
    // when we rename elements after they have been added, the object name stays
    // unchanged, thus we need to be careful when adding another bjec of the same 
    // type
    GstElement *e;
    if ((e = gst_bin_get_by_name (self->priv->bin, GST_OBJECT_NAME (bin)))) {
      gchar *name = g_strdup_printf ("%s_%p", GST_OBJECT_NAME (bin), bin);
      gst_object_set_name (GST_OBJECT (bin), name);
      g_free (name);
      gst_object_unref (e);
    }

    if (gst_bin_add (self->priv->bin, GST_ELEMENT (bin))) {
      GST_INFO_OBJECT (bin, "addded object: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (bin));
      // notify::GstObject.parent does not work
      // see https://bugzilla.gnome.org/show_bug.cgi?id=693281
      g_object_notify ((GObject *) bin, "parent");
    } else {
      GST_WARNING_OBJECT (bin, "error adding object: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (bin));
    }
  }
}

/*
 * rem_bin_in_pipeline:
 * @self: the setup object
 *
 * Remove machine or wires to/from the main pipeline.
 */
static void
rem_bin_in_pipeline (const BtSetup * const self, GstBin * bin)
{
  gboolean is_added = (GST_OBJECT_PARENT (bin) != NULL);

  GST_INFO_OBJECT (bin, "remove object: added=%d,%" G_OBJECT_REF_COUNT_FMT,
      is_added, G_OBJECT_LOG_REF_COUNT (bin));
  if (is_added) {
    if (gst_bin_remove (self->priv->bin, GST_ELEMENT (bin))) {
      GST_INFO_OBJECT (bin, "removed object: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (bin));
    } else {
      GST_WARNING_OBJECT (bin,
          "error removing object: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (bin));
    }
  }
}

/*
 * check_connected:
 * @self: the setup object
 * @dst_machine: the machine to start with, usually the master
 * @depth: graph depth
 *
 * Check if a machine is connected. It recurses up on the machines input side.
 * Its adds connected machines and wires to the song and removed unconnected
 * ones. It links connected bins and unlinks unconnected ones.
 *
 * Returns: %TRUE if there is a connection to a src.
 */
static gboolean
check_connected (const BtSetup * const self, BtMachine * dst_machine,
    GList ** not_visited_machines, GList ** not_visited_wires, gint depth)
{
  gboolean is_connected = FALSE, wire_is_connected;
  BtWire *wire;
  BtMachine *src_machine;
  GList *node;

  SET_GRAPH_DEPTH (self, dst_machine, depth);

  GST_INFO_OBJECT (dst_machine, "check %d incoming wires",
      g_list_length (dst_machine->dst_wires));

  for (node = dst_machine->dst_wires; node; node = g_list_next (node)) {
    wire = BT_WIRE (node->data);

    SET_GRAPH_DEPTH (self, wire, depth + 1);

    // check if wire is marked for removal
    if (GET_CONNECTION_STATE (self, wire) != CS_DISCONNECTING) {
      wire_is_connected = FALSE;
      g_object_get (wire, "src", &src_machine, NULL);
      if (BT_IS_SOURCE_MACHINE (src_machine)) {
        SET_GRAPH_DEPTH (self, src_machine, depth + 2);
        /* for source machine we can stop the recursion */
        wire_is_connected = TRUE;
        *not_visited_machines =
            g_list_remove (*not_visited_machines, (gconstpointer) src_machine);
      } else {
        /* for processor machine we might need to look further */
        BtSetupConnectionState state = GET_CONNECTION_STATE (self, src_machine);

        /* If machine is not in "not_visited_machines" anymore,
         * check if it is added or not and return */
        if ((!g_list_find (*not_visited_machines, src_machine))
            && (state == CS_CONNECTING || state == CS_CONNECTED)) {
          wire_is_connected = TRUE;
        } else {
          *not_visited_machines =
              g_list_remove (*not_visited_machines,
              (gconstpointer) src_machine);
          wire_is_connected |=
              check_connected (self, src_machine, not_visited_machines,
              not_visited_wires, depth + 2);
        }

      }
      GST_INFO_OBJECT (src_machine, "wire target checked, connected=%d?",
          wire_is_connected);
      if (!wire_is_connected) {
        set_disconnecting (self, GST_BIN (wire));
        set_disconnecting (self, GST_BIN (src_machine));
      }
      if (wire_is_connected) {
        if (!is_connected) {
          set_connecting (self, GST_BIN (dst_machine));
        }
        set_connecting (self, GST_BIN (src_machine));
        set_connecting (self, GST_BIN (wire));
      } else {
        GST_INFO ("skip disconnecting wire");
      }
      is_connected |= wire_is_connected;
      g_object_unref (src_machine);
    } else {
      GST_INFO_OBJECT (wire, "wire target checked, connected=0?");
    }
    *not_visited_wires =
        g_list_remove (*not_visited_wires, (gconstpointer) wire);
  }
  if (!is_connected) {
    set_disconnecting (self, GST_BIN (dst_machine));
  }
  *not_visited_machines =
      g_list_remove (*not_visited_machines, (gconstpointer) dst_machine);
  GST_INFO ("all wire targets checked, connected=%d?", is_connected);
  return (is_connected);
}

#ifndef STOP_PLAYBACK_FOR_UPDATES
static void
update_play_seek_event (const BtSetup * self)
{
  BtSequence *sequence;
  gboolean loop;
  glong loop_end, length;
  gulong play_pos;
  GstClockTime tick_duration;
  GstClockTime start, stop;

  bt_song_update_playback_position (self->priv->song);
  bt_child_proxy_get (self->priv->song, "sequence", &sequence, "play-pos",
      &play_pos, "song-info::tick-duration", &tick_duration, NULL);
  g_object_get (sequence, "loop", &loop, "loop-end", &loop_end, "length",
      &length, NULL);
  start = play_pos * tick_duration;
  // configure self->priv->play_seek_event (sent to newly activated elements)
  if (loop) {
    stop = (loop_end + 0) * tick_duration;
    self->priv->play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET, stop);
  } else {
    stop = (length + 1) * tick_duration;
    self->priv->play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET, stop);
  }
  self->priv->play_newsegment_event =
      gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_TIME, start, stop,
      start);
  g_object_unref (sequence);
}
#endif

static gint
sort_by_graph_depth_asc (gconstpointer e1, gconstpointer e2, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);
  gint d1 = GET_GRAPH_DEPTH (self, e1);
  gint d2 = GET_GRAPH_DEPTH (self, e2);

  return (d1 - d2);
}

static gint
sort_by_graph_depth_desc (gconstpointer e1, gconstpointer e2,
    gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);
  gint d1 = GET_GRAPH_DEPTH (self, e1);
  gint d2 = GET_GRAPH_DEPTH (self, e2);

  return (d1 - d2);
}

static void
prepare_add_del (gpointer key, gpointer value, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

  if (GPOINTER_TO_INT (value) == CS_CONNECTING) {
    if (BT_IS_MACHINE (key)) {
      self->priv->machines_to_add =
          g_list_insert_sorted_with_data (self->priv->machines_to_add, key,
          sort_by_graph_depth_asc, (gpointer) self);
    } else {
      self->priv->wires_to_add =
          g_list_insert_sorted_with_data (self->priv->wires_to_add, key,
          sort_by_graph_depth_asc, (gpointer) self);
    }
  } else if (GPOINTER_TO_INT (value) == CS_DISCONNECTING) {
    if (BT_IS_MACHINE (key)) {
      self->priv->machines_to_del =
          g_list_insert_sorted_with_data (self->priv->machines_to_del, key,
          sort_by_graph_depth_desc, (gpointer) self);
    } else {
      self->priv->wires_to_del =
          g_list_insert_sorted_with_data (self->priv->wires_to_del, key,
          sort_by_graph_depth_desc, (gpointer) self);
    }
  }
}

static void
add_machine_in_pipeline (gpointer key, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

  GST_INFO_OBJECT (key, "add machine");
  add_bin_in_pipeline (self, GST_BIN (key));
}

static void
add_wire_in_pipeline (gpointer key, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

  GST_INFO_OBJECT (key, "add & link wire");
  add_bin_in_pipeline (self, GST_BIN (key));
  link_wire (self, GST_ELEMENT (key));
  // TODO(ensonic): what to do if it fails? We should always be able to link in theory
  // maybe dump extensive diagnostics to add debugging
}

static void
del_wire_in_pipeline (gpointer key, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

  GST_INFO_OBJECT (key, "remove & unlink wire");
  unlink_wire (self, GST_ELEMENT (key));
  rem_bin_in_pipeline (self, GST_BIN (key));
}

static void
del_machine_in_pipeline (gpointer key, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

  GST_INFO_OBJECT (key, "remove machine");
  rem_bin_in_pipeline (self, GST_BIN (key));
}

static void
determine_state_change_lists (gpointer key, gpointer value, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

  if (GPOINTER_TO_INT (value) == CS_CONNECTING) {
    // list starts with elements close to the sink
    GST_DEBUG_OBJECT (GST_OBJECT (key), "added to play list with depth=%d",
        GET_GRAPH_DEPTH (self, key));
    self->priv->elements_to_play =
        g_list_insert_sorted_with_data (self->priv->elements_to_play, key,
        sort_by_graph_depth_asc, (gpointer) self);
  } else if (GPOINTER_TO_INT (value) == CS_DISCONNECTING) {
    // list starts with elements away from the sink
    GST_DEBUG_OBJECT (GST_OBJECT (key), "added to stop list with depth=%d",
        GET_GRAPH_DEPTH (self, key));
    self->priv->elements_to_stop =
        g_list_insert_sorted_with_data (self->priv->elements_to_stop, key,
        sort_by_graph_depth_desc, (gpointer) self);
  }
}

static void
activate_element (const BtSetup * const self, gpointer key)
{
  GstElement *elem = GST_ELEMENT (key);
  gst_element_set_locked_state (elem, FALSE);

#ifndef STOP_PLAYBACK_FOR_UPDATES
  if (GST_STATE (GST_OBJECT_PARENT (GST_OBJECT (key))) == GST_STATE_PLAYING) {
    GstStateChangeReturn ret;
    GST_INFO_OBJECT (GST_OBJECT (key), "set from %s to %s",
        gst_element_state_get_name (GST_STATE (key)),
        gst_element_state_get_name (GST_STATE_PLAYING));

    if (BT_IS_SOURCE_MACHINE (key)) {
      // TODO(ensonic): this is causing the lockups in the tests under valgrind
      // - it is not the flush-flag of the seek
      // - collect-pads needs a newsegment to define the segment on the pad for
      //   the clipfunc
      // - seek in ready will ensure it, but is somewhat non-standard and
      //   as a downside, we run into trouble at segment done
      // - we're trying to address that with the newsegment below, 
      //   but that only needs to be send to the active part we're linking to
      // - see gst-plugins-base/adder: 0cce8ab97d614ef53970292bd403e7f4460d79f9
      ret = gst_element_set_state (elem, GST_STATE_READY);
      // going to ready should not by async
      GST_INFO_OBJECT (key, "state-change to READY: %s",
          gst_element_state_change_return_get_name (ret));
      if (!(gst_element_send_event (elem,
                  gst_event_ref (self->priv->play_seek_event)))) {
        GST_WARNING_OBJECT (key, "failed to handle seek event");
      }
      GST_INFO_OBJECT (key, "sent seek event");
#if 0
    } else {
      // this seems to be a bad idea and causing the lockups
      if (!(gst_element_send_event (elem,
                  gst_event_ref (self->priv->play_newsegment_event)))) {
        GST_WARNING_OBJECT (key, "failed to handle newsegment event");
      }
      GST_INFO_OBJECT (key, "sent newsegment event");
#endif
    }
    ret = gst_element_set_state (elem, GST_STATE_PLAYING);
    GST_INFO_OBJECT (key, "state-change to PLAYING: %s",
        gst_element_state_change_return_get_name (ret));
  }
#endif
}

static void
deactivate_element (const BtSetup * const self, gpointer key)
{
  GstElement *elem = GST_ELEMENT (key);
  gst_element_set_locked_state (elem, TRUE);

#ifndef STOP_PLAYBACK_FOR_UPDATES
  if (GST_STATE (GST_OBJECT_PARENT (GST_OBJECT (key))) == GST_STATE_PLAYING) {
    GST_INFO_OBJECT (GST_OBJECT (key), "set from %s to %s",
        gst_element_state_get_name (GST_STATE (key)),
        gst_element_state_get_name (GST_STATE_PAUSED));

    gst_element_set_state (elem, GST_STATE_READY);
  }
#endif
}

static void
sync_states (const BtSetup * const self)
{
  GList *node;

  if (self->priv->elements_to_play) {
    GST_INFO ("starting elements");

#ifndef GST_DISABLE_DEBUG
    for (node = self->priv->elements_to_play; node; node = g_list_next (node)) {
      GST_INFO_OBJECT (GST_OBJECT (node->data),
          "will be set from %s to %s, depth=%d",
          gst_element_state_get_name (GST_STATE (node->data)),
          gst_element_state_get_name (GST_STATE (GST_OBJECT_PARENT (GST_OBJECT
                      (node->data)))), GET_GRAPH_DEPTH (self, node->data));
    }
#endif
    for (node = self->priv->elements_to_play; node; node = g_list_next (node)) {
      activate_element (self, node->data);
    }
    g_list_free (self->priv->elements_to_play);
    self->priv->elements_to_play = NULL;
  }
  if (self->priv->elements_to_stop) {
    GST_INFO ("stopping elements");
#ifndef GST_DISABLE_DEBUG
    for (node = self->priv->elements_to_stop; node; node = g_list_next (node)) {
      GST_INFO_OBJECT (GST_OBJECT (node->data),
          "will be set from %s to %s, depth=%d",
          gst_element_state_get_name (GST_STATE (node->data)),
          gst_element_state_get_name (GST_STATE_PAUSED), GET_GRAPH_DEPTH (self,
              node->data));
    }
#endif
    for (node = self->priv->elements_to_stop; node; node = g_list_next (node)) {
      deactivate_element (self, node->data);
    }
    g_list_free (self->priv->elements_to_stop);
    self->priv->elements_to_stop = NULL;
  }
}

static void
update_connection_states (gpointer key, gpointer value, gpointer user_data)
{
  const BtSetup *const self = BT_SETUP (user_data);

#ifndef GST_DISABLE_GST_DEBUG
  gchar *states[] =
      { "-", "disconnected", "disconnecting", "connecting", "connected" };
  GST_INFO_OBJECT (key, "%s", states[GPOINTER_TO_INT (value)]);
#endif

  if (GPOINTER_TO_INT (value) == CS_CONNECTING) {
    set_connected (self, GST_BIN (key));
  } else if (GPOINTER_TO_INT (value) == CS_DISCONNECTING) {
    set_disconnected (self, GST_BIN (key));
  }
}

static void
update_pipeline (const BtSetup * const self)
{
  GST_INFO ("updating pipeline ----------------------------------------");

  // do *one* g_hash_table_foreach and topologically sort machines and wires into 4 lists
  g_hash_table_foreach (self->priv->connection_state, prepare_add_del,
      (gpointer) self);
  gboolean need_add = self->priv->machines_to_add || self->priv->wires_to_add;
  gboolean need_del = self->priv->machines_to_del || self->priv->wires_to_del;
  GST_INFO ("adding m=%d, w=%d and deleting m=%d, w=%d",
      g_list_length (self->priv->machines_to_add),
      g_list_length (self->priv->wires_to_add),
      g_list_length (self->priv->machines_to_del),
      g_list_length (self->priv->wires_to_del));
  if (need_add && need_del) {
    GST_WARNING ("unexpected adding and deleting at the same time");
  }
#ifndef STOP_PLAYBACK_FOR_UPDATES
  // query segment and position
  update_play_seek_event (self);
#endif

  // builds elements_to_{play|stop} lists
  GST_INFO ("determine state change lists");
  g_hash_table_foreach (self->priv->connection_state,
      determine_state_change_lists, (gpointer) self);

#ifdef STOP_PLAYBACK_FOR_UPDATES
  gboolean cont = FALSE;
  if (self->priv->wires_to_add || self->priv->wires_to_del) {
    if ((cont = bt_song_update_playback_position (self->priv->song))) {
      bt_song_stop (self->priv->song);
    }
  }
#endif

  /* {add|del}_wire_in_pipeline can add pads to the self->priv->blocked_pads list
   * from {link|unlink}_wire. this is more complicated that it needs to be.
   *
   * 1.) when (un)linking a sink pad on an active element hat operates in
   *     push mode, we only need to block the most downstream src-pad
   *     (either g_list_first(wires_to_add):src or g_list_last(wires_to_del):src)
   * 2.) when (un)linking a src pad on an active element hat operates in push
   *     mode, we only need to block the pad itself
   *
   * 3.) we either add or remove stuff on exactly on pad at a time in practice
   *
   * this should make it easier to split the update into two parts to correctly
   * handle the async pad blocking
   */
  GST_INFO ("add machines");
  g_list_foreach (self->priv->machines_to_add, add_machine_in_pipeline,
      (gpointer) self);
  GST_INFO ("add and link wires");
  g_list_foreach (self->priv->wires_to_add, add_wire_in_pipeline,
      (gpointer) self);

  // apply state changes for the above lists
  GST_INFO ("sync states");
  sync_states (self);
  GST_INFO ("unlink and remove wires");
  g_list_foreach (self->priv->wires_to_del, del_wire_in_pipeline,
      (gpointer) self);
  GST_INFO ("remove machines");
  g_list_foreach (self->priv->machines_to_del, del_machine_in_pipeline,
      (gpointer) self);

#ifdef STOP_PLAYBACK_FOR_UPDATES
  if (cont) {
    bt_song_play (self->priv->song);
  }
#endif

  // free the lists  
  g_list_free (self->priv->machines_to_add);
  self->priv->machines_to_add = NULL;
  g_list_free (self->priv->wires_to_add);
  self->priv->wires_to_add = NULL;
  g_list_free (self->priv->machines_to_del);
  self->priv->machines_to_del = NULL;
  g_list_free (self->priv->wires_to_del);
  self->priv->wires_to_del = NULL;

  GST_INFO ("update connection states");
  g_hash_table_foreach (self->priv->connection_state, update_connection_states,
      (gpointer) self);
  GST_INFO ("pipeline updated ----------------------------------------");
  gst_event_replace (&self->priv->play_seek_event, NULL);
  gst_event_replace (&self->priv->play_newsegment_event, NULL);
}

/*
 * bt_setup_update_pipeline:
 *
 * Rebuilds the whole pipeline, after changing the setup (adding/removing and
 * linking/unlinking machines).
 */
static gboolean
bt_setup_update_pipeline (const BtSetup * const self)
{
  gboolean res = FALSE;
  BtMachine *master;

  // get master
  if ((master = bt_setup_get_machine_by_type (self, BT_TYPE_SINK_MACHINE))) {
    GList *not_visited_machines, *not_visited_wires, *node;
    BtWire *wire;
    BtMachine *machine;

    // make a copy of lists and check what is connected from master and
    // remove all visited items
    not_visited_machines = g_list_copy (self->priv->machines);
    not_visited_wires = g_list_copy (self->priv->wires);
    GST_INFO ("checking connections for %d machines and %d wires",
        g_list_length (not_visited_machines),
        g_list_length (not_visited_wires));
    // ... and start checking connections (recursively)
    res =
        check_connected (self, master, &not_visited_machines,
        &not_visited_wires, 0);

    // remove all items that we have not visited and set them to disconnected
    GST_INFO ("remove %d unconnected wires", g_list_length (not_visited_wires));
    for (node = not_visited_wires; node; node = g_list_next (node)) {
      wire = BT_WIRE (node->data);
      set_disconnecting (self, GST_BIN (wire));
    }
    g_list_free (not_visited_wires);
    GST_INFO ("remove %d unconnected machines",
        g_list_length (not_visited_machines));
    for (node = not_visited_machines; node; node = g_list_next (node)) {
      machine = BT_MACHINE (node->data);
      set_disconnecting (self, GST_BIN (machine));
    }
    g_list_free (not_visited_machines);

    update_pipeline (self);

    g_object_unref (master);
  }
  GST_INFO ("result of graph update = %d", res);
  return (res);
}

//-- public methods

/**
 * bt_setup_add_machine:
 * @self: the setup to add the machine to
 * @machine: the new machine instance
 *
 * Let the setup know that the supplied machine is now part of the song.
 *
 * Returns: return true, if the machine can be added. Returns false if the
 * machine is currently added to the setup.
 */
gboolean
bt_setup_add_machine (const BtSetup * const self,
    const BtMachine * const machine)
{
  gboolean ret = FALSE;

  g_return_val_if_fail (BT_IS_SETUP (self), FALSE);
  g_return_val_if_fail (BT_IS_MACHINE (machine), FALSE);

  GST_DEBUG_OBJECT (machine, "adding machine: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  // TODO(ensonic): check unique id? we could also have a hashtable to speed up
  // the lookups
  if (!g_list_find (self->priv->machines, machine)) {
    ret = TRUE;
    self->priv->machines =
        g_list_prepend (self->priv->machines, (gpointer) machine);
    gst_object_ref_sink ((gpointer) machine);
    set_disconnected (self, GST_BIN (machine));

    g_signal_emit ((gpointer) self, signals[MACHINE_ADDED_EVENT], 0, machine);
    GST_DEBUG_OBJECT (machine, "added machine: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
  } else {
    GST_WARNING_OBJECT (machine, "trying to add machine %p again", machine);
  }
  return ret;
}

/**
 * bt_setup_add_wire:
 * @self: the setup to add the wire to
 * @wire: the new wire instance
 *
 * Let the setup know that the supplied wire is now part of the song.
 *
 * Returns: returns true, if the wire is added. Returns false, if the setup
 * contains a wire witch is link between the same src and dst machines (cycle
 * check).

 */
gboolean
bt_setup_add_wire (const BtSetup * const self, const BtWire * const wire)
{
  gboolean ret = FALSE;

  g_return_val_if_fail (BT_IS_SETUP (self), FALSE);
  g_return_val_if_fail (BT_IS_WIRE (wire), FALSE);

  GST_DEBUG_OBJECT (wire, "adding wire: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  // avoid adding same wire twice, we're checking for unique wires when creating
  // them already
  if (!g_list_find (self->priv->wires, wire)) {
    BtMachine *src, *dst;

    // add to main list
    self->priv->wires = g_list_prepend (self->priv->wires, (gpointer) wire);
    gst_object_ref_sink ((gpointer) wire);

    // also add to convenience lists per machine
    g_object_get ((gpointer) wire, "src", &src, "dst", &dst, NULL);
    src->src_wires = g_list_prepend (src->src_wires, (gpointer) wire);
    dst->dst_wires = g_list_prepend (dst->dst_wires, (gpointer) wire);
    set_disconnected (self, GST_BIN (wire));
    bt_setup_update_pipeline (self);

    g_signal_emit ((gpointer) self, signals[WIRE_ADDED_EVENT], 0, wire);
    GST_DEBUG_OBJECT (wire, "added wire: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (wire));
    GST_DEBUG_OBJECT (src, "wire.src: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (src));
    GST_DEBUG_OBJECT (dst, "wire.dst: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (dst));

    g_object_unref (src);
    g_object_unref (dst);
    ret = TRUE;
  } else {
    GST_WARNING_OBJECT (wire, "trying to add wire %p again", wire);
  }
  return ret;
}

/**
 * bt_setup_remove_machine:
 * @self: the setup to remove the machine from
 * @machine: the machine instance to remove
 *
 * Let the setup know that the supplied machine is removed from the song.
 */
void
bt_setup_remove_machine (const BtSetup * const self,
    const BtMachine * const machine)
{
  GList *node;
  g_return_if_fail (BT_IS_SETUP (self));
  g_return_if_fail (BT_IS_MACHINE (machine));

  GST_DEBUG_OBJECT (machine, "removing machine: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  if ((node = g_list_find (self->priv->machines, machine))) {
    GST_DEBUG_OBJECT (machine, "emit remove signal: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    g_signal_emit ((gpointer) self, signals[MACHINE_REMOVED_EVENT], 0, machine);

    self->priv->machines = g_list_delete_link (self->priv->machines, node);
    g_hash_table_remove (self->priv->connection_state, (gpointer) machine);
    g_hash_table_remove (self->priv->graph_depth, (gpointer) machine);

    GST_DEBUG_OBJECT (machine, "releasing machine: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    // this triggers finalize if we don't have a ref
    gst_element_set_state (GST_ELEMENT (machine), GST_STATE_NULL);
    if (((GstObject *) machine)->parent) {
      gst_bin_remove (self->priv->bin, GST_ELEMENT (machine));
      GST_DEBUG_OBJECT (machine, "unparented machine: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (machine));
    }
    gst_object_unref (GST_OBJECT (machine));
  } else {
    GST_WARNING_OBJECT (machine, "machine is not in setup");
  }
}

/**
 * bt_setup_remove_wire:
 * @self: the setup to remove the wire from
 * @wire: the wire instance to remove
 *
 * Let the setup know that the supplied wire is removed from the song.
 */
void
bt_setup_remove_wire (const BtSetup * const self, const BtWire * const wire)
{
  GList *node;
  g_return_if_fail (BT_IS_SETUP (self));
  g_return_if_fail (BT_IS_WIRE (wire));

  GST_DEBUG_OBJECT (wire, "removing wire: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  if ((node = g_list_find (self->priv->wires, wire))) {
    BtMachine *src, *dst;

    // also remove from the convenience lists
    g_object_get ((gpointer) wire, "src", &src, "dst", &dst, NULL);
    src->src_wires = g_list_remove (src->src_wires, wire);
    dst->dst_wires = g_list_remove (dst->dst_wires, wire);
    GST_DEBUG_OBJECT (src, "wire.src: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (src));
    GST_DEBUG_OBJECT (dst, "wire.dst: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (dst));
    g_object_unref (src);
    g_object_unref (dst);

    set_disconnecting (self, GST_BIN (wire));
    bt_setup_update_pipeline (self);

    self->priv->wires = g_list_delete_link (self->priv->wires, node);

    GST_DEBUG_OBJECT (wire, "emit remove signal: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (wire));
    g_signal_emit ((gpointer) self, signals[WIRE_REMOVED_EVENT], 0, wire);

    g_hash_table_remove (self->priv->connection_state, (gpointer) wire);
    g_hash_table_remove (self->priv->graph_depth, (gpointer) wire);

    GST_DEBUG_OBJECT (wire, "releasing wire: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (wire));
    gst_element_set_state (GST_ELEMENT (wire), GST_STATE_NULL);
    // this triggers finalize if we don't have a ref
    if (((GstObject *) wire)->parent) {
      gst_bin_remove (self->priv->bin, GST_ELEMENT (wire));
      GST_DEBUG_OBJECT (wire, "unparented wire: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (wire));
    }
    gst_object_unref (GST_OBJECT (wire));
  } else {
    GST_WARNING_OBJECT (wire, "wire is not in setup");
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
BtMachine *
bt_setup_get_machine_by_id (const BtSetup * const self, const gchar * const id)
{
  gboolean found = FALSE;
  gchar *const machine_id;
  const GList *node;

  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_STRING (id), NULL);

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
     // TODO(ensonic): method puts key into a gvalue and gets the param-spec by name, then calls generalized search
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

  for (node = self->priv->machines; node; node = g_list_next (node)) {
    BtMachine *const machine = BT_MACHINE (node->data);
    g_object_get (machine, "id", &machine_id, NULL);
    if (!strcmp (machine_id, id))
      found = TRUE;
    g_free (machine_id);
    if (found) {
      GST_DEBUG_OBJECT (machine, "getting machine (%s): %"
          G_OBJECT_REF_COUNT_FMT, id, G_OBJECT_LOG_REF_COUNT (machine));
      return g_object_ref (machine);
    }
  }
  GST_DEBUG ("no machine found for id \"%s\"", id);
  return (NULL);
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
BtMachine *
bt_setup_get_machine_by_type (const BtSetup * const self, const GType type)
{
  const GList *node;

  g_return_val_if_fail (BT_IS_SETUP (self), NULL);

  for (node = self->priv->machines; node; node = g_list_next (node)) {
    BtMachine *const machine = BT_MACHINE (node->data);
    if (G_OBJECT_TYPE (machine) == type) {
      GST_DEBUG_OBJECT (machine, "getting machine: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (machine));
      return g_object_ref (machine);
    }
  }
  GST_DEBUG ("no machine found for this type");
  return (NULL);
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
GList *
bt_setup_get_machines_by_type (const BtSetup * const self, const GType type)
{
  GList *machines = NULL;
  const GList *node;

  g_return_val_if_fail (BT_IS_SETUP (self), NULL);

  for (node = self->priv->machines; node; node = g_list_next (node)) {
    BtMachine *const machine = BT_MACHINE (node->data);
    if (G_OBJECT_TYPE (machine) == type) {
      GST_DEBUG_OBJECT (machine, "getting machine: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (machine));
      machines = g_list_prepend (machines, g_object_ref (machine));
    }
  }
  return (machines);
}

// TODO(ensonic): remove this, use machine->dst_wires list instead
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
BtWire *
bt_setup_get_wire_by_src_machine (const BtSetup * const self,
    const BtMachine * const src)
{
  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_MACHINE (src), NULL);
  return (bt_setup_get_wire_by_machine_type (self, src, "src"));
}

// TODO(ensonic): remove this, use machine->src_wires list instead
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
BtWire *
bt_setup_get_wire_by_dst_machine (const BtSetup * const self,
    const BtMachine * const dst)
{
  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_MACHINE (dst), NULL);
  return (bt_setup_get_wire_by_machine_type (self, dst, "dst"));
}

// TODO(ensonic): remove this, use bt_machine_get_wire_by_dst_machine() instead
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
BtWire *
bt_setup_get_wire_by_machines (const BtSetup * const self,
    const BtMachine * const src, const BtMachine * const dst)
{
  gboolean found = FALSE;
  BtMachine *const machine;
  const GList *node;

  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_MACHINE (src), NULL);
  g_return_val_if_fail (BT_IS_MACHINE (dst), NULL);

  // either src or dst has no wires
  if (!src->src_wires || !dst->dst_wires)
    return (NULL);

  // check if src links to dst
  // ideally we would search the shorter of the lists
  // FIXME(ensonic): this also does not really need the setup object ...
  // -> change to
  // bt_machine_get_wire_by_{src,dst}(machine,peer_machine);
  for (node = src->src_wires; node; node = g_list_next (node)) {
    BtWire *const wire = BT_WIRE (node->data);
    g_object_get (wire, "dst", &machine, NULL);
    if (machine == dst)
      found = TRUE;
    g_object_unref (machine);
    if (found) {
      GST_DEBUG_OBJECT (wire, "getting wire: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (wire));
      return g_object_ref (wire);
    }
  }
#if 0
  for (node = dst->dst_wires; node; node = g_list_next (node)) {
    BtWire *const wire = BT_WIRE (node->data);
    g_object_get (wire, "src", &machine, NULL);
    if (machine == src)
      found = TRUE;
    g_object_unref (machine);
    if (found)
      return g_object_ref (wire);
  }
  for (node = self->priv->wires; node; node = g_list_next (node)) {
    BtWire *const wire = BT_WIRE (node->data);
    g_object_get (wire, "src", &src_machine, "dst", &dst_machine, NULL);
    if ((src_machine == src) && (dst_machine == dst))
      found = TRUE;
    g_object_unref (src_machine);
    g_object_unref (dst_machine);
    if (found)
      return g_object_ref (wire);
  }
#endif
  GST_DEBUG ("no wire found for machines %p:%s %p:%s", src,
      GST_OBJECT_NAME (src), dst, GST_OBJECT_NAME (dst));
  return (NULL);
}

// TODO(ensonic): remove this, use machine->dst_wires list instead
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
GList *
bt_setup_get_wires_by_src_machine (const BtSetup * const self,
    const BtMachine * const src)
{
  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_MACHINE (src), NULL);
  return (bt_setup_get_wires_by_machine_type (self, src, "src"));
}

// TODO(ensonic): remove this, use machine->src_wires list instead
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
GList *
bt_setup_get_wires_by_dst_machine (const BtSetup * const self,
    const BtMachine * const dst)
{
  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_MACHINE (dst), NULL);
  return (bt_setup_get_wires_by_machine_type (self, dst, "dst"));
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
gchar *
bt_setup_get_unique_machine_id (const BtSetup * const self,
    const gchar * const base_name)
{
  BtMachine *machine;
  guint8 i = 0;

  g_return_val_if_fail (BT_IS_SETUP (self), NULL);
  g_return_val_if_fail (BT_IS_STRING (base_name), NULL);

  if (!(machine = bt_setup_get_machine_by_id (self, base_name))) {
    return (g_strdup (base_name));
  } else {
    g_object_unref (machine);
    machine = NULL;
  }

  gchar *const id = g_strdup_printf ("%s 00", base_name);
  gchar *const ptr = &id[strlen (base_name) + 1];
  do {
    (void) g_sprintf (ptr, "%02u", i++);
    g_object_try_unref (machine);
  } while ((machine = bt_setup_get_machine_by_id (self, id)) && (i < 100));
  g_object_try_unref (machine);
  return (id);
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
void
bt_setup_remember_missing_machine (const BtSetup * const self,
    const gchar * const str)
{
  gboolean skip = FALSE;

  GST_INFO ("missing machine %s", str);
  if (self->priv->missing_machines) {
    GList *node;
    for (node = self->priv->missing_machines; node; node = g_list_next (node)) {
      if (!strcmp (node->data, str)) {
        skip = TRUE;
        break;
      }
    }
  }
  if (!skip) {
    self->priv->missing_machines =
        g_list_prepend (self->priv->missing_machines, (gpointer) str);
  } else {
    g_free ((gchar *) str);
  }
}

//-- io interface

static xmlNodePtr
bt_setup_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node)
{
  BtSetup *const self = BT_SETUP (persistence);
  xmlNodePtr node = NULL;
  xmlNodePtr child_node;

  GST_DEBUG ("PERSISTENCE::setup");

  if ((node = xmlNewChild (parent_node, NULL, XML_CHAR_PTR ("setup"), NULL))) {
    if ((child_node =
            xmlNewChild (node, NULL, XML_CHAR_PTR ("properties"), NULL))) {
      if (!bt_persistence_save_hashtable (self->priv->properties, child_node))
        goto Error;
    } else
      goto Error;
    if ((child_node =
            xmlNewChild (node, NULL, XML_CHAR_PTR ("machines"), NULL))) {
      bt_persistence_save_list (self->priv->machines, child_node);
    } else
      goto Error;
    if ((child_node = xmlNewChild (node, NULL, XML_CHAR_PTR ("wires"), NULL))) {
      bt_persistence_save_list (self->priv->wires, child_node);
    } else
      goto Error;
  }
Error:
  return (node);
}

static BtPersistence *
bt_setup_persistence_load (const GType type,
    const BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  BtSetup *const self = BT_SETUP (persistence);
  xmlNodePtr child_node;
  gboolean failed_parts = FALSE;

  GST_DEBUG ("PERSISTENCE::setup");
  g_assert (node);

  for (node = node->children; node; node = node->next) {
    if (!xmlNodeIsText (node)) {
      if (!strncmp ((gchar *) node->name, "machines\0", 9)) {
        GType type = 0;

        //bt_song_io_native_load_setup_machines(self,song,node->children);
        for (child_node = node->children; child_node;
            child_node = child_node->next) {
          if (!xmlNodeIsText (child_node)) {
            /* TODO(ensonic): it would be smarter to have nodes name like "sink-machine"
             * and do type=g_type_from_name((gchar *)child_node->name);
             */
            if (!strncmp ((gchar *) child_node->name, "machine\0", 8)) {
              xmlChar *const type_str =
                  xmlGetProp (child_node, XML_CHAR_PTR ("type"));
              if (!strncmp ((gchar *) type_str, "processor\0", 10)) {
                type = BT_TYPE_PROCESSOR_MACHINE;
              } else if (!strncmp ((gchar *) type_str, "sink\0", 5)) {
                type = BT_TYPE_SINK_MACHINE;
              } else if (!strncmp ((gchar *) type_str, "source\0", 7)) {
                type = BT_TYPE_SOURCE_MACHINE;
              } else {
                GST_WARNING ("machine node has no type");
              }
              if (type) {
                GError *err = NULL;
                BtMachine *machine =
                    BT_MACHINE (bt_persistence_load (type, NULL, child_node,
                        &err, "song", self->priv->song, NULL));
                if (err != NULL) {
                  // collect failed machines
                  gchar *const plugin_name;

                  g_object_get (machine, "plugin-name", &plugin_name, NULL);
                  // takes ownership of the name
                  bt_setup_remember_missing_machine (self, plugin_name);
                  failed_parts = TRUE;

                  GST_WARNING ("Can't create machine: %s", err->message);
                  g_error_free (err);
                  gst_object_unref (machine);
                }
              }
              xmlFree (type_str);
            }
          }
        }
      } else if (!strncmp ((gchar *) node->name, "wires\0", 6)) {
        for (child_node = node->children; child_node;
            child_node = child_node->next) {
          if (!xmlNodeIsText (child_node)) {
            GError *err = NULL;
            // TODO(ensonic): rework construction
            BtWire *const wire =
                BT_WIRE (bt_persistence_load (BT_TYPE_WIRE, NULL, child_node,
                    &err, "song", self->priv->song, NULL));
            if (err != NULL) {
              failed_parts = TRUE;

              GST_WARNING ("Can't create wire: %s", err->message);
              g_error_free (err);
              gst_object_unref (wire);
            }
          }
        }
      } else if (!strncmp ((gchar *) node->name, "properties\0", 11)) {
        bt_persistence_load_hashtable (self->priv->properties, node);
      }
    }
  }
  if (failed_parts) {
    bt_song_write_to_lowlevel_dot_file (self->priv->song);
  }
  return (BT_PERSISTENCE (persistence));
}

static void
bt_setup_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_setup_persistence_load;
  iface->save = bt_setup_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void
bt_setup_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtSetup *const self = BT_SETUP (object);
  return_if_disposed ();
  switch (property_id) {
    case SETUP_PROPERTIES:
      g_value_set_pointer (value, self->priv->properties);
      break;
    case SETUP_SONG:
      g_value_set_object (value, self->priv->song);
      break;
    case SETUP_MACHINES:
      /* TODO(ensonic): this is not good, lists returned by bt_setup_get_xxx_by_yyy
       * returns a new list where one has to unref the elements
       */
      g_value_set_pointer (value, g_list_copy (self->priv->machines));
      break;
    case SETUP_WIRES:
      /* TODO(ensonic): this is not good, lists returned by bt_setup_get_xxx_by_yyy
       * returns a new list where one has to unref the elements
       */
      g_value_set_pointer (value, g_list_copy (self->priv->wires));
      break;
    case SETUP_MISSING_MACHINES:
      g_value_set_pointer (value, self->priv->missing_machines);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_setup_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtSetup *const self = BT_SETUP (object);
  return_if_disposed ();
  switch (property_id) {
    case SETUP_SONG:
      self->priv->song = BT_SONG (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song);
      g_object_get ((gpointer) (self->priv->song), "bin", &self->priv->bin,
          NULL);
      GST_INFO ("set the song for setup: %p and get the bin: %"
          G_OBJECT_REF_COUNT_FMT,
          self->priv->song, G_OBJECT_LOG_REF_COUNT (self->priv->bin));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_setup_dispose (GObject * const object)
{
  BtSetup *const self = BT_SETUP (object);
  GList *node;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  // unref list of wires
  if (self->priv->wires) {
    for (node = self->priv->wires; node; node = g_list_next (node)) {
      if (node->data) {
        GObject *obj = node->data;

        GST_DEBUG_OBJECT (obj, "  free wire: %" G_OBJECT_REF_COUNT_FMT,
            G_OBJECT_LOG_REF_COUNT (obj));

        unlink_wire (self, GST_ELEMENT (obj));

        gst_element_set_state (GST_ELEMENT (obj), GST_STATE_NULL);
        if (self->priv->bin && ((GstObject *) obj)->parent) {
          gst_bin_remove (self->priv->bin, GST_ELEMENT (obj));
        }
        gst_object_unref (obj);
        node->data = NULL;
      }
    }
  }
  // unref list of machines
  if (self->priv->machines) {
    for (node = self->priv->machines; node; node = g_list_next (node)) {
      if (node->data) {
        GObject *obj = node->data;

        GST_DEBUG_OBJECT (obj, "  free machine: %" G_OBJECT_REF_COUNT_FMT,
            G_OBJECT_LOG_REF_COUNT (obj));

        gst_element_set_state (GST_ELEMENT (obj), GST_STATE_NULL);
        if (self->priv->bin && ((GstObject *) obj)->parent) {
          gst_bin_remove (self->priv->bin, GST_ELEMENT (obj));
        }
        gst_object_unref (obj);
        node->data = NULL;
      }
    }
  }

  if (self->priv->bin) {
    GST_DEBUG_OBJECT (self->priv->bin,
        "release bin: %" G_OBJECT_REF_COUNT_FMT ", num_children=%d",
        G_OBJECT_LOG_REF_COUNT (self->priv->bin),
        GST_BIN_NUMCHILDREN (self->priv->bin));
    gst_object_unref (self->priv->bin);
    self->priv->bin = NULL;
  }
  g_object_try_weak_unref (self->priv->song);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_setup_parent_class)->dispose (object);
}

static void
bt_setup_finalize (GObject * const object)
{
  BtSetup *const self = BT_SETUP (object);

  GST_DEBUG ("!!!! self=%p", self);

  // free list of wires
  if (self->priv->wires) {
    g_list_free (self->priv->wires);
    self->priv->wires = NULL;
  }
  // free list of machines
  if (self->priv->machines) {
    g_list_free (self->priv->machines);
    self->priv->machines = NULL;
  }
  // free list of missing_machines
  if (self->priv->missing_machines) {
    const GList *node;
    for (node = self->priv->missing_machines; node; node = g_list_next (node)) {
      g_free (node->data);
    }
    g_list_free (self->priv->missing_machines);
    self->priv->missing_machines = NULL;
  }

  g_hash_table_destroy (self->priv->properties);
  g_hash_table_destroy (self->priv->connection_state);
  g_hash_table_destroy (self->priv->graph_depth);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_setup_parent_class)->finalize (object);
}

//-- class internals

static void
bt_setup_init (BtSetup * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SETUP, BtSetupPrivate);

  self->priv->properties =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  self->priv->connection_state = g_hash_table_new (NULL, NULL);
  self->priv->graph_depth = g_hash_table_new (NULL, NULL);
}

static void
bt_setup_class_init (BtSetupClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSetupPrivate));

  gobject_class->set_property = bt_setup_set_property;
  gobject_class->get_property = bt_setup_get_property;
  gobject_class->dispose = bt_setup_dispose;
  gobject_class->finalize = bt_setup_finalize;

  /**
   * BtSetup::machine-added:
   * @self: the setup object that emitted the signal
   * @machine: the new machine
   *
   * A new machine item has been added to the setup.
   */
  signals[MACHINE_ADDED_EVENT] = g_signal_new ("machine-added", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,        // accumulator
      NULL,                     // acc data
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE,     // return type
      1,                        // n_params
      BT_TYPE_MACHINE           // param data
      );

  /**
   * BtSetup::wire-added:
   * @self: the setup object that emitted the signal
   * @wire: the new wire
   *
   * A new wire item has been added to the setup.
   */
  signals[WIRE_ADDED_EVENT] = g_signal_new ("wire-added", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,      // accumulator
      NULL,                     // acc data
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE,     // return type
      1,                        // n_params
      BT_TYPE_WIRE              // param data
      );

  /**
   * BtSetup::machine-removed:
   * @self: the setup object that emitted the signal
   * @machine: the old machine
   *
   * A machine item has been removed from the setup.
   */
  signals[MACHINE_REMOVED_EVENT] = g_signal_new ("machine-removed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,    // accumulator
      NULL,                     // acc data
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE,     // return type
      1,                        // n_params
      BT_TYPE_MACHINE           // param data
      );

  /**
   * BtSetup::wire-removed:
   * @self: the setup object that emitted the signal
   * @wire: the old wire
   *
   * A wire item has been removed from the setup.
   */
  signals[WIRE_REMOVED_EVENT] = g_signal_new ("wire-removed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,  // accumulator
      NULL,                     // acc data
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE,     // return type
      1,                        // n_params
      BT_TYPE_WIRE              // param data
      );

  g_object_class_install_property (gobject_class, SETUP_PROPERTIES,
      g_param_spec_pointer ("properties",
          "properties prop",
          "hashtable of setup properties",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SETUP_SONG,
      g_param_spec_object ("song", "song construct prop",
          "Set song object, the setup belongs to", BT_TYPE_SONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SETUP_MACHINES,
      g_param_spec_pointer ("machines",
          "machine list prop",
          "A copy of the list of machines",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SETUP_WIRES,
      g_param_spec_pointer ("wires",
          "wire list prop",
          "A copy of the list of wires",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SETUP_MISSING_MACHINES,
      g_param_spec_pointer ("missing-machines",
          "missing-machines list prop",
          "The list of missing machines, don't change",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
