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
#include <stdlib.h>
#include <glib/gstdio.h>

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN (bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN (btic_debug);
GST_DEBUG_CATEGORY_EXTERN (bt_edit_debug);

gchar *test_argv[] = { "check_buzztrax" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

BT_TEST_SUITE_E ("BtAboutDialog", bt_about_dialog);
BT_TEST_SUITE_T_E ("BtChangeLog", bt_change_log);
BT_TEST_SUITE_E ("BtCrashRecoverDialog", bt_crash_recover_dialog);
BT_TEST_SUITE_E ("BtEditApplication", bt_edit_application);
BT_TEST_SUITE_E ("BtInteractionControllerMenu", bt_interaction_controller_menu);
BT_TEST_SUITE_E ("BtMachineActions", bt_machine_actions);
BT_TEST_SUITE_E ("BtMachineCanvasItem", bt_machine_canvas_item);
BT_TEST_SUITE_E ("BtMachineListModel", bt_machine_list_model);
BT_TEST_SUITE_E ("BtMachinePreferencesDialog", bt_machine_preferences_dialog);
BT_TEST_SUITE_E ("BtMachinePresetPropertiesDialog",
    bt_machine_preset_properties_dialog);
BT_TEST_SUITE_E ("BtMachinePropertiesDialog", bt_machine_properties_dialog);
BT_TEST_SUITE_E ("BtMachineRenameDialog", bt_machine_rename_dialog);
BT_TEST_SUITE_E ("BtMainPageInfo", bt_main_page_info);
BT_TEST_SUITE_E ("BtMainPageMachines", bt_main_page_machines);
BT_TEST_SUITE_T_E ("BtMainPagePatterns", bt_main_page_patterns);
BT_TEST_SUITE_E ("BtMainPageSequence", bt_main_page_sequence);
BT_TEST_SUITE_T ("BtMainPageWaves", bt_main_page_waves);
BT_TEST_SUITE_E ("BtMainPages", bt_main_pages);
BT_TEST_SUITE_E ("BtMainWindow", bt_main_window);
BT_TEST_SUITE_E ("BtMissingFrameworkElementsDialog",
    bt_missing_framework_elements_dialog);
BT_TEST_SUITE_E ("BtMissingSongElementsDialog",
    bt_missing_song_elements_dialog);
BT_TEST_SUITE_T_E ("BtObjectListModel", bt_object_list_model);
BT_TEST_SUITE_E ("BtPatternListModel", bt_pattern_list_model);
BT_TEST_SUITE_E ("BtPatternPropertiesDialog", bt_pattern_properties_dialog);
BT_TEST_SUITE_E ("BtRenderDialog", bt_render_dialog);
BT_TEST_SUITE_E ("BtSequenceGridModel", bt_sequence_grid_model);
BT_TEST_SUITE_E ("BtSettingsDialog", bt_settings_dialog);
BT_TEST_SUITE_E ("BtSignalAnalysisDialog", bt_signal_analysis_dialog);
BT_TEST_SUITE_E ("BtTipDialog", bt_tip_dialog);
BT_TEST_SUITE_E ("BtUIResources", bt_ui_resources);
BT_TEST_SUITE_E ("BtWaveListModel", bt_wave_list_model);
BT_TEST_SUITE_E ("BtWavelevelListModel", bt_wavelevel_list_model);
BT_TEST_SUITE_E ("BtWireCanvasItem", bt_wire_canvas_item);

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
  /* BUG(749752): gdk_init() crashes when running under Xvfb.
   * it calls gdk_x11_window_foreign_new_for_display() with a window_xid=0
   * seems to be related to the xsettings manager
   *
   * Workaround: BT_CHECK_NO_XVFB=1 make bt_edit.check
   *
   * break gdk_x_error
   *
   #0  gdk_x_error (xdisplay=0x995230, error=0x7fffffffcf80) at /build/buildd/gtk+3.0-3.10.8/./gdk/x11/gdkmain-x11.c:268
   #1  0x00002aaab0a4c54b in _XError (dpy=dpy@entry=0x995230, rep=rep@entry=0x9b0800) at ../../src/XlibInt.c:1463
   #2  0x00002aaab0a495e7 in handle_error (dpy=0x995230, err=0x9b0800, in_XReply=<optimized out>) at ../../src/xcb_io.c:213
   #3  0x00002aaab0a49695 in handle_response (dpy=dpy@entry=0x995230, response=0x9b0800, in_XReply=in_XReply@entry=1) at ../../src/xcb_io.c:325
   #4  0x00002aaab0a4a578 in _XReply (dpy=dpy@entry=0x995230, rep=rep@entry=0x7fffffffd180, extra=extra@entry=0, discard=discard@entry=1) at ../../src/xcb_io.c:627
   #5  0x00002aaab0a31c74 in _XGetWindowAttributes (dpy=dpy@entry=0x995230, w=0, attr=0x7fffffffd220) at ../../src/GetWAttrs.c:114
   #6  0x00002aaab0a31de1 in XGetWindowAttributes (dpy=0x995230, w=w@entry=0, attr=attr@entry=0x7fffffffd220) at ../../src/GetWAttrs.c:149
   #7  0x00002aaaaca00f61 in gdk_x11_window_foreign_new_for_display (display=display@entry=0x9a3050, window=0) at /build/buildd/gtk+3.0-3.10.8/./gdk/x11/gdkwindow-x11.c:1216
   #8  0x00002aaaaca047c9 in check_manager_window (x11_screen=0x9aa1e0, notify_changes=notify_changes@entry=0) at /build/buildd/gtk+3.0-3.10.8/./gdk/x11/xsettings-client.c:506
   #9  0x00002aaaaca04926 in _gdk_x11_xsettings_init (x11_screen=<optimized out>) at /build/buildd/gtk+3.0-3.10.8/./gdk/x11/xsettings-client.c:580
   #10 0x00002aaaac9ec53e in _gdk_x11_display_open (display_name=<optimized out>) at /build/buildd/gtk+3.0-3.10.8/./gdk/x11/gdkdisplay-x11.c:1429
   #11 0x00002aaaac9cbe17 in gdk_display_manager_open_display (manager=<optimized out>, name=0x0) at /build/buildd/gtk+3.0-3.10.8/./gdk/gdkdisplaymanager.c:456
   #12 0x00002aaaac9cb3c5 in gdk_display_open (display_name=<optimized out>) at /build/buildd/gtk+3.0-3.10.8/./gdk/gdkdisplay.c:1799
   #13 0x00002aaaac9c3de9 in gdk_display_open_default_libgtk_only () at /build/buildd/gtk+3.0-3.10.8/./gdk/gdk.c:390
   #14 0x00002aaaac9c3dfe in gdk_init_check (argc=argc@entry=0x6e9770 <test_argc>, argv=argv@entry=0x6e9778 <test_argvptr>) at /build/buildd/gtk+3.0-3.10.8/./gdk/gdk.c:417
   #15 0x00002aaaac9c3e19 in gdk_init (argc=argc@entry=0x6e9770 <test_argc>, argv=argv@entry=0x6e9778 <test_argvptr>) at /build/buildd/gtk+3.0-3.10.8/./gdk/gdk.c:439
   #16 0x0000000000422a4a in bt_edit_setup () at tests/m-bt-edit.c:143
   #17 0x0000000000422d89 in test_setup () at tests/ui/edit/e-about-dialog.c:36
   #18 0x00000000004b0288 in tcase_run_checked_setup.isra ()
   #19 0x00000000004b02c2 in tcase_run_tfun_nofork.isra ()
   */
  //const gchar *display_name = g_getenv ("DISPLAY");
  //static gchar display_arg[100];
  //sprintf (display_arg, "--display=%s", display_name);
  //test_argv[1] = display_arg;
  gdk_init (&test_argc, &test_argvptr);
  check_setup_test_display ();

  gtk_init (&test_argc, &test_argvptr);
  if (clutter_init (&test_argc, &test_argvptr) != CLUTTER_INIT_SUCCESS)
    exit (1);
  bt_init (&test_argc, &test_argvptr);
  btic_init (&test_argc, &test_argvptr);
  /* TODO(ensonic): we need to ensure icons are found when running uninstalled
   * one problem is that we have them under "pixmaps" and not "icons"
   * maybe we should rename in the repo
   theme=gtk_icon_theme_get_default()
   gtk_icon_theme_append_search_path(theme,....);
   */
  /* TODO(ensonic); we need to ensure css files are found
   * they are loaded from DATADIR/PACKAGE/, uninstalled they would be in
   * src/ui/edit/
   */

  /* cleanup cache dir before (first) test run */
  cleanup_cache_dir ();
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

  g_set_application_name (PACKAGE_NAME);
  setup_log_base (argc, argv);
  setup_log_capture ();
  gst_init (NULL, NULL);

  bt_check_init ();

  GST_DEBUG_CATEGORY_INIT (bt_edit_debug, "bt-edit", 0,
      "music production environment / editor ui");

  check_setup_test_server ();

  sr = srunner_create (bt_about_dialog_suite ());
  srunner_add_suite (sr, bt_change_log_suite ());
  srunner_add_suite (sr, bt_crash_recover_dialog_suite ());
  srunner_add_suite (sr, bt_edit_application_suite ());
  srunner_add_suite (sr, bt_interaction_controller_menu_suite ());
  srunner_add_suite (sr, bt_machine_actions_suite ());
  srunner_add_suite (sr, bt_machine_list_model_suite ());
  srunner_add_suite (sr, bt_machine_canvas_item_suite ());
  srunner_add_suite (sr, bt_machine_preferences_dialog_suite ());
  srunner_add_suite (sr, bt_machine_preset_properties_dialog_suite ());
  srunner_add_suite (sr, bt_machine_properties_dialog_suite ());
  srunner_add_suite (sr, bt_machine_rename_dialog_suite ());
  srunner_add_suite (sr, bt_main_page_info_suite ());
  srunner_add_suite (sr, bt_main_page_machines_suite ());
  srunner_add_suite (sr, bt_main_page_patterns_suite ());
  srunner_add_suite (sr, bt_main_page_sequence_suite ());
  srunner_add_suite (sr, bt_main_page_waves_suite ());
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
  collect_logs (nf == 0);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
