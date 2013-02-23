/* open a window, let a js script add some labels
 *
 * gcc -g gtk.c -o gtk `pkg-config gtk+-3.0 seed --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#include <seed.h>


static GtkWidget *window = NULL;

/* Our function, with the signature of SeedFunctionCallback(); say hello! */
static SeedValue
bt_get_window (SeedContext ctx, SeedObject function, SeedObject this_object,
    gsize argument_count, const SeedValue arguments[],
    SeedException * exception)
{
  puts ("get window");
  return seed_value_from_object (ctx, (GObject *) window, NULL);
}

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

gint
main (gint argc, gchar ** argv)
{
  SeedEngine *eng;
  SeedScript *script;

  /* Initialize the Seed engine */
  eng = seed_init (&argc, &argv);
  seed_create_function (eng->context, "bt_get_window",
      (SeedFunctionCallback) bt_get_window, eng->global);

  /* init gtk */
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Hello seed");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  /* Create and run the script */
  if (script = seed_script_new_from_file (eng->context, "gtk.js")) {
    seed_evaluate (eng->context, script, 0);
    g_free (script);
  }

  /* run the main loop */
  gtk_widget_show_all (window);
  gtk_main ();

  /* clean up */
  return 0;
}
