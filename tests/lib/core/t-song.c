/** $Id: t-song.c,v 1.1 2004-08-17 17:02:17 waffel Exp $
**/

#include "t-song.h"

gboolean play_signal_invoke=FALSE;
//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  //puts(__FILE__":setup");
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
}

//-- tests

// helper method to test the play signal
static void play_event_test(void) {
  play_signal_invoke=TRUE;
}

// test if the constructor works with null
START_TEST(test_btsong_obj1) {
  BtSong *song;

  song=bt_song_new(NULL);
	fail_unless(song == NULL,"failed to get song");
}
END_TEST

// test if the default constructor works as aspected
START_TEST(test_btsong_obj2) {
	BtSong *song;
	GstElement *bin;
	
	/* create a new bin elemen */
  bin = gst_bin_new("bin");
	
	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, "failed to get song");
}
END_TEST

// test if the song play routine works wihtout failure
START_TEST(test_btsong_play1) {
	BtSong *song=NULL;
	BtSongIO *loader=NULL;
	GstElement *bin=NULL;
	gboolean load_ret = FALSE;
	
	bin = gst_thread_new("thread");
	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, "failed to get song");
	loader=bt_song_io_new("songs/test-simple1.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  play_signal_invoke=FALSE;
	g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event_test, NULL);
	bt_song_play(song);
	fail_unless(play_signal_invoke, NULL);
	g_object_unref(G_OBJECT(song));
  g_object_unref(G_OBJECT(loader));
}
END_TEST

// play without loading a song
START_TEST(test_btsong_play2) {
	BtSong *song=NULL;
	GstElement *bin=NULL;
	
	bin = gst_thread_new("thread");
	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, "failed to get song");
  play_signal_invoke=FALSE;
	g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event_test, NULL);
  mark_point();
	bt_song_play(song);
	fail_unless(play_signal_invoke, NULL);
	g_object_unref(G_OBJECT(song));
	
}
END_TEST

TCase *bt_song_obj_tcase(void) {
  TCase *tc = tcase_create("bt_song case");

  tcase_add_test(tc,test_btsong_obj1);
	tcase_add_test(tc,test_btsong_obj2);
	tcase_add_test(tc,test_btsong_play1);
	tcase_add_test(tc,test_btsong_play2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_song_suite(void) { 
  Suite *s=suite_create("BtSong"); 

  suite_add_tcase(s,bt_song_obj_tcase());
  return(s);
}

