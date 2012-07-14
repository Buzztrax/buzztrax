/* test gstreamer refcounts
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` ref.c -o ref
 *
 * ./ref
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

static void
print_details (const GstElement * elem, const gchar * action)
{
  GstObject *parent = GST_OBJECT_PARENT (elem);

  printf ("%10s, ref_count=%d, floating=%d, parent=%s\n",
      action,
      G_OBJECT (elem)->ref_count,
      GST_OBJECT_FLAG_IS_SET (elem, GST_OBJECT_FLOATING),
      (parent ? GST_OBJECT_NAME (parent) : ""));
}

int
main (int argc, char **argv)
{
  GstElement *bin, *src;

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);

  /* create */
  bin = gst_pipeline_new ("song");
  src = gst_element_factory_make ("fakesrc", NULL);
  print_details (src, "created");

  /* cycle one */
  gst_bin_add (GST_BIN (bin), src);
  print_details (src, "added");

  gst_object_ref (GST_OBJECT (src));
  print_details (src, "ref'ed");

  gst_bin_remove (GST_BIN (bin), src);
  GST_OBJECT_FLAG_SET (src, GST_OBJECT_FLOATING);
  print_details (src, "removed");


  /* cycle two */
  gst_bin_add (GST_BIN (bin), src);
  print_details (src, "added");

  gst_object_ref (GST_OBJECT (src));
  print_details (src, "ref'ed");

  gst_bin_remove (GST_BIN (bin), src);
  GST_OBJECT_FLAG_SET (src, GST_OBJECT_FLOATING);
  print_details (src, "removed");

  /* release */
  gst_object_unref (src);
  gst_object_unref (bin);

  exit (0);
}
