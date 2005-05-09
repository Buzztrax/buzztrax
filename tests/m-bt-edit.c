/* $Id: m-bt-edit.c,v 1.3 2005-05-09 20:30:09 ensonic Exp $
 * graphical editor app unit tests
 */

#define BT_CHECK
 
#include "bt-check.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

extern Suite *bt_edit_application_suite(void);
extern Suite *bt_settings_dialog_suite(void);

guint test_argc=2;
gchar *test_arg0="check_buzzard";
gchar *test_arg1="--sync";
gchar *test_argv[1];
gchar **test_argvptr;

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
	gst_debug_set_colored(FALSE);
	
  sr=srunner_create(bt_edit_application_suite());
	srunner_add_suite(sr, bt_settings_dialog_suite());
  srunner_run_all(sr,CK_NORMAL);
  nf=srunner_ntests_failed(sr);
  srunner_free(sr);

	return(nf==0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}
