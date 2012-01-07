/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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

// tests if the play method works without exceptions if we put NULL as filename.
// This test is a negative test
BT_START_TEST(test_play1) {
  BtCmdApplication *app=NULL;
  gboolean ret=FALSE;
  GstBin *bin=NULL;
  BtSettings *settings=NULL;
  
  app=bt_cmd_application_new(TRUE);
  fail_unless(G_OBJECT_REF_COUNT(app) == 1, NULL);
  
  check_init_error_trapp("bt_cmd_application_play","BT_IS_STRING(input_file_name)");
  ret = bt_cmd_application_play(app, NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(ret==FALSE, NULL);
  
  g_object_get(G_OBJECT(app),"bin",&bin,"settings",&settings,NULL);
  fail_unless(bin!=NULL, NULL);
  fail_unless(settings!=NULL, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// file not found test. this is a negative test
BT_START_TEST(test_play2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  app=bt_cmd_application_new(TRUE);
  
  check_init_error_trapp("bt_cmd_application_play","BT_IS_STRING(input_file_name)");
  ret = bt_cmd_application_play(app, "");
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(ret==FALSE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// test if the info method works with NULL argument for the filename,
// This is a negative test
BT_START_TEST(test_info1) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  app=bt_cmd_application_new(TRUE);
  
  check_init_error_trapp("bt_cmd_application_info","BT_IS_STRING(input_file_name)");
  ret = bt_cmd_application_info(app, NULL, NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(ret==FALSE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// test if the info method works with a empty filename.
// This is a negative test
BT_START_TEST(test_info2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;
  
  app=bt_cmd_application_new(TRUE);
  
  check_init_error_trapp("bt_cmd_application_info","BT_IS_STRING(input_file_name)");
  ret = bt_cmd_application_info(app, "", NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(ret==FALSE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_cmd_application_test_case(void) {
  TCase *tc = tcase_create("BtCmdApplicationTests");

  tcase_add_test(tc,test_play1);
  tcase_add_test(tc,test_play2);   
  tcase_add_test(tc,test_info1);
  tcase_add_test(tc,test_info2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
