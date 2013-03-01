/* open a window, let a js script add some labels
 *
 * links:
 * https://www.berrange.com/posts/2010/01/10/using-gobject-introspection-gjs-to-provide-a-javascript-plugin-engine/
 * gcc -g gtk.c -o gtk `pkg-config gtk+-3.0 gjs-1.0 gjs-internals-1.0 --cflags --libs`
 *
> ./gtk 
Hello GObject
get window
    JS ERROR: !!!   WARNING: 'var window is read-only'
    JS ERROR: !!!   WARNING: file 'gtk.js' line 5 exception 0 number 19
    JS ERROR: !!!   Exception was: TypeError: window.add is not a function
    JS ERROR: !!!     lineNumber = '11'
    JS ERROR: !!!     fileName = '"gtk.js"'
    JS ERROR: !!!     stack = '"@gtk.js:11
"'
    JS ERROR: !!!     message = '"window.add is not a function"'

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>

#include <gjs/gjs-module.h>
#include <gi/object.h>

static GtkWidget *window = NULL;

static JSBool
bt_get_window (JSContext * context, guint argc, jsval * vp)
{
  //jsval *argv = JS_ARGV (context, vp);

  puts ("get window");
  JSObject *obj = gjs_object_from_g_object (context, (GObject *) window);
  jsval retval = OBJECT_TO_JSVAL (obj);
  JS_SET_RVAL (context, vp, retval);
  return JS_TRUE;
}

static JSBool
bt_get_five (JSContext * context, guint argc, jsval * vp)
{
  //jsval *argv = JS_ARGV (context, vp);

  puts ("get five");
  jsval retval = INT_TO_JSVAL (5);
  JS_SET_RVAL (context, vp, retval);
  return JS_TRUE;
}

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

gint
main (gint argc, gchar ** argv)
{
  GjsContext *gjs_context;
  JSContext *context;
  JSObject *global;
  gint res;

  /* Initialize the GJs engine */
  g_type_init ();
  gjs_context = gjs_context_new ();
  context = (JSContext *) gjs_context_get_native_context (gjs_context);
  global = gjs_get_import_global (context);
  JS_DefineFunction (context, global, "bt_get_window",
      (JSNative) bt_get_window, 0, GJS_MODULE_PROP_FLAGS);
  JS_DefineFunction (context, global, "bt_get_five",
      (JSNative) bt_get_five, 0, GJS_MODULE_PROP_FLAGS);

  /* init gtk */
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Hello gjs");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  /* Create and run the script */
  gjs_context_eval_file (gjs_context, "gtk.js", &res, NULL);

  /* run the main loop */
  gtk_widget_show_all (window);
  gtk_main ();

  /* clean up */
  g_object_unref (gjs_context);
  return 0;
}
