/* test pattern editor layout using gtkextra2
 *
 * gcc -Wall -g pated.c -o pated `pkg-config gtk+-2.0 gtkextra-2.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gtkextra/gtksheet.h>

GtkWidget *window = NULL;
GtkWidget *scrolled_window = NULL;
GtkWidget *sheet = NULL;

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
init ()
{
  guint i;
  GtkSheetRange range;
  GdkColor source_bg1, source_bg2;
  GdkColormap *colormap;
  gchar *name;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Pattern editor");
  gtk_widget_set_size_request (GTK_WIDGET (window), 600, 300);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (window), scrolled_window);

  sheet = gtk_sheet_new (64, 10, "pattern");
  gtk_container_add (GTK_CONTAINER (scrolled_window), sheet);

  // allocate some colors
  colormap = gtk_widget_get_colormap (window);
  source_bg1.red = (guint16) (1.0 * 65535);
  source_bg1.green = (guint16) (0.9 * 65535);
  source_bg1.blue = (guint16) (0.9 * 65535);
  gdk_colormap_alloc_color (colormap, &source_bg1, FALSE, TRUE);
  source_bg2.red = (guint16) (1.0 * 65535);
  source_bg2.green = (guint16) (0.8 * 65535);
  source_bg2.blue = (guint16) (0.8 * 65535);
  gdk_colormap_alloc_color (colormap, &source_bg2, FALSE, TRUE);

  // set column titles
  name = "Note";
  gtk_sheet_column_button_add_label (GTK_SHEET (sheet), 0, name);
  gtk_sheet_set_column_title (GTK_SHEET (sheet), 0, name);
  name = "Volume";
  gtk_sheet_column_button_add_label (GTK_SHEET (sheet), 1, name);
  gtk_sheet_set_column_title (GTK_SHEET (sheet), 1, name);

  // justify row labels
  for (i = 0; i < 64; i++) {
    gtk_sheet_row_button_justify (GTK_SHEET (sheet), i, GTK_JUSTIFY_RIGHT);
  }

  // colorize rows
  range.col0 = 0;
  range.coli = 9;
  for (i = 0; i < 64; i += 2) {
    range.row0 = i;
    range.rowi = i;
    gtk_sheet_range_set_background (GTK_SHEET (sheet), &range, &source_bg1);
    range.row0 = i + 1;
    range.rowi = i + 1;
    gtk_sheet_range_set_background (GTK_SHEET (sheet), &range, &source_bg2);
  }

  // global justification
  range.col0 = 0;
  range.coli = 9;
  range.row0 = 0;
  range.rowi = 63;
  gtk_sheet_range_set_justification (GTK_SHEET (sheet), &range,
      GTK_JUSTIFY_RIGHT);

  // add some content
  gtk_sheet_set_cell (GTK_SHEET (sheet), 0, 0, GTK_JUSTIFY_RIGHT, "C-3");
  gtk_sheet_set_cell (GTK_SHEET (sheet), 1, 0, GTK_JUSTIFY_RIGHT, "---");
  gtk_sheet_set_cell (GTK_SHEET (sheet), 2, 0, GTK_JUSTIFY_RIGHT, "D#3");

  // add a delimiter
  range.col0 = 1;
  range.coli = 1;
  range.row0 = 0;
  range.rowi = 63;
  gtk_sheet_range_set_border (GTK_SHEET (sheet), &range, GTK_SHEET_RIGHT_BORDER,
      1, GDK_LINE_SOLID);

  gtk_widget_show_all (window);
}

static void
done ()
{

}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  init ();
  gtk_main ();
  done ();

  return (0);
}
