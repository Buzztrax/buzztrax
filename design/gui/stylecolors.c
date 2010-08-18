/*
 * show all colors in a GtkStyle
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` stylecolors.c -o stylecolors
 *
 * GTK2_RC_FILES=$HOME/.themes/Clearlooks-DarkOrange/gtk-2.0/gtkrc ./stylecolors
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

static void destroy (GtkWidget *widget,gpointer data) {
  gtk_main_quit ();
}

static gboolean set_colors (gpointer data) {
  GtkTable *table = GTK_TABLE (data);
  GtkWidget *widget = GTK_WIDGET (table);
  guint i,j;
  GdkColor *colors[]={
    widget->style->fg,
    widget->style->bg,
    widget->style->light,
    widget->style->mid,
    widget->style->dark,
    widget->style->text,
    widget->style->text_aa,
    widget->style->base,
  };

  for(j=0;j<5;j++) {
    for(i=0;i<8;i++) {
      gtk_table_attach_defaults (table, 
        gtk_color_button_new_with_color (&colors[i][j]),
        j+1, j+2, i+1, i+2);
    }
  }
  gtk_widget_show_all (GTK_WIDGET (table));
  return FALSE;
}

gint main (gint argc, gchar **argv) {
  GtkWidget *window;
  GtkTable *table;

  gtk_init (&argc,&argv);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Style colors");
  g_signal_connect (G_OBJECT (window), "destroy",	G_CALLBACK (destroy), NULL);

  // add a table
  table = GTK_TABLE (gtk_table_new (/*colors*/8+1,/*states*/5+1, FALSE));
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (table));

  // colors
  gtk_table_attach_defaults (table, gtk_label_new ("fg")     , 0, 1, 1, 2);
  gtk_table_attach_defaults (table, gtk_label_new ("bg")     , 0, 1, 2, 3);
  gtk_table_attach_defaults (table, gtk_label_new ("light")  , 0, 1, 3, 4);
  gtk_table_attach_defaults (table, gtk_label_new ("mid")    , 0, 1, 4, 5);
  gtk_table_attach_defaults (table, gtk_label_new ("dark")   , 0, 1, 5, 6);
  gtk_table_attach_defaults (table, gtk_label_new ("text")   , 0, 1, 6, 7);
  gtk_table_attach_defaults (table, gtk_label_new ("text aa"), 0, 1, 7, 8);
  gtk_table_attach_defaults (table, gtk_label_new ("base")   , 0, 1, 8, 9);

  // states
  gtk_table_attach_defaults (table, gtk_label_new ("normal")      , 1, 2, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("active")      , 2, 3, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("prelight")    , 3, 4, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("selected")    , 4, 5, 0, 1);
  gtk_table_attach_defaults (table, gtk_label_new ("insensitive") , 5, 6, 0, 1);

  g_idle_add (set_colors, (gpointer)table);
 
  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
