/** $Id: gst4.c,v 1.1 2004-11-01 11:38:42 ensonic Exp $
* try to reproduce what I belive is a bug in gst
*
* gcc -Wall -g `pkg-config gstreamer-0.8 --cflags --libs` gst4.c -o gst4
*/
 
#include <stdio.h>
#include <gst/gst.h>
#include <gst/control/control.h>

#define SINK_NAME "esdsink"
#define ELEM_NAME "level"
#define SRC_NAME "esdmon"

//#define SINK_NAME "alsasink"
//#define ELEM_NAME "level"
//#define SRC_NAME "alsasrc"


int main(int argc, char **argv) {
  GstBin *bin;
  /* elements used in pipeline */
	GstElement *audiosink, *elem, *audiosrc;
  
  /* init gstreamer */
	gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("thread");
  
  /* try to create an audio sink */
  audiosink = gst_element_factory_make (SINK_NAME, "audio-sink");
  if (audiosink == NULL) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }
  
  /* try to craete an audoiconvert */
  elem = gst_element_factory_make (ELEM_NAME, "element");
  if (elem == NULL) {
    fprintf(stderr,"Can't create element \""ELEM_NAME"\"\n");exit (-1);
  }
  
  /* try to create an audio source */
  audiosrc = gst_element_factory_make (SRC_NAME, "audio-source");
  if (audiosrc == NULL) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), audiosrc, elem, audiosink, NULL);
  
   /* link src to level to sink */
  gst_element_link_many (audiosrc, elem, audiosink, NULL);
  
  /* start playing
  if(gst_element_set_state (bin, GST_STATE_PLAYING)) {
    while (gst_bin_iterate (GST_BIN (bin))){
    }
  }
  else {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }
  */
  /* stop the pipeline
  gst_element_set_state (bin, GST_STATE_NULL);
  */

  gst_bin_remove(bin,audiosink);
  gst_bin_remove(bin,elem);
  gst_bin_remove(bin,audiosrc);
  
  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bin));

  exit (0);
}
