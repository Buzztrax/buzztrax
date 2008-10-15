/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * testing helpers
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_CHECK_H
#define BT_CHECK_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <check.h>
//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
//-- gtk/gdk
#include <gtk/gtk.h>
#include <gdk/gdk.h>
//-- gstreamer
#include <gst/gst.h>
//-- buzztard
#include <libbuzztard-core/core.h>
#include "bt-test-plugin.h"
#include "bt-test-settings.h"

extern gint test_argc;
extern gchar **test_argvptr;

#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_check_debug
#endif
#ifndef BT_CHECK
  GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

//-- wrappers for START_TEST and END_TEST

#define CHECK_VERSION (CHECK_MAJOR_VERSION * 10000 + CHECK_MINOR_VERSION * 100 + CHECK_MICRO_VERSION)
#if CHECK_VERSION <= 903
#define BT_START_TEST(__testname) \
static void __testname (void)\
{\
  GST_DEBUG ("test beg ----------------------------------------------------------------------"); \
  tcase_fn_start (""# __testname, __FILE__, __LINE__);
#else
#define BT_START_TEST(__testname) \
static void __testname (int _i __attribute__((unused)))\
{\
  GST_DEBUG ("test beg ----------------------------------------------------------------------"); \
  tcase_fn_start (""# __testname, __FILE__, __LINE__);
#endif

#define BT_END_TEST \
  GST_DEBUG ("test end ----------------------------------------------------------------------\n"); \
}


//-- testing helper methods

extern void bt_check_init(void); 

#define g_object_checked_unref(obj) \
{\
  gpointer __objref;\
  g_assert(obj);\
  GST_INFO("object %p ->ref_count: %d",obj,G_OBJECT(obj)->ref_count);\
  g_object_add_weak_pointer((gpointer)obj,&__objref);\
  g_object_unref((gpointer)obj);\
  fail_unless(__objref == NULL, NULL);\
}

extern void check_init_error_trapp(gchar *method, gchar *test);
extern gboolean check_has_error_trapped(void);

extern void setup_log(int argc, char **argv);
extern void setup_log_capture(void);

extern const gchar *check_get_test_song_path(const gchar *name);

extern gboolean file_contains_str(gchar *tmp_file_name, gchar *string);

extern gboolean check_gobject_properties(GObject *toCheck);

extern void check_setup_test_server(void);
extern void check_setup_test_display(void);
extern void check_shutdown_test_display(void);
extern void check_shutdown_test_server(void);

extern void check_make_widget_screenshot(GtkWidget *widget, const gchar *name);

#endif /* BT_CHECK_H */
