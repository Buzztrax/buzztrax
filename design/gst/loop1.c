/** $Id$
 * test handling looping in gstreamer
 *
 * gcc -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` loop1.c -o loop1
 */

#include <stdio.h>
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#define SINK_NAME "alsasink"
#define SRC_NAME "audiotestsrc"

static GMainLoop *main_loop=NULL;
static GstEvent *play_seek_event=NULL;

static void message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline) {
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    puts (sstr);
    g_free (sstr);
  }
  else {
    puts ("no message details");
  }

  g_main_loop_quit(main_loop);
}

static void state_changed(const GstBus * const bus, GstMessage *message,  GstElement *bin) {
  if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
    GstStateChangeReturn res;
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        // seek to start time
        puts("initial seek ===========================================================\n");
        if(!(gst_element_send_event(bin,gst_event_ref(play_seek_event)))) {
          fprintf(stderr,"element failed to handle seek event");
          g_main_loop_quit(main_loop);
        }
        // start playback
        puts("start playing ==========================================================\n");
        res=gst_element_set_state(bin,GST_STATE_PLAYING);
        if(res==GST_STATE_CHANGE_FAILURE) {
          fprintf(stderr,"can't go to playing state\n");
          g_main_loop_quit(main_loop);
        }
        else if(res==GST_STATE_CHANGE_ASYNC) {
          puts("->PLAYING needs async wait");
        }
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        puts("playback started =======================================================\n");
        break;
      default:
        break;
    }
  }
}

static void segment_done(const GstBus * const bus, const GstMessage * const message,  GstElement *bin) {
  puts("loop playback ==========================================================\n");
  if(!(gst_element_send_event(bin,gst_event_ref(play_seek_event)))) {
    fprintf(stderr,"element failed to handle continuing play seek event\n");
    g_main_loop_quit(main_loop);
  }
}


int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *src1,*src2,*mix,*sink;
  GstController *ctrl1,*ctrl2;
  GstBus *bus;
  GstStateChangeReturn res;
  GValue val = { 0, };

  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK(segment_done), bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK(state_changed), bin);
  gst_object_unref (G_OBJECT (bus));

  main_loop=g_main_loop_new(NULL,FALSE);

  /* make elements */
  if(!(src1 = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
  g_object_set (src1, "wave", 2, NULL);
  if(!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
  if(!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf(stderr,"Can't create element \"adder\"\n");exit (-1);
  }
  if(!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }

  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src1, src2, mix, sink, NULL);

  /* link elements */
  if(!gst_element_link_many (src1, mix, sink, NULL)) {
    fprintf(stderr,"Can't link part1\n");exit (-1);
  }
  if(!gst_element_link_many (src2, mix, NULL)) {
    fprintf(stderr,"Can't link part2\n");exit (-1);
  }

  /* prepare controller queues */
  g_value_init (&val, G_TYPE_DOUBLE);

  /* add a controller to the source */
  if (!(ctrl1 = gst_controller_new (G_OBJECT (src1), "freq", "volume", NULL))) {
    fprintf(stderr,"can't control source element");exit (-1);
  }
  /* set interpolation */
  gst_controller_set_interpolation_mode (ctrl1, "volume", GST_INTERPOLATE_LINEAR);
  gst_controller_set_interpolation_mode (ctrl1, "freq", GST_INTERPOLATE_LINEAR);
  /* set control values */
  g_value_set_double (&val, 1.0);
  gst_controller_set (ctrl1, "volume",   0 * GST_MSECOND, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl1, "volume", 500 * GST_MSECOND, &val);
  g_value_set_double (&val, 110.0);
  gst_controller_set (ctrl1, "freq", 0 * GST_MSECOND, &val);

  /* add a controller to the source */
  if (!(ctrl2 = gst_controller_new (G_OBJECT (src2), "freq", "volume", NULL))) {
    fprintf(stderr,"can't control source element");exit (-1);
  }
  /* set interpolation */
  gst_controller_set_interpolation_mode (ctrl2, "volume", GST_INTERPOLATE_LINEAR);
  gst_controller_set_interpolation_mode (ctrl2, "freq", GST_INTERPOLATE_LINEAR);
  /* set control values */
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl2, "volume",    0 * GST_MSECOND, &val);
  g_value_set_double (&val, 1.0);
  gst_controller_set (ctrl2, "volume",  500 * GST_MSECOND, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl2, "volume", 1000 * GST_MSECOND, &val);
  g_value_set_double (&val, 880.0);
  gst_controller_set (ctrl2, "freq", 0 * GST_MSECOND, &val);

  /* initial seek event (without flush) */
  play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, (GstClockTime)0,
        GST_SEEK_TYPE_SET, (GstClockTime)GST_SECOND);

  /* prepare playing */
  puts("prepare playing ========================================================\n");
  res=gst_element_set_state (bin, GST_STATE_PAUSED);
  if(res==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't go to paused\n");exit(-1);
  }
  else if(res==GST_STATE_CHANGE_ASYNC) {
    puts("->PAUSED needs async wait");
  }
  g_main_loop_run(main_loop);

  /* stop the pipeline */
  puts("exiting ================================================================\n");
  gst_element_set_state (bin, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bin));
  g_main_loop_unref(main_loop);

  exit (0);
}
