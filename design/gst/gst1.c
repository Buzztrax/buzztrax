/** $Id: gst1.c,v 1.2 2004-02-11 11:07:27 waffel Exp $
 */
 
#include <stdio.h>
#include <gst/gst.h>

int main(int argc, char **argv) {
	GstElement *audiosink, *sinesrc, *pipeline;
  int counter=0;
  GstElementStateReturn ret;
	
	gst_init(&argc, &argv);
  
  /* create a new pipeline to hold the elements */
  pipeline = gst_pipeline_new ("pipeline");
  
  /* and an audio sink */
  audiosink = gst_element_factory_make ("esdsink", "play_audio");
  if (audiosink == NULL) {
    exit (-1);
  }
  
  /* add an sine source */
  sinesrc = gst_element_factory_make ("sinesrc", "sine-source");
  if (sinesrc == NULL) {
    exit (-2);
  }
  
  /* add objects to the main pipeline */
  gst_bin_add_many (GST_BIN (pipeline), sinesrc, audiosink, NULL);
  
  /* start playing */
  ret = gst_element_set_state (pipeline, GST_STATE_READY);
  printf("%d\n",ret);
  
  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  printf("%d\n",ret);

  while (gst_bin_iterate (GST_BIN (pipeline))){
    counter++;
    printf("%d\n",counter);
  }

  /* stop the pipeline */
  gst_element_set_state (pipeline, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (pipeline));
  /* unreffing the pipeline unrefs the contained elements as well */

  exit (0);
}
