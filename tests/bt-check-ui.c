/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * ui testing helpers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION::btcheckui:
 * @short_description: ui testing helpers
 *
 * This module contains utilities for bringing up the default display on Xvfb
 * (if available). One can use the environment variable BT_CHECK_NO_XVFB=1 for
 * bypassing Xvfb.
 *
 * The module also contains tools to make screenshots and inject events.
 */

#include <math.h>
#include <stdlib.h>
//-- glib
#include <glib/gstdio.h>
//-- gdk
#include <gdk/gdkx.h>
//-- gstreamer
#include <gst/gst.h>

#include "bt-check.h"
#include "bt-check-ui.h"

// gtk+ gui tests

/* it could also be done in a makefile
 * http://git.imendio.com/?p=timj/gtk%2B-testing.git;a=blob;f=gtk/tests/Makefile.am;hb=72227f8f808a0343cb420f09ca480fc1847b6601
 *
 * for testing:
 * Xnest :2 -ac &
 * metacity --display=:2 --sm-disable &
 * ./bt-edit --display=:2 &
 */

#ifdef XVFB_PATH
static GPid server_pid;
static GdkDisplayManager *display_manager = NULL;
static GdkDisplay *default_display = NULL, *test_display = NULL;
static volatile gboolean wait_for_server;
static gchar display_name[3];
static gint display_number = -1;

static void
__test_server_watch (GPid pid, gint status, gpointer data)
{
  if (status == 0) {
    GST_INFO ("test x server %d process finished okay", pid);
  } else {
    GST_WARNING ("test x server %d process finished with error %d", pid,
        status);
  }
  wait_for_server = FALSE;
  g_spawn_close_pid (pid);
  server_pid = 0;
}
#endif

void
check_setup_test_server (void)
{
#ifdef XVFB_PATH
  //gulong flags=G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL;
  gulong flags = G_SPAWN_SEARCH_PATH;
  GError *error = NULL;
  gchar display_file[18];
  gchar lock_file[14];
  // we can also use Xnest, but the without "-screen"
  gchar *server_argv[] = {
    "Xvfb",
    //"Xnest",
    ":9",
    "-ac",
    "-nolisten", "tcp",
#ifdef XFONT_PATH
    "-fp", XFONT_PATH,          /*"/usr/X11R6/lib/X11/fonts/misc" */
#endif
    "-noreset",
    /*"-terminate", */
    /* 32 bit does not work */
    "-screen", "0", "1024x786x24",
    "+extension", "RANDR",
    // causes issue like reported under
    // https://bugzilla.gnome.org/show_bug.cgi?id=749752
    //"-render",                  /*"color", */
    NULL
  };
  gboolean found = FALSE, launched = FALSE, trying = TRUE;

  server_pid = 0;

  // allow running the test without Xvfb even though we have it.
  if (g_getenv ("BT_CHECK_NO_XVFB"))
    return;

  display_number = 0;
  // try display ids starting with '0'
  while (trying) {
    wait_for_server = TRUE;
    g_sprintf (display_name, ":%1d", display_number);
    g_sprintf (display_file, "/tmp/.X11-unix/X%1d", display_number);
    g_sprintf (lock_file, "/tmp/.X%1d-lock", display_number);

    // if we have a lock file, check if there is an alive process
    if (g_file_test (lock_file, G_FILE_TEST_EXISTS)) {
      FILE *pid_file;
      gchar pid_str[20];
      guint pid;
      gchar proc_file[15];

      // read pid, normal X11 is owned by root
      if ((pid_file = fopen (lock_file, "rt"))) {
        gchar *pid_str_res = fgets (pid_str, 20, pid_file);
        fclose (pid_file);

        if (pid_str_res) {
          pid = atol (pid_str);
          g_sprintf (proc_file, "/proc/%d", pid);
          // check proc entry
          if (!g_file_test (proc_file, G_FILE_TEST_EXISTS)) {
            // try to remove file and reuse display number
            if (!g_unlink (lock_file) && !g_unlink (display_file)) {
              found = TRUE;
            }
          }
        }
      }
    } else {
      found = TRUE;
    }

    // this display is not yet in use
    if (found && !g_file_test (display_file, G_FILE_TEST_EXISTS)) {
      // create the testing server
      server_argv[1] = display_name;
      if (!(g_spawn_async (NULL, server_argv, NULL, flags, NULL, NULL,
                  &server_pid, &error))) {
        GST_ERROR ("error creating virtual x-server : \"%s\"", error->message);
        g_error_free (error);
      } else {
        g_child_watch_add (server_pid, __test_server_watch, NULL);

        while (wait_for_server) {
          // try also waiting for /tmp/X%1d-lock" files
          if (g_file_test (display_file, G_FILE_TEST_EXISTS)) {
            wait_for_server = trying = FALSE;
            launched = TRUE;
          } else {
            g_usleep (G_USEC_PER_SEC);
          }
        }
      }
    }
    if (!launched) {
      display_number++;
      // stop after trying the first ten displays
      if (display_number == 10)
        trying = FALSE;
    }
  }
  if (!launched) {
    display_number = -1;
    GST_WARNING ("no free display number found");
  } else {
    // we still get a dozen of
    //   Xlib:  extension "RANDR" missing on display ...
    // this is a gdk bug:
    //   https://bugzilla.gnome.org/show_bug.cgi?id=645856
    //   and it seems to be fixed in Jan.2011

    //printf("####### Server started  \"%s\" is up (pid=%d)\n",display_name,server_pid);
    g_setenv ("DISPLAY", display_name, TRUE);
    GST_INFO ("test server \"%s\" is up (pid=%d)", display_name, server_pid);
    /* a window manager is not that useful
       gchar *wm_argv[]={"metacity", "--sm-disable", NULL };
       if(!(g_spawn_async(NULL,wm_argv,NULL,flags,NULL,NULL,NULL,&error))) {
       GST_WARNING("error running window manager : \"%s\"", error->message);
       g_error_free(error);
       }
     */
    /* this is not working
     ** (gnome-settings-daemon:17715): WARNING **: Failed to acquire org.gnome.SettingsDaemon
     ** (gnome-settings-daemon:17715): WARNING **: Could not acquire name
     gchar *gsd_argv[]={"/usr/lib/gnome-settings-daemon/gnome-settings-daemon", NULL };
     if(!(g_spawn_async(NULL,gsd_argv,NULL,flags,NULL,NULL,NULL,&error))) {
     GST_WARNING("error gnome-settings daemon : \"%s\"", error->message);
     g_error_free(error);
     }
     */
  }
#endif
}

void
check_setup_test_display (void)
{
#ifdef XVFB_PATH
  if (display_number > -1) {
    // activate the display for use with gtk
    if ((display_manager = gdk_display_manager_get ())) {
      if ((test_display = gdk_display_open (display_name))) {
        gdk_display_manager_set_default_display (display_manager, test_display);
        GST_INFO ("display %p,\"%s\" is active", test_display,
            gdk_display_get_name (test_display));
      } else {
        GST_WARNING ("failed to open display: \"%s\"", display_name);
      }
    } else {
      GST_WARNING ("can't get display-manager");
    }
  }
#endif
}

void
check_shutdown_test_display (void)
{
#ifdef XVFB_PATH
  if (test_display) {
    wait_for_server = TRUE;

    g_assert (GDK_IS_DISPLAY_MANAGER (display_manager));
    g_assert (GDK_IS_DISPLAY (test_display));
    g_assert (GDK_IS_DISPLAY (default_display));

    GST_INFO ("trying to shut down test display on server %d", server_pid);
    // restore default and close our display
    GST_DEBUG
        ("display_manager=%p, test_display=%p,\"%s\" default_display=%p,\"%s\"",
        display_manager, test_display, gdk_display_get_name (test_display),
        default_display, gdk_display_get_name (default_display));
    gdk_display_manager_set_default_display (display_manager, default_display);
    GST_INFO ("display has been restored");
    // TODO(ensonic): here it hangs, hmm not anymore
    //gdk_display_close(test_display);
    /* gdk_display_close() does basically the following (which still hangs):
       //g_object_run_dispose (G_OBJECT (test_display));
       GST_INFO("test_display has been disposed");
       //g_object_unref (test_display);
     */
    GST_INFO ("display has been closed");
    test_display = NULL;
  } else {
    GST_WARNING ("no test display");
  }
#endif
}

void
check_shutdown_test_server (void)
{
#ifdef XVFB_PATH
  if (server_pid) {
    guint wait_count = 5;
    wait_for_server = TRUE;
    GST_INFO ("shuting down test server");

    // kill the testing server - TODO(ensonic): try other signals (SIGQUIT, SIGTERM).
    kill (server_pid, SIGINT);
    // wait for the server to finish (use waitpid() here ?)
    while (wait_for_server && wait_count) {
      g_usleep (G_USEC_PER_SEC);
      wait_count--;
    }
    server_pid = 0;
    GST_INFO ("test server has been shut down (wait_count=%d", wait_count);
  } else {
    GST_WARNING ("no test server");
  }
#endif
}

// gtk+ gui screenshooter

// https://gitlab.gnome.org/GNOME/gtk/blob/gtk-3-24/testsuite/reftests/reftest-snapshot.c#L96

static gboolean
quit_when_idle (gpointer loop)
{
  g_main_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static void
check_for_draw (GdkEvent * event, gpointer data)
{
  if (event->type == GDK_EXPOSE) {
    g_idle_add (quit_when_idle, (GMainLoop *) data);
    gdk_event_handler_set ((GdkEventFunc) gtk_main_do_event, NULL, NULL);
  }
  gtk_main_do_event (event);
}

static cairo_surface_t *
make_screenshot (GtkWidget * widget, gint * iw, gint * ih)
{
  GdkWindow *window = gtk_widget_get_window (widget);
  GMainLoop *loop;
  gint w, h;
  cairo_surface_t *surface;
  cairo_t *cr;

  g_assert (gtk_widget_get_realized (widget));
  g_assert (iw);
  g_assert (ih);

  loop = g_main_loop_new (NULL, FALSE);

  /* We wait until the widget is drawn for the first time.
   * We can not wait for a GtkWidget::draw event, because that might not
   * happen if the window is fully obscured by windowed child widgets.
   * Alternatively, we could wait for an expose event on widget's window.
   * Both of these are rather hairy, not sure what's best.
   */
  gdk_event_handler_set (check_for_draw, loop, NULL);
  g_main_loop_run (loop);

  w = gtk_widget_get_allocated_width (widget);
  h = gtk_widget_get_allocated_height (widget);
  surface = gdk_window_create_similar_surface (window,
      CAIRO_CONTENT_COLOR, w, h);

  cr = cairo_create (surface);

  if (gdk_window_get_window_type (window) == GDK_WINDOW_TOPLEVEL ||
      gdk_window_get_window_type (window) == GDK_WINDOW_FOREIGN) {
    // give the WM/server some time to sync. They need it.
    gdk_display_sync (gdk_window_get_display (window));
    g_timeout_add (500, quit_when_idle, loop);
    g_main_loop_run (loop);
  }
  gdk_cairo_set_source_window (cr, window, 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);
  g_main_loop_unref (loop);

  *iw = w;
  *ih = h;
  return surface;
}

// creates files names under the test data dir
static gchar *
make_filename (GtkWidget * widget, const gchar * name)
{
  gchar *filename;

  if (!name) {
    filename = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.png",
        get_suite_log_base (), gtk_widget_get_name (widget));
  } else {
    filename = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s_%s.png",
        get_suite_log_base (), gtk_widget_get_name (widget), name);
  }
  g_strdelimit (filename, " ", '_');
  return filename;
}

static void
add_shadow_and_save (cairo_surface_t * image, gchar * filename, gint iw,
    gint ih)
{
  cairo_surface_t *shadow;
  cairo_t *cr;
  gint x, y;
  gint ss = 12;                 // shadow size
  gint so = 0;                  // shadow offset
  gint sd = 1 + (ss - so);      // number of draws

  // stupid simple drop shadow code
  shadow = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iw + ss, ih + ss);
  cr = cairo_create (shadow);
  cairo_rectangle (cr, 0.0, 0.0, iw + ss, ih + ss);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
  cairo_fill (cr);
  // we draw sd*sd-24 and we don't want the gray-level to become >0.7
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.7 / (sd * sd - 24.0));
  for (x = ss; x >= so; x--) {
    for (y = ss; y >= so; y--) {
      // skip corners (this would need to be dependend on shadow size)
      if ((x == ss || x == so) && (y > (ss - 2) || y < (so + 2)))
        continue;
      if ((y == ss || y == so) && (x > (ss - 2) || x < (so + 2)))
        continue;
      if ((x == (ss - 1) || x == (so + 1)) && (y > (ss - 1) || y < (so + 1)))
        continue;
      if ((y == (ss - 1) || y == (so + 1)) && (x > (ss - 1) || x < (so + 1)))
        continue;
      cairo_mask_surface (cr, image, x, y);
    }
  }
  cairo_set_source_surface (cr, image, 3, 3);
  cairo_paint (cr);

  cairo_surface_write_to_png (shadow, filename);

  // cleanup
  cairo_destroy (cr);
  cairo_surface_destroy (shadow);
}

/* TODO(ensonic): remember all names to build an index.html with a table and all images */
/* TODO(ensonic): create diff images
 * - check if we have a ref image, skip otherwise
 * - should we have ref images in repo?
 * - own diff
 *   - load old image and subtract pixels
 *   - should we return a boolean for detected pixel changes?
 * - imagemagic
 *   - http://www.imagemagick.org/script/compare.php
 * - need to make a html with table containing
 *   [ name, ref image, cur image, diff image ]
 */
/* TODO(ensonic): write a html image map for selected regions
 * - allows xrefs to the related part in the docs
 * - having tooltips on images wuld be cool
 */
/*
 * check_make_widget_screenshot:
 * @widget: a #GtkWidget to screenshoot
 * @name: filename or NULL.
 *
 * Captures the given widget as a png file. The filename is built from tmpdir,
 * application-name, widget-name and give @name.
 */
void
check_make_widget_screenshot (GtkWidget * widget, const gchar * name)
{
  gchar *filename;
  gint iw, ih;
  cairo_surface_t *surface;
  cairo_t *cr;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  cairo_surface_t *gtk_win_surface = make_screenshot (widget, &iw, &ih);
  iw *= 0.75;
  ih *= 0.75;

  // create a image surface with screenshot
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iw, ih);
  cr = cairo_create (surface);
  cairo_scale (cr, 0.75, 0.75);
  cairo_set_source_surface (cr, gtk_win_surface, 0, 0);
  cairo_paint (cr);
  GST_INFO ("made surface for shadow image");
  cairo_surface_destroy (gtk_win_surface);

  filename = make_filename (widget, name);
  add_shadow_and_save (surface, filename, iw, ih);
  GST_INFO ("made screenshot (%d,%d) for %s", iw, ih, filename);

  // cleanup
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  g_free (filename);
}

static GtkWidget *
find_child (GtkWidget * w, BtCheckWidgetScreenshotRegions * r)
{
  GtkWidget *f = NULL;
  gboolean match = TRUE;

  GST_INFO ("  trying widget: '%s', '%s', '%s',%" G_GSIZE_FORMAT,
      gtk_widget_get_name (w),
      (GTK_IS_LABEL (w) ? gtk_label_get_text ((GtkLabel *) w) : NULL),
      G_OBJECT_TYPE_NAME (w), G_OBJECT_TYPE (w));

  if (r->match & BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_TYPE) {
    if (!g_type_is_a (G_OBJECT_TYPE (w), r->type)) {
      GST_DEBUG ("    wrong type: %" G_GSIZE_FORMAT ",%s", G_OBJECT_TYPE (w),
          G_OBJECT_TYPE_NAME (w));
      match = FALSE;
    }
  }
  if (r->match & BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NAME) {
    if (g_strcmp0 (gtk_widget_get_name (w), r->name)) {
      GST_DEBUG ("    wrong name: %s", gtk_widget_get_name (w));
      match = FALSE;
    }
  }
  if (r->match & BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_LABEL) {
    if ((!GTK_IS_LABEL (w))
        || g_strcmp0 (gtk_label_get_text ((GtkLabel *) w), r->label)) {
      GST_DEBUG ("    not a label or wrong label text");
      match = FALSE;
    }
  }
  if (match) {
    f = w;
  } else if (GTK_IS_CONTAINER (w)) {
    GList *node, *list = gtk_container_get_children (GTK_CONTAINER (w));
    GtkWidget *c = NULL;

    GST_INFO ("  searching container: '%s'", G_OBJECT_TYPE_NAME (w));
    for (node = list; node; node = g_list_next (node)) {
      c = node->data;
      if ((f = find_child (c, r))) {
        break;
      }
    }
    g_list_free (list);
  }
  return f;
}

/*
 * check_make_widget_screenshot_with_highlight:
 * @widget: a #GtkWidget to screenshoot
 * @name: filename or NULL.
 * @regions: array of regions to highlight
 *
 * Captures the given widget as png files. The filename is built from tmpdir,
 * application-name, widget-name and give @name.
 *
 * Locates the widgets listed in @regions, highlights their position and draws
 * callouts to the given position. The callouts are numberd with the array
 * index. Widgets can be specified by name and/or type.
 *
 * The array needs to be terminated with an entry using
 * match=%BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NONE.
 */
void
check_make_widget_screenshot_with_highlight (GtkWidget * widget,
    const gchar * name, BtCheckWidgetScreenshotRegions * regions)
{
  GtkWidget *child, *parent;
  GtkAllocation a;
  gint iw, ih;
  gchar *filename;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_pattern_t *grad;
  cairo_text_extents_t extent;
  BtCheckWidgetScreenshotRegions *r;
  gint c = 12, b = (c / 4) + (c * 3), bl = c, br = c, bt = c, bb = c;
  gint wx, wy, ww, wh;
  gfloat lx1 = 0.0, ly1 = 0.0, lx2 = 0.0, ly2 = 0.0;
  gfloat cx = 0.0, cy = 0.0;
  gfloat f = 0.75;
  gint num = 1;
  gchar num_str[5];
  gboolean have_space;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  cairo_surface_t *gtk_win_surface = make_screenshot (widget, &iw, &ih);
  iw *= f;
  ih *= f;

  // check borders
  r = regions;
  while (r->match != BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NONE) {
    switch (r->pos) {
      case GTK_POS_LEFT:
        bl = b;
        break;
      case GTK_POS_RIGHT:
        br = b;
        break;
      case GTK_POS_TOP:
        bt = b;
        break;
      case GTK_POS_BOTTOM:
        bb = b;
        break;
    }
    r++;
  }

  // create a image surface with screenshot
  surface =
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iw + bl + br,
      ih + bt + bb);
  cr = cairo_create (surface);
  //cairo_save (cr);
  cairo_scale (cr, f, f);
  cairo_set_source_surface (cr, gtk_win_surface, bl, bt);
  cairo_paint (cr);
  cairo_surface_destroy (gtk_win_surface);
  //cairo_restore (cr);

  // locate widgets and highlight the areas
  r = regions;
  while (r->match != BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NONE) {
    GST_INFO ("searching widget: '%s', '%s', '%s',%" G_GSIZE_FORMAT, r->name,
        r->label, g_type_name (r->type), r->type);
    if ((child = find_child (widget, r))) {
      // for label matches we look for a sensible parent
      if (r->match & BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_LABEL) {
        // TODO(ensonic): should the region include gtk_label_get_mnemonic_widget()?
        if ((parent = gtk_widget_get_ancestor (child, GTK_TYPE_BUTTON))) {
          child = parent;
        }
        if ((parent = gtk_widget_get_ancestor (child, GTK_TYPE_TOOL_ITEM))) {
          child = parent;
        }
      }
      // highlight area
      parent = gtk_widget_get_parent (child);
      have_space = FALSE;
      if (parent) {
        if (GTK_IS_CONTAINER (parent)
            && gtk_container_get_border_width ((GtkContainer *) parent) > 0) {
          have_space = TRUE;
        }
        if (GTK_IS_BOX (parent) && gtk_box_get_spacing ((GtkBox *) parent) > 0) {
          have_space = TRUE;
        }
      }
      /* it seems that the API docs are lying and the allocations are relative to
       * the toplevel and not their parent
       */
      // TODO(ensonic): if we don't snapshot a top-level, we need to subtract the offset
      gtk_widget_get_allocation (child, &a);
      wx = a.x;
      wy = a.y;
      ww = a.width;
      wh = a.height;
      GST_INFO ("found widget at: %d,%d with %dx%d pixel size", a.x, a.y,
          a.width, a.height);

      if (have_space) {
        cairo_rectangle (cr, bl + wx, bt + wy, ww, wh);
      } else {
        // we make this a bit smaller to separate adjacent widgets
        cairo_rectangle (cr, bl + wx + 1, bt + wy + 1, ww - 2, wh - 2);
      }
      cairo_set_source_rgba (cr, 1.0, 0.0, 0.2, 0.3);
      cairo_fill (cr);
      cairo_set_line_width (cr, 4);
      // callouts positions
      switch (r->pos) {
        case GTK_POS_LEFT:
          lx1 = bl + wx;
          lx2 = b - c;
          cx = lx2 - c;
          cy = ly1 = ly2 = bt + wy + wh / 2.0;
          break;
        case GTK_POS_RIGHT:
          lx1 = bl + wx + ww;
          lx2 = bl + iw + c;
          cx = lx2 + c;
          cy = ly1 = ly2 = bt + wy + wh / 2.0;
          break;
        case GTK_POS_TOP:
          break;
        case GTK_POS_BOTTOM:
          break;
      }
      // callout line
      grad = cairo_pattern_create_linear (lx1, ly1, lx2, ly2);
      cairo_pattern_add_color_stop_rgba (grad, 0.00, 1.0, 0.0, 0.2, 0.3);
      cairo_pattern_add_color_stop_rgba (grad, 1.00, 1.0, 0.0, 0.2, 1.0);
      cairo_set_source (cr, grad);
      cairo_move_to (cr, lx1, ly1);
      cairo_line_to (cr, lx2, ly2);
      cairo_stroke (cr);
      cairo_pattern_destroy (grad);
      // callout circle
      cairo_arc (cr, cx, cy, c, 0.0, 2 * M_PI);
      cairo_set_source_rgba (cr, 1.0, 0.0, 0.2, 1.0);
      cairo_stroke_preserve (cr);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
      cairo_fill (cr);
      // callout label
      sprintf (num_str, "%d", num);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
      cairo_set_font_size (cr, 10);
      cairo_text_extents (cr, num_str, &extent);
      // this is the bottom left pos for the text
      cairo_move_to (cr, cx - (extent.width / 2.0), cy + (extent.height / 2.0));
      cairo_show_text (cr, num_str);
    } else {
      GST_WARNING ("widget not found: '%s',%" G_GSIZE_FORMAT, r->name, r->type);
    }
    r++;
    num++;
  }

  filename = make_filename (widget, name);
  add_shadow_and_save (surface, filename, iw + bl + br, ih + bt + bb);

  // cleanup
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  g_free (filename);
}

/*
 * check_send_key:
 * @widget: a #GtkWidget to send the #GdkEventKey to
 * @state: modifiers
 * @keyval: the key code
 * @hardware_keycode: the hardware keycode, if needed
 *
 * Send a key-press and a key-release of the given key to the @widget.
 */
void
check_send_key (GtkWidget * widget, guint state, guint keyval,
    guint16 hardware_keycode)
{
  GdkEventKey *e;
  GdkWindow *w = gtk_widget_get_window (widget);
#if GTK_CHECK_VERSION(3,20,0)
  GdkDevice *dev =
      gdk_seat_get_keyboard (gdk_display_get_default_seat
      (gdk_window_get_display (w)));
#endif

  e = (GdkEventKey *) gdk_event_new (GDK_KEY_PRESS);
  e->window = g_object_ref (w);
  e->keyval = keyval;
  e->hardware_keycode = hardware_keycode;
  e->state |= state;
#if GTK_CHECK_VERSION(3,20,0)
  gdk_event_set_device ((GdkEvent *) e, dev);
#endif
  gtk_main_do_event ((GdkEvent *) e);
  flush_main_loop ();
  gdk_event_free ((GdkEvent *) e);

  e = (GdkEventKey *) gdk_event_new (GDK_KEY_RELEASE);
  e->window = g_object_ref (w);
  e->keyval = keyval;
  e->hardware_keycode = hardware_keycode;
  e->state |= GDK_RELEASE_MASK;
#if GTK_CHECK_VERSION(3,20,0)
  gdk_event_set_device ((GdkEvent *) e, dev);
#endif
  gtk_main_do_event ((GdkEvent *) e);
  flush_main_loop ();
  gdk_event_free ((GdkEvent *) e);
}

/*
 * check_send_click:
 * @widget: a #GtkWidget to send the #GdkEventButton to
 * @button: the button number (1-3)
 * @x: the x-position of the mouse
 * @y: the y-position of the mouse
 *
 * Send a button-press and a button-release of the given mouse button to the
 * @widget.
 */
void
check_send_click (GtkWidget * widget, guint button, gdouble x, gdouble y)
{
  GdkEventButton *e;
  GdkWindow *w = gtk_widget_get_window (widget);
#if GTK_CHECK_VERSION(3,20,0)
  GdkDevice *dev =
      gdk_seat_get_pointer (gdk_display_get_default_seat (gdk_window_get_display
          (w)));
#endif

  e = (GdkEventButton *) gdk_event_new (GDK_BUTTON_PRESS);
  e->window = g_object_ref (w);
  e->button = button;
  e->x = x;
  e->y = y;
  e->state = GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK;
#if GTK_CHECK_VERSION(3,20,0)
  gdk_event_set_device ((GdkEvent *) e, dev);
#endif
  gtk_main_do_event ((GdkEvent *) e);
  flush_main_loop ();
  gdk_event_free ((GdkEvent *) e);

  e = (GdkEventButton *) gdk_event_new (GDK_BUTTON_RELEASE);
  e->window = g_object_ref (w);
  e->button = button;
  e->x = x;
  e->y = y;
  e->state = GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK;
#if GTK_CHECK_VERSION(3,20,0)
  gdk_event_set_device ((GdkEvent *) e, dev);
#endif
  gtk_main_do_event ((GdkEvent *) e);
  flush_main_loop ();
  gdk_event_free ((GdkEvent *) e);
}

/*
 * flush_main_loop:
 *
 * Process pending events.
 */
void
flush_main_loop (void)
{
  GMainContext *ctx = g_main_context_default ();
  gint num = 0;

  GST_INFO ("flushing pending events (main-depth=%d) ...", g_main_depth ());
  while (g_main_context_pending (ctx)) {
    g_main_context_iteration (ctx, FALSE);
    num++;
  }
  GST_INFO ("... done  (main-depth=%d, iterations=%d)", g_main_depth (), num);
}
