/* $Id: t-sequence.c,v 1.9 2005-07-19 22:04:28 ensonic Exp $ 
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

START_TEST(test_btsequence_properties) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSequence *sequence=NULL;
	gboolean check_prop_ret=FALSE;
	
  GST_INFO("--------------------------------------------------------------------------------");
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	song=bt_song_new(app);
	fail_unless(song != NULL, NULL);
	
	g_object_get(song,"sequence",&sequence,NULL);
	fail_unless(sequence!=NULL,NULL);
	
	check_prop_ret=check_gobject_properties(G_OBJECT(sequence));
	fail_unless(check_prop_ret==TRUE,NULL);
}
END_TEST


/* try to create a new sequence with NULL for song object */
START_TEST(test_btsequence_obj1) {
	BtSequence *sequence=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	check_init_error_trapp("bt_sequence_new","BT_IS_SONG(song)");
  sequence=bt_sequence_new(NULL);
  fail_unless(sequence == NULL, NULL);
  fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/* try to add a NULL machine to the sequence */
START_TEST(test_btsequence_track1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSequence *sequence=NULL;
	
  GST_INFO("--------------------------------------------------------------------------------");

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	song=bt_song_new(app);
	g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
	fail_unless(sequence!=NULL,NULL);
	
	check_init_error_trapp("bt_sequence_set_machine","BT_IS_MACHINE(machine)");
	bt_sequence_set_machine(sequence,0,NULL);
	fail_unless(check_has_error_trapped(), NULL);
	
	g_object_try_unref(sequence);
	g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

/* try to set a new machine for the sequence with NULL for the sequence parameter */
START_TEST(test_btsequence_track2) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSourceMachine *machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
  /* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	
	/* try to create a source machine */
	machine=bt_source_machine_new(song,"id","sinesrc",0);
	fail_unless(machine!=NULL, NULL);
	
	check_init_error_trapp("bt_sequence_set_machine","BT_IS_SEQUENCE(self)");
	bt_sequence_set_machine(NULL,0,BT_MACHINE(machine));
	fail_unless(check_has_error_trapped(), NULL);
	
	g_object_try_unref(machine);
	g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

/* try to add a machine to the sequence beyond the number of tracks */
START_TEST(test_btsequence_track3) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSequence *sequence=NULL;
  BtMachine *machine;
	
  GST_INFO("--------------------------------------------------------------------------------");

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	song=bt_song_new(app);
	g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
	fail_unless(sequence!=NULL,NULL);
  /* create a source machine */
	machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0));
	fail_unless(machine!=NULL,NULL);
  /* enlarge tracks */
	g_object_set(sequence,"tracks",4L,NULL);
  
	check_init_error_trapp("bt_sequence_set_machine","track<self->priv->tracks");
	bt_sequence_set_machine(sequence,5,machine);
	fail_unless(check_has_error_trapped(), NULL);
	
  g_object_try_unref(machine);
	g_object_try_unref(sequence);
	g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

/* try to add a label to the sequence beyond the sequence length */
START_TEST(test_btsequence_length1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSequence *sequence=NULL;
	
  GST_INFO("--------------------------------------------------------------------------------");

  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	song=bt_song_new(app);
	g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
	fail_unless(sequence!=NULL,NULL);

  /* enlarge length */
	g_object_set(sequence,"length",4L,NULL);
	
	check_init_error_trapp("bt_sequence_set_label","time<self->priv->length");
	bt_sequence_set_label(sequence,5,"test");
	fail_unless(check_has_error_trapped(), NULL);
	
	g_object_try_unref(sequence);
	g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST


TCase *bt_sequence_test_case(void) {
  TCase *tc = tcase_create("BtSequenceTests");

	tcase_add_test(tc,test_btsequence_properties);
  tcase_add_test(tc,test_btsequence_obj1);
	tcase_add_test(tc,test_btsequence_track1);
	tcase_add_test(tc,test_btsequence_track2);
  tcase_add_test(tc,test_btsequence_track3);
  tcase_add_test(tc,test_btsequence_length1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
