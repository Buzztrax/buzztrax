/* $Id: e-sequence.c,v 1.11 2005-12-16 21:54:44 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

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

BT_START_TEST(test_btsequence_enlarge_length) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  gulong length;

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
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_enlarge_length_vals) {
  BtApplication *app;
  BtSong *song;
  BtSequence *sequence;
  gchar label1[]="test",*label2;
  gulong i,length;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);

  /* try to enlarge length */
  g_object_set(sequence,"length",8L,NULL);
  g_object_get(sequence,"length",&length,NULL);
  fail_unless(length==8, NULL);

  /* nothing should be there for all times */
  for(i=0;i<length;i++) {
    label2=bt_sequence_get_label(sequence,i);
    fail_unless(label2==NULL, NULL);
  }
  
  /* set label twice */
  bt_sequence_set_label(sequence,0,label1);
  bt_sequence_set_label(sequence,7,label1);
  
  /* now label should be at time=0 */
  label2=bt_sequence_get_label(sequence,0);
  fail_unless(label2!=NULL, NULL);
  fail_unless(!strcmp(label2,label1), NULL);
  g_free(label2);
  
  /* try to enlarge length again */
  g_object_set(sequence,"length",16L,NULL);
  g_object_get(sequence,"length",&length,NULL);
  fail_unless(length==16, NULL);
  
  /* now pattern should still be at time=0 */
  label2=bt_sequence_get_label(sequence,0);
  fail_unless(label2!=NULL, NULL);
  fail_unless(!strcmp(label2,label1), NULL);
  g_free(label2);

  /* nothing should be at time=8 */
  label2=bt_sequence_get_label(sequence,8);
  fail_unless(label2==NULL, NULL);
  
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_shrink_length) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  gulong length;

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
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_enlarge_track) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  gulong tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  
  /* now tracks should be 0 */
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==0, NULL);

  /* try to enlarge tracks */
  g_object_set(sequence,"tracks",16L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==16, NULL);
  
  /* clean up */
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_enlarge_track_vals) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine1,*machine2;
  gulong tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine1=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0));
  fail_unless(machine1!=NULL, NULL);
  
  /* now tracks should be 0 */
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==0, NULL);

  /* try to enlarge tracks */
  g_object_set(sequence,"tracks",2L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==2, NULL);

  /* nothing should be at track 0 */
  machine2=bt_sequence_get_machine(sequence,0);
  fail_unless(machine2==NULL, NULL);
  g_object_try_unref(machine2);

  /* set machine twice */
  bt_sequence_set_machine(sequence,0,machine1);
  bt_sequence_set_machine(sequence,1,machine1);

  /* now machine should be at track 0 */
  machine2=bt_sequence_get_machine(sequence,0);
  fail_unless(machine2==machine1, NULL);
  g_object_try_unref(machine2);

  /* try to enlarge tracks again */
  g_object_set(sequence,"tracks",4L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==4, NULL);

  /* now machine should still be at track 0 */
  machine2=bt_sequence_get_machine(sequence,0);
  fail_unless(machine2==machine1, NULL);
  g_object_try_unref(machine2);

  /* nothing should be at track 2 */
  machine2=bt_sequence_get_machine(sequence,2);
  fail_unless(machine2==NULL, NULL);
  g_object_try_unref(machine2);

  /* clean up */
  g_object_try_unref(machine1);
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_shrink_track) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  gulong tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==0, NULL);
	g_object_set(sequence,"length",1L,NULL);

  g_object_set(sequence,"tracks",16L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==16, NULL);

  g_object_set(sequence,"tracks",8L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==8, NULL);

  /* clean up */
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_enlarge_both_vals) {
  BtApplication *app;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;
  gulong i,j,length,tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0));
  fail_unless(machine!=NULL, NULL);
  /* try to create a pattern */
  pattern1=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern1!=NULL, NULL);

  /* try to enlarge length */
  g_object_set(sequence,"length",8L,NULL);
  g_object_get(sequence,"length",&length,NULL);
  fail_unless(length==8, NULL);

  /* try to enlarge tracks */
  g_object_set(sequence,"tracks",2L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==2, NULL);
  
  /* set machine twice */
  bt_sequence_set_machine(sequence,0,machine);
  bt_sequence_set_machine(sequence,1,machine);

  /* nothing should be there for all times */
  for(i=0;i<length;i++) {
    for(j=0;j<tracks;j++) {
      pattern2=bt_sequence_get_pattern(sequence,i,j);
      fail_unless(pattern2==NULL, "pattern!=NULL at %d,%d",i,j);
    }
  }
  
  /* set pattern twice */
  bt_sequence_set_pattern(sequence,0,0,pattern1);
  mark_point();
  bt_sequence_set_pattern(sequence,7,1,pattern1);
  mark_point();
  
  /* now pattern should be at time=0,track=0 */
  pattern2=bt_sequence_get_pattern(sequence,0,0);
  fail_unless(pattern2==pattern1, NULL);
  g_object_try_unref(pattern2);
  
  /* try to enlarge length again */
  g_object_set(sequence,"length",16L,NULL);
  g_object_get(sequence,"length",&length,NULL);
  fail_unless(length==16, NULL);
  /* try to enlarge tracks again */
  g_object_set(sequence,"tracks",4L,NULL);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==4, NULL);
  
  /* now pattern should still be at time=0,track=0 */
  pattern2=bt_sequence_get_pattern(sequence,0,0);
  fail_unless(pattern2==pattern1, NULL);
  g_object_try_unref(pattern2);

  /* nothing should be at time=8,track=0 */
  pattern2=bt_sequence_get_pattern(sequence,8,0);
  fail_unless(pattern2==NULL, NULL);
  g_object_try_unref(pattern2);

  /* nothing should be at time=0,track=2 */
  pattern2=bt_sequence_get_pattern(sequence,0,2);
  fail_unless(pattern2==NULL, NULL);
  g_object_try_unref(pattern2);
  
  /* clean up */
  g_object_try_unref(pattern1);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_update) {
  BtApplication *app;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0));
  fail_unless(machine!=NULL, NULL);
  /* create a pattern */
  pattern1=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern1!=NULL, NULL);

  /* enlarge length */
  g_object_set(sequence,"length",4L,"tracks",1L,NULL);

  /* set machine */
  bt_sequence_set_machine(sequence,0,machine);

  /* set pattern */
  bt_sequence_set_pattern(sequence,0,0,pattern1);

  /* remove the pattern from the machine */
  bt_machine_remove_pattern(machine,pattern1);

  /* nothing should be at time=0,track=0 */
  pattern2=bt_sequence_get_pattern(sequence,0,0);
  fail_unless(pattern2==NULL, NULL);
  g_object_try_unref(pattern2);

  /* clean up */
  g_object_try_unref(pattern1);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_sequence_example_case(void) {
  TCase *tc = tcase_create("BtSequenceExamples");

  tcase_add_test(tc,test_btsequence_enlarge_length);
  tcase_add_test(tc,test_btsequence_enlarge_length_vals);
  tcase_add_test(tc,test_btsequence_shrink_length);
  tcase_add_test(tc,test_btsequence_enlarge_track);
  tcase_add_test(tc,test_btsequence_enlarge_track_vals);
  tcase_add_test(tc,test_btsequence_shrink_track);
  tcase_add_test(tc,test_btsequence_enlarge_both_vals);
  tcase_add_test(tc,test_btsequence_update);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
