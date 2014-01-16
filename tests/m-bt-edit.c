/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* TODO(ensonic): try gdk_error_trap_push() to use FORK mode and trap XErrors
 */

#define BT_CHECK

#include "bt-check.h"
#include "bt-check-ui.h"
#include "../src/ui/edit/bt-edit.h"

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN (bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN (btic_debug);
GST_DEBUG_CATEGORY_EXTERN (bt_edit_debug);

extern Suite *bt_about_dialog_suite (void);
extern Suite *bt_change_log_suite (void);
extern Suite *bt_crash_recover_dialog_suite (void);
extern Suite *bt_edit_application_suite (void);
extern Suite *bt_interaction_controller_learn_dialog_suite (void);
extern Suite *bt_interaction_controller_menu_suite (void);
extern Suite *bt_machine_actions_suite (void);
extern Suite *bt_machine_canvas_item_suite (void);
extern Suite *bt_machine_list_model_suite (void);
extern Suite *bt_machine_preset_properties_dialog_suite (void);
extern Suite *bt_machine_preferences_dialog_suite (void);
extern Suite *bt_machine_properties_dialog_suite (void);
extern Suite *bt_machine_rename_dialog_suite (void);
extern Suite *bt_main_page_info_suite (void);
extern Suite *bt_main_page_machines_suite (void);
extern Suite *bt_main_page_patterns_suite (void);
extern Suite *bt_main_page_sequence_suite (void);
extern Suite *bt_main_pages_suite (void);
extern Suite *bt_main_window_suite (void);
extern Suite *bt_missing_framework_elements_dialog_suite (void);
extern Suite *bt_missing_song_elements_dialog_suite (void);
extern Suite *bt_object_list_model_suite (void);
extern Suite *bt_pattern_list_model_suite (void);
extern Suite *bt_pattern_properties_dialog_suite (void);
extern Suite *bt_render_dialog_suite (void);
extern Suite *bt_sequence_grid_model_suite (void);
extern Suite *bt_settings_dialog_suite (void);
extern Suite *bt_signal_analysis_dialog_suite (void);
extern Suite *bt_tip_dialog_suite (void);
extern Suite *bt_ui_resources_suite (void);
extern Suite *bt_wave_list_model_suite (void);
extern Suite *bt_wavelevel_list_model_suite (void);
extern Suite *bt_wire_canvas_item_suite (void);

gchar *test_argv[] = { "check_buzzard" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

static BtSettings *settings;

/* common setup and teardown code */

static void
cleanup_cache_dir (void)
{
  /* clean up cache dir */
  if (g_file_test ("buzztrax", G_FILE_TEST_IS_DIR)) {
    GDir *dir;
    const gchar *log_name;
    gchar log_path[FILENAME_MAX];

    if ((dir = g_dir_open ("buzztrax", 0, NULL))) {
      while ((log_name = g_dir_read_name (dir))) {
        if (!g_str_has_suffix (log_name, ".log")) {
          GST_WARNING ("unexpected file %s found in temp log dir", log_name);
          continue;
        }
        g_sprintf (log_path, "buzztrax" G_DIR_SEPARATOR_S "%s", log_name);
        g_remove (log_path);
      }
      g_dir_close (dir);
      g_rmdir ("buzztrax");
    }
  }
}

/* we *must* call this from a checked fixture, as only this runs in the same
 * process context
 */
void
bt_edit_setup (void)
{
  GST_INFO
      ("................................................................................");
  gtk_init (&test_argc, &test_argvptr);
  if (clutter_init (&test_argc, &test_argvptr) != CLUTTER_INIT_SUCCESS)
    exit (1);
  bt_init (&test_argc, &test_argvptr);
  btic_init (&test_argc, &test_argvptr);
  add_pixmap_directory ("." G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S);
  /* TODO(ensonic): we need to ensure icons are found when running uninstalled
   * one problem is that we have them under "pixmaps" and not "icons"
   * maybe we should rename in the repo
   theme=gtk_icon_theme_get_default()
   gtk_icon_theme_append_search_path(theme,....);
   */
  bt_check_init ();

  GST_DEBUG_CATEGORY_INIT (bt_edit_debug, "bt-edit", 0,
      "music production environment / editor ui");

  /* cleanup cache dir before (first) test run */
  cleanup_cache_dir ();
  GST_INFO
      ("................................................................................");
  check_setup_test_display ();
  GST_INFO
      ("................................................................................");

  /* set some good settings for the tests */
  settings = bt_settings_make ();
  GST_INFO ("tests have settings %p", settings);
  g_object_set (settings, "show-tips", FALSE, NULL);
}

void
bt_edit_teardown (void)
{
  GST_INFO
      ("................................................................................");
  if (settings) {
    g_object_unref (settings);
    settings = NULL;
  }

  GST_INFO
      ("................................................................................");
  check_shutdown_test_display ();
  /* cleanup cache dir after test run */
  cleanup_cache_dir ();
  GST_INFO
      ("................................................................................");
}

/* start the test run */
gint
main (gint argc, gchar ** argv)
{
  gint nf;
  SRunner *sr;

  g_type_init ();
  g_set_application_name ("Buzztrax");
  setup_log_base (argc, argv);
  setup_log_capture ();

  check_setup_test_server ();

  sr = srunner_create (bt_about_dialog_suite ());
  srunner_add_suite (sr, bt_change_log_suite ());
  srunner_add_suite (sr, bt_crash_recover_dialog_suite ());
  srunner_add_suite (sr, bt_edit_application_suite ());
  srunner_add_suite (sr, bt_interaction_controller_learn_dialog_suite ());
  srunner_add_suite (sr, bt_interaction_controller_menu_suite ());
  srunner_add_suite (sr, bt_machine_actions_suite ());
  srunner_add_suite (sr, bt_machine_list_model_suite ());
  srunner_add_suite (sr, bt_machine_canvas_item_suite ());
  srunner_add_suite (sr, bt_machine_preset_properties_dialog_suite ());
  srunner_add_suite (sr, bt_machine_preferences_dialog_suite ());
  srunner_add_suite (sr, bt_machine_properties_dialog_suite ());
  srunner_add_suite (sr, bt_machine_rename_dialog_suite ());
  srunner_add_suite (sr, bt_main_page_info_suite ());
  srunner_add_suite (sr, bt_main_page_machines_suite ());
  srunner_add_suite (sr, bt_main_page_patterns_suite ());
  srunner_add_suite (sr, bt_main_page_sequence_suite ());
  srunner_add_suite (sr, bt_main_pages_suite ());
  srunner_add_suite (sr, bt_main_window_suite ());
  srunner_add_suite (sr, bt_missing_framework_elements_dialog_suite ());
  srunner_add_suite (sr, bt_missing_song_elements_dialog_suite ());
  srunner_add_suite (sr, bt_object_list_model_suite ());
  srunner_add_suite (sr, bt_pattern_list_model_suite ());
  srunner_add_suite (sr, bt_pattern_properties_dialog_suite ());
  srunner_add_suite (sr, bt_render_dialog_suite ());
  srunner_add_suite (sr, bt_sequence_grid_model_suite ());
  srunner_add_suite (sr, bt_settings_dialog_suite ());
  srunner_add_suite (sr, bt_signal_analysis_dialog_suite ());
  srunner_add_suite (sr, bt_tip_dialog_suite ());
  srunner_add_suite (sr, bt_ui_resources_suite ());
  srunner_add_suite (sr, bt_wave_list_model_suite ());
  srunner_add_suite (sr, bt_wavelevel_list_model_suite ());
  srunner_add_suite (sr, bt_wire_canvas_item_suite ());
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  check_shutdown_test_server ();
  bt_deinit ();

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
