/** $Id: net.h,v 1.2 2004-04-08 15:29:32 ensonic Exp $
 * network 
 *   we need a helper to link two elements
 *   elements are identified by their ID, connections by src.ID,dst.ID
 *   we need to store for each element, if it already has an adder, or a oneton element
 *   we need to store for each connection, which adaptor elements we have inserted
 * patterns/sequence
 *
 */

//#define GST_DEBUG_ENABLED 1
#undef GST_DISABLE_GST_DEBUG

#include <gst/gst.h>
#include <gst/control/control.h>

GST_DEBUG_CATEGORY_STATIC(bt_core_debug);
#define GST_CAT_DEFAULT bt_core_debug


/* types */
typedef struct BtMachine BtMachine;
typedef BtMachine *BtMachinePtr;
struct BtMachine {
	GstElement *machine,*adder,*spreader;
	GstDParamManager *dparam_manager;
	// convinience pointer
	GstElement *dst_elem,*src_elem;
};

typedef struct BtConnection BtConnection;
typedef BtConnection *BtConnectionPtr;
struct BtConnection {
	BtMachinePtr src, dst;
	GstElement *convert,*scale;
	// convinience pointer
	GstElement *dst_elem,*src_elem;
};

typedef struct BTPattern BTPattern;
typedef BTPattern *BTPatternPtr;
struct BTPattern {
	BtMachinePtr machine;
	// parameters[tick-len] // with NULL end-marker
	// *parameter
};

/* global vars */

// all used machines
GList *machines=NULL;
// add used connections
GList *connections=NULL;

// array, where each slot point to one timeline array, where each slot points to NULL or a BTPattern
gpointer *timeline=NULL;
BTPatternPtr *pat=NULL;

/* methods */

//-- Machines ------------------------------------------------------------------

/** @brief creates a machines wrapped by an adder and a spreader */
BtMachinePtr bt_machine_new(GstElement *bin, gchar *factory, gchar *id) {
	BtMachinePtr machine=g_new0(BtMachine,1);

  machine->machine=gst_element_factory_make(factory,id);
  g_assert(machine->machine != NULL);
	// there is no adder or spreader in use by default
	machine->dst_elem=machine->src_elem=machine->machine;
	
	if((machine->dparam_manager=gst_dpman_get_manager(machine->machine))) {
		// setting param mode. Only synchronized is currently supported
		gst_dpman_set_mode(machine->dparam_manager, "synchronous");
		GST_INFO("  machine supports dparams\n");
	}

	gst_bin_add(GST_BIN(bin), machine->machine);
	machines=g_list_append(machines,machine);
	return(machine);
}


//-- Connections ---------------------------------------------------------------

BtConnectionPtr bt_connection_find_by_src(BtMachinePtr src) {
	GList *node=g_list_first(connections);
	BtConnectionPtr connection;
	
	while(node) {
		connection=(BtConnectionPtr)node->data;
		if(connection->src==src) return(connection);
		node=g_list_next(node);
	}
	return(NULL);
}


BtConnectionPtr bt_connection_find_by_dst(BtMachinePtr dst) {
	GList *node=g_list_first(connections);
	BtConnectionPtr connection;
	
	while(node) {
		connection=(BtConnectionPtr)node->data;
		if(connection->dst==dst) return(connection);
		node=g_list_next(node);
	}
	return(NULL);
}

/** @brief creates a connection between two machines and cares about adder/spreader as well as connection-converters */
BtConnectionPtr bt_connection_new(GstElement *bin, BtMachinePtr src, BtMachinePtr dst) {
	BtConnectionPtr connection=g_new0(BtConnection,1),other_connection;
	
	GST_INFO("  trying to connect %p -> %p\n",src,dst);
	// if there is already a connection from src && src has not yet an spreader (in use)
	if((other_connection=bt_connection_find_by_src(src)) && (src->src_elem!=src->spreader)) {
		GST_INFO("  other connection from src found\n");
		// unlink the elements from the other connection
		gst_element_unlink(other_connection->src->src_elem, other_connection->dst_elem);
		// create spreader (if needed)
		if(!src->spreader) {
			src->spreader=gst_element_factory_make("oneton",g_strdup_printf("oneton_%p",src));
			gst_bin_add(GST_BIN(bin), src->spreader);
			if(!gst_element_link(src->machine, src->spreader)) {
				GST_ERROR("failed to link the machines internal spreader\n");return(NULL);
			}
		}
		if(other_connection->src_elem==src->src_elem) {
			// we need to fix the src_elem of the other connect, as we have inserted the spreader
			other_connection->src_elem=src->spreader;
		}
		// activate spreader
		src->src_elem=src->spreader;
		GST_INFO("  spreader activated for \"%s\"\n",gst_element_get_name(src->machine));
		// correct the link for the other connection
		if(!gst_element_link(other_connection->src->src_elem, other_connection->dst_elem)) {
			GST_ERROR("failed to re-link the machines after inserting internal spreaker\n");return(NULL);
		}
	}
	
	// if there is already a connection to dst and dst has not yet an adder (in use)
	if((other_connection=bt_connection_find_by_dst(dst)) && (dst->dst_elem!=dst->adder)) {
		GST_INFO("  other connection to dst found\n");
		// unlink the elements from the other connection
		gst_element_unlink(other_connection->src_elem, other_connection->dst->dst_elem);
		// create adder (if needed)
		if(!dst->adder) {
			dst->adder=gst_element_factory_make("adder",g_strdup_printf("adder_%p",dst));
			gst_bin_add(GST_BIN(bin), dst->adder);
			if(!gst_element_link(dst->adder, dst->machine)) {
				GST_ERROR("failed to link the machines internal adder\n");return(NULL);
			}
		}
		if(other_connection->dst_elem==dst->dst_elem) {
			// we need to fix the dst_elem of the other connect, as we have inserted the adder
			other_connection->dst_elem=dst->adder;
		}
		// activate adder
		dst->dst_elem=dst->adder;
		GST_INFO("  spreader adder for \"%s\"\n",gst_element_get_name(dst->machine));
		// correct the link for the other connection
		if(!gst_element_link(other_connection->src_elem, other_connection->dst->dst_elem)) {
			GST_ERROR("failed to re-link the machines after inserting internal adder\n");return(NULL);
		}
	}
	
	connection->src=src;
	connection->dst=dst;

	// try link src to dst {directly, with convert, with scale, with ...}
	if(!gst_element_link(src->src_elem, dst->dst_elem)) {
		connection->convert=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",connection));
		gst_bin_add(GST_BIN(bin), connection->convert);
		if(!gst_element_link_many(src->src_elem, connection->convert, dst->dst_elem, NULL)) {
			connection->scale=gst_element_factory_make("audioscale",g_strdup_printf("audioscale_%p",connection));
			gst_bin_add(GST_BIN(bin), connection->convert);
			if(!gst_element_link_many(src->src_elem, connection->scale, dst->dst_elem, NULL)) {
				if(!gst_element_link_many(src->src_elem, connection->convert, connection->scale, dst->dst_elem, NULL)) {
					// try harder (scale, convert)
					GST_DEBUG("failed to link the machines\n");return(NULL);
				}
				else {
					connection->src_elem=connection->scale;
					connection->dst_elem=connection->convert;
				}
			}
			else {
				connection->src_elem=connection->scale;
				connection->dst_elem=connection->scale;
			}
		}
		else {
			connection->src_elem=connection->convert;
			connection->dst_elem=connection->convert;
		}
	}
	else {
		connection->src_elem=src->src_elem;
		connection->dst_elem=dst->dst_elem;
	}
	GST_INFO("  connection okay\n");

	connections=g_list_append(connections,connection);
	return(connection);
}

