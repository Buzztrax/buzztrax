/** $Id: gst3.c,v 1.4 2004-02-18 17:38:23 ensonic Exp $
 */
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

int main(int argc, char **argv) {
	// pipeline elemnts
	GstElement *audiosink, *audiosrc, *audioconvert, *thread;
	// timing
  GstClock *clock;
	guint tick;
	// params
  GstDParamManager *audiosrcParam;
	// volume
	gdouble vol_doubleval;
	GValue *vol_val;
	GstDParam *vol;
	// frequency
	gdouble frq_doubleval;
	GValue *frq_val;
	GstDParam *frq;
  
	// init gst
	gst_init(&argc, &argv);
  gst_control_init(&argc,&argv);
  
  if (argc < 3) {
    g_print("usage: %s <src> <sink> <value>\nvalue should be between 0 ... 100\n",argv[0]);
		g_print("example: %s sinesrc esdsink 100\n",argv[0]);
    exit(-1);
  }
  
  /* create a new thread to hold the elements */
  thread = gst_thread_new("thread");
  g_assert(thread != NULL);

  /* and an audio sink */
  audiosink = gst_element_factory_make(argv[2], "audio-sink");
  g_assert(audiosrc != NULL);
  
  /* add a sine source */
  audiosrc = gst_element_factory_make (argv[1], "audio-source");
  g_assert(audiosrc != NULL);

  //audioconvert = gst_element_factory_make("audioconvert", "convert");
	//g_assert(audioconvert != NULL);
  
  /* add objects to the thread */
  gst_bin_add_many(GST_BIN (thread), audiosrc, /*audioconvert,*/ audiosink, NULL);
	
  /* and link them together */
  //gst_element_link_many (audiosrc, audioconvert, audiosink, NULL);
	if(!(gst_element_link(audiosrc, audiosink))) {
		g_print("spider required\n");
		audioconvert = gst_element_factory_make("spider", "spider_1");
		g_assert(audioconvert != NULL);
		gst_bin_add(GST_BIN (thread),audioconvert);
		if(!(gst_element_link_many (audiosrc, audioconvert, audiosink, NULL))) {
			g_print("using spider failed\n");exit(1);
		}
	} else g_print("no spider required\n");


  /* getting the clock */
  //gst_bin_auto_clock(GST_BIN (thread));
  g_print("audiosink provides clock? %d\n",gst_element_provides_clock (audiosink));
  if (gst_element_provides_clock(audiosink)) {
    gst_bin_use_clock(GST_BIN (thread), gst_element_get_clock (audiosink));
    clock = gst_element_get_clock(audiosink);
    g_print("using %s clock which is instance_of(%s)\n",argv[2],GST_OBJECT_NAME(clock));
  } else {
    gst_bin_use_clock(GST_BIN (thread), gst_system_clock_obtain());
    clock = gst_bin_get_clock(GST_BIN (thread));
		g_print("using system clock which is instance_of(%s)\n",GST_OBJECT_NAME(clock));
  }
  
  g_print("clock::speed: %f\n",gst_clock_get_speed (clock));
  g_print("clock::resolution: %llu\n",gst_clock_get_resolution (clock));
  g_print("clock::is active: %d\n",gst_clock_is_active (clock));
  g_print("clock::flags: %d (single_sync=1,single_asych=2,peri_sync=4,peri_asych=8,resolution=16,speed=32)\n",GST_CLOCK_FLAGS(clock));


  /* getting param manager */
  audiosrcParam = gst_dpman_get_manager(audiosrc);
  /* setting param mode. Only synchronized is currently supported */
  gst_dpman_set_mode(audiosrcParam, "synchronous");
	/* preparing volume parameter */
  vol = gst_dparam_new(G_TYPE_DOUBLE);
  if (gst_dpman_attach_dparam(audiosrcParam, "volume", vol)){
		g_print("volume param attached\n");
    /* the dparam was successfully attached */
		vol_doubleval = atof(argv[3])/100.0;
    vol_val = g_new0(GValue,1);
    g_value_init(vol_val, G_TYPE_DOUBLE);
    g_value_set_double(vol_val, vol_doubleval);
		g_print("volume param initialized\n");
    g_object_set_property(G_OBJECT(vol), "value_double", vol_val);
		g_print("volume param set in plugin\n");
  }
	/* preparing frequency parameter */
  frq = gst_dparam_new(G_TYPE_DOUBLE);
  //if (gst_dpman_attach_dparam(audiosrcParam, "freq", frq)){
    // the dparam was successfully attached
		frq_doubleval = 50.0;
    frq_val = g_new0(GValue,1);
    //g_value_init(frq_val, G_TYPE_DOUBLE);
    //g_value_set_double(frq_val, frq_doubleval);
    //g_object_set_property(G_OBJECT(frq), "value_double", frq_val);
  //}
	//else g_print("freq failed\n");


  /* start playing */
  gst_element_set_state (thread, GST_STATE_PLAYING);
  
  /* do whatever you want here, the thread will be playing */
  g_print("thread is playing\n");

	tick=1;
	while(vol_doubleval > 0.0) {
    g_print("tick : vol=%f frq=%f - clock time %"G_GUINT64_FORMAT" - sink time %"G_GUINT64_FORMAT"\n",vol_doubleval,frq_doubleval,gst_clock_get_time(clock), gst_element_get_time(audiosink));
		//-- update params
    g_object_set_property(G_OBJECT(vol), "value_double", vol_val);
    //g_object_set_property(G_OBJECT(frq), "value_double", frq_val);
		//-- update values
    vol_doubleval-=0.1;//g_value_set_double(vol_val, vol_doubleval);
		//frq_doubleval*=1.5 ;g_value_set_double(frq_val, frq_doubleval);
		gst_element_wait(audiosink, GST_SECOND*tick++);
	}

  /* stop the pipeline */
  gst_element_set_state(thread, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref(GST_OBJECT (thread));
	
	g_free(vol_val);
	g_free(frq_val);
  
  exit(0);
}
