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
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  gulong tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen-m","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* now tracks should be 0 */
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==0, NULL);

  /* try to enlarge tracks */
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==4, NULL);

  /* clean up */
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_enlarge_track_vals) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine1,*machine2;
  gulong tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine1=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* now tracks should be 0 */
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==0, NULL);

  /* set machine twice */
  bt_sequence_add_track(sequence,machine1);
  bt_sequence_add_track(sequence,machine1);

  /* check number of tracks */
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==2, NULL);

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
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  gulong tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==0, NULL);
	g_object_set(sequence,"length",1L,NULL);

  /* set machine 4 times */
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==4, NULL);

  bt_sequence_remove_track_by_ix(sequence,0);
  bt_sequence_remove_track_by_ix(sequence,0);
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==2, NULL);

  /* clean up */
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_enlarge_both_vals) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;
  gulong i,j,length,tracks;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* try to create a pattern */
  pattern1=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern1!=NULL, NULL);

  /* try to enlarge length */
  g_object_set(sequence,"length",8L,NULL);
  g_object_get(sequence,"length",&length,NULL);
  fail_unless(length==8, NULL);

  /* set machine twice */
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);

  /* check tracks */
  g_object_get(sequence,"tracks",&tracks,NULL);
  fail_unless(tracks==2, NULL);

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
  bt_sequence_add_track(sequence,machine);
  bt_sequence_add_track(sequence,machine);
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
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create a pattern */
  pattern1=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern1!=NULL, NULL);

  /* enlarge length */
  g_object_set(sequence,"length",4L,NULL);

  /* set machine */
  bt_sequence_add_track(sequence,machine);

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

BT_START_TEST(test_btsequence_change_pattern) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern;
  GstObject *element;
  gulong val;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine and get the gstreamer element */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern!=NULL, NULL);

  /* enlarge length */
  g_object_set(sequence,"length",4L,NULL);

  /* set machine */
  bt_sequence_add_track(sequence,machine);

  /* set pattern */
  bt_sequence_set_pattern(sequence,0,0,pattern);

  bt_pattern_set_global_event(pattern,0,0,"100");

  g_object_get(machine,"machine",&element,NULL);

  /* we should still have the default value */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==0, NULL);

  /* pull in the change */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0));

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==100, NULL);

  gst_object_unref(element);

  /* clean up */
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


BT_START_TEST(test_btsequence_validate_loop) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  gulong loop_start,loop_end;
  gboolean loop;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  g_object_set(sequence,"length",16L,NULL);

  g_object_get(sequence,"loop",&loop,"loop-start",&loop_start,"loop-end",&loop_end,NULL);
  fail_unless(loop==FALSE, NULL);
  fail_unless(loop_start==-1, NULL);
  fail_unless(loop_end==-1, NULL);

  g_object_set(sequence,"loop",TRUE,NULL);
  g_object_get(sequence,"loop-start",&loop_start,"loop-end",&loop_end,NULL);
  fail_unless(loop_start==0L, NULL);
  fail_unless(loop_end==16L, NULL);

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
  tcase_add_test(tc,test_btsequence_change_pattern);
  tcase_add_test(tc,test_btsequence_validate_loop);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
