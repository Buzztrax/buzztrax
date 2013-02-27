/* open a window, let a lua script add some labels
 *
 * gcc -g gtk.c -o gtk `pkg-config gtk+-3.0 lua5.1-lgi --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

static GtkWidget *window = NULL;

static gint
bt_get_window (lua_State * L)
{
  puts ("get window");
  lua_pushlightuserdata (L, window);
  return 1;
}

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

gint
main (gint argc, gchar ** argv)
{
  lua_State *L;

  /* initialize Lua and load base libraries */
  L = lua_open ();
  luaL_openlibs (L);
  /* register our function */
  lua_register (L, "bt_get_window", bt_get_window);

  /* init gtk */
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Hello lua");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  /* run the script */
  luaL_dofile (L, "gtk.lua");

  /* run the main loop */
  gtk_widget_show_all (window);
  gtk_main ();

  /* clean up */
  lua_close (L);
  return (0);
}
