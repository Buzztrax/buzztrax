/* $Id: e-bt-settings-dialog.c,v 1.6 2005-07-22 23:12:22 ensonic Exp $
 */

#include "m-bt-edit.h"

//-- globals

//-- fixtures

static void test_setup(void) {
	g_type_init();
	if(!g_thread_supported()) {	// are g_threads() already initialized
		g_thread_init(NULL);
	}
	gdk_threads_init();
	bt_threads_init();

	gtk_init(NULL,NULL);
  bt_init(NULL,NULL,NULL);
  add_pixmap_directory(".."G_DIR_SEPARATOR_S"pixmaps"G_DIR_SEPARATOR_S);

	setup_log_capture();
  //g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
	g_log_set_always_fatal(G_LOG_LEVEL_ERROR);
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_WARNING); // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("bt-*",GST_LEVEL_DEBUG);

	GST_DEBUG_CATEGORY_INIT(bt_edit_debug, "bt-edit", 0, "music production environment / editor ui");
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_edit_debug,GST_LEVEL_DEBUG);
	gst_debug_set_colored(FALSE);

	gdk_threads_try_enter();

  GST_INFO("================================================================================");
}

static void test_teardown(void) {
	gdk_threads_try_leave();
}

//-- tests

// create app and then unconditionally destroy window
START_TEST(test_create_dialog) {
	BtEditApplication *app;
	BtMainWindow *main_window;
	GtkWidget *dialog;

  GST_INFO("--------------------------------------------------------------------------------");

	app=bt_edit_application_new();
	GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
	fail_unless(app != NULL, NULL);

	// get window
	g_object_get(app,"main-window",&main_window,NULL);
	fail_unless(main_window != NULL, NULL);

  // create, show and destroy dialog
	dialog=GTK_WIDGET(bt_settings_dialog_new(app));
	gtk_widget_show_all(dialog);
	// leave out that line! (modal dialog)
	//gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	
	// close window
	g_object_unref(main_window);
	gtk_widget_destroy(GTK_WIDGET(main_window));
	while(gtk_events_pending()) gtk_main_iteration();
	
	// free application
	GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);	
}
END_TEST

TCase *bt_settings_dialog_example_case(void) {
  TCase *tc = tcase_create("BtSettingsDialogExamples");
	
  tcase_add_test(tc,test_create_dialog);
  // we *must* use a checked fixture, as only this runns in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
	// we need to disable the default timeout of 3 seconds a little
	tcase_set_timeout(tc, 0);
  return(tc);
}
