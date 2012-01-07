/* test tag writing in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` tags1.c -o tags1
 */
 
#include <stdio.h>
#include <gst/gst.h>

#define WAIT_LENGTH 2

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink;
  GstElement *conv,*enc,*mux;
  GstElement *src;
  /* the tags */
  GstTagList *taglist;
  /* timing objects */  
  GstClock *clock;
  GstClockID clock_id;
  GstClockReturn wait_ret;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));
 
  /* make a sink */
  if(!(sink = gst_element_factory_make ("filesink", "sink"))) {
    fprintf(stderr,"Can't create element \"filesink\"\n");exit (-1);
  }
  g_object_set(sink,"location","tags1.ogg",NULL);
  
  /* make encoder & muxer */
  if(!(conv=gst_element_factory_make("audioconvert","makefloat"))) {
    fprintf(stderr,"Can't create element \"audioconvert\"\n");exit (-1);
  }
  if(!(enc=gst_element_factory_make("vorbisenc","vorbisenc"))) {
    fprintf(stderr,"Can't create element \"vorbisenc\"\n");exit (-1);
  }
  if(!(mux=gst_element_factory_make("oggmux","oggmux"))) {
    fprintf(stderr,"Can't create element \"oggmux\"\n");exit (-1);
  }

  /* make source */
  if(!(src = gst_element_factory_make ("audiotestsrc", "src1"))) {
    fprintf(stderr,"Can't create element \"audiotestsrc\"\n");exit (-1);
  }
  
  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src, conv,enc,mux, sink, NULL);
  
  /* link elements */
  if(!gst_element_link_many (src, conv,enc,mux, sink, NULL)) {
    fprintf(stderr,"Can't link elements\n");exit (-1);
  }

  /* prepare tags */
  taglist=gst_tag_list_new();
  gst_tag_list_add(taglist, GST_TAG_MERGE_REPLACE,
    GST_TAG_TITLE, "cool song",
    GST_TAG_ARTIST, "master sine",
    NULL);
  
  /* prepare playing */
  if(gst_element_set_state (bin, GST_STATE_PAUSED)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't prepare playing\n");exit(-1);
  }

  clock_id = gst_clock_new_periodic_id (clock,
    gst_clock_get_time (clock)+(WAIT_LENGTH * GST_SECOND), (WAIT_LENGTH * GST_SECOND));
  
  /* send tags */
  if(argc>1) {
    puts("sending tags to bin");
    gst_element_found_tags(bin, taglist);
  }
  else {
    puts("sending tags to src");
    gst_element_found_tags(src, taglist);
  }
  /*
  iter=gst_bin_iterate_all_by_interface (bin, GST_TYPE_TAG_SETTER);
  ...
  
  gst_tag_setter_merge_tags(setter,taglist,GST_TAG_MERGE_REPLACE)
  */

  /* start playing */
  if(gst_element_set_state (bin, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }
  puts("play everything");
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
      GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }
 
  /* stop the pipeline */
  gst_element_set_state (bin, GST_STATE_NULL);
  
  /* we don't need a reference to these objects anymore */
  gst_clock_id_unref (clock_id);
  gst_object_unref (GST_OBJECT (clock));
  gst_object_unref (GST_OBJECT (bin));

  if(argc>1) {
    puts("FAILS");
    exit (1);
  }
  else {
    puts("PASSES");
    exit (0);
  }
}
