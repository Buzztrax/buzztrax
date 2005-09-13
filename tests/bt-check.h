/* $Id: bt-check.h,v 1.7 2005-09-13 18:51:00 ensonic Exp $
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

extern guint test_argc;
extern gchar **test_argvptr;

#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_check_debug
#endif
#ifndef BT_CHECK
  GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

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

extern gboolean file_contains_str(gchar *tmp_file_name, gchar *string);

extern gboolean check_gobject_properties(GObject *toCheck);

extern void check_setup_test_server(void);
extern void check_setup_test_display(void);
extern void check_shutdown_test_display(void);
extern void check_shutdown_test_server(void);

#endif /* BT_CHECK_H */
