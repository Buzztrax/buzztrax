/** $Id: net1.c,v 1.1 2004-04-01 15:29:49 ensonic Exp $
 * we need a helper to link two elements
 * elements are identified by their ID, connections by src.ID,dst.ID
 * we need to store for each element, if it already has an adder, or a oneton element
 * we need to store for each connection, which adaptor elements we have inserted
 */
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

/* types */
typedef struct BtMachine BtMachine;
typedef BtMachine *BtMachinePtr;
struct BtMachine {
	GstElement *machine,*adder,*spreader;
	// convinience pointer
	GstElement *sink,*src;
};

typedef struct BtConnection BtConnection;
typedef BtConnection *BtConnectionPtr;
struct BtConnection {
	BtMachinePtr src, dst;
	GstElement *convert,*scale;
};

/* global vars */
GList *machines=NULL;
GList *connections=NULL;

/* methods */

//-- Machines ------------------------------------------------------------------

BtMachinePtr bt_machine_new(GstElement *bin, gchar *factory, gchar *id) {
	BtMachinePtr machine=g_new0(BtMachine,1);

  machine->machine=gst_element_factory_make(factory,id);
  g_assert(machine->machine != NULL);
	// there is no adder or spreader in use by default
	machine->sink=machine->src=machine->machine;

	gst_bin_add(GST_BIN(bin), machine->machine);
	machines=g_list_append(machines,machine);
	return(machine);
}


//-- Connections ---------------------------------------------------------------

BtConnectionPtr bt_connection_find_by_src(BtMachinePtr src) {
	GList *node=g_list_first(connections);
	BtConnectionPtr connection=NULL;
	
	while(node) {
		connection=(BtConnectionPtr)node->data;
		if(connection->src==src) break;
		node=g_list_next(node);
	}
	return(connection);
}


BtConnectionPtr bt_connection_find_by_dst(BtMachinePtr dst) {
	GList *node=g_list_first(connections);
	BtConnectionPtr connection=NULL;
	
	while(node) {
		connection=(BtConnectionPtr)node->data;
		if(connection->dst==dst) break;
		node=g_list_next(node);
	}
	return(connection);
}

BtConnectionPtr bt_connection_new(GstElement *bin, BtMachinePtr src, BtMachinePtr dst) {
	BtConnectionPtr connection=g_new0(BtConnection,1),other_connection;
	
	// if there is already a connection from src && src has not yet an spreader (in use)
	if((other_connection=bt_connection_find_by_src(src)) && (src->src!=src->spreader)) {
		// unlink the elements from the other connection
		gst_element_unlink(other_connection->src->src, other_connection->dst->sink);
		// create spreader (if needed)
		if(!src->spreader) {
			src->spreader=gst_element_factory_make("oneton",g_strdup_printf("oneton_%p",src));
			gst_bin_add(GST_BIN(bin), src->spreader);
			if(!gst_element_link(src->machine, src->spreader)) {
				g_print("failed to link the machines internal spreader\n");return(NULL);
			}
		}
		// activate spreader
		src->src=src->spreader;
		g_print("  spreader activated\n");
		// correct the link for the other connection
		gst_element_link(other_connection->src->src, other_connection->dst->sink);
	}
	
	// if there is already a connection to dst and dst has not yet an adder (in use)
	if((other_connection=bt_connection_find_by_dst(dst)) && (dst->sink!=dst->adder)) {
		// unlink the elements from the other connection
		gst_element_unlink(other_connection->src->src, other_connection->dst->sink);
		// create adder (if needed)
		if(!dst->adder) {
			dst->adder=gst_element_factory_make("adder",g_strdup_printf("adder_%p",dst));
			gst_bin_add(GST_BIN(bin), src->adder);
			if(!gst_element_link(dst->adder, dst->machine)) {
				g_print("failed to link the machines internal adder\n");return(NULL);
			}
		}
		// activate adder
		dst->sink=dst->adder;
		g_print("  spreader adder\n");
		// correct the link for the other connection
		gst_element_link(other_connection->src->src, other_connection->dst->sink);
	}
	
	// try link src to dst {directly, with convert, with scale, with ...}
	if(!gst_element_link(src->src, dst->sink)) {
		connection->convert=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",connection));
		gst_bin_add(GST_BIN(bin), connection->convert);
		if(!gst_element_link_many(src->src, connection->convert, dst->sink, NULL)) {
			connection->scale=gst_element_factory_make("audioscale",g_strdup_printf("audioscale_%p",connection));
			gst_bin_add(GST_BIN(bin), connection->convert);
			if(!gst_element_link_many(src->src, connection->scale, dst->sink, NULL)) {
				if(!gst_element_link_many(src->src, connection->convert, connection->scale, dst->sink, NULL)) {
					// try harder (scale, convert)
					g_print("failed to link the machines\n");return(NULL);
				}
			}
		}
	}
	g_print("  connection okay\n");
	
	connections=g_list_append(connections,connection);
	return(connection);
}


//-- main ----------------------------------------------------------------------

int main(int argc, char **argv) {
	// pipeline element
	GstElement *thread;
	// machines
	BtMachinePtr gen1,sink;
	// timing
	guint tick=0,max_ticks;
  
	// init gst
	gst_init(&argc, &argv);
  gst_control_init(&argc,&argv);
  
  if (argc < 3) {
    g_print("usage: %s <sink> <source> <seconds>\n",argv[0]);
		g_print("examples:\n");
		g_print("\t%s esdsink sinesrc 10\n",argv[0]);
		g_print("\t%s esdsink ladspa-organ 10\n",argv[0]);
    exit(-1);
  }
	max_ticks=atoi(argv[3]);

  /* create a new thread to hold the elements */
  thread = gst_thread_new("thread");
  g_assert(thread != NULL);

	/* create machines */
	sink=bt_machine_new(thread,argv[1],"master");
	gen1=bt_machine_new(thread,argv[2],"generator1");
	
	g_print("machines created\n");
	
	/* link machines */
	if(!(bt_connection_new(thread,gen1,sink))) {
		g_print("can't connect machines\n");
	}
	g_print("machines connected\n");
	
  /* start playing */
	g_print("start playing\n");
  if(gst_element_set_state(thread, GST_STATE_PLAYING)) {
		
		/* do whatever you want here, the thread will be playing */
		g_print("thread is playing\n");
	
		while(tick<max_ticks) {
			gst_element_wait(sink->machine, GST_SECOND*tick++);
		}
	
		/* stop the pipeline */
		gst_element_set_state(thread, GST_STATE_NULL);
	}
	else g_print("starting to play failed\n");

  /* we don't need a reference to these objects anymore */
  gst_object_unref(GST_OBJECT (thread));
	
  exit(0);
}
