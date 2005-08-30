/* $Id: m-bt-edit.c,v 1.11 2005-08-30 21:12:20 ensonic Exp $
 * graphical editor app unit tests
 */

#define BT_CHECK
 
#include "bt-check.h"
#include "../src/ui/edit/bt-edit.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_edit_debug);

extern Suite *bt_edit_application_suite(void);
extern Suite *bt_pattern_properties_dialog_suite(void);
extern Suite *bt_settings_dialog_suite(void);

guint test_argc=2;
gchar *test_arg0="check_buzzard";
gchar *test_arg1="--sync";
gchar *test_argv[1];
gchar **test_argvptr;

void bt_edit_setup(void) {
  g_type_init();
	/*
  if(!g_thread_supported()) {  // are g_threads() already initialized
    g_thread_init(NULL);
  }
  gdk_threads_init();
  bt_threads_init();
	*/

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

  check_setup_test_display();

  //gdk_threads_try_enter();
	
  GST_INFO("================================================================================");
}

void bt_edit_teardown(void) {
  GST_INFO("................................................................................");
  //gdk_threads_try_leave();

  check_shutdown_test_display();
  GST_INFO("::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::");
}

/* start the test run */
int main(int argc, char **argv) {
  int nf; 
  SRunner *sr;

  g_type_init();
  setup_log(argc,argv);
  test_argv[0]=test_arg0;
  test_argv[1]=test_arg1;
  test_argvptr=test_argv;
 
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-check", 0, "music production environment / unit tests");
  gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);

	/*
  if(!g_thread_supported()) {  // are g_threads() already initialized
    g_thread_init(NULL);
  }
  gdk_threads_init();
	*/
  gtk_init(NULL,NULL);
  check_setup_test_server();
  
  sr=srunner_create(bt_edit_application_suite());
  srunner_add_suite(sr, bt_pattern_properties_dialog_suite());
  srunner_add_suite(sr, bt_settings_dialog_suite());
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

  check_shutdown_test_server();
	
  return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}
