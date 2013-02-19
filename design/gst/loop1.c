/* test handling looping in gstreamer
 *
 * gcc -g loop1.c -o loop1 `pkg-config gstreamer-1.0 gstreamer-controller-1.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>

#include <gst/gst.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstdirectcontrolbinding.h>

#define SINK_NAME "alsasink"
#define SRC_NAME "audiotestsrc"

#define GST_CAT_DEFAULT gst_test_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GMainLoop *main_loop = NULL;
static GstEvent *play_seek_event = NULL;
static GstEvent *loop_seek_event = NULL;
static gint num_loops = 10;
static gboolean flushing_loops = FALSE;

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    puts (sstr);
    g_free (sstr);
  } else {
    puts ("no message details");
  }

  g_main_loop_quit (main_loop);
}

static void
state_changed (const GstBus * const bus, GstMessage * message, GstElement * bin)
{
  if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
    GstStateChangeReturn res;
    GstState oldstate, newstate, pending;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        // seek to start time
        GST_INFO
            ("initial seek ===========================================================");
        if (!(gst_element_send_event (bin, gst_event_copy (play_seek_event)))) {
          fprintf (stderr, "element failed to handle seek event");
          g_main_loop_quit (main_loop);
        }
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
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        GST_INFO
            ("playback started =======================================================");
        break;
      default:
        break;
    }
  }
}

static void
segment_done (const GstBus * const bus, const GstMessage * const message,
    GstElement * bin)
{
  static gint loop = 0;
  GstEvent *event =
      gst_event_copy (flushing_loops ? play_seek_event : loop_seek_event);

  GST_INFO
      ("loop playback (%2d) =====================================================",
      loop);
  gst_event_set_seqnum (event, gst_util_seqnum_next ());
  if (!(gst_element_send_event (bin, event))) {
    fprintf (stderr, "element failed to handle continuing play seek event\n");
    g_main_loop_quit (main_loop);
  } else {
    if (loop == num_loops) {
      g_main_loop_quit (main_loop);
    }
    loop++;
  }
}

static void
double_ctrl_value_set (GstControlSource * cs, GstClockTime t, gdouble v)
{
  gst_timed_value_control_source_set ((GstTimedValueControlSource *) cs,
      t * GST_MSECOND, v);
}


int
main (int argc, char **argv)
{
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *src1, *src2, *mix, *sink;
  GstControlSource *cs;
  GstBus *bus;
  GstStateChangeReturn res;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "loop", 0, "loop test");

  if (argc > 1) {
    num_loops = atoi (argv[1]);
    if (argc > 2) {
      flushing_loops = (atoi (argv[2]) != 0);
    }
  }

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK (segment_done),
      bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (state_changed),
      bin);
  gst_object_unref (bus);

  main_loop = g_main_loop_new (NULL, FALSE);

  /* make elements and add them to the bin */
  if (!(src1 = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src1, "wave", 2, "freq", 110.0, NULL);
  if (!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src2, "wave", 3, "freq", 440.0, NULL);
  if (!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf (stderr, "Can't create element \"adder\"\n");
    exit (-1);
  }
  if (!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }
  gst_bin_add_many (GST_BIN (bin), src1, src2, mix, sink, NULL);

  /* link elements */
  if (!gst_element_link_many (src1, mix, sink, NULL)) {
    fprintf (stderr, "Can't link part1\n");
    exit (-1);
  }
  if (!gst_element_link_many (src2, mix, NULL)) {
    fprintf (stderr, "Can't link part2\n");
    exit (-1);
  }

  /* setup controller */
  cs = gst_interpolation_control_source_new ();
  g_object_set (cs, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  gst_object_add_control_binding (GST_OBJECT_CAST (src1),
      gst_direct_control_binding_new (GST_OBJECT_CAST (src1), "volume", cs));
  /* set control values */
  double_ctrl_value_set (cs, 0, 1.0);
  double_ctrl_value_set (cs, 1000, 0.0);

  /* add a controller to the source */
  cs = gst_interpolation_control_source_new ();
  g_object_set (cs, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  gst_object_add_control_binding (GST_OBJECT_CAST (src2),
      gst_direct_control_binding_new (GST_OBJECT_CAST (src2), "volume", cs));
  /* set control values */
  double_ctrl_value_set (cs, 0, 0.0);
  double_ctrl_value_set (cs, 1000, 1.0);

  /* initial seek event (without flush) */
  play_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) GST_SECOND);
  /* loop seek event (without flush) */
  loop_seek_event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime) 0,
      GST_SEEK_TYPE_SET, (GstClockTime) GST_SECOND);

  /* prepare playing */
  GST_INFO
      ("prepare playing ========================================================");
  res = gst_element_set_state (bin, GST_STATE_PAUSED);
  if (res == GST_STATE_CHANGE_FAILURE) {
    fprintf (stderr, "Can't go to paused\n");
    exit (-1);
  } else if (res == GST_STATE_CHANGE_ASYNC) {
    GST_INFO ("->PAUSED needs async wait");
  }
  g_main_loop_run (main_loop);

  /* stop the pipeline */
  GST_INFO
      ("exiting ================================================================");
  gst_element_set_state (bin, GST_STATE_NULL);

  gst_event_unref (play_seek_event);
  gst_event_unref (loop_seek_event);
  gst_object_unref (GST_OBJECT (bin));
  g_main_loop_unref (main_loop);
  exit (0);
}
