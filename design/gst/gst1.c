/** $Id: gst1.c,v 1.4 2004-02-12 18:01:30 ensonic Exp $
 */
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

int main(int argc, char **argv) {
	GstElement *audiosink, *audiosrc, *pipeline, *audioconvert;
  GstDParamManager *audiosrcParam;
  GstDParam *volume;
  gfloat set_to_value;
  GValue *set_val;
  
	gst_init(&argc, &argv);
  gst_control_init(&argc,&argv);
  
  if (argc < 3) {
    printf("use: %s <src> <sink> <value>\nvalue should be between 0 ... 100\n",argv[0]);
    exit(-1);
  }
  
  
  /* create a new pipeline to hold the elements */
  pipeline = gst_pipeline_new ("pipeline");
  
  /* and an audio sink */
  audiosink = gst_element_factory_make (argv[2], "play_audio");
  if (audiosrc == NULL) {
    fprintf(stderr,"Can't create element \"%s\"\n",argv[2]);exit (-1);
  }
  
  audioconvert = gst_element_factory_make ("float2int", "convert");
  if (audioconvert == NULL) {
    fprintf(stderr,"Can't create element \"float2int\"\n");exit (-1);
  }
  
  /* add an sine source */
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
  volume = gst_dparam_new(G_TYPE_FLOAT);
  if (gst_dpman_attach_dparam (audiosrcParam, "Amplitude", volume)){
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
		printf("playing ...\n");
    while (gst_bin_iterate (GST_BIN (pipeline))){
      set_to_value=set_to_value-0.01;
      g_value_set_float(set_val, set_to_value);
      g_object_set_property(G_OBJECT(volume), "value_float", set_val);
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
  /* unreffing the pipeline unrefs the contained elements as well */

  exit (0);
}
