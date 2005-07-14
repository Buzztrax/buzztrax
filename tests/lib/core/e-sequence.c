/* $Id: e-sequence.c,v 1.2 2005-07-14 21:44:10 ensonic Exp $
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
  //puts(__FILE__":teardown");
}

//-- tests

START_TEST(test_btsequence_enlarge_length) {
	BtApplication *app=NULL;
	BtSong *song;
  BtSequence *sequence;
	gulong length;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  
	g_object_get(sequence,"length",&length,NULL);
	fail_unless(length==0, NULL);

	g_object_set(sequence,"length",16L,NULL);
	g_object_get(sequence,"length",&length,NULL);
	fail_unless(length==16, NULL);
	
	g_object_try_unref(sequence);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

START_TEST(test_btsequence_shrink_length) {
	BtApplication *app=NULL;
	BtSong *song;
  BtSequence *sequence;
	gulong length;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  
	g_object_get(sequence,"length",&length,NULL);
	fail_unless(length==0, NULL);

	g_object_set(sequence,"length",16L,NULL);
	g_object_get(sequence,"length",&length,NULL);
	fail_unless(length==16, NULL);

	g_object_set(sequence,"length",8L,NULL);
	g_object_get(sequence,"length",&length,NULL);
	fail_unless(length==8, NULL);

	g_object_try_unref(sequence);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

// @todo have tests with (cmd-)pattern values in the sequence

START_TEST(test_btsequence_enlarge_track) {
	BtApplication *app=NULL;
	BtSong *song;
  BtSequence *sequence;
	gulong tracks;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  
	g_object_get(sequence,"tracks",&tracks,NULL);
	fail_unless(tracks==0, NULL);

	g_object_set(sequence,"tracks",16L,NULL);
	g_object_get(sequence,"tracks",&tracks,NULL);
	fail_unless(tracks==16, NULL);
	
	g_object_try_unref(sequence);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

START_TEST(test_btsequence_shrink_track) {
	BtApplication *app=NULL;
	BtSong *song;
  BtSequence *sequence;
	gulong tracks;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  
	g_object_get(sequence,"tracks",&tracks,NULL);
	fail_unless(tracks==0, NULL);

	g_object_set(sequence,"tracks",16L,NULL);
	g_object_get(sequence,"tracks",&tracks,NULL);
	fail_unless(tracks==16, NULL);

	g_object_set(sequence,"tracks",8L,NULL);
	g_object_get(sequence,"tracks",&tracks,NULL);
	fail_unless(tracks==8, NULL);

	g_object_try_unref(sequence);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

TCase *bt_sequence_example_case(void) {
  TCase *tc = tcase_create("BtSequenceExamples");

  tcase_add_test(tc,test_btsequence_enlarge_length);
  tcase_add_test(tc,test_btsequence_shrink_length);
  tcase_add_test(tc,test_btsequence_enlarge_track);
  tcase_add_test(tc,test_btsequence_shrink_track);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
