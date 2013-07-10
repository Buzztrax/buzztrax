/*
 * show all colors in a GtkStyleContext
 *
 * gcc -Wall -g stylecolors3.c -o stylecolors3 `pkg-config gtk+-3.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>

static gboolean
set_colors (gpointer data)
{
  GtkTable *table = GTK_TABLE (data);
  GtkStyleContext *ctx = gtk_widget_get_style_context (GTK_WIDGET (table));
  GdkRGBA color;
  guint i = 0, j;
  GtkStateFlags flags[] = {
    GTK_STATE_FLAG_NORMAL,
    GTK_STATE_FLAG_ACTIVE,
    GTK_STATE_FLAG_PRELIGHT,
    GTK_STATE_FLAG_SELECTED,
    GTK_STATE_FLAG_INSENSITIVE,
    GTK_STATE_FLAG_INCONSISTENT,
    GTK_STATE_FLAG_FOCUSED,
    GTK_STATE_FLAG_BACKDROP
  };

  for (j = 0; j < 8; j++) {
    gtk_style_context_get_color (ctx, flags[j], &color);
    gtk_table_attach_defaults (table,
        gtk_color_button_new_with_rgba (&color), j + 1, j + 2, i + 1, i + 2);
  }
  i++;
  for (j = 0; j < 8; j++) {
    gtk_style_context_get_background_color (ctx, flags[j], &color);
    gtk_table_attach_defaults (table,
        gtk_color_button_new_with_rgba (&color), j + 1, j + 2, i + 1, i + 2);
  }
  i++;
  for (j = 0; j < 8; j++) {
    gtk_style_context_get_border_color (ctx, flags[j], &color);
    gtk_table_attach_defaults (table,
        gtk_color_button_new_with_rgba (&color), j + 1, j + 2, i + 1, i + 2);
  }

  gtk_widget_show_all (GTK_WIDGET (table));
  return FALSE;
}

gint
main (gint argc, gchar ** argv)
{
  GtkWidget *window;
  GtkTable *table;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "StyleContext colors");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  // add a table
  table =
      GTK_TABLE (gtk_table_new ( /*colors */ 3 + 1, /*states */ 8 + 1, FALSE));
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (table));

  // colors
  gtk_table_attach_defaults (table, gtk_label_new ("fg"), 0, 1, 1, 2);
  gtk_table_attach_defaults (table, gtk_label_new ("bg"), 0, 1, 2, 3);
  gtk_table_attach_defaults (table, gtk_label_new ("border"), 0, 1, 3, 4);

  // states
  gtk_table_attach_defaults (table, gtk_label_new ("normal"), 1, 2, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("active"), 2, 3, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("prelight"), 3, 4, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("selected"), 4, 5, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("insensitive"), 5, 6, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("inconsitent"), 6, 7, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("focused"), 7, 8, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("backdrop"), 8, 9, 0, 1);

  g_idle_add (set_colors, (gpointer) table);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
