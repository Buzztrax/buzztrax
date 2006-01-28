/** $Id: states2.c,v 1.3 2006-01-28 17:00:20 ensonic Exp $
 * test state changing in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` states2.c -o states2
 */
 
#include <stdio.h>
#include <unistd.h>

#include <gst/gst.h>

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *src1,*src2,*mix,*conv,*proc,*sink;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new("song");
 
  /* make elements */
  if(!(src1 = gst_element_factory_make ("audiotestsrc", "src1"))) {
    fprintf(stderr,"Can't create element \"audiotestsrc\"\n");exit (-1);
  }
  g_object_set(src1,"freq",440.0,NULL);
  if(!(src2 = gst_element_factory_make ("audiotestsrc", "src2"))) {
    fprintf(stderr,"Can't create element \"audiotestsrc\"\n");exit (-1);
  }
  g_object_set(src2,"freq",2000.0,NULL);
  if(!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf(stderr,"Can't create element \"adder\"\n");exit (-1);
  }
  if(!(conv = gst_element_factory_make ("audioconvert", "conv"))) {
    fprintf(stderr,"Can't create element \"audioconvert\"\n");exit (-1);
  }
  if(!(proc = gst_element_factory_make ("volume", "proc"))) {
    fprintf(stderr,"Can't create element \"volume\"\n");exit (-1);
  }
  if(!(sink = gst_element_factory_make ("alsasink", "sink"))) {
    fprintf(stderr,"Can't create element \"alsasink\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add_many(GST_BIN (bin), src1, src2, mix, conv, proc, sink, NULL);
  
  /* link chains */
  if(!gst_element_link (src1, mix)) {
    fprintf(stderr,"Can't link chain 1\n");exit (-1);
  }
  if(!gst_element_link (src2, mix)) {
    fprintf(stderr,"Can't link chain 2\n");exit (-1);
  }
  if(!gst_element_link (mix, conv)) {
    fprintf(stderr,"Can't link chain 3\n");exit (-1);
  }
  if(!gst_element_link (conv, proc)) {
    fprintf(stderr,"Can't link chain 4\n");exit (-1);
  }
  if(!gst_element_link (proc, sink)) {
    fprintf(stderr,"Can't link chain 5\n");exit (-1);
  }

  /* prepare playing */
  if(gst_element_set_state (bin, GST_STATE_PAUSED)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't prepare playing\n");exit(-1);
  }

  /* start playing */
  if(gst_element_set_state (bin, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }

  sleep(1);
  
  /* stop the pipeline */
  gst_element_set_state (bin, GST_STATE_NULL);
 
  
  /* we don't need a reference to these objects anymore */
  gst_object_unref(G_OBJECT(bin));

  exit (0);
}
