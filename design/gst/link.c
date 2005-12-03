/** $Id: link.c,v 1.3 2005-12-03 16:24:21 ensonic Exp $
 * test linking in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` link.c -o link
 */
 
#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
//#define SINK_NAME "esdsink"
#define ELEM_NAME "audioconvert"
#define SRC_NAME "sinesrc"

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink, *mix;
	GstElement *proc1,*proc2;
	GstElement *src, *tee;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
 
  /* make elements */
  if(!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }
  if(!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf(stderr,"Can't create element \"adder\"\n");exit (-1);
  }
  if(!(proc1 = gst_element_factory_make (ELEM_NAME, "proc1"))) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }
  if(!(proc2 = gst_element_factory_make (ELEM_NAME, "proc2"))) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }
  if(!(tee = gst_element_factory_make ("tee", "tee"))) {
    fprintf(stderr,"Can't create element \"tee\"\n");exit (-1);
  }
  if(!(src = gst_element_factory_make (SRC_NAME, "src"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src, tee, proc1, proc2, mix, sink, NULL);
  
  /* link chain1 */
  if(!gst_element_link_many (src, tee, proc1, mix, sink, NULL)) {
    fprintf(stderr,"Can't link chain 1\n");exit (-1);
  }
	
	/* now link chain2 */
	if(!gst_element_link_many (tee, proc2, mix, NULL)) {
    fprintf(stderr,"Can't link chain2\n");exit (-1);
  }
	
  /* we don't need a reference to these objects anymore */
  g_object_unref (G_OBJECT (bin));

  exit (0);
}
