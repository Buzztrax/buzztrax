/** $Id: net1.c,v 1.2 2004-04-02 18:00:31 ensonic Exp $ */
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

#include "net.h"

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
