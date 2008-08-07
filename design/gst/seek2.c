/** $Id$
 *
 * Build a pipeline with testaudiosource->alsasink and sweep frequency and
 * volume. Use seeks to play partially or as a loop and adds a tee + analyzers.
 * 
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` seek2.c -o seek2
 */

#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// select loop or start offset mode
static gboolean loop=FALSE;

static void
event_loop (GstElement * bin)
{
  GstBus *bus;
  GstMessage *message = NULL;
  GstEvent *event;

  bus = gst_element_get_bus (GST_ELEMENT (bin));

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);

    g_assert (message != NULL);

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_SEGMENT_DONE: {
        event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
            GST_SEEK_FLAG_SEGMENT,
            GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
            GST_SEEK_TYPE_SET, (GstClockTime)6 * GST_SECOND);
        if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
          GST_WARNING("element failed to handle seek event");
        }
      } break;
      case GST_MESSAGE_STATE_CHANGED:
        if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
          GstStateChangeReturn state_res;
          GstState oldstate,newstate,pending;

          gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
          GST_INFO("state change on the bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
          switch(GST_STATE_TRANSITION(oldstate,newstate)) {
            case GST_STATE_CHANGE_READY_TO_PAUSED:
              if (loop) {
                event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
                    GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
                    GST_SEEK_TYPE_SET, (GstClockTime)6 * GST_SECOND);
              } else {
                event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, (GstClockTime)3 * GST_SECOND,
                    GST_SEEK_TYPE_SET, (GstClockTime)7 * GST_SECOND);
              }
              if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
                GST_WARNING("element failed to handle seek event");
              }
            
              // start playback for 7 second
              if ((state_res = gst_element_set_state(GST_ELEMENT(bin),GST_STATE_PLAYING))==GST_STATE_CHANGE_FAILURE) {
                GST_WARNING("can't go to playing state");
                return;
              }
              if(state_res == GST_STATE_CHANGE_ASYNC) {
                GST_INFO("->PLAYING needs async wait");
              }
              break;
            default:
              break;
          }
        }
        break;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR: {
        GError *gerror;
        gchar *debug;

        gst_message_parse_error (message, &gerror, &debug);
        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        gst_message_unref (message);
        g_error_free (gerror);
        g_free (debug);
        return;
      }
      default:
        gst_message_unref (message);
        break;
    }
  }
}

gint
main (gint argc, gchar ** argv)
{
  gint res = 1;
  GstElement *src, *tee, *queue, *elem, *fakesink, *sink;
  GstElement *bin;
  GstController *ctrl;
  GstClock *clock;
  GstClockID clock_id;
  //GstClockReturn wait_ret;
  GValue vol = { 0, };
  GstStateChangeReturn state_res;
  GstBus *bus;

  if(argc>1) {
    if(!strcasecmp("loop",argv[1])) loop=TRUE;
    else if(!strcasecmp("seek",argv[1])) loop=FALSE;
  }
  else {
    puts("Usage: seek2 [loop|seek]\n  using seek by default");
  }

  gst_init (&argc, &argv);
  gst_controller_init (&argc, &argv);

  // build pipeline
  bin = gst_pipeline_new ("pipeline");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));
  src = gst_element_factory_make ("audiotestsrc", "generate");
  tee = gst_element_factory_make ("tee", "tee");
  queue = gst_element_factory_make ("queue", "queue");
  // FIXME: it works when using identity/volume/audioconvert instead level/spectrum
  elem = gst_element_factory_make ("level", "elem");
  //elem = gst_element_factory_make ("identity", "identity");
  fakesink = gst_element_factory_make ("fakesink", "fakesink");
  sink = gst_element_factory_make ("alsasink", "render");
  
  g_object_set(src, "wave", 7, NULL);
  g_object_set(fakesink, "sync", FALSE, "qos", FALSE, "silent", TRUE, NULL);
  
  gst_bin_add_many (GST_BIN (bin), src, tee, sink, queue, elem, fakesink, NULL);
  if (!gst_element_link_many (src, tee, sink, NULL)) {
    GST_WARNING ("can't link elements: src ! tee ! sink");
    goto Error;
  }
  if (!gst_element_link_many (tee, queue, elem, fakesink, NULL)) {
    GST_WARNING ("can't link elements: tee ! queue ! elem ! fakesink");
    goto Error;
  }
  // add a controller to the source
  if (!(ctrl = gst_controller_new (G_OBJECT (src), "freq", "volume", NULL))) {
    GST_WARNING ("can't control source element");
    goto Error;
  }
  // set interpolation
  gst_controller_set_interpolation_mode (ctrl, "volume", GST_INTERPOLATE_LINEAR);
  gst_controller_set_interpolation_mode (ctrl, "freq", GST_INTERPOLATE_LINEAR);

  // set control values
  g_value_init (&vol, G_TYPE_DOUBLE);
  g_value_set_double (&vol, 0.3);
  gst_controller_set (ctrl, "volume", 0 * GST_SECOND, &vol);
  g_value_set_double (&vol, 1.0);
  gst_controller_set (ctrl, "volume", 5 * GST_SECOND, &vol);
  g_value_set_double (&vol, 0.3);
  gst_controller_set (ctrl, "volume", 6 * GST_SECOND, &vol);

  g_value_set_double (&vol, 440.0);
  gst_controller_set (ctrl, "freq", 0 * GST_SECOND, &vol);
  g_value_set_double (&vol, 2500.0);
  gst_controller_set (ctrl, "freq", 2 * GST_SECOND, &vol);
  g_value_set_double (&vol, 440.0);
  gst_controller_set (ctrl, "freq", 6 * GST_SECOND, &vol);

  clock_id =
      gst_clock_new_single_shot_id (clock,
      gst_clock_get_time (clock) + (7 * GST_SECOND));


  // connect to bus
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  // prepare playback
  if ((state_res = gst_element_set_state(GST_ELEMENT(bin),GST_STATE_PAUSED))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to paused state");
    goto Error;
  }
  if(state_res == GST_STATE_CHANGE_ASYNC) {
    GST_INFO("->PAUSED needs async wait");
  }

  event_loop (bin);
  /*
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }
  */

  gst_element_set_state (bin, GST_STATE_NULL);
  res = 0;
Error:
  // cleanup
  g_object_unref (G_OBJECT (ctrl));
  gst_clock_id_unref (clock_id);
  gst_object_unref (GST_OBJECT (clock));
  gst_object_unref (GST_OBJECT (bin));
  return (res);
}
