/* testing dealing with type pun warnings
 *
 * building:
 * gcc -Wall -g `pkg-config glib-2.0 gobject-2.0 --cflags --libs` typepun.c -o typepun
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>

gint main(gint argc, gchar *argv[]) {
  GObject *o1, *o2;
  
  g_type_init();
  
  o1 = g_object_new (G_TYPE_OBJECT, NULL);
  g_object_add_weak_pointer(o1, (gpointer *)&o1);
  g_object_unref(o1);
  g_assert(o1 == NULL);
  
  // also test g_atomic_pointer_compare_and_exchange()
  // seems that glib uses G_GNUC_MAY_ALIAS nowadays
  
  return 0;
}
