/* $Id: t-song-info.c,v 1.1 2006-08-02 19:34:20 ensonic Exp $
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
  //puts(__FILE__":teardown");
}

//-- tests

/**
* try to create a new setup with a NULL song object
*/
BT_START_TEST(test_btsonginfo_obj1) {
  BtSongInfo *song_info=NULL;
  
  check_init_error_trapp("bt_song_info_new","BT_IS_SONG(song)");
  song_info=bt_song_info_new(NULL);
  fail_unless(check_has_error_trapped(), NULL);
}
BT_END_TEST


TCase *bt_song_info_test_case(void) {
  TCase *tc = tcase_create("BtSongInfoTests");

  tcase_add_test(tc,test_btsonginfo_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  // we need to raise the default timeout of 3 seconds
  tcase_set_timeout(tc, 10);
  return(tc);
}
