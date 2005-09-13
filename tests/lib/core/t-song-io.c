/* $Id: t-song-io.c,v 1.12 2005-09-13 22:12:13 ensonic Exp $
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

// try to create a songIO object with NULL pointer
BT_START_TEST(test_btsong_io_obj1) {
  BtSongIO *songIO;
  
  songIO=bt_song_io_new(NULL);
  fail_unless(songIO == NULL, NULL);
}
BT_END_TEST

// try to create a songIO object with empty string
BT_START_TEST(test_btsong_io_obj2) {
  BtSongIO *songIO;

  songIO=bt_song_io_new("");
  fail_unless(songIO==NULL, NULL);
}
BT_END_TEST

// try to create a songIO object with wrong song name
BT_START_TEST(test_btsong_io_obj3) {
  BtSongIO *songIO;

  songIO=bt_song_io_new("test");
  fail_unless(songIO==NULL, NULL);
}
BT_END_TEST

// try to create a songIO object with a nativ file name
// this test is a positiv test
BT_START_TEST(test_btsong_io_obj4) {
  BtSongIO *songIO;
  
  songIO=bt_song_io_new("test.xml");
  // check if the type of songIO is native
  fail_unless(BT_IS_SONG_IO_NATIVE(songIO), NULL);
  fail_unless(songIO!=NULL, NULL);
  g_object_checked_unref(songIO);
}
BT_END_TEST

// try to create a songIO object with a file name that contains mixed case letters
// this test is a positiv test
BT_START_TEST(test_btsong_io_obj5) {
  BtSongIO *songIO;
  
  songIO=bt_song_io_new("test.XmL");
  // check if the type of songIO is native
  fail_unless(BT_IS_SONG_IO_NATIVE(songIO), NULL);
  fail_unless(songIO!=NULL, NULL);
  g_object_checked_unref(songIO);
}
BT_END_TEST


TCase *bt_song_io_test_case(void) {
  TCase *tc = tcase_create("BtSongIOTests");

  tcase_add_test(tc,test_btsong_io_obj1);
  tcase_add_test(tc,test_btsong_io_obj2);
  tcase_add_test(tc,test_btsong_io_obj3);
  tcase_add_test(tc,test_btsong_io_obj4);
  tcase_add_test(tc,test_btsong_io_obj5);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
