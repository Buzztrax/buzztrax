/* $Id: bt-check.h,v 1.12 2006-01-16 21:39:25 ensonic Exp $
 * testing helpers
 */

#ifndef BT_CHECK_H
#define BT_CHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <check.h>
//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
//-- gstreamer
#include <gst/gst.h>
//-- buzztard
#include <libbtcore/core.h>
#include "bt-test-plugin.h"

extern gint test_argc;
extern gchar **test_argvptr;

#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_check_debug
#endif
#ifndef BT_CHECK
  GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

//-- wrappers for START_TEST and END_TEST

#define BT_START_TEST(__testname) \
static void __testname (void)\
{\
  GST_DEBUG ("test beg ----------------------------------------------------------------------"); \
  tcase_fn_start (""# __testname, __FILE__, __LINE__);

#define BT_END_TEST \
  GST_DEBUG ("test end ----------------------------------------------------------------------\n"); \
}


//-- testing helper methods

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

extern gchar *check_get_test_song_path(const gchar *name);

extern gboolean file_contains_str(gchar *tmp_file_name, gchar *string);

extern gboolean check_gobject_properties(GObject *toCheck);

extern void check_setup_test_server(void);
extern void check_setup_test_display(void);
extern void check_shutdown_test_display(void);
extern void check_shutdown_test_server(void);

#endif /* BT_CHECK_H */
