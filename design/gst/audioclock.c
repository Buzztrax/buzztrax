/* Check audioclock precission
 *
 * gcc -Wall -g audioclock.c -o audioclock `pkg-config gstreamer-1.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>

static GstClockTime mi = GST_CLOCK_TIME_NONE;
static GstClockTime ma = G_GUINT64_CONSTANT (0);

static gboolean
on_clock_time_reached (GstClock * clock, GstClockTime time, GstClockID id,
    gpointer user_data)
{
  static GstClockTime p1 = G_GUINT64_CONSTANT (0);
  static GstClockTime p2 = G_GUINT64_CONSTANT (0);
  GstClockTime rt = gst_util_get_timestamp ();

  if (p1) {
    //GstClockTime d1 = GST_CLOCK_DIFF (p1, time);    
    GstClockTime d2 = GST_CLOCK_DIFF (p2, rt);

    if (d2 < mi)
      mi = d2;
    if (d2 > ma)
      ma = d2;

    /*
       printf ("%"GST_TIME_FORMAT
       ", %8"G_GUINT64_FORMAT
       ", %8"G_GUINT64_FORMAT"\n", 
       GST_TIME_ARGS (time), d1, d2);
     */
  }
  p1 = time;
  p2 = rt;
  return TRUE;
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin, *src, *sink;
  GstClock *clock;
  GstClockID clock_id;
  GstClockTime s = 100;         // 100 ms

  /* init gstreamer */
  gst_init (&argc, &argv);

  if (argc > 1) {
    s = atol (argv[1]);
  }

  bin = gst_pipeline_new ("song");
  src = gst_element_factory_make ("audiotestsrc", NULL);
  sink = gst_element_factory_make ("pulsesink", NULL);
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  gst_element_link (src, sink);

  // configure src
  g_object_set (src, "is-live", TRUE, NULL);

  // configure buffer size
  gint64 chunk = GST_TIME_AS_USECONDS (GST_MSECOND * s);

  printf ("audio chunk-size to %" G_GUINT64_FORMAT " µs = %"
      G_GUINT64_FORMAT " ms\n", chunk, (chunk / G_GINT64_CONSTANT (1000)));
  g_object_set (sink, "latency-time", chunk, "buffer-time", chunk << 1, NULL);

  gst_element_set_state (bin, GST_STATE_READY);
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

  clock_id = gst_clock_new_periodic_id (clock, GST_MSECOND * 10, GST_MSECOND);
  gst_clock_id_wait_async (clock_id, on_clock_time_reached, NULL, NULL);
  gst_clock_id_unref (clock_id);

  gst_element_set_state (bin, GST_STATE_PLAYING);

  /* tests clock */
  g_usleep (5 * G_USEC_PER_SEC);

  /* cleanup */
  gst_element_set_state (bin, GST_STATE_NULL);
  g_usleep (G_USEC_PER_SEC / 10);
  gst_object_unref (clock);
  gst_object_unref (bin);

  printf ("%8" G_GUINT64_FORMAT ", %8" G_GUINT64_FORMAT "\n", mi, ma);

  return 0;
}
