/* Buzztard
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

/* apply generic property tests to sequence */
BT_START_TEST(test_btsequence_properties) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSequence *sequence=NULL;
  gboolean check_prop_ret=FALSE;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);

  g_object_get(song,"sequence",&sequence,NULL);
  fail_unless(sequence!=NULL,NULL);

  check_prop_ret=check_gobject_properties(G_OBJECT(sequence));
  fail_unless(check_prop_ret==TRUE,NULL);

  /* clean up */
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* try to create a new sequence with NULL for song object */
BT_START_TEST(test_btsequence_obj1) {
  BtSequence *sequence=NULL;

  sequence=bt_sequence_new(NULL);
  fail_unless(sequence != NULL, NULL);
  g_object_unref(sequence);
}
BT_END_TEST

/* try to add a NULL machine to the sequence */
BT_START_TEST(test_btsequence_add_track1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSequence *sequence=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
  fail_unless(sequence!=NULL,NULL);

  check_init_error_trapp("","BT_IS_MACHINE(machine)");
  bt_sequence_add_track(sequence,NULL,-1);
  fail_unless(check_has_error_trapped(), NULL);

  /* clean up */
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* try to add a new machine for the sequence with NULL for the sequence parameter */
BT_START_TEST(test_btsequence_add_track2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a source machine */
  machine=bt_source_machine_new(song,"id","audiotestsrc",0,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  check_init_error_trapp("","BT_IS_SEQUENCE(self)");
  bt_sequence_add_track(NULL,BT_MACHINE(machine),-1);
  fail_unless(check_has_error_trapped(), NULL);

  /* clean up */
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* try to remove a NULL machine from the sequence */
BT_START_TEST(test_btsequence_rem_track1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSequence *sequence=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
  fail_unless(sequence!=NULL,NULL);

  check_init_error_trapp("","BT_IS_MACHINE(machine)");
  bt_sequence_remove_track_by_machine(sequence,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  /* clean up */
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* try to remove a machine from the sequence that has never added */
BT_START_TEST(test_btsequence_rem_track2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSequence *sequence=NULL;
  BtSourceMachine *machine=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
  fail_unless(sequence!=NULL,NULL);

  /* try to create a source machine */
  machine=bt_source_machine_new(song,"id","audiotestsrc",0,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  bt_sequence_remove_track_by_machine(sequence,BT_MACHINE(machine));

  /* clean up */
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* try to add a label to the sequence beyond the sequence length */
BT_START_TEST(test_btsequence_length1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSequence *sequence=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(BT_SONG(song), "sequence", &sequence, NULL);
  fail_unless(sequence!=NULL,NULL);

  /* enlarge length */
  g_object_set(sequence,"length",4L,NULL);

  check_init_error_trapp("bt_sequence_set_label","time<self->priv->length");
  bt_sequence_set_label(sequence,5,"test");
  fail_unless(check_has_error_trapped(), NULL);

  /* clean up */
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_pattern1) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
  /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen-m","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern!=NULL, NULL);

  /* enlarge length & tracks */
  g_object_set(sequence,"length",4L,NULL);

  /* set pattern (which should be rejected - no track has been added) */
	check_init_error_trapp("bt_sequence_set_pattern","track<self->priv->tracks");
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
	fail_unless(check_has_error_trapped(), NULL);

  /* clean up */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_pattern2) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine1,*machine2;
  BtCmdPattern *pattern1,*pattern2,*pattern3;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create two source machines */
  machine1=BT_MACHINE(bt_source_machine_new(song,"gen-m","buzztard-test-mono-source",0,&err));
  fail_unless(machine1!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  machine2=BT_MACHINE(bt_source_machine_new(song,"gen-p","buzztard-test-poly-source",1,&err));
  fail_unless(machine2!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create a pattern */
  pattern1=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,machine1);
  fail_unless(pattern1!=NULL, NULL);

  /* enlarge length*/
  g_object_set(sequence,"length",4L,NULL);

  /* add tracks */
  bt_sequence_add_track(sequence,machine1,-1);
  bt_sequence_add_track(sequence,machine2,-1);

  /* get pattern */
  pattern2=bt_sequence_get_pattern(sequence,0,1);
  /* set pattern (which should be rejected - wrong machine) */
  bt_sequence_set_pattern(sequence,0,1,pattern1);
  /* get pattern again and verify */
  pattern3=bt_sequence_get_pattern(sequence,0,1);
  fail_unless(pattern2==pattern3, NULL);

  /* clean up */
  g_object_unref(pattern1);
  g_object_unref(machine1);
  g_object_unref(machine2);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_sequence_test_case(void) {
  TCase *tc = tcase_create("BtSequenceTests");

  tcase_add_test(tc,test_btsequence_properties);
  tcase_add_test(tc,test_btsequence_obj1);
  tcase_add_test(tc,test_btsequence_add_track1);
  tcase_add_test(tc,test_btsequence_add_track2);
  tcase_add_test(tc,test_btsequence_rem_track1);
  tcase_add_test(tc,test_btsequence_rem_track2);
  tcase_add_test(tc,test_btsequence_length1);
  tcase_add_test(tc,test_btsequence_pattern1);
  tcase_add_test(tc,test_btsequence_pattern2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
