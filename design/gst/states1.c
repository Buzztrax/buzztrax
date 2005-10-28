/** $Id: states1.c,v 1.2 2005-10-28 16:54:17 ensonic Exp $
 * test mute, solo, bypass stuff in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.9 --cflags --libs` states1.c -o states1
 */
 
#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
//#define SINK_NAME "esdsink"
#define ELEM_NAME "audioconvert"
#define SRC_NAME "sinesrc"
#define SILENCE_NAME "audiotestsrc"

#define WAIT_LENGTH 4

static void query_and_print(GstElement *element, GstQuery *query) {
  gint64 pos;

  if(gst_element_query(element,query)) {
    gst_query_parse_position(query,NULL,&pos);
    printf("%s playback-pos : cur=%"G_GINT64_FORMAT"\n",GST_OBJECT_NAME(element),pos);
  }
  else {
    printf("%s playback-pos : cur=???\n",GST_OBJECT_NAME(element));
  }
}

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink;
  GstElement *src1,*src2;
  GstElement *vol1,*vol2;
  GstElement *mix,*silence;
  GstClock *clock;
  GstClockID clock_id;
  GstClockReturn wait_ret;
  GstQuery *query;
  GstPad *src1_sink;
  GstPad *src2_sink;
  GstPad *silence_sink;
  gboolean ret;
  
  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));
 
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
  g_object_set(src1,"freq",440.0,NULL);
  if(!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
  g_object_set(src2,"freq",2000.0,NULL);
  if(!(silence = gst_element_factory_make (SILENCE_NAME, "silence"))) {
    fprintf(stderr,"Can't create element \""SILENCE_NAME"\"\n");exit (-1);
  }
  g_object_set(silence,"wave",4,NULL);
  
  /* make a mixer */
  if(!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf(stderr,"Can't create element \"adder\"\n");exit (-1);
  }
    
  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src1,src2, vol1,vol2, silence, mix, sink, NULL);
  
  /* link elements */
  if(!gst_element_link_many (src1, vol1, mix, sink, NULL)) {
    fprintf(stderr,"Can't link first part\n");exit (-1);
  }
  if(!gst_element_link_many (src2, vol2, mix, NULL)) {
    fprintf(stderr,"Can't link second part\n");exit (-1);
  }
   
  /* make a position query */
  if(!(query=gst_query_new_position(GST_FORMAT_TIME))) {
    fprintf(stderr,"Can't make a position query\n");exit (-1);
  }

  
  /* prepare playing */
  if(gst_element_set_state (bin, GST_STATE_PAUSED)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't prepare playing\n");exit(-1);
  }

  /* get pads */
  if(!(src1_sink=gst_element_get_pad(src1,"src"))) {
    fprintf(stderr,"Can't get src pad of src1\n");exit (-1);
  }
  if(!(src2_sink=gst_element_get_pad(src2,"src"))) {
    fprintf(stderr,"Can't get src pad of src1\n");exit (-1);
  }
  if(!(silence_sink=gst_element_get_pad(silence,"src"))) {
    fprintf(stderr,"Can't get src pad of silence\n");exit (-1);
  }

  clock_id = gst_clock_new_periodic_id (clock,
    gst_clock_get_time (clock)+(WAIT_LENGTH * GST_SECOND), (WAIT_LENGTH * GST_SECOND));

  /* start playing */
  if(gst_element_set_state (bin, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }

  /* do the state tests */
  
  puts("play everything");
  query_and_print(src1,query);
  query_and_print(src2,query);
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
      GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }

  printf("trying to pause source2\n");
  ret=gst_pad_set_blocked(src2_sink,TRUE);
  printf("  =%d\n",ret);
  gst_element_unlink(src2,vol2);
  gst_element_link(silence,vol2);
  ret=gst_pad_set_blocked(src2_sink,FALSE);
  printf("  =%d\n",ret);
  if(gst_element_set_state (silence, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }
  
  printf("paused source2\n");
  query_and_print(src1,query);
  query_and_print(src2,query);
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
      GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }
  
  printf("trying to continue source2 and pause source1\n");
  ret=gst_pad_set_blocked(silence_sink,TRUE);
  printf("  =%d\n",ret);
  gst_element_unlink(silence,vol2);
  gst_element_link(src2,vol2);
  ret=gst_pad_set_blocked(silence_sink,FALSE);
  printf("  =%d\n",ret);
  if(gst_element_set_state (src2, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }
  ret=gst_pad_set_blocked(src1_sink,TRUE);
  printf("  =%d\n",ret);
  gst_element_unlink(src1,vol1);
  gst_element_link(silence,vol1);
  ret=gst_pad_set_blocked(src1_sink,FALSE);
  printf("  =%d\n",ret);
  if(gst_element_set_state (silence, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }

  printf("continued source2, paused source1\n");
  query_and_print(src1,query);
  query_and_print(src2,query);
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
      GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }
  
  /* stop the pipeline */
  gst_element_set_state (bin, GST_STATE_NULL);
  
  /* we don't need a reference to these objects anymore */
  gst_query_unref(query);
  g_object_unref (G_OBJECT (clock));
  g_object_unref (G_OBJECT (bin));

  exit (0);
}
