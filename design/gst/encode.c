/* Record n seconds of beep to a file
 *
 * gcc -Wall -g encode.c -o encode `pkg-config gstreamer-1.0 gstreamer-base-1.0 --cflags --libs`
 *
 * for fmt in `seq 0 6`; do ./encode $fmt; done
 *
 * when passing '1' as a second parameter, we don't seek, but set a duration,
 * this is not enough to trigger an eos though.
 */

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

#include <stdio.h>
#include <stdlib.h>

static void
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
  GstMessage *message = NULL;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);
    GST_INFO ("message %s received", GST_MESSAGE_TYPE_NAME (message));

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR:{
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
  gchar *pipeline = NULL;
  gint format = 0;
  gint method = 0;
  gint64 duration;

  gst_init (&argc, &argv);

  if (argc > 1) {
    format = atoi (argv[1]);
    if (argc > 2) {
      method = atoi (argv[2]);
    }
  }
  switch (format) {
    case 0:
      pipeline =
          "audiotestsrc name=s ! vorbisenc ! oggmux ! filesink location=encode.vorbis.ogg";
      puts ("encoding ogg vorbis");
      break;
    case 1:
      pipeline =
          "audiotestsrc name=s ! lamemp3enc ! filesink location=encode.mp3";
      puts ("encoding mp3");
      break;
    case 2:
      pipeline = "audiotestsrc name=s ! wavenc ! filesink location=encode.wav";
      puts ("encoding wav");
      break;
    case 3:
      pipeline =
          "audiotestsrc name=s ! flacenc ! flacparse ! flactag ! oggmux ! filesink location=encode.flac.ogg";
      puts ("encoding ogg flac");
      break;
    case 4:
      pipeline =
          "audiotestsrc name=s ! faac ! mp4mux ! filesink location=encode.m4a";
      puts ("encoding m4a");
      break;
    case 5:
      pipeline =
          "audiotestsrc name=s ! flacenc ! flacparse ! flactag ! filesink location=encode.flac";
      puts ("encoding ogg flac");
      break;
    case 6:
      pipeline = "audiotestsrc name=s ! filesink location=encode.raw";
      puts ("encoding raw");
      break;
    default:
      puts ("format must be 0-6");
      return -1;
  }
  bin = gst_parse_launch (pipeline, NULL);
  src = gst_bin_get_by_name (GST_BIN (bin), "s");

  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_READY);

  if (method == 1) {
    GST_BASE_SRC (src)->segment.duration = GST_SECOND;
  }
  if (method == 0) {
    if (!gst_element_seek (src, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
            GST_SEEK_TYPE_SET, GST_SECOND * 0,
            GST_SEEK_TYPE_SET, GST_SECOND * 1))
      puts ("seek on src in ready failed");
  }
  gst_object_unref (src);

  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PLAYING);

  if (!gst_element_query_duration (GST_ELEMENT (bin), GST_FORMAT_TIME,
          &duration)) {
    puts ("duration query failed");
  } else {
    printf ("duration: time :%" GST_TIME_FORMAT "\n", GST_TIME_ARGS (duration));
  }
  event_loop (bin);
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
