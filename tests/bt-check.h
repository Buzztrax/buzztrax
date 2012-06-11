/* Buzztard
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
#include "core.h"
#include "bt-test-application.h"
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

/* Hack to allow run-time selection of unit tests to run via the
 * BT_CHECKS environment variable (test function names, comma-separated)
 * (borrowed from gstreamer)
 */
gboolean _bt_check_run_test_func (const gchar * func_name);

#if CHECK_VERSION <= 903
static inline void
__bt_tcase_add_test (TCase * tc, TFun tf, const char * fname, int signal)
{
  if (_bt_check_run_test_func (fname)) {
    _tcase_add_test (tc, tf, fname, signal);
  }
}
#else
#if CHECK_VERSION <= 906
static inline void
__bt_tcase_add_test (TCase * tc, TFun tf, const char * fname, int signal,
    int start, int end)
{
  if (_bt_check_run_test_func (fname)) {
    _tcase_add_test (tc, tf, fname, signal, start, end);
  }
}
#else
static inline void
__bt_tcase_add_test (TCase * tc, TFun tf, const char * fname, int signal,
    int allowed_exit_value, int start, int end)
{
  if (_bt_check_run_test_func (fname)) {
    _tcase_add_test (tc, tf, fname, signal, allowed_exit_value, start, end);
  }
}
#endif
#endif

#define _tcase_add_test __bt_tcase_add_test


//-- testing helper methods

void bt_check_init(void);

#define g_object_checked_unref(obj) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  GST_INFO("object %p ->ref_count: %d",__objref,G_OBJECT(__objref)->ref_count);\
  g_object_add_weak_pointer(__objref,&__objref);\
  g_object_unref(__objref);\
  fail_unless(__objref == NULL, NULL);\
}

void check_init_error_trapp(gchar *method, gchar *test);
gboolean check_has_error_trapped(void);

void setup_log(gint argc, gchar **argv);
void setup_log_capture(void);

void check_run_main_loop_for_usec(gulong usec);

const gchar *check_get_test_song_path(const gchar *name);

gboolean file_contains_str(gchar *tmp_file_name, gchar *string);

gboolean check_gobject_properties(GObject *toCheck);
gboolean check_gobject_get_boolean_property(gpointer obj, const gchar *prop);
guint check_gobject_get_uint_property(gpointer obj, const gchar *prop);
glong check_gobject_get_long_property(gpointer obj, const gchar *prop);
gulong check_gobject_get_ulong_property(gpointer obj, const gchar *prop);
GObject *check_gobject_get_object_property(gpointer obj, const gchar *prop);
gchar *check_gobject_get_str_property(gpointer obj, const gchar *prop);
gpointer check_gobject_get_ptr_property(gpointer obj, const gchar *prop);

/* comparsion macros with improved output compared to fail_unless(). */
#define _ck_assert_gboolean(O, X, C, Y) do { \
  gboolean __ck = check_gobject_get_boolean_property((O), (X)); \
  fail_unless(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%ld, "#Y"==%ld", __ck, Y); \
} while (0)
#define ck_assert_gobject_boolean_eq(O, X, Y) _ck_assert_gboolean(O, X, ==, Y)
#define ck_assert_gobject_boolean_ne(O, X, Y) _ck_assert_gboolean(O, X, !=, Y)

#define _ck_assert_guint(O, X, C, Y) do { \
  guint __ck = check_gobject_get_uint_property((O), (X)); \
  fail_unless(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%u, "#Y"==%u", __ck, Y); \
} while (0)
#define ck_assert_gobject_guint_eq(O, X, Y) _ck_assert_guint(O, X, ==, Y)
#define ck_assert_gobject_guint_ne(O, X, Y) _ck_assert_guint(O, X, !=, Y)
#define ck_assert_gobject_guint_gt(O, X, Y) _ck_assert_guint(O, X, >, Y)
#define ck_assert_gobject_guint_lt(O, X, Y) _ck_assert_guint(O, X, <, Y)
#define ck_assert_gobject_guint_ge(O, X, Y) _ck_assert_guint(O, X, >=, Y)
#define ck_assert_gobject_guint_le(O, X, Y) _ck_assert_guint(O, X, <=, Y)

#define _ck_assert_glong(O, X, C, Y) do { \
  glong __ck = check_gobject_get_long_property((O), (X)); \
  fail_unless(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%ld, "#Y"==%ld", __ck, Y); \
} while (0)
#define ck_assert_gobject_glong_eq(O, X, Y) _ck_assert_glong(O, X, ==, Y)
#define ck_assert_gobject_glong_ne(O, X, Y) _ck_assert_glong(O, X, !=, Y)
#define ck_assert_gobject_glong_gt(O, X, Y) _ck_assert_glong(O, X, >, Y)
#define ck_assert_gobject_glong_lt(O, X, Y) _ck_assert_glong(O, X, <, Y)
#define ck_assert_gobject_glong_ge(O, X, Y) _ck_assert_glong(O, X, >=, Y)
#define ck_assert_gobject_glong_le(O, X, Y) _ck_assert_glong(O, X, <=, Y)

#define _ck_assert_gulong(O, X, C, Y) do { \
  gulong __ck = check_gobject_get_ulong_property((O), (X)); \
  fail_unless(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%lu, "#Y"==%lu", __ck, Y); \
} while (0)
#define ck_assert_gobject_gulong_eq(O, X, Y) _ck_assert_gulong(O, X, ==, Y)
#define ck_assert_gobject_gulong_ne(O, X, Y) _ck_assert_gulong(O, X, !=, Y)
#define ck_assert_gobject_gulong_gt(O, X, Y) _ck_assert_gulong(O, X, >, Y)
#define ck_assert_gobject_gulong_lt(O, X, Y) _ck_assert_gulong(O, X, <, Y)
#define ck_assert_gobject_gulong_ge(O, X, Y) _ck_assert_gulong(O, X, >=, Y)
#define ck_assert_gobject_gulong_le(O, X, Y) _ck_assert_gulong(O, X, <=, Y)

#define _ck_assert_gobject(O, X, C, Y) do { \
  GObject *__ck = check_gobject_get_object_property ((O), (X)); \
  fail_unless(__ck C (GObject *)(Y), "Assertion '"#X#C#Y"' failed: "#X"==%p, "#Y"==%p", __ck, Y); \
  if(__ck) g_object_unref(__ck); \
} while(0)
#define ck_assert_gobject_object_eq(O, X, Y) _ck_assert_gobject(O, X, ==, Y)
#define ck_assert_gobject_object_ne(O, X, Y) _ck_assert_gobject(O, X, !=, Y)

#define _ck_assert_str_and_free(F, X, C, Y) do { \
  gchar *__ck = (X); \
  fail_unless(F(__ck, (Y)), "Assertion '"#X#C#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\"", __ck, Y); \
  g_free(__ck); \
} while(0)
#define ck_assert_str_eq_and_free(X, Y) _ck_assert_str_and_free(!g_strcmp0, X, ==, Y)
#define ck_assert_str_ne_and_free(X, Y) _ck_assert_str_and_free(g_strcmp0, X, !=, Y)

#define ck_assert_gobject_str_eq(O, X, Y) _ck_assert_str_and_free(!g_strcmp0, check_gobject_get_str_property((O), (X)), ==, Y)
#define ck_assert_gobject_str_ne(O, X, Y) _ck_assert_str_and_free(g_strcmp0, check_gobject_get_str_property((O), (X)), !=, Y)


#define _ck_assert_gobject_and_unref(X, C, Y) do { \
  GObject *__ck = (GObject *)(X); \
  fail_unless(__ck C (GObject *)(Y), "Assertion '"#X#C#Y"' failed: "#X"==%p, "#Y"==%p", __ck, Y); \
  if(__ck) g_object_unref(__ck); \
} while(0)
#define ck_assert_gobject_eq_and_unref(X, Y) _ck_assert_gobject_and_unref(X, ==, Y)
#define ck_assert_gobject_ne_and_unref(X, Y) _ck_assert_gobject_and_unref(X, !=, Y)

#define _ck_assert_ulong(X, O, Y) ck_assert_msg((X) O (Y), "Assertion '"#X#O#Y"' failed: "#X"==%lu, "#Y"==%lu", X, Y)
#define ck_assert_ulong_eq(X, Y) _ck_assert_ulong(X, ==, Y)
#define ck_assert_ulong_ne(X, Y) _ck_assert_ulong(X, !=, Y)
#define ck_assert_ulong_gt(X, Y) _ck_assert_ulong(X, >, Y)
#define ck_assert_ulong_lt(X, Y) _ck_assert_ulong(X, <, Y)
#define ck_assert_ulong_ge(X, Y) _ck_assert_ulong(X, >=, Y)
#define ck_assert_ulong_le(X, Y) _ck_assert_ulong(X, <=, Y)

#define _ck_assert_uint64(X, O, Y) ck_assert_msg((X) O (Y), "Assertion '"#X#O#Y"' failed: "#X"==%"G_GUINT64_FORMAT", "#Y"==%"G_GUINT64_FORMAT, X, Y)
#define ck_assert_uint64_eq(X, Y) _ck_assert_uint64(X, ==, Y)
#define ck_assert_uint64_ne(X, Y) _ck_assert_uint64(X, !=, Y)
#define ck_assert_uint64_gt(X, Y) _ck_assert_uint64(X, >, Y)
#define ck_assert_uint64_lt(X, Y) _ck_assert_uint64(X, <, Y)
#define ck_assert_uint64_ge(X, Y) _ck_assert_uint64(X, >=, Y)
#define ck_assert_uint64_le(X, Y) _ck_assert_uint64(X, <=, Y)

#define _ck_assert_int64(X, O, Y) ck_assert_msg((X) O (Y), "Assertion '"#X#O#Y"' failed: "#X"==%"G_GINT64_FORMAT", "#Y"==%"G_GINT64_FORMAT, X, Y)
#define ck_assert_int64_eq(X, Y) _ck_assert_int64(X, ==, Y)
#define ck_assert_int64_ne(X, Y) _ck_assert_int64(X, !=, Y)
#define ck_assert_int64_gt(X, Y) _ck_assert_int64(X, >, Y)
#define ck_assert_int64_lt(X, Y) _ck_assert_int64(X, <, Y)
#define ck_assert_int64_ge(X, Y) _ck_assert_int64(X, >=, Y)
#define ck_assert_int64_le(X, Y) _ck_assert_int64(X, <=, Y)

void check_setup_test_server(void);
void check_setup_test_display(void);
void check_shutdown_test_display(void);
void check_shutdown_test_server(void);

enum _BtCheckWidgetScreenshotRegionsMatch {
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NONE = 0,
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_TYPE = (1<<0),
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NAME = (1<<1),
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_LABEL = (1<<2),
};

typedef enum _BtCheckWidgetScreenshotRegionsMatch BtCheckWidgetScreenshotRegionsMatch;

struct _BtCheckWidgetScreenshotRegions {
  BtCheckWidgetScreenshotRegionsMatch match;
  gchar *name;
  gchar *label;
  GType type;
  GtkPositionType pos;
};
typedef struct _BtCheckWidgetScreenshotRegions BtCheckWidgetScreenshotRegions;

void check_make_widget_screenshot(GtkWidget *widget, const gchar *name);
void check_make_widget_screenshot_with_highlight(GtkWidget *widget, const gchar *name, BtCheckWidgetScreenshotRegions *regions);

void check_send_key(GtkWidget *widget, guint keyval, guint16 hardware_keycode);
void check_send_click(GtkWidget *widget, guint button, gdouble x, gdouble y);

#endif /* BT_CHECK_H */
