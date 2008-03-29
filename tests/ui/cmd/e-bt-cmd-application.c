/* $Id: e-bt-cmd-application.c,v 1.12 2007-07-11 20:41:31 ensonic Exp $
 *
 * Buzztard
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

BT_START_TEST(test_create_app) {
  BtCmdApplication *app;

  app=bt_cmd_application_new(TRUE);
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

  app=bt_cmd_application_new(TRUE);
  ret = bt_cmd_application_play(app, check_get_test_song_path("test-simple1.xml"));
  fail_unless(ret==TRUE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// postive test, this test should not fail
BT_START_TEST(test_play2) {
  BtCmdApplication *app;
  gboolean ret=FALSE;

  app=bt_cmd_application_new(TRUE);
  ret = bt_cmd_application_play(app, check_get_test_song_path("test-simple2.xml"));
  fail_unless(ret==TRUE, NULL);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

// Tests to play one song after another
// This is a positive test.
BT_START_TEST(test_play3) {
  BtCmdApplication *app;
  gboolean ret=FALSE;

  app=bt_cmd_application_new(TRUE);

  ret = bt_cmd_application_play(app, check_get_test_song_path("test-simple1.xml"));
  fail_unless(ret==TRUE, NULL);

  ret = bt_cmd_application_play(app, check_get_test_song_path("test-simple2.xml"));
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

  app=bt_cmd_application_new(TRUE);
  tmp_file_name=g_build_filename(g_get_tmp_dir(),"test-simple1.xml.txt",NULL);
  ret = bt_cmd_application_info(app, check_get_test_song_path("test-simple1.xml"), tmp_file_name);
  fail_unless(ret==TRUE, NULL);
  fail_unless(file_contains_str(tmp_file_name, "song.song_info.name: \"test simple 1\""), NULL);
  // remove tmp-file and free filename
  g_unlink(tmp_file_name);
  g_free(tmp_file_name);
  // free application
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_cmd_application_example_case(void) {
  TCase *tc = tcase_create("BtCmdApplicationExamples");

  tcase_add_test(tc,test_create_app);
  tcase_add_test(tc,test_play1);
  tcase_add_test(tc,test_play2);
  tcase_add_test(tc,test_play3);
  tcase_add_test(tc,test_info1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
