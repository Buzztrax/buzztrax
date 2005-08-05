/* $Id: e-song-info.c,v 1.2 2005-08-05 08:59:48 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
}

/**
* In this test we show, how to get the creation date of an song from the 
* song info class. We load a example song and try to retrive the creation date
* from it.
*/
START_TEST(test_btsonginfo_createdate) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  BtSongInfo *song_info=NULL;
  gchar *create_dts=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");

  // creating new empty app
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  mark_point();
  // creating new song
  song=bt_song_new(app);
  mark_point();
  // loading a song xml file
  loader=bt_song_io_new("songs/test-simple1.xml");
  mark_point();
  bt_song_io_load(loader,song);
  mark_point();
  // get the song info class from the song property
  g_object_get(song,"song-info",&song_info,NULL);
  mark_point();
  // get the creating date property from the song info
  g_object_get(song_info,"create-dts",&create_dts,NULL);
  fail_unless(create_dts!=NULL,NULL);
  g_free(create_dts);

  g_object_unref(song_info);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);

}
END_TEST

TCase *bt_song_info_example_case(void) {
  TCase *tc = tcase_create("BtSongInfoExamples");

  tcase_add_test(tc,test_btsonginfo_createdate);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
