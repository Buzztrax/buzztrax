/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * graphical editor app unit tests
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

/* @todo: try gdk_error_trap_push() to use FORK mode and trap XErrors
 */

#define BT_CHECK

#include "bt-check.h"
#include "../src/ui/edit/bt-edit.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_ic_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_edit_debug);

extern Suite *bt_about_dialog_suite(void);
extern Suite *bt_edit_application_suite(void);
extern Suite *bt_pattern_page_suite(void);
extern Suite *bt_machine_preset_properties_dialog_suite(void);
extern Suite *bt_machine_preferences_dialog_suite(void);
extern Suite *bt_machine_properties_dialog_suite(void);
extern Suite *bt_machine_rename_dialog_suite(void);
extern Suite *bt_missing_framework_elements_dialog_suite(void);
extern Suite *bt_missing_song_elements_dialog_suite(void);
extern Suite *bt_pattern_properties_dialog_suite(void);
extern Suite *bt_render_dialog_suite(void);
extern Suite *bt_settings_dialog_suite(void);
extern Suite *bt_wire_analysis_dialog_suite(void);

gint test_argc=2;
gchar test_arg0[]="check_buzzard";
gchar test_arg1[]="--sync";
gchar *test_argv[2];
gchar **test_argvptr;

/* common setup and teardown code */
void bt_edit_setup(void) {
  //g_type_init();
#if 1
  gtk_init(&test_argc,&test_argvptr);
  bt_init(&test_argc,&test_argvptr);
  btic_init(&test_argc,&test_argvptr);
  add_pixmap_directory(".."G_DIR_SEPARATOR_S"pixmaps"G_DIR_SEPARATOR_S);
  bt_check_init();
  /* @todo: we need to ensure icons are found when running uninstalled
   * one problem is that we have them under "pixmaps" and not "icons"
   * maybe we should rename in svn
  theme=gtk_icon_theme_get_default()
  gtk_icon_theme_append_search_path(theme,....);
  */
#endif

  ///setup_log_capture();
  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL);
  //g_log_set_always_fatal(G_LOG_LEVEL_ERROR);

  GST_DEBUG_CATEGORY_INIT(bt_edit_debug, "bt-edit", 0, "music production environment / editor ui");
   // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_WARNING);
  gst_debug_set_threshold_for_name("bt-*",GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_ic_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_edit_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);

  check_setup_test_display();
  GST_INFO("================================================================================");
}

void bt_edit_teardown(void) {
  GST_INFO("................................................................................");
  check_shutdown_test_display();
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
  test_argv[1]=test_arg1;
  test_argvptr=test_argv;

  //GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-check", 0, "music production environment / unit tests");
  ///gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);
  g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);

  check_setup_test_server();
#if 0
  gtk_init(&test_argc,&test_argvptr);
  bt_init(&test_argc,&test_argvptr);
  btic_init(&test_argc,&test_argvptr);
  add_pixmap_directory(".."G_DIR_SEPARATOR_S"pixmaps"G_DIR_SEPARATOR_S);
  bt_check_init();
#endif

  sr=srunner_create(bt_about_dialog_suite());
  srunner_add_suite(sr, bt_edit_application_suite());
  srunner_add_suite(sr, bt_pattern_page_suite());
  srunner_add_suite(sr, bt_machine_preset_properties_dialog_suite());
  srunner_add_suite(sr, bt_machine_preferences_dialog_suite());
  srunner_add_suite(sr, bt_machine_properties_dialog_suite());
  srunner_add_suite(sr, bt_machine_rename_dialog_suite());
  srunner_add_suite(sr, bt_missing_framework_elements_dialog_suite());
  srunner_add_suite(sr, bt_missing_song_elements_dialog_suite());
  srunner_add_suite(sr, bt_pattern_properties_dialog_suite());
  srunner_add_suite(sr, bt_render_dialog_suite());
  srunner_add_suite(sr, bt_settings_dialog_suite());
  srunner_add_suite(sr, bt_wire_analysis_dialog_suite());
  //srunner_set_fork_status(sr,CK_NOFORK);
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

  check_shutdown_test_server();

  return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
