/* $Id: m-bt-core.c,v 1.6 2005-05-25 13:32:00 ensonic Exp $
 * core library unit tests
 */

#define BT_CHECK
 
#include "bt-check.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

extern Suite *bt_core_suite(void);
extern Suite *bt_machine_suite(void);
extern Suite *bt_network_suite(void);
extern Suite *bt_processor_machine_suite(void);
extern Suite *bt_sequence_suite(void);
extern Suite *bt_setup_suite(void);
extern Suite *bt_sink_machine_suite(void);
extern Suite *bt_song_suite(void);
extern Suite *bt_song_io_suite(void);
extern Suite *bt_song_info_suite(void);
extern Suite *bt_source_machine_suite(void);
extern Suite *bt_timeline_suite(void);
extern Suite *bt_wire_suite(void);
extern Suite *bt_settings_suite(void);

guint test_argc=1;
gchar *test_arg0="check_buzzard";
gchar *test_argv[1];
gchar **test_argvptr;

/* start the test run */
int main(int argc, char **argv) {
  int nf; 
  SRunner *sr;

	//g_mem_set_vtable(glib_mem_profiler_table);

  setup_log(argc,argv);
	setup_log_capture();
  test_argv[0]=test_arg0;
  test_argvptr=test_argv;
  
  g_type_init();
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-check", 0, "music production environment / unit tests");
  gst_debug_set_threshold_for_name("GST_*",GST_LEVEL_DEBUG); // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_threshold_for_name("bt-*",GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_check_debug,GST_LEVEL_DEBUG);
	gst_debug_set_colored(FALSE);

  sr=srunner_create(bt_core_suite());
	srunner_add_suite(sr, bt_machine_suite());
	srunner_add_suite(sr, bt_network_suite());
	srunner_add_suite(sr, bt_processor_machine_suite());
	srunner_add_suite(sr, bt_sequence_suite());
	srunner_add_suite(sr, bt_setup_suite());
	srunner_add_suite(sr, bt_sink_machine_suite());
	srunner_add_suite(sr, bt_song_suite());
	srunner_add_suite(sr, bt_song_io_suite());
	srunner_add_suite(sr, bt_song_info_suite());
	srunner_add_suite(sr, bt_source_machine_suite());
	srunner_add_suite(sr, bt_timeline_suite());
	srunner_add_suite(sr, bt_wire_suite());
	srunner_add_suite(sr, bt_settings_suite());
  // this make tracing errors with gdb easier (use env CK_FORK="no" gdb ...)
  //srunner_set_fork_status(sr,CK_NOFORK);
	srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);
	
	//g_mem_profile();

	return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}
