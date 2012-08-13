/* try to set initial focus
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` initfocus.c -o initfocus
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
on_window_show (GtkWidget * widget, gpointer user_data)
{
  puts ("window::show");
  gtk_widget_grab_focus (user_data);
}

static void
init ()
{
  GtkWidget *window = NULL;
  GtkButton *button;
  GtkWidget *vbox;
  GtkWidget *tb, *ti;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Toolbar with accelerator");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  // toolbar
  tb = gtk_toolbar_new ();
  gtk_container_add (GTK_CONTAINER (vbox), tb);

  ti = GTK_WIDGET (gtk_toggle_tool_button_new_from_stock
      (GTK_STOCK_MEDIA_PLAY));
  gtk_toolbar_insert (GTK_TOOLBAR (tb), GTK_TOOL_ITEM (ti), -1);

  ti = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_STOP));
  gtk_toolbar_insert (GTK_TOOLBAR (tb), GTK_TOOL_ITEM (ti), -1);

  // dialog area
  button = gtk_button_new_with_label ("click");
  gtk_container_add (GTK_CONTAINER (vbox), gtk_label_new ("some test message"));
  gtk_container_add (GTK_CONTAINER (vbox), button);

  // set focus on the button
  g_signal_connect ((gpointer) window, "show", G_CALLBACK (on_window_show),
      button);

  puts ("run mainloop");
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
