/* test mute, solo, bypass stuff in gst
 *
 * gcc -Wall -g `pkg-config gstreamer-0.9 --cflags --libs` states.c -o states
 */

#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
#define SRC_NAME "sinesrc"


int
main (int argc, char **argv)
{
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink, *src;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");

  /* make a sink */
  if (!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf (stderr, "Can't create element \"" SINK_NAME "\"\n");
    exit (-1);
  }

  /* make a source */
  if (!(src = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf (stderr, "Can't create element \"" SRC_NAME "\"\n");
    exit (-1);
  }

  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);

  /* link elements */
  if (!gst_element_link (src, sink)) {
    fprintf (stderr, "Can't link elements\n");
    exit (-1);
  }
  puts ("linking again");
  if (!gst_element_link (src, sink)) {
    fprintf (stderr, "Can't link elements\n");
  }

  /* we don't need a reference to these objects anymore */
  g_object_unref (GST_OBJECT (bin));

  exit (0);
}
