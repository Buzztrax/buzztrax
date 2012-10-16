/* test handling disconnected elements in gstreamer
 *
 * gcc -g connection1.c -o connection1 `pkg-config gstreamer-0.10 --cflags --libs`
 */

#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
//#define SINK_NAME "esdsink"
#define SRC_NAME "audiotestsrc"
#define EFFECT_NAME "volume"

#define TEST_UNCONNECTED_SRC 0
#define TEST_UNCONNECTED_EFFECT 1
#define TEST_UNCONNECTED_SINK 2

static GMainLoop *main_loop = NULL;

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

static gboolean
test_timeout (gpointer data)
{
  puts ("timeout occured");
  g_main_loop_quit (main_loop);
  return (FALSE);
}

int
main (int argc, char **argv)
{
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink, *effect, *src1, *src2;
  GstBus *bus;
  int test = TEST_UNCONNECTED_SRC;

  if (argc > 1) {
    test = atoi (argv[1]);
  }

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK (message_received), bin);
  gst_object_unref (G_OBJECT (bus));

  main_loop = g_main_loop_new (NULL, FALSE);

  /* make elements */
  if (!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }
  if (!(src1 = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src1, "num-buffers", 25, NULL);
  if (!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }
  g_object_set (src2, "num-buffers", 25, NULL);
  if (!(effect = gst_element_factory_make (EFFECT_NAME, "effect"))) {
    fprintf (stderr, "Can't create element \"" EFFECT_NAME "\"\n");
    exit (-1);
  }

  switch (test) {
    case TEST_UNCONNECTED_SRC:
      puts ("unconnected src ========================================================\n");
      /* add objects to the main bin */
      gst_bin_add_many (GST_BIN (bin), src1, src2, sink, NULL);

      /* link elements */
      if (!gst_element_link_many (src1, sink, NULL)) {
        fprintf (stderr, "Can't link elements\n");
        exit (-1);
      }

      /* start playing */
      if (gst_element_set_state (bin,
              GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        fprintf (stderr, "Can't start playing\n");
        exit (-1);
      }
      break;
    case TEST_UNCONNECTED_EFFECT:
      puts ("unconnected effect =====================================================\n");
      /* add objects to the main bin */
      gst_bin_add_many (GST_BIN (bin), src1, effect, sink, NULL);

      /* link elements */
      if (!gst_element_link_many (src1, sink, NULL)) {
        fprintf (stderr, "Can't link elements\n");
        exit (-1);
      }

      /* start playing */
      if (gst_element_set_state (bin,
              GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        fprintf (stderr, "Can't start playing\n");
        exit (-1);
      }
      break;
    case TEST_UNCONNECTED_SINK:
      puts ("unconnected sink =====================================================\n");
      /* add objects to the main bin */
      gst_bin_add_many (GST_BIN (bin), sink, NULL);

      /* start playing */
      if (gst_element_set_state (bin,
              GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        fprintf (stderr, "Can't start playing\n");
        exit (-1);
      }
      break;
  }
  g_timeout_add (2 * 1000, test_timeout, NULL);
  g_main_loop_run (main_loop);

  /* stop the pipeline */
  puts ("playing done ===========================================================\n");
  gst_element_set_state (bin, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bin));
  g_main_loop_unref (main_loop);

  exit (0);
}
