/** $Id: t-song-io.c,v 1.2 2004-08-24 14:26:59 ensonic Exp $
**/

#include "t-song.h"

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
}

//-- tests

START_TEST(test_btsong_io_obj1) {
	BtSongIO *songIO;
	
	songIO=bt_song_io_new(NULL);
	fail_unless(songIO == NULL, NULL);
}
END_TEST

TCase *bt_song_io_obj_tcase(void) {
  TCase *tc = tcase_create("bt_song_io case");

  tcase_add_test(tc,test_btsong_io_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_song_io_suite(void) { 
  Suite *s=suite_create("BtSongIO"); 

  suite_add_tcase(s,bt_song_io_obj_tcase());
  return(s);
}
