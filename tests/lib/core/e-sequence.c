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

BT_START_TEST(test_btsequence_new) {
  /* arrange */  

  /* act */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));

  /* assert */
  ck_assert_gobject_gulong_eq(sequence,"length",0);
  ck_assert_gobject_gulong_eq(sequence,"tracks",0);
  ck_assert_gobject_boolean_eq(sequence,"loop",FALSE);
  ck_assert_gobject_glong_eq(sequence,"loop-start",-1);
  ck_assert_gobject_glong_eq(sequence,"loop-end",-1);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_labels) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  g_object_set(sequence,"length",8L,NULL);
  
  /* act */
  bt_sequence_set_label(sequence,0,"test");
  bt_sequence_set_label(sequence,7,"test");
  
  /* assert */
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,0),"test");
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,1),NULL);
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,7),"test");

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_tracks) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song,"sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  
  /* act */
  bt_sequence_add_track(sequence,machine,-1);

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_sequence_get_machine(sequence,0),machine);

  /* cleanup */
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_pattern) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtCmdPattern *pattern=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  g_object_set(sequence,"length",8L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  
  /* act */
  bt_sequence_set_pattern(sequence,0,0,pattern);
  bt_sequence_set_pattern(sequence,7,1,pattern);

  /* assert */
  fail_unless(bt_sequence_is_pattern_used(sequence,(BtPattern *)pattern),NULL);
  ck_assert_gobject_eq_and_unref(bt_sequence_get_pattern(sequence,0,0),pattern);
  ck_assert_gobject_eq_and_unref(bt_sequence_get_pattern(sequence,7,1),pattern);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_enlarge_length) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));

  /* act */
  g_object_set(sequence,"length",16L,NULL);
  
  /* assert */
  ck_assert_gobject_gulong_eq(sequence,"length",16);
  gint i;for(i=0;i<16;i++) {
    ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,i),NULL);
  }

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_enlarge_length_labels) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  g_object_set(sequence,"length",8L,NULL);
  bt_sequence_set_label(sequence,0,"test");
  bt_sequence_set_label(sequence,7,"test");

  /* act */
  g_object_set(sequence,"length",16L,NULL);

  /* assert */
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,0),"test");
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,1),NULL);
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,7),"test");
  ck_assert_str_eq_and_free(bt_sequence_get_label(sequence,8),NULL);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_shrink_length) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  g_object_set(sequence,"length",16L,NULL);

  /* act */
  g_object_set(sequence,"length",8L,NULL);

  /* assert */
  ck_assert_gobject_gulong_eq(sequence,"length",8);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_enlarge_track) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen-m","buzztard-test-mono-source",0,NULL));

  /* act */
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);

  /* assert */
  ck_assert_gobject_gulong_eq(sequence,"tracks",2);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_enlarge_track_vals) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  bt_sequence_add_track(sequence,machine,-1);

  /* act */
  g_object_set(sequence,"tracks",2L,NULL);

  /* assert */
  ck_assert_gobject_gulong_eq(sequence,"tracks",2);
  ck_assert_gobject_eq_and_unref(bt_sequence_get_machine(sequence,0),machine);
  ck_assert_gobject_eq_and_unref(bt_sequence_get_machine(sequence,1),NULL);

  /* cleanup */
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_shrink_track) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
	g_object_set(sequence,"length",1L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);

  /* act */
  bt_sequence_remove_track_by_ix(sequence,0);
  ck_assert_gobject_gulong_eq(sequence,"tracks",1);
  ck_assert_gobject_eq_and_unref(bt_sequence_get_machine(sequence,0),machine);

  /* cleanup */
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_enlarge_both_vals) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtCmdPattern *pattern=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  g_object_set(sequence,"length",8L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,pattern);
  bt_sequence_set_pattern(sequence,7,1,pattern);

  /* act */
  g_object_set(sequence,"length",16L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_sequence_get_pattern(sequence,0,0),pattern);
  ck_assert_gobject_eq_and_unref(bt_sequence_get_pattern(sequence,7,1),pattern);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST


/* we moved these updates to the app, to give the undo/redo framework a chance
 * to backup the data
 */
#ifdef __CHECK_DISABLED__
// test that removing patterns updates the sequence
BT_START_TEST(test_btsequence_update) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  g_object_set(sequence,"length",4L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,pattern);

  /* act */
  bt_machine_remove_pattern(machine,pattern);

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_sequence_get_pattern(sequence,0,0),NULL);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_try_unref(sequence);
}
BT_END_TEST
#endif


BT_START_TEST(test_btsequence_change_pattern) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(machine,"machine"));
  g_object_set(sequence,"length",4L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  bt_pattern_set_global_event(pattern,0,0,"100");

  /* act */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0));

  /* assert */
  ck_assert_gobject_gulong_eq(element,"g-ulong",100);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_hold) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  GstClockTime tick_time=bt_sequence_get_bar_time(sequence);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(machine,"machine"));
  g_object_set(sequence,"length",4L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_pattern_set_global_event(pattern,4,0,"100");
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);

  /* act */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(1)*tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq(element,"g-ulong",50);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_combine_pattern_shadows) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern1=bt_pattern_new(song,"pattern1","pattern1",8L,machine);
  BtPattern *pattern2=bt_pattern_new(song,"pattern2","pattern2",8L,machine);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(machine,"machine"));
  GstClockTime tick_time=bt_sequence_get_bar_time(sequence);
  g_object_set(sequence,"length",16L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern1);
  bt_sequence_set_pattern(sequence,4,0,(BtCmdPattern *)pattern2);
  bt_pattern_set_global_event(pattern1,0,0,"50");
  bt_pattern_set_global_event(pattern1,4,0,"100");
  bt_pattern_set_global_event(pattern2,0,0,"200"); /* value shadows above */

  /* act */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq(element,"g-ulong",200);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern1);
  g_object_unref(pattern2);
  g_object_unref(machine);
  g_object_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_combine_pattern_unshadows) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern1=bt_pattern_new(song,"pattern1","pattern1",8L,machine);
  BtPattern *pattern2=bt_pattern_new(song,"pattern2","pattern2",8L,machine);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(machine,"machine"));
  GstClockTime tick_time=bt_sequence_get_bar_time(sequence);
  g_object_set(sequence,"length",16L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern1);
  bt_sequence_set_pattern(sequence,4,0,(BtCmdPattern *)pattern2);
  bt_pattern_set_global_event(pattern1,0,0,"50");
  bt_pattern_set_global_event(pattern1,4,0,"100");
  bt_pattern_set_global_event(pattern2,0,0,"200"); /* value shadows above */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* act */
  bt_sequence_set_pattern(sequence,4,0,NULL);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq(element,"g-ulong",100);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern1);
  g_object_unref(pattern2);
  g_object_unref(machine);
  g_object_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_combine_value_unshadows) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern1=bt_pattern_new(song,"pattern1","pattern1",8L,machine);
  BtPattern *pattern2=bt_pattern_new(song,"pattern2","pattern2",8L,machine);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(machine,"machine"));
  GstClockTime tick_time=bt_sequence_get_bar_time(sequence);
  g_object_set(sequence,"length",16L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern1);
  bt_sequence_set_pattern(sequence,4,0,(BtCmdPattern *)pattern2);
  bt_pattern_set_global_event(pattern1,0,0,"50");
  bt_pattern_set_global_event(pattern1,4,0,"100");
  bt_pattern_set_global_event(pattern2,0,0,"200"); /* value shadows above */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* act */
  bt_pattern_set_global_event(pattern2,0,0,NULL);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq(element,"g-ulong",50);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern1);
  g_object_unref(pattern2);
  g_object_unref(machine);
  g_object_unref(sequence);
}
BT_END_TEST


BT_START_TEST(test_btsequence_combine_two_tracks) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(machine,"machine"));
  GstClockTime tick_time=bt_sequence_get_bar_time(sequence);
  g_object_set(sequence,"length",4L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  bt_sequence_set_pattern(sequence,1,1,(BtCmdPattern *)pattern);
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_pattern_set_global_event(pattern,1,0,"100");
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(1)*tick_time);
  
  /* act */
  bt_sequence_set_pattern(sequence,1,1,NULL);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(1)*tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq(element,"g-ulong",100);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_unref(sequence);
}
BT_END_TEST


typedef struct {
  gint ct;
  gint values[8];
} BtSequenceTicksTestData;

static void on_btsequence_ticks_notify(GstObject *machine,GParamSpec *arg,gpointer user_data) {
  BtSequenceTicksTestData *data=(BtSequenceTicksTestData *)user_data;

  if(data->ct<9) {
    g_object_get(machine,"wave",&data->values[data->ct],NULL);
  }
  data->ct++;
}

BT_START_TEST(test_btsequence_ticks) {
  /* arrange */
  BtSequenceTicksTestData data = {0,};
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtSongInfo *song_info=BT_SONG_INFO(check_gobject_get_object_property(song, "song-info"));
  g_object_set(song_info,"bpm",150L,"tpb",16L,NULL);
  /* need a real element that handles tempo and calls gst_object_sync */
  BtMachine *src=BT_MACHINE(bt_source_machine_new(song,"gen","simsyn",0,NULL));
  BtMachine *sink = BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire = bt_wire_new(song,src,sink,NULL);  
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,src);
  GstObject *element=GST_OBJECT(check_gobject_get_object_property(src,"machine"));
  g_object_set(sequence,"length",8L,NULL);
  bt_sequence_add_track(sequence,src,-1);
  bt_pattern_set_global_event(pattern,0,1,"0");
  bt_pattern_set_global_event(pattern,1,1,"1");
  bt_pattern_set_global_event(pattern,2,1,"2");
  bt_pattern_set_global_event(pattern,3,1,"3");
  bt_pattern_set_global_event(pattern,4,1,"4");
  bt_pattern_set_global_event(pattern,5,1,"5");
  bt_pattern_set_global_event(pattern,6,1,"6");
  bt_pattern_set_global_event(pattern,7,1,"7");
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);

  g_signal_connect(G_OBJECT(element),"notify::wave",G_CALLBACK(on_btsequence_ticks_notify),&data);

  /* act */
  bt_song_play(song);
  g_usleep(G_USEC_PER_SEC/5);
  bt_song_stop(song);
  
  /* assert */
  ck_assert_int_eq(data.ct,8);
  ck_assert_int_eq(data.values[0],0);
  ck_assert_int_eq(data.values[1],1);
  ck_assert_int_eq(data.values[2],2);
  ck_assert_int_eq(data.values[3],3);
  ck_assert_int_eq(data.values[4],4);
  ck_assert_int_eq(data.values[5],5);
  ck_assert_int_eq(data.values[6],6);
  ck_assert_int_eq(data.values[7],7);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(src);
  g_object_unref(sink);
  g_object_unref(wire);
  g_object_unref(sequence);
  g_object_unref(song_info);
}
BT_END_TEST


BT_START_TEST(test_btsequence_validate_loop) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  g_object_set(sequence,"length",16L,NULL);

  /* act */
  g_object_set(sequence,"loop",TRUE,NULL);

  /* assert */
  ck_assert_gobject_glong_eq(sequence,"loop-start",0);
  ck_assert_gobject_glong_eq(sequence,"loop-end",16);

  /* cleanup */
  g_object_try_unref(sequence);
}
BT_END_TEST


TCase *bt_sequence_example_case(void) {
  TCase *tc = tcase_create("BtSequenceExamples");

  tcase_add_test(tc,test_btsequence_new);
  tcase_add_test(tc,test_btsequence_labels);
  tcase_add_test(tc,test_btsequence_tracks);
  tcase_add_test(tc,test_btsequence_pattern);
  tcase_add_test(tc,test_btsequence_enlarge_length);
  tcase_add_test(tc,test_btsequence_enlarge_length_labels);
  tcase_add_test(tc,test_btsequence_shrink_length);
  tcase_add_test(tc,test_btsequence_enlarge_track);
  tcase_add_test(tc,test_btsequence_enlarge_track_vals);
  tcase_add_test(tc,test_btsequence_shrink_track);
  tcase_add_test(tc,test_btsequence_enlarge_both_vals);
  //tcase_add_test(tc,test_btsequence_update);
  tcase_add_test(tc,test_btsequence_change_pattern);
  tcase_add_test(tc,test_btsequence_hold);
  tcase_add_test(tc,test_btsequence_combine_pattern_shadows);
  tcase_add_test(tc,test_btsequence_combine_pattern_unshadows);
  tcase_add_test(tc,test_btsequence_combine_value_unshadows);
  tcase_add_test(tc,test_btsequence_combine_two_tracks);
  tcase_add_test(tc,test_btsequence_ticks);
  tcase_add_test(tc,test_btsequence_validate_loop);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
