/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-core.h"

//-- globals

#if 0
static gboolean play_signal_invoked=FALSE;
#endif

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
#if 0
static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  play_signal_invoked=TRUE;
}
#endif

// test if the default constructor works as expected
BT_START_TEST(test_btsong_obj1) {
  BtApplication *app=NULL;
  BtSong *song;
  gboolean unsaved;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  /* song should be unchanged */
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);

  g_object_checked_unref(song);

  g_object_checked_unref(app);
}
BT_END_TEST

// test if the song loading works without failure
BT_START_TEST(test_btsong_load1) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  gboolean load_ret = FALSE;
  gboolean unsaved;

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_DEBUG);
  loader=bt_song_io_new(check_get_test_song_path("test-simple1.xml"));
  mark_point();
  fail_unless(loader != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_WARNING);
  load_ret = bt_song_io_load(loader,song);
  mark_point();
  fail_unless(load_ret, NULL);
  /* song should be unchanged */
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);

  mark_point();
  g_object_checked_unref(loader);
  mark_point();
  g_object_checked_unref(song);
  mark_point();
  g_object_checked_unref(app);
}
BT_END_TEST

// test if subsequent song loading works without failure
BT_START_TEST(test_btsong_load2) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  gboolean load_ret = FALSE;

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_new(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_new(check_get_test_song_path("test-simple2.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);

  g_object_checked_unref(app);
}
BT_END_TEST

// test if after the song loading everything is there
BT_START_TEST(test_btsong_load3) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  gboolean load_ret = FALSE;
  BtSinkMachine *master;

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_DEBUG);
  loader=bt_song_io_new(check_get_test_song_path("test-simple1.xml"));
  mark_point();
  fail_unless(loader != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_WARNING);
  load_ret = bt_song_io_load(loader,song);
  mark_point();
  fail_unless(load_ret, NULL);

  g_object_get(song,"master",&master,NULL);
  fail_unless(master != NULL, NULL);
  g_object_unref(master);

  mark_point();
  g_object_checked_unref(loader);
  mark_point();
  g_object_checked_unref(song);
  mark_point();
  g_object_checked_unref(app);
}
BT_END_TEST

// test if the song play routine works without failure
BT_START_TEST(test_btsong_play1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;
  gboolean res;

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_new(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  //play_signal_invoked=FALSE;
  //g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);

  res=bt_song_play(song);
  fail_unless(res, NULL);

  // @todo: this needs a mainloop!
  sleep(3);
  //fail_unless(play_signal_invoked, NULL);
  bt_song_stop(song);

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* @todo: play, wait a little, stop, play again */
/* @todo: load a new song which the first plays */

// test, if a newly created song contains empty setup, sequence, song-info and
// wavetable
BT_START_TEST(test_btsong_new1){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  BtSongInfo *songinfo=NULL;
  BtWavetable *wavetable=NULL;

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
BT_END_TEST

TCase *bt_song_example_case(void) {
  TCase *tc = tcase_create("BtSongExamples");

  tcase_add_test(tc,test_btsong_obj1);
  tcase_add_test(tc,test_btsong_load1);
  tcase_add_test(tc,test_btsong_load2);
  tcase_add_test(tc,test_btsong_load3);
  tcase_add_test(tc,test_btsong_play1);
  tcase_add_test(tc,test_btsong_new1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
