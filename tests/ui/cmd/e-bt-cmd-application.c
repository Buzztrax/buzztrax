/* $Id: e-bt-cmd-application.c,v 1.3 2005-08-05 09:36:19 ensonic Exp $ 
 */

#include "m-bt-cmd.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  GST_DEBUG_CATEGORY_INIT(bt_cmd_debug, "bt-cmd", 0, "music production environment / command ui");
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  gst_debug_category_set_threshold(bt_cmd_debug,GST_LEVEL_DEBUG);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {  
}

//-- tests

START_TEST(test_create_app) {
  BtCmdApplication *app;

  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  fail_unless(app != NULL, NULL);
  fail_unless(G_OBJECT(app)->ref_count == 1, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST


// postive test, this test should not fail
START_TEST(test_play1) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  ret = bt_cmd_application_play(app, "songs/test-simple1.xml");
  fail_unless(ret==TRUE, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST

// postive test, this test should not fail
START_TEST(test_play2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  ret = bt_cmd_application_play(app, "songs/test-simple2.xml");
  fail_unless(ret==TRUE, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST

// Tests if the info method works as expected.
// This is a positive test.
START_TEST(test_info1) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  gchar *tmp_file_name;
    
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  tmp_file_name=tmpnam(NULL);
  ret = bt_cmd_application_info(app, "songs/test-simple1.xml", tmp_file_name);
  fail_unless(ret==TRUE, NULL);
  fail_unless(file_contains_str(tmp_file_name, "song.song_info.name: \"test simple 1\""), NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST

TCase *bt_cmd_application_example_case(void) {
  TCase *tc = tcase_create("BtCmdApplicationExamples");

  tcase_add_test(tc,test_create_app);
  tcase_add_test(tc,test_play1);
  tcase_add_test(tc,test_play2);
  tcase_add_test(tc,test_info1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
