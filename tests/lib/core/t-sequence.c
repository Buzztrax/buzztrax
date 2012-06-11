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

static BtApplication *app;
static BtSong *song;

//-- fixtures

static void case_setup(void) {
  GST_INFO("================================================================================");
}

static void test_setup(void) {
  app=bt_test_application_new();
  song=bt_song_new(app);
}

static void test_teardown(void) {
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}

static void case_teardown(void) {
}


//-- tests

/* apply generic property tests to sequence */
BT_START_TEST(test_btsequence_properties) {
  /* arrange */
  GObject *sequence=check_gobject_get_object_property(song,"sequence");

  /* act & assert */
  fail_unless(check_gobject_properties(sequence), NULL);

  /* cleanup */
}
BT_END_TEST


/* try to create a new sequence with NULL for song object */
BT_START_TEST(test_btsequence_new_null_song) {
  /* act */
  BtSequence *sequence=bt_sequence_new(NULL);
  
  /* assert */
  fail_unless(sequence != NULL, NULL);
  
  /* cleanup */
  g_object_unref(sequence);
}
BT_END_TEST

/* try to add a NULL machine to the sequence */
BT_START_TEST(test_btsequence_add_track1) {
  /* arrange */
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  check_init_error_trapp("","BT_IS_MACHINE(machine)");
  
  /* act */
  bt_sequence_add_track(sequence,NULL,-1);
  
  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


/* try to add a new machine for the sequence with NULL for the sequence parameter */
BT_START_TEST(test_btsequence_add_track2) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,NULL));
  check_init_error_trapp("","BT_IS_SEQUENCE(self)");
  
  /* act */
  bt_sequence_add_track(NULL,machine,-1);
  
  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_try_unref(machine);
}
BT_END_TEST


/* try to remove a NULL machine from the sequence */
BT_START_TEST(test_btsequence_rem_track1) {
  /* arrange */
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  check_init_error_trapp("","BT_IS_MACHINE(machine)");

  /* act */
  bt_sequence_remove_track_by_machine(sequence,NULL);
  
  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


/* try to remove a machine from the sequence that has never added */
BT_START_TEST(test_btsequence_rem_track2) {
  /* arrange */
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,NULL));

  /* act */
  bt_sequence_remove_track_by_machine(sequence,machine);
  
  /* assert */
  mark_point();

  /* cleanup */
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


/* try to add a label to the sequence beyond the sequence length */
BT_START_TEST(test_btsequence_length1) {
  /* arrange */
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  g_object_set(sequence,"length",4L,NULL);
  check_init_error_trapp("bt_sequence_set_label","time<self->priv->length");

  /* act */
  bt_sequence_set_label(sequence,5,"test");
  
  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_pattern1) {
  /* arrange */
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,NULL));
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  g_object_set(sequence,"length",4L,NULL);
	check_init_error_trapp("bt_sequence_set_pattern","track<self->priv->tracks");

  /* act */
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  
  /* assert */
	fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_pattern2) {
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  BtMachine *machine1=BT_MACHINE(bt_source_machine_new(song,"genm","buzztard-test-mono-source",0L,NULL));
  BtMachine *machine2=BT_MACHINE(bt_source_machine_new(song,"genp","buzztard-test-poly-source",1L,NULL));
  BtCmdPattern *pattern1=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,machine1);
  g_object_set(sequence,"length",4L,NULL);
  bt_sequence_add_track(sequence,machine1,-1);
  bt_sequence_add_track(sequence,machine2,-1);
  check_init_error_trapp("bt_sequence_set_pattern_quick","adding a pattern to a track with different machine!");

  /* act */
  bt_sequence_set_pattern(sequence,0,1,pattern1);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(pattern1);
  g_object_unref(machine1);
  g_object_unref(machine2);
  g_object_unref(sequence);
}
BT_END_TEST

TCase *bt_sequence_test_case(void) {
  TCase *tc = tcase_create("BtSequenceTests");

  tcase_add_test(tc,test_btsequence_properties);
  tcase_add_test(tc,test_btsequence_new_null_song);
  tcase_add_test(tc,test_btsequence_add_track1);
  tcase_add_test(tc,test_btsequence_add_track2);
  tcase_add_test(tc,test_btsequence_rem_track1);
  tcase_add_test(tc,test_btsequence_rem_track2);
  tcase_add_test(tc,test_btsequence_length1);
  tcase_add_test(tc,test_btsequence_pattern1);
  tcase_add_test(tc,test_btsequence_pattern2);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
