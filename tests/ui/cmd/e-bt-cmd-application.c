/* $Id: e-bt-cmd-application.c,v 1.5 2005-09-14 10:16:34 ensonic Exp $ 
 */

#include "m-bt-cmd.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_cmd_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_cmd_teardown();
}

//-- tests

BT_START_TEST(test_create_app) {
  BtCmdApplication *app;

  app=bt_cmd_application_new();
  fail_unless(app != NULL, NULL);
  fail_unless(G_OBJECT(app)->ref_count == 1, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST


// postive test, this test should not fail
BT_START_TEST(test_play1) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  app=bt_cmd_application_new();
  ret = bt_cmd_application_play(app, "songs/test-simple1.xml");
  fail_unless(ret==TRUE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// postive test, this test should not fail
BT_START_TEST(test_play2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  app=bt_cmd_application_new();
  ret = bt_cmd_application_play(app, "songs/test-simple2.xml");
  fail_unless(ret==TRUE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// Tests if the info method works as expected.
// This is a positive test.
BT_START_TEST(test_info1) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  gchar *tmp_file_name;
    
  app=bt_cmd_application_new();
  tmp_file_name=tmpnam(NULL);
  ret = bt_cmd_application_info(app, "songs/test-simple1.xml", tmp_file_name);
  fail_unless(ret==TRUE, NULL);
  fail_unless(file_contains_str(tmp_file_name, "song.song_info.name: \"test simple 1\""), NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_cmd_application_example_case(void) {
  TCase *tc = tcase_create("BtCmdApplicationExamples");

  tcase_add_test(tc,test_create_app);
  tcase_add_test(tc,test_play1);
  tcase_add_test(tc,test_play2);
  tcase_add_test(tc,test_info1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
