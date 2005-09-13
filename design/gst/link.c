/** $Id: link.c,v 1.1 2005-09-13 21:18:33 ensonic Exp $
 * test linking in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.9 --cflags --libs` link.c -o link
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
  GstElement *sink;
	GstElement *proc;
	GstElement *src;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
 
  /* make elements */
  if(!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }
  if(!(proc = gst_element_factory_make (ELEM_NAME, "proc"))) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }
  if(!(src = gst_element_factory_make (SRC_NAME, "src"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src, proc, sink, NULL);
  
  /* link elements */
  if(!gst_element_link_many (src, proc, sink, NULL)) {
    fprintf(stderr,"Can't link elements\n");exit (-1);
  }
	
	/* now link again */
	if(!gst_element_link_many (src, proc, sink, NULL)) {
    fprintf(stderr,"Okay we can't link elements again\n");exit (-1);
  }
	
  /* we don't need a reference to these objects anymore */
  g_object_unref (G_OBJECT (bin));

  exit (0);
}
