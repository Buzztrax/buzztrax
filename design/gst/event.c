/* test event execution
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` event.c -o event
 * GST_DEBUG="*:4" GST_DEBUG_NO_COLOR=1 ./event 2>debug.log
 * grep -i "event" debug.log >debug2.log
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

static gboolean
event_snoop (GstPad *pad, GstEvent *event, gpointer user_data) {
  GST_WARNING_OBJECT(pad, "%"GST_PTR_FORMAT, event);
  return TRUE;
}

static GstBusSyncReply
bus_snoop (GstBus * bus, GstMessage * message, gpointer user_data) {
  GST_WARNING_OBJECT(user_data,"%"GST_PTR_FORMAT, message);
  return GST_BUS_PASS;
}

int
main (int argc, char **argv)
{
  GstElement *bin;
  GstElement *sink;
  GstPad *pad;
  GstBus *bus;

  /* init gstreamer */
  gst_init (&argc, &argv);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "latency", 0, "latency test");

  bin = gst_parse_launch ("audiotestsrc ! pulsesink name=sink", NULL);
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_set_sync_handler (bus, bus_snoop, bin);
  gst_object_unref (bus);
  
  sink = gst_bin_get_by_name (GST_BIN (bin), "sink");
  pad = gst_element_get_static_pad (sink, "sink");
  gst_pad_add_event_probe (pad, G_CALLBACK (event_snoop), NULL);
  gst_object_unref (pad);
  gst_object_unref (sink);
  gst_element_set_state (bin, GST_STATE_PLAYING);
  GST_WARNING("started playback");
  
  g_usleep (500);
  GST_WARNING("before seek");
  gst_element_send_event (bin, gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, (GstClockTime)0,
        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE));
  /* This returns when flush start, seek, flush stop have been sent and
   * considerered by all elments, but before the newsegment has reached
   * the sink.
   * The sink is not posting any message for newsegment.
   */
  GST_WARNING("after seek");
  g_usleep (500);
  
  /* cleanup */
  GST_WARNING("stoping playback");
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (bin);
  
  exit (0);
}
