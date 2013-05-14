/* port gtkvumeter to gtk3
 *
 * gcc -Wall -g -I. vumeter.c gtkvumeter.c -o vumeter `pkg-config gtk+-3.0 --cflags --libs`
 */

#include <gtk/gtk.h>
#include <glib.h>
#include "gtkvumeter.h"

static gboolean
change_level (gpointer data)
{
  static gint level = 0, inc = 10;
  GtkVUMeter *vumeter = GTK_VUMETER (data);

  gtk_vumeter_set_levels (vumeter, level, level);
  level += inc;
  if (level > 100) {
    level = 100;
    inc = -inc;
  } else if (level < 0) {
    level = 0;
    inc = -inc;
  }
  return TRUE;
}

gint
main (gint argc, gchar ** argv)
{
  GtkWidget *window, *vumeter;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Levelmeter");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 6);

  vumeter = gtk_vumeter_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_vumeter_set_min_max (GTK_VUMETER (vumeter), 0, 100);
  gtk_vumeter_set_scale (GTK_VUMETER (vumeter), GTK_VUMETER_SCALE_LINEAR);
  gtk_container_add (GTK_CONTAINER (window), vumeter);

  gtk_widget_show_all (window);
  g_timeout_add_seconds (1, change_level, vumeter);
  gtk_main ();

  return 0;
}
