/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_CHECK_H
#define BT_CHECK_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <check.h>
//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
//-- gstreamer
#include <gst/gstinfo.h>
//-- buzztrax
#include "core/core.h"
#include "bt-test-application.h"
#include "bt-test-plugin.h"
#include "btic-test-device.h"

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
#define BT_TEST_ARGS  void
#else
#define BT_TEST_ARGS int _i __attribute__((unused))
#endif

#define BT_CASE_START setup_log_case (__FILE__)


#define BT_TEST_START \
  setup_log_test (__FUNCTION__, _i); \
  tcase_fn_start (__FUNCTION__, __FILE__, __LINE__); \
  {

#define BT_TEST_END \
  setup_log_test (NULL, 0); \
  }

#define BT_TEST_SUITE_E(N, n) \
extern TCase *n##_example_case (void); \
static inline Suite * \
n##_suite (void) \
{ \
  Suite *s = suite_create (N); \
  suite_add_tcase (s, n##_example_case ()); \
  return s; \
}

#ifdef G_DISABLE_CHECKS
#define BT_TEST_SUITE_T_E(N, n) \
extern TCase *n##_example_case (void); \
static inline Suite * \
n##_suite (void) \
{ \
  Suite *s = suite_create (N); \
  suite_add_tcase (s, n##_example_case ()); \
  return s; \
}

#define BT_TEST_SUITE_T(N, n)  \
static inline Suite * \
n##_suite (void) \
{ \
  Suite *s = suite_create (N); \
  return s; \
}
#else
#define BT_TEST_SUITE_T_E(N, n) \
extern TCase *n##_test_case (void); \
extern TCase *n##_example_case (void); \
static inline Suite * \
n##_suite (void) \
{ \
  Suite *s = suite_create (N); \
  suite_add_tcase (s, n##_test_case ()); \
  suite_add_tcase (s, n##_example_case ()); \
  return s; \
}

#define BT_TEST_SUITE_T(N, n) \
extern TCase *n##_test_case (void); \
static inline Suite * \
n##_suite (void) \
{ \
  Suite *s = suite_create (N); \
  suite_add_tcase (s, n##_test_case ()); \
  return s; \
}
#endif

/* Hack to allow run-time selection of unit tests to run via the
 * BT_CHECKS environment variable (test function names, comma-separated)
 * (borrowed from gstreamer)
 */
gboolean _bt_check_run_test_func (const gchar * func_name);

static inline void
__bt_tcase_add_test (TCase * tc, const TTest * ttest, int signal,
    int allowed_exit_value, int start, int end)
{
  if (_bt_check_run_test_func (ttest->name)) {
    _tcase_add_test (tc, ttest, signal, allowed_exit_value, start, end);
  }
}

#define _tcase_add_test __bt_tcase_add_test


//-- testing helper methods

void bt_check_init(void);

#define ck_g_object_final_unref(obj) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  gint __objrefct=G_OBJECT(__objref)->ref_count;\
  GST_INFO("object <%s>:%p,ref_ct=%d",G_OBJECT_TYPE_NAME(__objref),__objref,__objrefct);\
  g_object_add_weak_pointer(__objref,&__objref);\
  g_object_unref(__objref);\
  ck_assert_msg(__objref == NULL, "%d ref(s) left",__objrefct-1);\
}

#define ck_g_object_remaining_unref(obj, n) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  gint __objrefct=G_OBJECT(__objref)->ref_count;\
  GST_INFO("object <%s>:%p,ref_ct=%d",G_OBJECT_TYPE_NAME(__objref),__objref,__objrefct);\
  g_object_add_weak_pointer(__objref,&__objref);\
  g_object_unref(__objref);\
  ck_assert_msg(__objref != NULL, "was last unref, %d remaining refs expectd", n); \
  g_object_remove_weak_pointer(__objref,&__objref);\
  ck_assert_int_eq(G_OBJECT(__objref)->ref_count, n);\
}

#define ck_g_object_logged_unref(obj) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  gint __objrefct=G_OBJECT(__objref)->ref_count;\
  GST_INFO("object <%s>:%p,ref_ct=%d",G_OBJECT_TYPE_NAME(__objref),__objref,__objrefct);\
  g_object_unref(__objref);\
}

#define ck_gst_object_final_unref(obj) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  gint __objrefct=G_OBJECT(__objref)->ref_count;\
  GST_INFO_OBJECT(__objref, "object <%s>:%p,ref_ct=%d",G_OBJECT_TYPE_NAME(__objref),__objref,__objrefct);\
  g_object_add_weak_pointer(__objref,&__objref);\
  gst_object_unref(__objref);\
  ck_assert_msg(__objref == NULL, "%d ref(s) left",__objrefct-1);\
}

#define ck_gst_object_remaining_unref(obj, n) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  gint __objrefct=G_OBJECT(__objref)->ref_count;\
  GST_INFO_OBJECT(__objref, "object <%s>:%p,ref_ct=%d",G_OBJECT_TYPE_NAME(__objref),__objref,__objrefct);\
  g_object_add_weak_pointer(__objref,&__objref);\
  gst_object_unref(__objref);\
  ck_assert_msg(__objref != NULL, "was last unref, %d remaining refs expectd", n); \
  g_object_remove_weak_pointer(__objref,&__objref);\
  ck_assert_int_eq(G_OBJECT(__objref)->ref_count, n);\
}

#define ck_gst_object_logged_unref(obj) \
{\
  gpointer __objref=obj;\
  g_assert(__objref);\
  gint __objrefct=G_OBJECT(__objref)->ref_count;\
  GST_INFO_OBJECT(__objref, "object <%s>:%p,ref_ct=%d",G_OBJECT_TYPE_NAME(__objref),__objref,__objrefct);\
  gst_object_unref(__objref);\
}

#define ck_assert_gobject_ref_ct(obj,n ) \
  ck_assert_int_eq(G_OBJECT(obj)->ref_count, n);

void check_init_error_trapp(gchar *method, gchar *test);
gboolean check_has_error_trapped(void);
gboolean _check_log_contains(gchar *text);

#define check_log_contains(text, msg) \
  ck_assert_msg (_check_log_contains(text), msg)

void setup_log_base(gint argc, gchar **argv);
void setup_log_case(const gchar * file_name);
void setup_log_test(const gchar * func_name, gint i);
void setup_log_capture(void);
void collect_logs(gboolean no_failures);
const gchar *get_suite_log_base(void);
const gchar *get_suite_log_filename(void);

void check_run_main_loop_for_usec(gulong usec);
gboolean check_run_main_loop_until_msg_or_error(BtSong * song, const gchar * msg);
gboolean check_run_main_loop_until_playing_or_error(BtSong * song);
gboolean check_run_main_loop_until_eos_or_error(BtSong * song);

void check_remove_gst_feature(gchar *feature_name);

const gchar *check_get_test_song_path(const gchar *name);

gboolean check_file_contains_str(FILE *input_file, gchar *input_file_name, gchar *string);

gboolean check_gobject_properties(GObject *toCheck);
gboolean check_gobject_get_boolean_property(gpointer obj, const gchar *prop);
gint check_gobject_get_int_property(gpointer obj, const gchar *prop);
guint check_gobject_get_uint_property(gpointer obj, const gchar *prop);
glong check_gobject_get_long_property(gpointer obj, const gchar *prop);
gulong check_gobject_get_ulong_property(gpointer obj, const gchar *prop);
guint64 check_gobject_get_uint64_property(gpointer obj, const gchar *prop);
GObject *check_gobject_get_object_property(gpointer obj, const gchar *prop);
gchar *check_gobject_get_str_property(gpointer obj, const gchar *prop);
gpointer check_gobject_get_ptr_property(gpointer obj, const gchar *prop);

/* comparsion macros with improved output compared to regular 'check.h' macros. */
#define _ck_assert_gboolean(O, X, C, Y) do { \
  gboolean __ck = check_gobject_get_boolean_property((O), (X)); \
  ck_assert_msg(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%d, "#Y"==%d", __ck, Y); \
} while (0)
#define ck_assert_gobject_gboolean_eq(O, X, Y) _ck_assert_gboolean(O, X, ==, Y)
#define ck_assert_gobject_gboolean_ne(O, X, Y) _ck_assert_gboolean(O, X, !=, Y)

#define _ck_assert_guint(O, X, C, Y) do { \
  guint __ck = check_gobject_get_uint_property((O), (X)); \
  ck_assert_msg(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%u, "#Y"==%u", __ck, Y); \
} while (0)
#define ck_assert_gobject_guint_eq(O, X, Y) _ck_assert_guint(O, X, ==, Y)
#define ck_assert_gobject_guint_ne(O, X, Y) _ck_assert_guint(O, X, !=, Y)
#define ck_assert_gobject_guint_gt(O, X, Y) _ck_assert_guint(O, X, >, Y)
#define ck_assert_gobject_guint_lt(O, X, Y) _ck_assert_guint(O, X, <, Y)
#define ck_assert_gobject_guint_ge(O, X, Y) _ck_assert_guint(O, X, >=, Y)
#define ck_assert_gobject_guint_le(O, X, Y) _ck_assert_guint(O, X, <=, Y)

#define _ck_assert_glong(O, X, C, Y) do { \
  glong __ck = check_gobject_get_long_property((O), (X)); \
  ck_assert_msg(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%ld, "#Y"==%ld", __ck, Y); \
} while (0)
#define ck_assert_gobject_glong_eq(O, X, Y) _ck_assert_glong(O, X, ==, Y)
#define ck_assert_gobject_glong_ne(O, X, Y) _ck_assert_glong(O, X, !=, Y)
#define ck_assert_gobject_glong_gt(O, X, Y) _ck_assert_glong(O, X, >, Y)
#define ck_assert_gobject_glong_lt(O, X, Y) _ck_assert_glong(O, X, <, Y)
#define ck_assert_gobject_glong_ge(O, X, Y) _ck_assert_glong(O, X, >=, Y)
#define ck_assert_gobject_glong_le(O, X, Y) _ck_assert_glong(O, X, <=, Y)

#define _ck_assert_gulong(O, X, C, Y) do { \
  gulong __ck = check_gobject_get_ulong_property((O), (X)); \
  ck_assert_msg(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%lu, "#Y"==%lu", __ck, Y); \
} while (0)
#define ck_assert_gobject_gulong_eq(O, X, Y) _ck_assert_gulong(O, X, ==, Y)
#define ck_assert_gobject_gulong_ne(O, X, Y) _ck_assert_gulong(O, X, !=, Y)
#define ck_assert_gobject_gulong_gt(O, X, Y) _ck_assert_gulong(O, X, >, Y)
#define ck_assert_gobject_gulong_lt(O, X, Y) _ck_assert_gulong(O, X, <, Y)
#define ck_assert_gobject_gulong_ge(O, X, Y) _ck_assert_gulong(O, X, >=, Y)
#define ck_assert_gobject_gulong_le(O, X, Y) _ck_assert_gulong(O, X, <=, Y)

#define _ck_assert_genum(O, X, C, Y) do { \
  gulong __ck = check_gobject_get_int_property((O), (X)); \
  ck_assert_msg(__ck C (Y), "Assertion '"#X#C#Y"' failed: "#X"==%lu, "#Y"==%lu", __ck, Y); \
} while (0)
#define ck_assert_gobject_genum_eq(O, X, Y) _ck_assert_genum(O, X, ==, Y)
#define ck_assert_gobject_genum_ne(O, X, Y) _ck_assert_genum(O, X, !=, Y)


#define _ck_assert_gobject(O, X, C, Y) do { \
  GObject *__ck = check_gobject_get_object_property ((O), (X)); \
  ck_assert_msg(__ck C (GObject *)(Y), "Assertion '"#X#C#Y"' failed: "#X"==%p, "#Y"==%p", __ck, Y); \
  if(__ck) g_object_unref(__ck); \
} while(0)
#define ck_assert_gobject_object_eq(O, X, Y) _ck_assert_gobject(O, X, ==, Y)
#define ck_assert_gobject_object_ne(O, X, Y) _ck_assert_gobject(O, X, !=, Y)

#define _ck_assert_str_and_free(F, X, C, Y) do { \
  gchar *__ck = (X); \
  ck_assert_msg(F(__ck, (Y)), "Assertion '"#X#C#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\"", __ck, Y); \
  g_free(__ck); \
} while(0)
#define ck_assert_str_eq_and_free(X, Y) _ck_assert_str_and_free(!g_strcmp0, X, ==, Y)
#define ck_assert_str_ne_and_free(X, Y) _ck_assert_str_and_free(g_strcmp0, X, !=, Y)

#define ck_assert_gobject_str_eq(O, X, Y) _ck_assert_str_and_free(!g_strcmp0, check_gobject_get_str_property((O), (X)), ==, (gchar*)(Y))
#define ck_assert_gobject_str_ne(O, X, Y) _ck_assert_str_and_free(g_strcmp0, check_gobject_get_str_property((O), (X)), !=, (gchar*)(Y))


#define _ck_assert_gobject_and_unref(X, C, Y) do { \
  GObject *__ck = (GObject *)(X); \
  ck_assert_msg(__ck C (GObject *)(Y), "Assertion '"#X#C#Y"' failed: "#X"==%p, "#Y"==%p", __ck, Y); \
  if(__ck) g_object_unref(__ck); \
} while(0)
#define ck_assert_gobject_eq_and_unref(X, Y) _ck_assert_gobject_and_unref(X, ==, Y)
#define ck_assert_gobject_ne_and_unref(X, Y) _ck_assert_gobject_and_unref(X, !=, Y)

#define ck_assert_int_gt(X, Y) _ck_assert_int(X, >, Y)
#define ck_assert_int_lt(X, Y) _ck_assert_int(X, <, Y)
#define ck_assert_int_ge(X, Y) _ck_assert_int(X, >=, Y)
#define ck_assert_int_le(X, Y) _ck_assert_int(X, <=, Y)

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

#define _ck_assert_float(X, O, Y) ck_assert_msg((X) O (Y), "Assertion '"#X#O#Y"' failed: "#X"==%f, "#Y"==%f", X, Y)

// plotting helper

gint check_plot_data_int16 (gint16 * d, guint size, const gchar * base, const gchar * name);
gint check_plot_data_double (gdouble * d, guint size, const gchar * base, const gchar * name, const gchar *cfg);

#endif /* BT_CHECK_H */
