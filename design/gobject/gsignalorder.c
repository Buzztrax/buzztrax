/* test glib signal calling order
 *
 * building:
 * gcc -Wall -g gsignalorder.c -o gsignalorder `pkg-config glib-2.0 gobject-2.0 --cflags --libs`
 *
 * running:
 * ./gsignalorder
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>

static void
on_notify_1 (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  puts ("notify 1");
}

static void
on_notify_2 (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  puts ("notify 2");
}

static void
on_notify_3 (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  puts ("notify 3");
}

int
main (int argc, char **argv)
{
  GObject *o;
  gint v = 0;

  if (argc > 1)
    v = atoi (argv[1]);

  g_type_init ();

  o = g_object_new (G_TYPE_OBJECT, NULL);

  switch (v) {
    case 0:
      g_signal_connect (o, "notify", (GCallback) on_notify_1, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_2, NULL);
      g_signal_connect_after (o, "notify", (GCallback) on_notify_3, NULL);
      break;
    case 1:
      g_signal_connect (o, "notify", (GCallback) on_notify_2, NULL);
      g_signal_connect_after (o, "notify", (GCallback) on_notify_3, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_1, NULL);
      break;
    case 2:
      g_signal_connect_after (o, "notify", (GCallback) on_notify_3, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_1, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_2, NULL);
      break;
    case 3:
      g_signal_connect_after (o, "notify", (GCallback) on_notify_3, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_2, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_1, NULL);
      break;
    case 4:
      g_signal_connect (o, "notify", (GCallback) on_notify_2, NULL);
      g_signal_connect (o, "notify", (GCallback) on_notify_1, NULL);
      g_signal_connect_after (o, "notify", (GCallback) on_notify_3, NULL);
      break;
  }

  g_signal_emit_by_name (o, "notify", NULL, NULL);
  g_object_unref (o);

  return 0;
}
