/** $Id: states.c,v 1.1 2005-09-02 16:32:41 ensonic Exp $
 * test mute, solo, bypass stuff in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.9 --cflags --libs` states.c -o states
 */
 
#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
//#define SINK_NAME "esdsink"
#define ELEM_NAME "volume"
#define SRC_NAME "sinesrc"


int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink;
	GstElement *src1,*src2;
	GstElement *vol1,*vol2;
	GstElement *mix;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  
  /* make a sink */
  if(!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }
  
  /* make filters */
  if(!(vol1 = gst_element_factory_make (ELEM_NAME, "filter1"))) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }
  if(!(vol2 = gst_element_factory_make (ELEM_NAME, "filter2"))) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }

  /* make sources */
  if(!(src1 = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
	g_object_set(src1,"freq",440,NULL);
  if(!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
	g_object_set(src2,"freq",2000,NULL);
  
  /* make a mixer */
  if(!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf(stderr,"Can't create element \"adder\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src1,src2, vol1,vol2, mix, sink, NULL);
  
  /* link elements */
  gst_element_link_many (src1, vol1, mix, sink, NULL);
	gst_element_link_many (src2, vol2, mix, NULL);
  
  /* start playing */
  if(gst_element_set_state (bin, GST_STATE_PLAYING)==GST_STATE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }

	/* do the state tests */
	
	puts("play everything");
	getchar();
	
  if(gst_element_set_state (src2, GST_STATE_PAUSED)==GST_STATE_FAILURE) {
    fprintf(stderr,"Can't pause src2\n");exit(-1);
  }
	puts("paused source2");
	getchar();

  if(gst_element_set_state (src1, GST_STATE_PAUSED)==GST_STATE_FAILURE) {
    fprintf(stderr,"Can't pause src1\n");exit(-1);
  }
  if(gst_element_set_state (src2, GST_STATE_PLAYING)==GST_STATE_FAILURE) {
    fprintf(stderr,"Can't continue src2\n");exit(-1);
  }
	puts("continued source2, paused source1");
	getchar();
	
  /* stop the pipeline */
  gst_element_set_state (bin, GST_STATE_NULL);
  
  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bin));

  exit (0);
}
