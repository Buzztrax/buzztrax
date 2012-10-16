/* test seek event execution
 *
 * gcc -Wall -g event.c -o event `pkg-config gstreamer-0.10 --cflags --libs`
 * GST_DEBUG="event:2" GST_DEBUG_NO_COLOR=1 ./event 2>debug.log <flush:0/1> <pipeline:0..n>
 *
 * A finished flushing seek is indicated by a async-done message on the bus.
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

#define GST_CAT_DEFAULT bt_event
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static gboolean
event_snoop (GstPad * pad, GstEvent * event, gpointer user_data)
{
  GST_WARNING_OBJECT (pad, "%" GST_PTR_FORMAT, event);
  return TRUE;
}

static GstBusSyncReply
bus_snoop (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GST_WARNING_OBJECT (user_data, "%" GST_PTR_FORMAT, message);
  return GST_BUS_PASS;
}

static void
add_probes (GstBin * bin)
{
  GstIterator *it1, *it2;
  gpointer data1, data2;
  gboolean done1, done2;

  it1 = gst_bin_iterate_recurse (bin);
  done1 = FALSE;
  while (!done1) {
    switch (gst_iterator_next (it1, &data1)) {
      case GST_ITERATOR_OK:
        it2 = gst_element_iterate_pads (GST_ELEMENT (data1));
        done2 = FALSE;
        while (!done2) {
          switch (gst_iterator_next (it2, &data2)) {
            case GST_ITERATOR_OK:
              gst_pad_add_event_probe (GST_PAD (data2),
                  G_CALLBACK (event_snoop), NULL);
              gst_object_unref (data2);
            case GST_ITERATOR_DONE:
              done2 = TRUE;
              break;
            case GST_ITERATOR_RESYNC:
              gst_iterator_resync (it2);
              break;
            case GST_ITERATOR_ERROR:
              g_return_if_reached ();
              break;
          }
        }
        gst_iterator_free (it2);
        gst_object_unref (data1);
        break;
      case GST_ITERATOR_DONE:
        done1 = TRUE;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it1);
        break;
      case GST_ITERATOR_ERROR:
        g_return_if_reached ();
        break;
    }
  }
  gst_iterator_free (it1);
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin;
  GstBus *bus;
  GstSeekFlags flags = GST_SEEK_FLAG_FLUSH;
  gint type = 0;

  /* init gstreamer */
  gst_init (&argc, &argv);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "event", 0, "seek event test");
  if (argc > 1) {
    flags = (atoi (argv[1]) == 0) ? GST_SEEK_FLAG_NONE : GST_SEEK_FLAG_FLUSH;
    if (argc > 2) {
      type = atoi (argv[2]);
    }
  }

  /* TODO(ensonic): also add queues */
  switch (type) {
    case 1:
      bin = gst_parse_launch ("audiotestsrc ! adder ! pulsesink", NULL);
      break;
    case 2:
      bin =
          gst_parse_launch
          ("audiotestsrc ! adder name=adder0 ! pulsesink audiotestsrc ! adder0.",
          NULL);
      break;
    case 3:
      bin =
          gst_parse_launch
          ("audiotestsrc ! adder name=adder0 ! adder name=adder1 ! pulsesink audiotestsrc ! adder0. audiotestsrc ! adder1.",
          NULL);
      break;
    case 0:
    default:
      bin = gst_parse_launch ("audiotestsrc ! pulsesink", NULL);
      break;
  }

  /* instrument the pipeline */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_set_sync_handler (bus, bus_snoop, bin);
  gst_object_unref (bus);
  add_probes (GST_BIN (bin));

  /* play */
  gst_element_set_state (bin, GST_STATE_PLAYING);
  gst_element_get_state (bin, NULL, NULL, GST_CLOCK_TIME_NONE);
  GST_WARNING ("started playback");

  g_usleep (G_USEC_PER_SEC / 100);
  GST_WARNING ("before seek");
  gst_element_send_event (bin, gst_event_new_seek (1.0, GST_FORMAT_TIME, flags,
          GST_SEEK_TYPE_SET, (GstClockTime) 0,
          GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE));
  /* This returns when flush start, seek, flush stop have been sent and
   * considerered by all elments, but before the newsegment has reached
   * the sink.
   * The sink is not posting any message for newsegment.
   */
  GST_WARNING ("after seek");
  g_usleep (G_USEC_PER_SEC / 200);

  /* cleanup */
  GST_WARNING ("stopping playback");
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (bin);

  exit (0);
}
