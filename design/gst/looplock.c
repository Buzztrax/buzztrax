/* test looping
 *
 * gcc -Wall -g looplock.c -o looplock `pkg-config gstreamer-0.10 --cflags --libs`
 *
 * things we excluded:
 * - seek-events for the looping are non-flushing
 * - it is not pulsesink specific, it also happens with alsasink and even fakesink
 *
 * things to check:
 * - order of events:
 *   - segments_done is first emitted from sources
 *   - does it travel to sinks, then gets via the bus to the apps?
 *   - if so the sources will not do anything after segment_done and the queues are
 *     drained
 *   - when we seek to restrat the loop, the event goes from the sink to the sources
 *   - sources send new-segment events followed by data packets
 * - duplicated events
 *   - we get a couple of "duplicate event found"
 *   - if we drop duplicated seeks, we get seek failures in adder and it also blocks
 *   - the duplicated seeks also cause duplicated newsegments (which the adder might
 *     expect)
 *   - we're getting more newsegments then we sent seeks from the first loop onwards
 *     - these seem to be the ones that close the running segment
 *     - if we're getting one for each src the loop continues
 *     - ideally tee would handle this 
 * - we might need a copy of tee in buzztard that is seek aware, so that
 *   1) flush_{start,stop} and new_segment events go back to the pad, from which it
 *      got the seek
 *   2) it could do something sensible with duplicated events (like not forwarding
 *      them, but returning TRUE, so that the seek does not fail and sending a copy
 *      of the new-segment reply)
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

/* parameter */
static gchar *audio_sink = "autoaudiosink";
static gint num_samples = 64;
static gint num_srcs = 10;
/* compile time parameters */
static const gint loop_secs = 1;

#define MAX_SRC 25

static GstBin *bin;
static GMainLoop *main_loop = NULL;
static GstEvent *play_seek_event = NULL;
static GstEvent *loop_seek_event = NULL;

static void
message_received (GstBus * bus, GstMessage * message, gpointer user_data)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "%s", sstr);
    g_free (sstr);
  } else {
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "no message details");
  }

  g_main_loop_quit (main_loop);
}

static void
state_changed (GstBus * bus, GstMessage * message, gpointer user_data)
{
  if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
    GstState oldstate, newstate, pending;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        gst_element_send_event ((GstElement *) bin, play_seek_event);
        gst_element_set_state ((GstElement *) bin, GST_STATE_PLAYING);
        break;
      default:
        break;
    }
  }
}

static void
segment_done (GstBus * bus, GstMessage * message, gpointer user_data)
{
  puts ("loop end ======================================================");
  GstEvent *event = gst_event_copy (loop_seek_event);
  gst_event_set_seqnum (event, gst_util_seqnum_next ());
  if (!(gst_element_send_event ((GstElement *) bin, event))) {
    fprintf (stderr, "element failed to handle continuing play seek event\n");
    g_main_loop_quit (main_loop);
  }
}

static gboolean
tee_event_filter (GstPad * pad, GstEvent * event, gpointer user_data)
{
  gint i = GPOINTER_TO_INT (user_data);
  static guint32 seek_seqnums[MAX_SRC] = { G_MAXUINT32, };
  static gint ev_balance[MAX_SRC] = { 0, };
  guint32 seq_num = gst_event_get_seqnum (event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      if (seq_num == seek_seqnums[i]) {
        fprintf (stderr, "%s.%s: dup seek: %lu from %s.%s\n",
            GST_DEBUG_PAD_NAME (pad), (gulong) seq_num,
            GST_DEBUG_PAD_NAME (GST_EVENT_SRC (event)));
        ev_balance[i]++;
      } else {
        fprintf (stderr, "%s.%s: new seek: %lu from %s.%s\n",
            GST_DEBUG_PAD_NAME (pad), (gulong) seq_num,
            GST_DEBUG_PAD_NAME (GST_EVENT_SRC (event)));
        seek_seqnums[i] = seq_num;
        ev_balance[i] = 1;
      }
      break;
    case GST_EVENT_NEWSEGMENT:
      ev_balance[i]--;
      if (ev_balance[i] < 0) {
        fprintf (stderr, "%s.%s: dropping newseg: %lu from %s.%s\n",
            GST_DEBUG_PAD_NAME (pad), (gulong) seq_num,
            GST_DEBUG_PAD_NAME (GST_EVENT_SRC (event)));
        return FALSE;
      } else {
        fprintf (stderr, "%s.%s: newseg: %lu from %s.%s\n",
            GST_DEBUG_PAD_NAME (pad), (gulong) seq_num,
            GST_DEBUG_PAD_NAME (GST_EVENT_SRC (event)));
      }
      break;
    default:
      break;
  }
  return TRUE;
}

int
main (int argc, char **argv)
{
  GstBus *bus;
  GstElement *src, *tee, *q1, *q2, *mix, *conv, *sink;
  GstPad *tsp;
  GstCaps *caps;
  gint i;
  gdouble freq = 50.0;

  if (argc > 1) {
    i = 1;
    audio_sink = argv[i++];
    if (argc > i) {
      num_samples = atoi (argv[i++]);
      num_samples = MAX (1, num_samples);
      if (argc > i) {
        num_srcs = atoi (argv[i++]);
        num_srcs = CLAMP (1, num_srcs, MAX_SRC);
      }
    }
  }
  printf
      ("Playing %d sources with buffers of %d samples on %s.\n",
      num_srcs, num_samples, audio_sink);

  /* init gstreamer */
  gst_init (&argc, &argv);

  /* build pipeline */
  bin = (GstBin *) gst_pipeline_new ("song");
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), NULL);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      NULL);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK (segment_done),
      NULL);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (state_changed),
      NULL);

  caps = gst_caps_new_simple ("audio/x-raw-float",
      "width", G_TYPE_INT, 32,
      "channels", G_TYPE_INT, 1,
      "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT,
      "endianness", G_TYPE_INT, G_BYTE_ORDER, NULL);

  sink = gst_element_factory_make (audio_sink, NULL);
  gst_bin_add (bin, sink);
  conv = gst_element_factory_make ("audioconvert", NULL);
  gst_bin_add (bin, conv);
  mix = gst_element_factory_make ("adder", NULL);
  g_object_set (mix, "caps", caps, NULL);
  gst_caps_unref (caps);
  gst_bin_add (bin, mix);
  gst_element_link_many (mix, conv, sink, NULL);

  for (i = 0; i < num_srcs; i++) {
    src = gst_element_factory_make ("audiotestsrc", NULL);
    g_object_set (src, "freq", freq + (i * freq), "samplesperbuffer",
        num_samples, NULL);
    gst_bin_add (bin, src);

    tee = gst_element_factory_make ("tee", NULL);
    gst_bin_add (bin, tee);
    tsp = gst_element_get_static_pad (tee, "sink");
    gst_pad_add_event_probe (tsp, (GCallback) tee_event_filter,
        GINT_TO_POINTER (i));
    gst_object_unref (tsp);

    q1 = gst_element_factory_make ("queue", NULL);
    g_object_set (q1, "silent", TRUE, "max-size-buffers", 1,
        "max-size-bytes", 0, "max-size-time", G_GUINT64_CONSTANT (0), NULL);
    gst_bin_add (bin, q1);
    q2 = gst_element_factory_make ("queue", NULL);
    g_object_set (q2, "silent", TRUE, "max-size-buffers", 1,
        "max-size-bytes", 0, "max-size-time", G_GUINT64_CONSTANT (0), NULL);
    gst_bin_add (bin, q2);

    gst_element_link_many (src, tee, q1, mix, NULL);
    gst_element_link_many (tee, q2, mix, NULL);
  }

  /* setup events */
  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) (loop_secs * GST_SECOND));
  loop_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) (loop_secs * GST_SECOND));

  /* loop pipeline */
  main_loop = g_main_loop_new (NULL, FALSE);
  gst_element_set_state ((GstElement *) bin, GST_STATE_PAUSED);
  g_main_loop_run (main_loop);

  /* no cleanup, we exit with Ctrl-C */
  exit (0);
}
