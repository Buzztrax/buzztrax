/* test linking in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` link2.c -o link2
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
//#define SINK_NAME "esdsink"
#define ELEM_NAME "audioconvert"
#define SRC_NAME "sinesrc"

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
	GstElement *proc;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
 
  /* make elements */
  if(!(proc = gst_element_factory_make (ELEM_NAME, "proc"))) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add (GST_BIN (bin), proc);
  
  /* link chain1 */
  if(!gst_element_link (proc, proc)) {
    fprintf(stderr,"Can't link chain\n");exit (-1);
  }
	
  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bin));

  exit (0);
}
