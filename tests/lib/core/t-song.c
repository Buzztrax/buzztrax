/* $Id: t-song.c,v 1.23 2005-09-19 16:14:06 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

static gboolean play_signal_invoke=FALSE;

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

// helper method to test the play signal
static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  play_signal_invoke=TRUE;
}

BT_START_TEST(test_btsong_properties) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  gboolean check_prop_ret=FALSE;
  
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  check_prop_ret=check_gobject_properties(G_OBJECT(song));
  fail_unless(check_prop_ret==TRUE,NULL);
}
BT_END_TEST

// test if the default constructor handles NULL
BT_START_TEST(test_btsong_obj1) {
  BtSong *song;
  
  /* create a new song */
  check_init_error_trapp("bt_song_new","BT_IS_APPLICATION(app)");
  song=bt_song_new(NULL);
  fail_unless(song == NULL, NULL);
  fail_unless(check_has_error_trapped(), NULL);
}
BT_END_TEST



// play without loading a song
BT_START_TEST(test_btsong_play1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);

  play_signal_invoke=FALSE;
  g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);
  bt_song_play(song);
  sleep(1);
  fail_unless(play_signal_invoke, NULL);
  bt_song_stop(song);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// test if we can get a empty setup from an empty song
BT_START_TEST(test_btsong_setup1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);
  
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* test if the play method from the song works as aspected if the self parameter
* is NULL
*/
BT_START_TEST(test_btsong_play2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  /* check if a correct error message is thrown */
  check_init_error_trapp("bt_song_play","BT_IS_SONG(self)");
  bt_song_play(NULL);
  fail_unless(check_has_error_trapped(),NULL);
  
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_song_test_case(void) {
  TCase *tc = tcase_create("BtSongTests");

  tcase_add_test(tc,test_btsong_properties);
  tcase_add_test(tc,test_btsong_obj1);
  tcase_add_test(tc,test_btsong_play1);
  tcase_add_test(tc,test_btsong_play2);
  tcase_add_test(tc,test_btsong_setup1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  // we need to raise the default timeout of 3 seconds
  tcase_set_timeout(tc, 10);
  return(tc);
}
