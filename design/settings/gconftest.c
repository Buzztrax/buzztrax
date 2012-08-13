/* test gobject construction/destruction order
 *
 * building:
 * gcc -Wall -g `pkg-config glib-2.0 gconf-2.0 --cflags --libs` gconftest.c -o gconftest
 *
 * running:
 * GCONF_DEBUG_TRACE_CLIENT=1 ./gconftest
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <gconf/gconf-client.h>

#define BT_GCONF_PATH_GNOME "/desktop/gnome/interface"

gboolean
test (void)
{
  GConfClient *client;

  client = gconf_client_get_default ();
  if (client) {
    gchar *value;

    // init
    gconf_client_set_error_handling (client, GCONF_CLIENT_HANDLE_UNRETURNED);
    gconf_client_add_dir (client, BT_GCONF_PATH_GNOME,
        GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

    // read a value
    value =
        gconf_client_get_string (client, BT_GCONF_PATH_GNOME "/toolbar_style",
        NULL);
    printf (BT_GCONF_PATH_GNOME "/toolbar_style = %s\n", value);

    // cleanup
    gconf_client_remove_dir (client, BT_GCONF_PATH_GNOME, NULL);
    g_object_unref (client);
    return TRUE;
  }
  return FALSE;
}

gint
main (gint argc, gchar ** argv)
{
  guint i;

  g_type_init ();

  for (i = 0; i < 10; i++) {
    test ();
  }

  return 0;
}
