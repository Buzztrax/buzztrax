/* test level meter drawing
 *
 * gcc -Wall -g -I../../src/ui/edit/ levelmeter.c ../../src/ui/edit/gtkvumeter.c -o levelmeter `pkg-config gtk+-2.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>
#include "gtkvumeter.h"


static GtkWidget *window = NULL;


static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

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

static void
init ()
{
  GtkWidget *vumeter;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Levelmeter");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  vumeter = gtk_vumeter_new (FALSE);
  gtk_vumeter_set_min_max (GTK_VUMETER (vumeter), 0, 100);
  gtk_vumeter_set_scale (GTK_VUMETER (vumeter), GTK_VUMETER_SCALE_LINEAR);

  gtk_container_set_border_width (GTK_CONTAINER (window), 6);
  gtk_container_add (GTK_CONTAINER (window), vumeter);
  gtk_widget_show_all (window);

  g_timeout_add_seconds (1, change_level, vumeter);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  init ();
  gtk_main ();

  return (0);
}
