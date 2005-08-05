/* $Id: t-bt-cmd-application.c,v 1.3 2005-08-05 09:36:19 ensonic Exp $ 
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

// tests if the play method works without exceptions if we put NULL as filename.
// This test is a negative test
START_TEST(test_play1) {
  BtCmdApplication *app=NULL;
  gboolean ret=FALSE;
  GstBin *bin=NULL;
  BtSettings *settings=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  fail_unless(G_OBJECT(app)->ref_count == 1, NULL);
  ret = bt_cmd_application_play(app, NULL);
  fail_unless(ret==FALSE, NULL);
  g_object_get(G_OBJECT(app),"bin",&bin,"settings",&settings,NULL);
  fail_unless(bin!=NULL, NULL);
  fail_unless(settings!=NULL, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST

// file not found test. this is a negative test
START_TEST(test_play2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  ret = bt_cmd_application_play(app, "");
  fail_unless(ret==FALSE, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST

// test if the info method works with NULL argument for the filename,
// This is a negative test
START_TEST(test_info1) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  ret = bt_cmd_application_info(app, NULL, NULL);
  fail_unless(ret==FALSE, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST

// test if the info method works with a empty filename.
// This is a negative test
START_TEST(test_info2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  GST_INFO("--------------------------------------------------------------------------------");

  app=bt_cmd_application_new();
  ret = bt_cmd_application_info(app, "", NULL);
  fail_unless(ret==FALSE, NULL);
  // free application
  g_object_checked_unref(app);
}
END_TEST


TCase *bt_cmd_application_test_case(void) {
  TCase *tc = tcase_create("BtCmdApplicationTests");

  tcase_add_test(tc,test_play1);
  tcase_add_test(tc,test_play2);   
  tcase_add_test(tc,test_info1);
  tcase_add_test(tc,test_info2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
