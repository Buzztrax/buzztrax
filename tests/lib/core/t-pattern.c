/* $Id: t-pattern.c,v 1.4 2005-09-13 18:51:00 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
	bt_core_teardown();
}

//-- tests

START_TEST(test_btpattern_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtPattern *pattern=NULL;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  
  // assertions make test abort (signal 6) :(
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",1L,NULL);
  fail_unless(pattern == NULL, NULL);
  g_object_try_unref(pattern);

  pattern=bt_pattern_new(song,NULL,"pattern-name",1L,NULL);
  fail_unless(pattern == NULL, NULL);
  g_object_try_unref(pattern);

  pattern=bt_pattern_new(song,NULL,NULL,1L,NULL);
  fail_unless(pattern == NULL, NULL);
  g_object_try_unref(pattern);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
END_TEST

TCase *bt_pattern_test_case(void) {
  TCase *tc = tcase_create("BtPatternTests");

  tcase_add_test_raise_signal(tc,test_btpattern_obj1,SIGABRT);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
