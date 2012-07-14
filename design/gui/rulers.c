/*
 * demo app for rulers
 * 
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` rulers.c -o rulers
 *
 * This demo will be used to improve and test the ruler widget. See:
 * http://live.gnome.org/GTK%2B/GtkRuler
 *
 * gtk_ruler_set_range:
 * improve api-docs, position and max_size parameters are not understandable
 * - position is overridden by motion-notify anyway right now
 *   meant for manual ruler-mark placement
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

static GtkWidget *rel_h_ruler;
static GtkWidget *rel_v_ruler;
static GtkWidget *info_label;

#define MAX_HEIGHT 30.0

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
on_size_changed (GtkWidget * widget, GtkAllocation * allocation,
    gpointer user_data)
{
  static gint w = 0, h = 0;

  if (w != allocation->width) {
    gtk_ruler_set_range (GTK_RULER (rel_h_ruler), 0.0, allocation->width, 0.0,
        MAX_HEIGHT);
  }
  if (h != allocation->height) {
    gtk_ruler_set_range (GTK_RULER (rel_v_ruler), 0.0, allocation->height, 0.0,
        MAX_HEIGHT);
  }
  if (w != allocation->width || h != allocation->height) {
    gchar info[100];

    sprintf (info, "%d x %d", allocation->width, allocation->height);
    gtk_label_set_text (GTK_LABEL (info_label), info);
    w = allocation->width;
    h = allocation->height;
  }
}

gint
main (gint argc, gchar ** argv)
{
  GtkWidget *window;
  GtkTable *table;
  GtkWidget *label;
  GtkWidget *ruler;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Rulers");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  // add a table
  table = GTK_TABLE (gtk_table_new (5, 5, FALSE));
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (table));

  // add labels
  label = gtk_label_new ("relative");
  gtk_table_attach (table, label, 2, 3, 0, 1, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 1, 1);
  label = gtk_label_new ("relative");
  gtk_label_set_angle (GTK_LABEL (label), 90.0);
  gtk_table_attach (table, label, 0, 1, 2, 3, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 1, 1);

  label = gtk_label_new ("absolute");
  gtk_table_attach (table, label, 2, 3, 4, 5, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 1, 1);
  label = gtk_label_new ("absolute");
  gtk_label_set_angle (GTK_LABEL (label), 270.0);
  gtk_table_attach (table, label, 4, 5, 2, 3, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 1, 1);

  label = info_label = gtk_label_new ("");
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_widget_set_size_request (label, 150, 150);
  gtk_table_attach (table, label, 2, 3, 2, 3, GTK_EXPAND | GTK_FILL,
      GTK_EXPAND | GTK_FILL, 0, 0);

  // add rulers
  ruler = gtk_hruler_new ();
  gtk_ruler_set_range (GTK_RULER (ruler), 0.0, 100.0, 0.0, MAX_HEIGHT);
  gtk_table_attach (table, ruler, 2, 3, 1, 2, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 0, 0);
  ruler = gtk_vruler_new ();
  gtk_ruler_set_range (GTK_RULER (ruler), 0.0, 100.0, 0.0, MAX_HEIGHT);
  gtk_table_attach (table, ruler, 1, 2, 2, 3, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 0, 0);

  ruler = rel_h_ruler = gtk_hruler_new ();
  gtk_table_attach (table, ruler, 2, 3, 3, 4, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 0, 0);
  ruler = rel_v_ruler = gtk_vruler_new ();
  gtk_table_attach (table, ruler, 3, 4, 2, 3, GTK_SHRINK | GTK_FILL,
      GTK_SHRINK | GTK_FILL, 0, 0);

  // handle resizes
  g_signal_connect (G_OBJECT (label), "size-allocate",
      G_CALLBACK (on_size_changed), NULL);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
