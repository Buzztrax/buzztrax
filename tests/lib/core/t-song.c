/** $Id: t-song.c,v 1.6 2004-10-01 16:01:47 ensonic Exp $
**/

#include "t-core.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
gboolean play_signal_invoke=FALSE;

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
}

//-- tests

// helper method to test the play signal
static void play_event_test(void) {
  play_signal_invoke=TRUE;
}

// test if the default constructor works as expected
START_TEST(test_btsong_obj1) {
	BtSong *song;
	GstElement *bin;
	
  GST_INFO("--------------------------------------------------------------------------------");

	/* create a new bin element */
  bin = gst_bin_new("bin");
	
	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
  g_object_checked_unref(G_OBJECT(song));
}
END_TEST

// test if the song loading works without failure
START_TEST(test_btsong_load1) {
	BtSong *song;
	BtSongIO *loader;
	GstElement *bin;
	gboolean load_ret = FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");

	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple1.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  g_object_checked_unref(G_OBJECT(loader));
	g_object_checked_unref(G_OBJECT(song));
}
END_TEST

// test if subsequent song loading works without failure
START_TEST(test_btsong_load2) {
	BtSong *song;
	BtSongIO *loader;
	GstElement *bin;
	gboolean load_ret = FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");

	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple1.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  g_object_checked_unref(G_OBJECT(loader));
	g_object_checked_unref(G_OBJECT(song));

	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple2.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  g_object_checked_unref(G_OBJECT(loader));
	g_object_checked_unref(G_OBJECT(song));
}
END_TEST

// test if the song play routine works without failure
START_TEST(test_btsong_play1) {
	BtSong *song=NULL;
	BtSongIO *loader=NULL;
	GstElement *bin=NULL;
	gboolean load_ret = FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");

	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple1.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  play_signal_invoke=FALSE;
	g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event_test, NULL);
	bt_song_play(song);
	fail_unless(play_signal_invoke, NULL);
  g_object_checked_unref(G_OBJECT(loader));
	g_object_checked_unref(G_OBJECT(song));
}
END_TEST

// play without loading a song
START_TEST(test_btsong_play2) {
	BtSong *song=NULL;
	GstElement *bin=NULL;
	
  GST_INFO("--------------------------------------------------------------------------------");

	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
  play_signal_invoke=FALSE;
	g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event_test, NULL);
  mark_point();
	bt_song_play(song);
	fail_unless(play_signal_invoke, NULL);
	g_object_checked_unref(G_OBJECT(song));
}
END_TEST

// test if we can get a empty setup from an empty song
START_TEST(test_btsong_setup1) {
	BtSong *song=NULL;
	GstElement *bin=NULL;
	BtSetup *setup=NULL;
	
  GST_INFO("--------------------------------------------------------------------------------");

	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	fail_unless(song != NULL, NULL);
	
	g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
  g_object_checked_unref(setup);
	g_object_checked_unref(song);
}
END_TEST

TCase *bt_song_obj_tcase(void) {
  TCase *tc = tcase_create("bt_song case");

  tcase_add_test(tc,test_btsong_obj1);
  tcase_add_test(tc,test_btsong_load1);
  tcase_add_test(tc,test_btsong_load2);
	tcase_add_test(tc,test_btsong_play1);
	tcase_add_test(tc,test_btsong_play2);
	tcase_add_test(tc,test_btsong_setup1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_song_suite(void) { 
  Suite *s=suite_create("BtSong"); 

  suite_add_tcase(s,bt_song_obj_tcase());
  return(s);
}

