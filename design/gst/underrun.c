/* test underrun behaviour for various audiosinks
 *
 * gcc -Wall -g underrun.c -o underrun `pkg-config gstreamer-0.10 --cflags --libs`
 *
 * things we excluded:
 * - seek-events for the looping are non-flushing
 *
 * things to check:
 * - order of events:
 *   - segments_done is first emitted from sources
 *   - does it travel to sinks, then gets via the bus to the apps?
 *   - if so the sources will not do anything after segment_done and the queues are
 *     drained
 *   - when we seek to restrat the loop, the event goes from the sink to the sources
 *   - sources send new-segment events followed by data packets
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

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
  puts ("loop playback ======================================================");
  if (!(gst_element_send_event ((GstElement *) bin,
              gst_event_ref (loop_seek_event)))) {
    fprintf (stderr, "element failed to handle continuing play seek event\n");
    g_main_loop_quit (main_loop);
  }
}


int
main (int argc, char **argv)
{
  GstBus *bus;
  GstElement *src, *queue, *mix, *sink;
  GstCaps *caps;
  gint i;
  gdouble freq = 50.0;
  /* parameter */
  gchar *audio_sink = "autoaudiosink";
  gint num_samples = 2048;
  gint num_srcs = 10;
  gint loop_secs = 2;

  if (argc > 1) {
    audio_sink = argv[1];
    if (argc > 2) {
      num_samples = atoi (argv[2]);
      num_samples = MAX (1, num_samples);
      if (argc > 3) {
        num_srcs = atoi (argv[3]);
        num_srcs = MAX (1, num_srcs);
        if (argc > 4) {
          loop_secs = atoi (argv[4]);
          loop_secs = MAX (1, loop_secs);
        }
      }
    }
  }
  printf
      ("Playing %d sources with buffers of %d samples on %s and looping for %d secs.\n",
      num_srcs, num_samples, audio_sink, loop_secs);

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
  mix = gst_element_factory_make ("adder", NULL);
  g_object_set (mix, "caps", caps, NULL);
  gst_caps_unref (caps);
  gst_bin_add (bin, mix);
  gst_element_link (mix, sink);

  for (i = 0; i < num_srcs; i++) {
    src = gst_element_factory_make ("audiotestsrc", NULL);
    g_object_set (src, "freq", freq + (i * freq), "samplesperbuffer",
        num_samples, NULL);
    gst_bin_add (bin, src);
    queue = gst_element_factory_make ("queue", NULL);
    g_object_set (queue, "silent", TRUE, "max-size-buffers", 1,
        "max-size-bytes", 0, "max-size-time", G_GUINT64_CONSTANT (0), NULL);
    gst_bin_add (bin, queue);
    gst_element_link_many (src, queue, mix, NULL);
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
