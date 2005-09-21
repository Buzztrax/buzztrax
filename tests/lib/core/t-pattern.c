/* $Id: t-pattern.c,v 1.7 2005-09-21 19:46:04 ensonic Exp $
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

BT_START_TEST(test_btpattern_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtPattern *pattern=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  
  check_init_error_trapp("bt_pattern_new","BT_IS_MACHINE(machine)");
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",1L,NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(pattern == NULL, NULL);

  check_init_error_trapp("bt_pattern_new","BT_IS_STRING(id)");
  pattern=bt_pattern_new(song,NULL,"pattern-name",1L,NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(pattern == NULL, NULL);

  check_init_error_trapp("bt_pattern_new","BT_IS_STRING(name)");
  pattern=bt_pattern_new(song,"pattern-id",NULL,1L,NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(pattern == NULL, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_pattern_test_case(void) {
  TCase *tc = tcase_create("BtPatternTests");

  tcase_add_test(tc,test_btpattern_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
