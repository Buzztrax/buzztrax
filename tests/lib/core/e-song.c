/* $Id: e-song.c,v 1.4 2005-04-15 17:05:13 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

static gboolean play_signal_invoke=FALSE;

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
  BtApplication *app=NULL;
	BtSong *song;
	
  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
		
	/* create a new song */
	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
  g_object_checked_unref(song);

  g_object_checked_unref(app);
}
END_TEST

// test if the song loading works without failure
START_TEST(test_btsong_load1) {
  BtApplication *app=NULL;
	BtSong *song;
	BtSongIO *loader;
	gboolean load_ret = FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");
  
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
	//gst_debug_set_threshold_for_name("bt*",GST_LEVEL_DEBUG);
	loader=bt_song_io_new("songs/test-simple1.xml");
  mark_point();
	fail_unless(loader != NULL, NULL);
	//gst_debug_set_threshold_for_name("bt*",GST_LEVEL_WARNING);
	load_ret = bt_song_io_load(loader,song);
  mark_point();
	fail_unless(load_ret, NULL);
  
  mark_point();
  g_object_checked_unref(loader);
  mark_point();
	g_object_checked_unref(song);
  mark_point();
  g_object_checked_unref(app);
}
END_TEST

// test if subsequent song loading works without failure
START_TEST(test_btsong_load2) {
  BtApplication *app=NULL;
	BtSong *song;
	BtSongIO *loader;
	gboolean load_ret = FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple1.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  g_object_checked_unref(loader);
	g_object_checked_unref(song);

	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple2.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  g_object_checked_unref(loader);
	g_object_checked_unref(song);
  
  g_object_checked_unref(app);
}
END_TEST

// test if the song play routine works without failure
START_TEST(test_btsong_play1) {
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSongIO *loader=NULL;
	gboolean load_ret = FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
	loader=bt_song_io_new("songs/test-simple1.xml");
	fail_unless(loader != NULL, NULL);
	load_ret = bt_song_io_load(loader,song);
	fail_unless(load_ret, NULL);
  play_signal_invoke=FALSE;
	g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event_test, NULL);
	bt_song_play(song);
	fail_unless(play_signal_invoke, NULL);
  g_object_checked_unref(loader);
	g_object_checked_unref(song);

  g_object_checked_unref(app);
}
END_TEST

// test, if a newly created song contains empty setup, sequence, song-info and 
// wavetable
START_TEST(test_btsong_new1){
  BtApplication *app=NULL;
	BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  BtSongInfo *songinfo=NULL;
  BtWavetable *wavetable=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");

  // create a dummy application
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  // create a new, empty song
	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
  
  // get the setup property
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  g_object_unref(setup);
  
  // get the sequence property
  g_object_get(song,"sequence",&sequence,NULL);
  fail_unless(sequence!=NULL,NULL);
  g_object_unref(sequence);
  
  // get the song-info property
  g_object_get(song,"song-info",&songinfo,NULL);
  fail_unless(songinfo!=NULL,NULL);
  g_object_unref(songinfo);
  
  // get the wavetable property
  g_object_get(song,"wavetable",&wavetable,NULL);
  fail_unless(wavetable!=NULL,NULL);
  g_object_unref(wavetable);
  
  g_object_checked_unref(song);
  g_object_checked_unref(app);
  
  
}
END_TEST

TCase *bt_song_example_case(void) {
  TCase *tc = tcase_create("BtSongExamples");

  tcase_add_test(tc,test_btsong_obj1);
  tcase_add_test(tc,test_btsong_load1);
  tcase_add_test(tc,test_btsong_load2);
	tcase_add_test(tc,test_btsong_play1);
  tcase_add_test(tc,test_btsong_new1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
