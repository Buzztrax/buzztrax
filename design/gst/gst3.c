/** $Id: gst3.c,v 1.3 2004-02-17 14:27:00 ensonic Exp $
 */
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

int main(int argc, char **argv) {
	GstElement *audiosink, *audiosrc, *audioconvert, *thread;
  GstClock *clock;
	guint tick;
  GstDParamManager *audiosrcParam;
	gfloat vol_floatval;	// volume
	GValue *vol_val;
	GstDParam *vol;
	gfloat frq_floatval;	// frequency
	GValue *frq_val;
	GstDParam *frq;
  
	gst_init(&argc, &argv);
  gst_control_init(&argc,&argv);
  
  if (argc < 3) {
    g_print("usage: %s <src> <sink> <value>\nvalue should be between 0 ... 100\n",argv[0]);
		g_print("example: %s sinesrc esdsink 100\n",argv[0]);
    exit(-1);
  }
  
  /* create a new thread to hold the elements */
  thread = gst_thread_new ("thread");
  g_assert (thread != NULL);
  
  /* and an audio sink */
  audiosink = gst_element_factory_make (argv[2], "play_audio");
  g_assert (audiosrc != NULL);
  
  audioconvert = gst_element_factory_make ("audioconvert", "convert");
  
  /* add an sine source */
  audiosrc = gst_element_factory_make (argv[1], "audio-source");
  g_assert (audiosrc != NULL);
  
  /* getting param manager */
  audiosrcParam = gst_dpman_get_manager (audiosrc);
  /* setting param mode. Only synchronized is currently supported */
  gst_dpman_set_mode(audiosrcParam, "synchronous");
	/* preparing volume parameter */
  vol = gst_dparam_new(G_TYPE_FLOAT);
  if (gst_dpman_attach_dparam (audiosrcParam, "volume", vol)){
    /* the dparam was successfully attached */
		vol_floatval = atof(argv[3])/100.0;
    vol_val = g_new0(GValue,1);
    g_value_init(vol_val, G_TYPE_FLOAT);
    g_value_set_float(vol_val, vol_floatval);
    g_object_set_property(G_OBJECT(vol), "value_float", vol_val);
  }
	/* preparing frequency parameter */
  frq = gst_dparam_new(G_TYPE_FLOAT);
  if (gst_dpman_attach_dparam (audiosrcParam, "freq", frq)){
    /* the dparam was successfully attached */
		frq_floatval = 50.0;
    frq_val = g_new0(GValue,1);
    g_value_init(frq_val, G_TYPE_FLOAT);
    g_value_set_float(frq_val, frq_floatval);
    g_object_set_property(G_OBJECT(frq), "value_float", frq_val);
  }
	else g_print("freq failed\n");
  
  /* add objects to the thread */
  gst_bin_add_many (GST_BIN (thread), audiosrc, audioconvert, audiosink, NULL);
  /* link them in the logical order */
  gst_element_link_many (audiosrc, audioconvert, audiosink, NULL);
  
  /* getting the clock */
  //gst_bin_auto_clock(GST_BIN (thread));
  g_print("audiosink provides clock? %d\n",gst_element_provides_clock (audiosink));
  if (gst_element_provides_clock (audiosink)) {
    gst_bin_use_clock(GST_BIN (thread), gst_element_get_clock (audiosink));
    clock = gst_element_get_clock (audiosink);
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
  
  /* start playing */
  gst_element_set_state (thread, GST_STATE_PLAYING);
  
  /* do whatever you want here, the thread will be playing */
  g_print("thread is playing\n");

	tick=1;
	while(vol_floatval > 0.0) {
    g_print("tick : vol=%f frq=%f - clock time %"G_GUINT64_FORMAT" - sink time %"G_GUINT64_FORMAT"\n",vol_floatval,frq_floatval,gst_clock_get_time(clock), gst_element_get_time(audiosink));
		//-- update params
    g_object_set_property(G_OBJECT(vol), "value_float", vol_val);
    g_object_set_property(G_OBJECT(frq), "value_float", frq_val);
		//-- update values
    vol_floatval-=0.05;//g_value_set_float(vol_val, vol_floatval);
		frq_floatval*=1.5 ;g_value_set_float(frq_val, frq_floatval);
		gst_element_wait(audiosink, GST_SECOND*tick++);
	}

  /* stop the pipeline */
  gst_element_set_state (thread, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (thread));
	
	g_free(vol_val);
	g_free(frq_val);
  
  exit (0);
}
