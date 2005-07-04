/* $Id: e-sequence.c,v 1.1 2005-07-04 11:37:22 ensonic Exp $
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

/* get some timelines, check that they are different and not NULL
 */
START_TEST(test_btsequence_timeline){
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSequence *sequence=NULL;
	BtTimeLine *timeline1=NULL,*timeline2=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	/* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
	
	/* create and get some timelines */
	g_object_set(G_OBJECT(sequence),"length",2,NULL);
	timeline1=bt_sequence_get_timeline_by_time(sequence,0);
	fail_unless(timeline1 != NULL, NULL);
	timeline2=bt_sequence_get_timeline_by_time(sequence,1);
	fail_unless(timeline2 != NULL, NULL);
	fail_unless(timeline1 != timeline2, NULL);
	
	g_object_try_unref(timeline1);
	g_object_try_unref(timeline2);	
	g_object_unref(sequence);
	g_object_unref(song);
	g_object_checked_unref(app);
}
END_TEST


TCase *bt_sequence_example_case(void) {
  TCase *tc = tcase_create("BtSequenceExamples");

  tcase_add_test(tc,test_btsequence_timeline);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
