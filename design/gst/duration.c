/* Record n seconds of beep to an ogg file and checks duration queries
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` duration.c -o duration
 *
 * - the bin will query the duration from the sink(s)
 * - this will take the max value
 * - we only have one sink - filesink
 * - filesink does not know the duration and thus asks upstream
 * - once we hit adder, adder will ask each upstream branch and take the max,
 *   but as soon as a source reports -1 (unknown) adder overrides the max with
 *   -1 in turn
 */

#include <gst/gst.h>

#include <stdio.h>
#include <stdlib.h>

static void
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
  GstMessage *message = NULL;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);   
    GST_INFO("message %s received",GST_MESSAGE_TYPE_NAME(message));

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
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
  GstElement *bin, *src;
  GstFormat query_fmt = GST_FORMAT_TIME;
  gint64 duration;
  gchar *name = "s0";

  gst_init (&argc, &argv);
    
  bin = gst_parse_launch (
    "audiotestsrc name=s0 ! adder name=mix ! vorbisenc ! oggmux ! filesink location=duration.ogg "
    "audiotestsrc name=s1 ! mix. audiotestsrc name=s2 ! mix."
    /*
    "audiotestsrc name=s0 num-buffers=10000 ! adder name=mix ! vorbisenc ! oggmux ! filesink location=duration.ogg "
    "audiotestsrc name=s1 num-buffers=10000  ! mix. audiotestsrc name=s2 num-buffers=10000 ! mix."
    */
    , NULL);
  
  if (argc>1) {
    name = argv[1];
  }
  
  src = gst_bin_get_by_name (GST_BIN (bin), name);
  if (src) {
    g_object_set (src, "num-buffers", 1000, NULL);
    gst_object_unref (src);
  } else {
    puts("unknown src, not setting duration");
  }
  
  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PAUSED);
  if(!gst_element_query_duration (GST_ELEMENT (bin), &query_fmt, &duration)) {
    puts("duration query failed");
  } else {
    if (duration==GST_CLOCK_TIME_NONE) {
      puts("duration: unknown");
    } else {
      printf("duration: %s :%" GST_TIME_FORMAT "\n", 
          gst_format_get_name (query_fmt), GST_TIME_ARGS (duration));
    }
  }
  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PLAYING);
  event_loop (bin);
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
