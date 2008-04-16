/** $Id$
* small example, how to create a pipeline with one source and one sink and how to
* use dparams to control the volume. The volume in this example is decrementes from
* the given volume to silence. After silence is reached, the example stpped.
*/
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

int main(int argc, char **argv) {
  /* elements used in pipeline */
  GstElement *audiosink, *audiosrc, *pipeline, *audioconvert;
  /* dynamic parameters manager*/
  GstDParamManager *audiosrcParam;
  /* one dynamic parameter, volume */
  GstDParam *volume;
  /* float value, used to set value to the dynamic parameter */
  gfloat set_to_value;
  /* the value to set dynamicly */
  GValue *set_val;
   
  /* if wrong count of commandline parameters are given, print a short help and exit */
  if (argc < 3) {
    g_print("use: %s <src> <sink> <value>\nvalue should be between 0 ... 100\n",argv[0]);
    exit(-1);
  }
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  /* init gstreamer control */
  gst_control_init(&argc,&argv);
  
  /* create a new pipeline to hold the elements */
  pipeline = gst_pipeline_new ("pipeline");
  
  /* try to create an audio sink */
  audiosink = gst_element_factory_make (argv[2], "play_audio");
  if (audiosink == NULL) {
    fprintf(stderr,"Can't create element \"%s\"\n",argv[2]);exit (-1);
  }
  
  /* try to craete an audoiconvert */
  audioconvert = gst_element_factory_make ("audioconvert", "convert");
  if (audioconvert == NULL) {
    fprintf(stderr,"Can't create element \"audioconvert\"\n");exit (-1);
  }
  
  /* try to create an audio source */
  audiosrc = gst_element_factory_make (argv[1], "audio-source");
  if (audiosrc == NULL) {
    fprintf(stderr,"Can't create element \"%s\"\n",argv[1]);exit (-1);
  }
  
  set_to_value = atof(argv[3]);
  set_to_value = set_to_value/100.0;
  /* getting param manager */
  audiosrcParam = gst_dpman_get_manager (audiosrc);
  
  /* setting param mode. Only synchronized currently supported */
  gst_dpman_set_mode(audiosrcParam, "synchronous");
  volume = gst_dparam_new(G_TYPE_DOUBLE);
  
  if (gst_dpman_attach_dparam(audiosrcParam, "volume", volume)){
    /* the dparam was successfully attached */
    set_val = g_new0(GValue,1);
    g_value_init(set_val, G_TYPE_FLOAT);
    g_value_set_float(set_val, set_to_value);
    g_object_set_property(G_OBJECT(volume), "value_float", set_val);
  }
  else {
    fprintf(stderr,"Can't attach dparam\n");exit(-1);
  }
  
  /* add objects to the main pipeline */
  gst_bin_add_many (GST_BIN (pipeline), audiosrc, audioconvert, audiosink, NULL);
  
   /* link src to sink */
  gst_element_link_many (audiosrc, audioconvert, audiosink, NULL);
  
  /* start playing */
  if(gst_element_set_state (pipeline, GST_STATE_PLAYING)) {
    g_print("playing ...\n");
    while (gst_bin_iterate (GST_BIN (pipeline))){
      /* decrement the volume */
      set_to_value=set_to_value-0.01;
      g_value_set_float(set_val, set_to_value);
      g_object_set_property(G_OBJECT(volume), "value_double", set_val);
      /* if silence reached, break the loop */
      if (set_to_value < 0.1) {
        break;
      }
    }
  }
  else {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }
  /* stop the pipeline */
  gst_element_set_state (pipeline, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (pipeline));

  exit (0);
}
