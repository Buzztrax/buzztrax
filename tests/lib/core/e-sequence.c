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

BT_START_TEST(test_btsequence_enlarge_length) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSequence *sequence;
  gulong length;
  /* create app and song */
  app=bt_test_application_new();
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

  /* create app and song */
  app=bt_test_application_new();
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
  /* create app and song */
  app=bt_test_application_new();
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

  /* create app and song */
  app=bt_test_application_new();
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
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
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

  /* create app and song */
  app=bt_test_application_new();
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
  bt_sequence_add_track(sequence,machine1,-1);
  bt_sequence_add_track(sequence,machine1,-1);

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

  /* create app and song */
  app=bt_test_application_new();
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
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
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
  BtCmdPattern *pattern1,*pattern2;
  gulong i,j,length,tracks;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* try to create a pattern */
  pattern1=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern1!=NULL, NULL);

  /* try to enlarge length */
  g_object_set(sequence,"length",8L,NULL);
  g_object_get(sequence,"length",&length,NULL);
  fail_unless(length==8, NULL);

  /* set machine twice */
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);

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
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);
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

/* we moved these updates to the app, to give the undo/redo framework a chance
 * to backup the data
 */
#ifdef __CHECK_DISABLED__
// test that removing patterns updates the sequence
BT_START_TEST(test_btsequence_update) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;

  /* create app and song */
  app=bt_test_application_new();
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
  bt_sequence_add_track(sequence,machine,-1);

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
#endif

BT_START_TEST(test_btsequence_change_pattern) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern;
  GstObject *element;
  gulong val;

  /* create app and song */
  app=bt_test_application_new();
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
  bt_sequence_add_track(sequence,machine,-1);

  /* set pattern */
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);

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

  /* clean up */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_combine_pattern_shadows) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;
  GstObject *element;
  gulong val;
  GstClockTime tick_time;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine and get the gstreamer element */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create patterns */
  pattern1=bt_pattern_new(song,"pattern1","pattern1",8L,machine);
  fail_unless(pattern1!=NULL, NULL);
  pattern2=bt_pattern_new(song,"pattern2","pattern2",8L,machine);
  fail_unless(pattern2!=NULL, NULL);

  /* setup sequence */
  g_object_set(sequence,"length",16L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  tick_time=bt_sequence_get_bar_time(sequence);

  /* set patterns */
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern1);
  bt_sequence_set_pattern(sequence,4,0,(BtCmdPattern *)pattern2);

  /* value in 2nd pattern shadows above */
  bt_pattern_set_global_event(pattern1,0,0,"50");
  bt_pattern_set_global_event(pattern1,4,0,"100");
  bt_pattern_set_global_event(pattern2,0,0,"200");

  g_object_get(machine,"machine",&element,NULL);

  /* pull in the change */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==200, NULL);

  /* unset pattern */
  bt_sequence_set_pattern(sequence,4,0,NULL);

  /* pull in the change */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==100, NULL);

  /* clean up */
  gst_object_unref(element);
  g_object_unref(pattern1);
  g_object_unref(pattern2);
  g_object_unref(machine);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_combine_value_shadows) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern1,*pattern2;
  GstObject *element;
  gulong val;
  GstClockTime tick_time;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine and get the gstreamer element */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create patterns */
  pattern1=bt_pattern_new(song,"pattern1","pattern1",8L,machine);
  fail_unless(pattern1!=NULL, NULL);
  pattern2=bt_pattern_new(song,"pattern2","pattern2",8L,machine);
  fail_unless(pattern2!=NULL, NULL);

  /* setup sequence */
  g_object_set(sequence,"length",16L,NULL);
  bt_sequence_add_track(sequence,machine,-1);
  tick_time=bt_sequence_get_bar_time(sequence);

  /* set patterns */
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern1);
  bt_sequence_set_pattern(sequence,4,0,(BtCmdPattern *)pattern2);

  bt_pattern_set_global_event(pattern1,0,0,"50");
  bt_pattern_set_global_event(pattern1,4,0,"100");
  bt_pattern_set_global_event(pattern2,0,0,"200");

  g_object_get(machine,"machine",&element,NULL);

  /* pull in the change */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==200, NULL);

  /* unset value in 2nd pattern */
  bt_pattern_set_global_event(pattern2,0,0,NULL);

  /* pull in the change */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(4)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==50, NULL);

  /* clean up */
  gst_object_unref(element);
  g_object_unref(pattern1);
  g_object_unref(pattern2);
  g_object_unref(machine);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsequence_combine_two_tracks) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtMachine *machine;
  BtPattern *pattern;
  GstObject *element;
  gulong val;
  GstClockTime tick_time;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,NULL);
   /* create a source machine and get the gstreamer element */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err));
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  fail_unless(pattern!=NULL, NULL);

  /* setup sequence */
  g_object_set(sequence,"length",4L,NULL);
  tick_time=bt_sequence_get_bar_time(sequence);

  /* add two tracks for the machine */
  bt_sequence_add_track(sequence,machine,-1);
  bt_sequence_add_track(sequence,machine,-1);

  /* set pattern */
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  bt_sequence_set_pattern(sequence,1,1,(BtCmdPattern *)pattern);

  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_pattern_set_global_event(pattern,1,0,"100");

  g_object_get(machine,"machine",&element,NULL);
  
  /* pull in the change for timestamp of the 1st tick */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(0)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==50, NULL);

  /* pull in the change for timestamp of the 2nd tick */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(1)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==50, NULL);
  
  /* unset pattern */
  bt_sequence_set_pattern(sequence,1,1,NULL);

  /* pull in the change for timestamp of the 2nd tick */
  gst_object_sync_values(G_OBJECT(element),G_GUINT64_CONSTANT(1)*tick_time);

  /* and verify */
  g_object_get(element,"g-ulong",&val,NULL);
  fail_unless(val==100, NULL);

  /* clean up */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

typedef struct {
  gint ct;
  gint *values;
} BtSequenceTicksTestData;

static void on_btsequence_ticks_notify(GstObject *machine,GParamSpec *arg,gpointer user_data) {
  BtSequenceTicksTestData *data=(BtSequenceTicksTestData *)user_data;
  
  g_object_get(machine,"wave",&data->values[data->ct],NULL);
  GST_INFO("val[%d]=%d",data->ct,data->values[data->ct]);
  data->ct++;
}

BT_START_TEST(test_btsequence_ticks) {
  BtApplication *app;
  GError *err=NULL;
  BtSong *song;
  BtSequence *sequence;
  BtSongInfo *song_info;
  BtMachine *src,*sink;
  BtPattern *pattern;
  GstObject *element;
  BtWire *wire=NULL;
  gint val;
  BtSequenceTicksTestData data = {0,};
  gint values[8];

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"sequence",&sequence,"song-info",&song_info,NULL);
  /* use hight ticks-per-beat to make the test short ~ 0.2 sec */
  g_object_set(song_info,"bpm",150L,"tpb",16L,NULL);
  /* create a source machine and get the gstreamer element, 
   * need a real one that handles tempo and calls gst_object_sync */
  src=BT_MACHINE(bt_source_machine_new(song,"gen","simsyn",0,&err));
  fail_unless(src!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  g_object_get(src,"machine",&element,NULL);
  
  /* create sink and link src and sink */
  sink = BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  wire = bt_wire_new(song,src,sink,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,src);
  fail_unless(pattern!=NULL, NULL);

  /* enlarge length */
  g_object_set(sequence,"length",8L,NULL);

  /* set machine */
  bt_sequence_add_track(sequence,src,-1);

  /* set pattern */
  bt_pattern_set_global_event(pattern,0,1,"0");
  bt_pattern_set_global_event(pattern,1,1,"1");
  bt_pattern_set_global_event(pattern,2,1,"2");
  bt_pattern_set_global_event(pattern,3,1,"3");
  bt_pattern_set_global_event(pattern,4,1,"4");
  bt_pattern_set_global_event(pattern,5,1,"5");
  bt_pattern_set_global_event(pattern,6,1,"6");
  bt_pattern_set_global_event(pattern,7,1,"7");

  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);

  /* we should still have the default value */
  g_object_get(element,"wave",&val,NULL);
  fail_unless(val==0, NULL);

  data.values=values;
  g_signal_connect(G_OBJECT(element),"notify::wave",G_CALLBACK(on_btsequence_ticks_notify),&data);

  /* play the songs */
  bt_song_play(song);
  g_usleep(G_USEC_PER_SEC/5);
  bt_song_stop(song);
  GST_INFO("ct=%d",data.ct);
  
  /* check the captured values changes */
  fail_unless(data.ct==8, NULL);
  fail_unless(values[0]==0, NULL);
  fail_unless(values[1]==1, NULL);
  fail_unless(values[2]==2, NULL);
  fail_unless(values[3]==3, NULL);
  fail_unless(values[4]==4, NULL);
  fail_unless(values[5]==5, NULL);
  fail_unless(values[6]==6, NULL);
  fail_unless(values[7]==7, NULL);

  /* clean up */
  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(src);
  g_object_unref(sink);
  g_object_unref(wire);
  g_object_unref(sequence);
  g_object_unref(song_info);
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

  /* create app and song */
  app=bt_test_application_new();
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
  //tcase_add_test(tc,test_btsequence_update);
  tcase_add_test(tc,test_btsequence_change_pattern);
  tcase_add_test(tc,test_btsequence_combine_pattern_shadows);
  tcase_add_test(tc,test_btsequence_combine_value_shadows);
  tcase_add_test(tc,test_btsequence_combine_two_tracks);
  tcase_add_test(tc,test_btsequence_ticks);
  tcase_add_test(tc,test_btsequence_validate_loop);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
