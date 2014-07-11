/* Test seek in ready and position queries.
 *
 * gcc -Wall -g seekinready.c -o seekinready `pkg-config gstreamer-1.0 gstreamer-controller-1.0 --cflags --libs`
 *
 * GST_DEBUG="*:3,seekinready:4" ./seekinready 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gst/gst.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>

/* global stuff */

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GMainLoop *main_loop = NULL;
static GstEvent *seek_event = NULL;

/* helper */

static void
eos (const GstBus * const bus, GstMessage * message, GstElement * bin)
{
  GST_INFO
      ("eos ====================================================================");
  g_main_loop_quit (main_loop);
}

static void
send_initial_seek (GstBin * bin)
{
  GstIterator *it = gst_bin_iterate_sources (bin);
  GstElement *e;
  gboolean done = FALSE;
  GValue item = { 0, };

  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        e = GST_ELEMENT (g_value_get_object (&item));
        if (GST_IS_BIN (e)) {
          send_initial_seek ((GstBin *) e);
        } else {
          if (!(gst_element_send_event (e, gst_event_ref (seek_event)))) {
            fprintf (stderr, "element failed to handle seek event\n");
            g_main_loop_quit (main_loop);
          }
        }
        g_value_reset (&item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING ("wrong parameter for iterator");
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  g_value_unset (&item);
  gst_iterator_free (it);
}

static void
state_changed (const GstBus * const bus, GstMessage * message, GstElement * bin)
{
  if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
    GstStateChangeReturn res;
    GstState oldstate, newstate, pending;
    GstQuery *query;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS ((GstBin *) bin, GST_DEBUG_GRAPH_SHOW_ALL,
        "seekinready");
    GST_INFO ("state change on the bin: %s -> %s",
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate));
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_NULL_TO_READY:
        GST_INFO
            ("initial seek ===========================================================");
        send_initial_seek ((GstBin *) bin);
        // start playback
        GST_INFO
            ("start playing ==========================================================");
        res = gst_element_set_state (bin, GST_STATE_PLAYING);
        if (res == GST_STATE_CHANGE_FAILURE) {
          fprintf (stderr, "can't go to playing state\n");
          g_main_loop_quit (main_loop);
        } else if (res == GST_STATE_CHANGE_ASYNC) {
          GST_INFO ("->PLAYING needs async wait");
        }
        break;
      case GST_STATE_CHANGE_READY_TO_PAUSED:
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        query = gst_query_new_position (GST_FORMAT_TIME);
        if (gst_element_query (GST_ELEMENT (bin), query)) {
          gint64 pos;
          gst_query_parse_position (query, NULL, &pos);
          if (GST_CLOCK_TIME_IS_VALID (pos)) {
            GST_INFO ("playback pos: %" GST_TIME_FORMAT, GST_TIME_ARGS (pos));
          } else {
            GST_WARNING ("invalid playback pos");
          }
        } else {
          GST_WARNING ("query failed");
        }
        gst_query_unref (query);
        break;
      default:
        break;
    }
  }
}

static void
on_wave_notify (GstObject * src, GParamSpec * arg, gpointer user_data)
{
  gint val;

  g_object_get (src, "wave", &val, NULL);
  // we never want to see '0' here
  GST_INFO ("notify::wave: %d", val);
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin, *src, *sink;
  GstBus *bus;
  GstControlSource *cs;
  GstStateChangeReturn res;

  /* init gstreamer */
  gst_init (&argc, &argv);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "seekinready", 0, "seek test");

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");

  /* bus handlers */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK (eos), bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (state_changed),
      bin);

  main_loop = g_main_loop_new (NULL, FALSE);

  /* make elements and add them to the bin */
  src = gst_element_factory_make ("audiotestsrc", NULL);
  /* setup controller */
  cs = gst_interpolation_control_source_new ();
  gst_object_add_control_binding (GST_OBJECT_CAST (src),
      gst_direct_control_binding_new (GST_OBJECT_CAST (src), "wave", cs));
  gst_timed_value_control_source_set ((GstTimedValueControlSource *) cs, 0 * GST_SECOND, 0.0);  // sine
  gst_timed_value_control_source_set ((GstTimedValueControlSource *) cs, 1 * GST_SECOND, 1.0 / 12.0);   // square
  gst_timed_value_control_source_set ((GstTimedValueControlSource *) cs, 2 * GST_SECOND, 2.0 / 12.0);   // saw

  g_signal_connect (GST_OBJECT_CAST (src), "notify::wave",
      G_CALLBACK (on_wave_notify), NULL);
  gst_bin_add (GST_BIN (bin), src);

  sink = gst_element_factory_make ("autoaudiosink", NULL);
  gst_bin_add (GST_BIN (bin), sink);

  /* link elements */
  if (!gst_element_link (src, sink)) {
    fprintf (stderr, "Can't link elements\n");
    exit (-1);
  }

  /* initial seek event */
  seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 1 * GST_SECOND,
      GST_SEEK_TYPE_SET, (GstClockTime) 2 * GST_SECOND);

  /* prepare playing */
  GST_INFO
      ("prepare playing ========================================================");
  res = gst_element_set_state (bin, GST_STATE_READY);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to ready\n");
    exit (-1);
  }
  g_main_loop_run (main_loop);

  /* stop the pipeline */
  GST_INFO
      ("exiting ================================================================");
  gst_element_set_state (bin, GST_STATE_NULL);

  gst_event_unref (seek_event);
  gst_object_unref (bin);
  g_main_loop_unref (main_loop);
  exit (0);
}
