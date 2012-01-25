/* Buzztard
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

/* TODO(ensonic): try gdk_error_trap_push() to use FORK mode and trap XErrors
 */

#define BT_CHECK

#include "bt-check.h"
#include "../src/ui/edit/bt-edit.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN(btic_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_edit_debug);

extern Suite *bt_about_dialog_suite(void);
extern Suite *bt_change_log_suite(void);
extern Suite *bt_controller_learn_dialog_suite(void);
extern Suite *bt_crash_recover_dialog_suite(void);
extern Suite *bt_edit_application_suite(void);
extern Suite *bt_interaction_controller_menu_suite(void);
extern Suite *bt_machine_actions_suite(void);
extern Suite *bt_machine_page_suite(void);
extern Suite *bt_machine_preset_properties_dialog_suite(void);
extern Suite *bt_machine_preferences_dialog_suite(void);
extern Suite *bt_machine_properties_dialog_suite(void);
extern Suite *bt_machine_rename_dialog_suite(void);
extern Suite *bt_main_window_suite(void);
extern Suite *bt_missing_framework_elements_dialog_suite(void);
extern Suite *bt_missing_song_elements_dialog_suite(void);
extern Suite *bt_pattern_page_suite(void);
extern Suite *bt_pattern_properties_dialog_suite(void);
extern Suite *bt_render_dialog_suite(void);
extern Suite *bt_render_progress_dialog_suite(void);
extern Suite *bt_sequence_page_suite(void);
extern Suite *bt_settings_dialog_suite(void);
extern Suite *bt_signal_analysis_dialog_suite(void);
extern Suite *bt_tip_dialog_suite(void);

gint test_argc=2;
gchar test_arg0[]="check_buzzard";
gchar test_arg1[]="--sync";
gchar *test_argv[2];
gchar **test_argvptr;

static BtSettings *settings;

/* common setup and teardown code */

static void cleanup_cache_dir(void) {
  /* clean up cache dir */
  if(g_file_test("buzztard",G_FILE_TEST_IS_DIR)) {
    GDir *dir;
    const gchar *log_name;
    gchar log_path[FILENAME_MAX];

    if((dir=g_dir_open("buzztard",0,NULL))) {
      while((log_name=g_dir_read_name(dir))) {
        if(!g_str_has_suffix(log_name,".log")) {
          GST_WARNING("unexpected file %s found in temp log dir",log_name);
          continue;
        }
        g_sprintf(log_path,"buzztard"G_DIR_SEPARATOR_S"%s",log_name);
        g_remove(log_path);
      }
      g_dir_close(dir);
      g_rmdir("buzztard");
    }
  }
}

void bt_edit_setup(void) {
  gtk_init(&test_argc,&test_argvptr);
  bt_init(&test_argc,&test_argvptr);
  btic_init(&test_argc,&test_argvptr);
  add_pixmap_directory(".."G_DIR_SEPARATOR_S"pixmaps"G_DIR_SEPARATOR_S);
  /* TODO(ensonic): we need to ensure icons are found when running uninstalled
   * one problem is that we have them under "pixmaps" and not "icons"
   * maybe we should rename in the repo
  theme=gtk_icon_theme_get_default()
  gtk_icon_theme_append_search_path(theme,....);
  */
  bt_check_init();

  GST_DEBUG_CATEGORY_INIT(bt_edit_debug, "bt-edit", 0, "music production environment / editor ui");
   // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_WARNING);
  gst_debug_set_threshold_for_name("bt-*",GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(btic_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_edit_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);
  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);  
  
  /* cleanup cache dir before (first) test run */
  cleanup_cache_dir();
  GST_INFO("................................................................................");
  check_setup_test_display();
  GST_INFO("................................................................................");

  /* set some good settings for the tests */
  settings=bt_settings_make();
  GST_INFO("tests have settings %p",settings);
  g_object_set(settings,"show-tips",FALSE,NULL);
}

void bt_edit_teardown(void) {
  if (settings) {
    g_object_unref(settings);
    settings=NULL;
  }

  GST_INFO("................................................................................");
  check_shutdown_test_display();
  /* cleanup cache dir after test run */
  cleanup_cache_dir();
  GST_INFO("................................................................................");
}

/* start the test run */
int main(int argc, char **argv) {
  int nf;
  SRunner *sr;

#if !GLIB_CHECK_VERSION (2, 31, 0) 
  // initialize as soon as possible
  if(!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

  g_type_init();
  g_set_application_name("Buzztard");
  setup_log(argc,argv);
  setup_log_capture();
  test_argv[0]=test_arg0;
  test_argv[1]=test_arg1;
  test_argvptr=test_argv;

  check_setup_test_server();

  sr=srunner_create(bt_about_dialog_suite());
  srunner_add_suite(sr, bt_change_log_suite());
  srunner_add_suite(sr, bt_controller_learn_dialog_suite());
  srunner_add_suite(sr, bt_crash_recover_dialog_suite());
  srunner_add_suite(sr, bt_edit_application_suite());
  srunner_add_suite(sr, bt_interaction_controller_menu_suite());
  srunner_add_suite(sr, bt_machine_actions_suite());
  srunner_add_suite(sr, bt_machine_page_suite());
  srunner_add_suite(sr, bt_machine_preset_properties_dialog_suite());
  srunner_add_suite(sr, bt_machine_preferences_dialog_suite());
  srunner_add_suite(sr, bt_machine_properties_dialog_suite());
  srunner_add_suite(sr, bt_machine_rename_dialog_suite());
  srunner_add_suite(sr, bt_main_window_suite());
  srunner_add_suite(sr, bt_missing_framework_elements_dialog_suite());
  srunner_add_suite(sr, bt_missing_song_elements_dialog_suite());
  srunner_add_suite(sr, bt_pattern_page_suite());
  srunner_add_suite(sr, bt_pattern_properties_dialog_suite());
  srunner_add_suite(sr, bt_render_dialog_suite());
  srunner_add_suite(sr, bt_render_progress_dialog_suite());
  srunner_add_suite(sr, bt_sequence_page_suite());
  srunner_add_suite(sr, bt_settings_dialog_suite());
  srunner_add_suite(sr, bt_signal_analysis_dialog_suite());
  srunner_add_suite(sr, bt_tip_dialog_suite());
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

  check_shutdown_test_server();
  bt_deinit();

  return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
