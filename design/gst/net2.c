/** $Id: net2.c,v 1.2 2004-04-08 15:29:32 ensonic Exp $
 * this is beta code !
 *  - it does not free any memory ;-)
 *  - it contains hardcoded stuff
 */
 
#include <stdio.h>

#include "net.h"

#define TIMELINES 4
#define TRACKS 2
#define PATTERNS 2

// pipeline and timing element
GstElement *thread, *synch;

void setup_network(gchar *master) {
	// machines
	BtMachinePtr gen1,gen2,sink;

	/* create machines */
	sink=bt_machine_new(thread,master,"master");
	synch=sink->machine;
	gen1=bt_machine_new(thread,"sinesrc","generator1");
	gen2=bt_machine_new(thread,"sinesrc","generator2");
	
	GST_INFO("machines created\n");
	
	/* link machines */
	if(!(bt_connection_new(thread,gen1,sink))) { GST_DEBUG("can't connect machines: gen1,sink\n");exit(1);	}
	if(!(bt_connection_new(thread,gen2,sink))) { GST_DEBUG("can't connect machines: gen2,sink\n");exit(1);	}
	GST_INFO("machines connected\n");	
}

void setup_patterns(void) {
	pat=g_new0(BTPatternPtr,PATTERNS);	// 2 patterns
	pat[0]=g_new0(BTPattern,1);
	pat[1]=g_new0(BTPattern,1);
}

void setup_sequence(void) {
	guint i;
	BTPatternPtr *tracks;
	
	timeline=g_new0(gpointer,TIMELINES);	// 4 time-lines
	for(i=0;i<TIMELINES;i++) {
		timeline[i]=g_new0(BTPatternPtr,TRACKS);	// 2 tracks
	}
	// this is hardcoded for demo-purpose
	tracks=(BTPatternPtr *)timeline[0];tracks[0]=NULL  ;tracks[1]=NULL;
	tracks=(BTPatternPtr *)timeline[1];tracks[0]=pat[0];tracks[1]=NULL;
	tracks=(BTPatternPtr *)timeline[2];tracks[0]=NULL  ;tracks[1]=pat[1];
	tracks=(BTPatternPtr *)timeline[3];tracks[0]=pat[0];tracks[1]=pat[1];
}

void play_timeline(BTPatternPtr *tracks,guint position) {
	guint track,ticks;

	position<<=3;
	// run x ticks
	for(ticks=0;ticks<8;ticks++,position++) {
		// process commands from each track
		for(track=0;track<TRACKS;track++) {
			// track is not NULL
			if(*tracks) {
				// get the machine
				// if(*parameter) {
					// loop over params from machine and set them
				// }
				// parameter++;
			}
			tracks++;
		}
		// wait one tick (which is hardcoded to half a second now)
		gst_element_wait(synch, (GST_SECOND*position)>>1);
	}
}

void play_sequence(void) {
	guint position;

	// initiaslise all machines to be silent here

  if(gst_element_set_state(thread, GST_STATE_PLAYING)) {
		/* do whatever you want here, the thread will be playing */
		GST_INFO("thread is playing\n");

		// step through the timeline and 
		for(position=0;position<TIMELINES;position++) {
			play_timeline((BTPatternPtr *)timeline[position],position);
		}
	
		/* stop the pipeline */
		gst_element_set_state(thread, GST_STATE_NULL);
	}
	else GST_INFO("starting to play failed\n");
}

//-- main ----------------------------------------------------------------------

int main(int argc, char **argv) {
	// init gst
	gst_init(&argc, &argv);
  gst_control_init(&argc,&argv);
	GST_DEBUG_CATEGORY_INIT(bt_core_debug, "buzztard", 0, "music production environment");

  if (argc < 2) {
    g_print("usage: %s <sink>\n",argv[0]);
		g_print("examples:\n");
		g_print("\t%s esdsink\n",argv[0]);
    exit(-1);
  }

  /* create a new thread to hold the elements */
  thread = gst_thread_new("thread");
  g_assert(thread != NULL);
	
	/* create song date and machines network */
	setup_network(argv[1]);
	setup_patterns();
	setup_sequence();
	
  /* start playing */
	GST_INFO("start playing\n");
	play_sequence();

  /* we don't need a reference to these objects anymore */
  gst_object_unref(GST_OBJECT (thread));
	
  exit(0);
}
