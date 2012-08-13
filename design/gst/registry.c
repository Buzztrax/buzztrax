/* test for elements in registry
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` registry.c -o registry
 * ./registry
 */

#include <stdio.h>
#include <gst/gst.h>

gboolean
bt_plugin_feature_check (const gchar ** names)
{
  GstRegistry *registry;
  GstPluginFeature *feature;
  guint i = 0;
  gboolean res = TRUE;

  registry = gst_registry_get_default ();

  do {
    if ((feature = gst_registry_lookup_feature (registry, names[i]))) {
      printf ("Feature '%s' is available\n", names[i]);
      gst_object_unref (feature);
    } else {
      printf ("Feature '%s' is missing\n", names[i]);
      res = FALSE;
    }
  } while (names[++i]);
  gst_object_unref (registry);
  return (res);
}

int
main (int argc, char **argv)
{
  const gchar *elements[] = {
    "volume",
    "level",
    /*"peng", */
    "spectrum",
    NULL
  };

  /* init gstreamer */
  gst_init (&argc, &argv);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING);

  if (bt_plugin_feature_check (elements)) {
    puts ("All plugins available");
  } else {
    puts ("Some plugins are missing");
  }
  exit (0);
}
