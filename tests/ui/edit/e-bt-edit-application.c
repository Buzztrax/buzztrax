/* $Id: e-bt-edit-application.c,v 1.7 2005-07-22 13:26:03 ensonic Exp $ 
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

//-- helper

/*
static gboolean timeout(gpointer data) {
  GST_INFO("shutting down the gui");
  gtk_main_quit();
	//gtk_widget_destroy(GTK_WIDGET(data));
  return FALSE;
}
*/

//-- tests

// create app and then unconditionally destroy window
START_TEST(test_create_app) {
	BtEditApplication *app;
	BtMainWindow *main_window;

  GST_INFO("--------------------------------------------------------------------------------");

	app=bt_edit_application_new();
	GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
	fail_unless(app != NULL, NULL);

	// get window and close it
	g_object_get(app,"main-window",&main_window,NULL);
	fail_unless(main_window != NULL, NULL);
	GST_INFO("main_window->ref_ct=%d",G_OBJECT(main_window)->ref_count);
	g_object_unref(main_window);
	GST_INFO("main_window->ref_ct=%d",G_OBJECT(main_window)->ref_count);

	// needs a main-loop (version 1,2)
	gtk_widget_destroy(GTK_WIDGET(main_window));
	// version 1
	while(gtk_events_pending()) gtk_main_iteration();
	//GST_INFO("mainlevel is %d",gtk_main_level());
	//while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);
	// after this loop the window should be gone
	
	// version 2 (makes the window visible :( )
	//g_timeout_add(2000,timeout,main_window);
  //gtk_main();

	// version 3 (does not work)
	//g_object_checked_unref(main_window);
	
	// free application
	GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);	
}
END_TEST

// create a new song
/* fails with gtk-2.4
 * -> when destroying menus one gets
 * (bt-edit:22237): Gtk-WARNING **: mnemonic "g" wasn't removed for widget (0x856cd40)
 */
START_TEST(test_new1) {
	BtEditApplication *app;
	BtMainWindow *main_window;
	BtSong *song;

  GST_INFO("--------------------------------------------------------------------------------");

	app=bt_edit_application_new();
	GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
	fail_unless(app != NULL, NULL);

	// create a new song
	bt_edit_application_new_song(app);
	g_object_get(app,"song",&song,NULL);
	fail_unless(song != NULL, NULL);
	g_object_unref(song);
	GST_INFO("song created");
	
	// get window and close it
	g_object_get(app,"main-window",&main_window,NULL);
	fail_unless(main_window != NULL, NULL);

	// close window
	g_object_unref(main_window);
	gtk_widget_destroy(GTK_WIDGET(main_window));
	while(gtk_events_pending()) gtk_main_iteration();
	//GST_INFO("mainlevel is %d",gtk_main_level());
	//while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);
	
	// free application
	GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);
}
END_TEST

// load a song
START_TEST(test_load1) {
	BtEditApplication *app;
	BtMainWindow *main_window;
	BtSong *song;

  GST_INFO("--------------------------------------------------------------------------------");

	app=bt_edit_application_new();
	GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
	fail_unless(app != NULL, NULL);
	
	bt_edit_application_load_song(app,"songs/test-simple1.xml");
	g_object_get(app,"song",&song,NULL);
	fail_unless(song != NULL, NULL);
	g_object_unref(song);
	GST_INFO("song loaded");

	// get window and close it
	g_object_get(app,"main-window",&main_window,NULL);
	fail_unless(main_window != NULL, NULL);

	// close window
	g_object_unref(main_window);
	gtk_widget_destroy(GTK_WIDGET(main_window));
	while(gtk_events_pending()) gtk_main_iteration();
	//GST_INFO("mainlevel is %d",gtk_main_level());
	//while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

	// free application
	GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);
	
}
END_TEST

// view all tabs
START_TEST(test_tabs1) {
	BtEditApplication *app;
	BtMainWindow *main_window;
	BtMainPages *pages;
	BtSong *song;
	guint i,num_pages;

  GST_INFO("--------------------------------------------------------------------------------");

	app=bt_edit_application_new();
	GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
	fail_unless(app != NULL, NULL);
	
	bt_edit_application_load_song(app,"songs/test-simple1.xml");
	g_object_get(app,"song",&song,NULL);
	fail_unless(song != NULL, NULL);
	g_object_unref(song);
	GST_INFO("song loaded");

	// get window
	g_object_get(app,"main-window",&main_window,NULL);
	fail_unless(main_window != NULL, NULL);
	
	// view all tabs
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
	num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(pages));
	for(i=0;i<num_pages;i++) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),i);
		while(gtk_events_pending()) gtk_main_iteration();
	}
	g_object_unref(pages);
	
	// close window
	g_object_unref(main_window);
	gtk_widget_destroy(GTK_WIDGET(main_window));
	while(gtk_events_pending()) gtk_main_iteration();
	//GST_INFO("mainlevel is %d",gtk_main_level());
	//while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

	// free application
	GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);
	
}
END_TEST

TCase *bt_edit_application_example_case(void) {
  TCase *tc = tcase_create("BtEditApplicationExamples");
	
  tcase_add_test(tc,test_create_app);
  tcase_add_test(tc,test_new1);
  tcase_add_test(tc,test_load1);
	tcase_add_test(tc,test_tabs1);
  // we *must* use a checked fixture, as only this runns in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
	// we need to disable the default timeout of 3 seconds
	tcase_set_timeout(tc, 0);
  return(tc);
}
