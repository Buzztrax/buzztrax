/** $Id$
 *
 * Build a pipeline with two mixed audiotestsrc elements
 * and sweep frequency and volume
 *
 * Use different seek variants
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` seek3.c -o seek3
 */

#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
          GST_SEEK_TYPE_SET, (GstClockTime)5 * GST_SECOND);
        if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
          GST_WARNING("element failed to handle seek event");
        }
      } break;
      case GST_MESSAGE_STATE_CHANGED:
        if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
          GstState oldstate,newstate,pending;

          gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
          GST_INFO("state change on the bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
          switch(GST_STATE_TRANSITION(oldstate,newstate)) {
            case GST_STATE_CHANGE_NULL_TO_READY:
              event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
                  GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
                  GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
                  GST_SEEK_TYPE_SET, (GstClockTime)5 * GST_SECOND);
              if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
                GST_WARNING("element failed to handle seek event");
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

static gboolean
seek_back (GstElement * bin)
{
#if 0 /* needs flush, otherwise the two source get slighly off */
  GstEvent *event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
      GST_SEEK_TYPE_SET, (GstClockTime)5 * GST_SECOND);
#endif
#if 0
  /* gst_event_new_new_segment_full: assertion `start <= stop' failed */
  GstEvent *event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
      GST_SEEK_TYPE_SET, (GstClockTime)5 * GST_SECOND/2);
#endif
#if 0 /* works ! */
  GstEvent *event = gst_event_new_seek(0.5, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
#endif
  GST_INFO("seek back");
  if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
    GST_WARNING("element failed to handle seek event");
  }
  return(FALSE);
}

gint
main (gint argc, gchar ** argv)
{
  gint res = 1;
  GstElement *src1,*src2,*mix,*sink;
  GstElement *bin;
  GstController *ctrl1, *ctrl2;
  GValue vol = { 0, };
  GstStateChangeReturn state_res;
  GstBus *bus;
  gint i;
  GstClockTime hsec = GST_SECOND/2;


  gst_init (&argc, &argv);
  gst_controller_init (&argc, &argv);

  g_value_init (&vol, G_TYPE_DOUBLE);


  // build pipeline
  bin = gst_pipeline_new ("pipeline");
  src1 = gst_element_factory_make ("audiotestsrc", NULL);
  src2 = gst_element_factory_make ("audiotestsrc", NULL);
  mix = gst_element_factory_make ("adder", NULL);
  sink = gst_element_factory_make ("alsasink", NULL);
  gst_bin_add_many (GST_BIN (bin), src1, src2, mix, sink, NULL);
  if (!gst_element_link_many (src1, mix, sink, NULL)) {
    GST_WARNING ("can't link elements");
    goto Error;
  }
  if (!gst_element_link_many (src2, mix, NULL)) {
    GST_WARNING ("can't link elements");
    goto Error;
  }


  // configure source 1
  g_object_set(src1,"freq",200.0,NULL);
  // add a controller to the source1
  if (!(ctrl1 = gst_controller_new (G_OBJECT (src1), "volume", NULL))) {
    GST_WARNING ("can't control source element");
    goto Error;
  }
  // set interpolation
  gst_controller_set_interpolation_mode (ctrl1, "volume", GST_INTERPOLATE_NONE);

  // set control values
  for(i=0;i<5;i++) {
    g_value_set_double (&vol, 0.0);
    gst_controller_set (ctrl1, "volume", (i * 2) * hsec, &vol);
    g_value_set_double (&vol, 1.0);
    gst_controller_set (ctrl1, "volume", (1 + i * 2) * hsec, &vol);
  }

  // configure source 2
  g_object_set(src2,"freq",800.0,NULL);
  // add a controller to the source1
  if (!(ctrl2 = gst_controller_new (G_OBJECT (src2), "volume", NULL))) {
    GST_WARNING ("can't control source element");
    goto Error;
  }
  // set interpolation
  gst_controller_set_interpolation_mode (ctrl2, "volume", GST_INTERPOLATE_NONE);

  // set control values
  for(i=0;i<5;i++) {
    g_value_set_double (&vol, 1.0);
    gst_controller_set (ctrl2, "volume", (i * 2) * hsec, &vol);
    g_value_set_double (&vol, 0.0);
    gst_controller_set (ctrl2, "volume", (1 + i * 2) * hsec, &vol);
  }

  // connect to bus
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  // start playback
  if ((state_res = gst_element_set_state(GST_ELEMENT(bin),GST_STATE_PLAYING))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to playing state");
    goto Error;
  }
  if(state_res == GST_STATE_CHANGE_ASYNC) {
    GST_INFO("->PLAYING needs async wait");
  }
  
  g_timeout_add(2000, (GSourceFunc)seek_back, bin);

  event_loop (bin);
  gst_element_set_state (bin, GST_STATE_NULL);
  res = 0;
Error:
  // cleanup
  g_object_unref (G_OBJECT (ctrl1));
  g_object_unref (G_OBJECT (ctrl2));
  gst_object_unref (GST_OBJECT (bin));
  return (res);
}
