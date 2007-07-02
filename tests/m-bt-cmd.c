/* $Id: m-bt-cmd.c,v 1.16 2007-07-02 11:23:30 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * command line app unit tests
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

#define BT_CHECK
 
#include "bt-check.h"
#include "../src/ui/cmd/bt-cmd.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_cmd_debug);

extern Suite *bt_cmd_application_suite(void);

gint test_argc=1;
gchar test_arg0[]="check_buzzard";
gchar *test_argv[1];
gchar **test_argvptr;

/* common setup and teardown code */
void bt_cmd_setup(void) {
  bt_init(&test_argc,&test_argvptr);

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-check", 0, "music production environment / unit tests");
  GST_DEBUG_CATEGORY_INIT(bt_cmd_debug, "bt-cmd", 0, "music production environment / command ui");
  // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_WARNING);
  gst_debug_set_threshold_for_name("bt-*",GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_cmd_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);
  // no ansi color codes in logfiles please
  gst_debug_set_colored(FALSE);

  GST_INFO("================================================================================");
}

void bt_cmd_teardown(void) {
  GST_INFO("................................................................................");
  GST_INFO("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::");
}

/* start the test run */
int main(int argc, char **argv) {
  int nf; 
  SRunner *sr;

  // initialize as soon as possible
  if(!g_thread_supported()) {
    g_thread_init(NULL);
  }
  g_type_init();
  setup_log(argc,argv);
  setup_log_capture();
  test_argv[0]=test_arg0;
  test_argvptr=test_argv;
  
  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL);
  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);
  
  sr=srunner_create(bt_cmd_application_suite());
  // this make tracing errors with gdb easier
  //srunner_set_fork_status(sr,CK_NOFORK);
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

  return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}
