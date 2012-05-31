/* Buzztard
 * Copyright (C) 2009 Buzztard team <buzztard-devel@lists.sf.net>
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
static BtSettings *settings;

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  app=bt_test_application_new();
  song=bt_song_new(app);
  settings=bt_settings_make();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  g_object_unref(settings);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
  bt_core_teardown();
}


//-- tests

BT_START_TEST(test_btsinkmachine_new) {
  /* arrange */
  g_object_set(settings,"audiosink","fakesink",NULL);

  /* act */
  GError *err=NULL;
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",&err);

  /* assert */
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* cleanup */
  g_object_try_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_def_patterns) {
  /* arrange */
  g_object_set(settings,"audiosink","fakesink",NULL);
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);

  /* act */
  GList *list;
  g_object_get(G_OBJECT(machine),"patterns",&list,NULL);

  /* assert */
  fail_unless(list!=NULL, NULL);
  ck_assert_int_eq(g_list_length(list),2);

  /* cleanup */
  g_list_foreach(list,(GFunc)g_object_unref,NULL);
  g_list_free(list);
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_pattern) {
  /* arrange */
  g_object_set(settings,"audiosink","fakesink",NULL);
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);
  
  /* act */
  BtCmdPattern *pattern=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  
  /* assert */
  fail_unless(pattern!=NULL, NULL);
  ck_assert_gobject_gulong_eq(pattern,"voices",0);

  /* cleanup */
  g_object_unref(pattern);
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_pattern_by_id) {
  /* arrange */
  g_object_set(settings,"audiosink","fakesink",NULL);
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);
  
  /* act */
  BtCmdPattern *pattern=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_machine_get_pattern_by_id(BT_MACHINE(machine),"pattern-id"),pattern);

  /* cleanup */
  g_object_unref(pattern);
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_pattern_by_list) {
  GList *list,*node;

  /* arrange */
  g_object_set(settings,"audiosink","fakesink",NULL);
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);
  BtCmdPattern *pattern=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  g_object_get(G_OBJECT(machine),"patterns",&list,NULL);

  /* act */
  node=g_list_last(list);

  /* assert */
  fail_unless(node->data==pattern, NULL);

  /* cleanup */
  g_list_foreach(list,(GFunc)g_object_unref,NULL);
  g_list_free(list);

  g_object_unref(pattern);
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_default) {
  BtSinkMachine *machine;
  GError *err=NULL;

  /* configure a sink for testing */
  g_object_set(settings,"audiosink",NULL,NULL);

  /* arrange */

  /* create a machine */
  machine=bt_sink_machine_new(song,"master",&err);
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  g_object_try_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_fallback) {
  BtSinkMachine *machine;
  GError *err=NULL;
  gchar *settings_str=NULL;

  /* configure a sink for testing */
  g_object_set(settings,"audiosink",NULL,NULL);
  bt_test_settings_set(BT_TEST_SETTINGS(settings),"system-audiosink",&settings_str);

  /* arrange */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* create a machine */
  machine=bt_sink_machine_new(song,"master",&err);
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  g_object_try_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_latency) {
  BtSongInfo *song_info=NULL;
  BtSinkMachine *machine;
  GstElement *sink_bin,*sink=NULL;
  GError *err=NULL;
  GList *node;
  gulong bpm, tpb, st, c_bpm, c_tpb;
  gint64 latency_time,c_latency_time;
  guint latency;

  /* arrange */
  g_object_get(song,"song-info",&song_info,NULL);

  /* create a machine */
  machine=bt_sink_machine_new(song,"master",&err);
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  g_object_get(machine,"machine",&sink_bin,NULL);
  fail_unless(sink_bin != NULL, NULL);
  fail_unless(BT_IS_SINK_BIN(sink_bin), NULL);
  // grab the audio sink from the child_list
  for(node=GST_BIN_CHILDREN(sink_bin);node;node=g_list_next(node)) {
    if(GST_IS_BASE_AUDIO_SINK(node->data)) {
      sink=node->data;
      break;
    }
  }
  fail_unless(sink != NULL, NULL);
  GST_INFO_OBJECT(sink,"before test loop");
  
  // set various bpm, tpb on song_info, set various latency on settings
  // assert the resulting latency-time properties on the audio_sink
  for(latency=20;latency<=80;latency+=10) {
    g_object_set(settings,"latency",latency,NULL);
    for(bpm=80;bpm<=160;bpm+=10) {
      g_object_set(song_info,"bpm",bpm,NULL);
      for(tpb=4;tpb<=8;tpb+=2) {
        g_object_set(song_info,"bpm",bpm,"tpb",tpb,NULL);
        g_object_get(sink_bin,"subticks-per-tick",&st,"ticks-per-beat",&c_tpb,"beats-per-minute",&c_bpm,NULL);
        g_object_get(sink,"latency-time",&c_latency_time,NULL);
        latency_time=GST_TIME_AS_USECONDS((GST_SECOND*60)/(bpm*tpb*st));
        GST_INFO_OBJECT(sink,
          "bpm=%3lu=%3lu, tpb=%1lu=%1lu, stpb=%2lu, target-latency=%2u , latency-time=%6"G_GINT64_FORMAT"=%6"G_GINT64_FORMAT", delta=%+4d ",
          bpm,c_bpm,tpb,c_tpb,
          st,latency,
          latency_time,c_latency_time,
          (latency_time-((gint)latency*1000))/1000);
        fail_unless(c_bpm == bpm, NULL);
        fail_unless(c_tpb == tpb, NULL);
        fail_unless(c_latency_time == latency_time, NULL);
      }
    }
  }

  gst_object_unref(sink_bin);
  g_object_try_unref(machine);
  g_object_unref(song_info);
}
BT_END_TEST


TCase *bt_sink_machine_example_case(void) {
  TCase *tc = tcase_create("BtSinkMachineExamples");

  tcase_add_test(tc,test_btsinkmachine_new);
  tcase_add_test(tc,test_btsinkmachine_def_patterns);
  tcase_add_test(tc,test_btsinkmachine_pattern);
  tcase_add_test(tc,test_btsinkmachine_pattern_by_id);
  tcase_add_test(tc,test_btsinkmachine_pattern_by_list);
  tcase_add_test(tc,test_btsinkmachine_default);
  tcase_add_test(tc,test_btsinkmachine_fallback);
  tcase_add_test(tc,test_btsinkmachine_latency);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
