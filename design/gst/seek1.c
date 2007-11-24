/** $Id: seek1.c,v 1.7 2007-11-24 19:07:36 ensonic Exp $
 *
 * Build a pipeline with testaudiosource->alsasink
 * and sweep frequency and volume
 * Use seeks to play partially or as a loop
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` seek1.c -o seek1
 */

#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// select loop or start offset mode
static gboolean loop=FALSE;

//#define PLAY_BROKEN 1


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
    
    GST_INFO("message %s received",GST_MESSAGE_TYPE_NAME(message));

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

// this fails, if we do this in main it works
#ifdef PLAY_BROKEN
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
      case GST_MESSAGE_ASYNC_DONE: {
          GstStateChangeReturn state_res;
          GST_INFO("retry playing after async wait");
          // start playback for 7 second
          if ((state_res = gst_element_set_state(GST_ELEMENT(bin),GST_STATE_PLAYING))==GST_STATE_CHANGE_FAILURE) {
            GST_WARNING("can't go to playing state");
            return;
          }
          if(state_res == GST_STATE_CHANGE_ASYNC) {
            GST_INFO("->PLAYING needs async wait");
          }
        }
        break;
#endif
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
  GstElement *src, *sink;
  GstElement *bin;
  GstController *ctrl;
  GValue vol = { 0, };
  GstStateChangeReturn state_res;
  GstBus *bus;

  if(argc>1) {
    if(!strcasecmp("loop",argv[1])) loop=TRUE;
    else if(!strcasecmp("seek",argv[1])) loop=FALSE;
  }
  else {
    puts("Usage: seek1 [loop|seek]\n  using seek by default");
  }

  gst_init (&argc, &argv);
  gst_controller_init (&argc, &argv);

  // build pipeline
  bin = gst_pipeline_new ("pipeline");
  src = gst_element_factory_make ("audiotestsrc", "gen_audio");
  sink = gst_element_factory_make ("alsasink", "play_audio");
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  if (!gst_element_link (src, sink)) {
    GST_WARNING ("can't link elements");
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

  // this works, if we do this in the bus handler it does not :/
#ifndef PLAY_BROKEN
  {
    GstEvent *event;

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
    if ((state_res = gst_element_set_´state(GST_ELEMENT(bin),GST_STATE_PLAYING))==GST_STATE_CHANGE_FAILURE) {
      GST_WARNING("can't go to playing state");
      goto Error;
    }
    if(state_res == GST_STATE_CHANGE_ASYNC) {
      GST_INFO("->PLAYING needs async wait");
    }
  }
#endif

  event_loop (bin);
  gst_element_set_state (bin, GST_STATE_NULL);
  res = 0;
Error:
  // cleanup
  g_object_unref (G_OBJECT (ctrl));
  gst_object_unref (GST_OBJECT (clock));
  gst_object_unref (GST_OBJECT (bin));
  return (res);
}
