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

static void suite_setup(void) {
  bt_core_setup();
}

static void test_setup(void) {
  app=bt_test_application_new();
  song=bt_song_new(app);
  settings=bt_settings_make();
}

static void test_teardown(void) {
  g_object_unref(settings);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}

static void suite_teardown(void) {
  bt_core_teardown();
}

//-- helper

static GstElement *get_sink_element(GstBin *bin) {
  GstElement *e;
  GList *node;
  
  GST_INFO_OBJECT(bin,"looking for audio_sink");
  for(node=GST_BIN_CHILDREN(bin);node;node=g_list_next(node)) {
    e=(GstElement *)node->data;
    if(GST_IS_BIN(e) && GST_OBJECT_FLAG_IS_SET(e,GST_ELEMENT_IS_SINK)) {
      return get_sink_element((GstBin *)e);
    }
    if(GST_IS_BASE_AUDIO_SINK(e)) {
      return e;
    }
  }
  return NULL;
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
  GList *list=(GList *)check_gobject_get_ptr_property(machine,"patterns");

  /* assert */
  fail_unless(list!=NULL, NULL);
  ck_assert_int_eq(g_list_length(list),2); /* break+mute */

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
  /* arrange */
  g_object_set(settings,"audiosink","fakesink",NULL);
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);
  BtCmdPattern *pattern=(BtCmdPattern *)bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  GList *list=(GList *)check_gobject_get_ptr_property(machine,"patterns");

  /* act */
  GList *node=g_list_last(list);

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
  /* arrange */
  g_object_set(settings,"audiosink",NULL,NULL);

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


BT_START_TEST(test_btsinkmachine_fallback) {
  /* arrange */
  gchar *settings_str=NULL;
  g_object_set(settings,"audiosink",NULL,NULL);
  bt_test_settings_set(BT_TEST_SETTINGS(settings),"system-audiosink",&settings_str);

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


BT_START_TEST(test_btsinkmachine_actual_sink) {
  /* arrange */
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);

  /* act */
  GstElement *sink_bin=GST_ELEMENT(check_gobject_get_object_property(machine,"machine"));
  gst_element_set_state(sink_bin, GST_STATE_READY);
  GstElement *sink=get_sink_element((GstBin *)sink_bin);
  
  /* assert */
  fail_unless(sink != NULL, NULL);

  /* cleanup */
  gst_object_unref(sink_bin);
  g_object_try_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btsinkmachine_latency) {
  gulong bpm, tpb, st, c_bpm, c_tpb;
  gint64 latency_time,c_latency_time;
  guint latency;

  /* arrange */
  BtSongInfo *song_info=BT_SONG_INFO(check_gobject_get_object_property(song, "song-info"));
  BtSinkMachine *machine=bt_sink_machine_new(song,"master",NULL);
  GstElement *sink_bin=GST_ELEMENT(check_gobject_get_object_property(machine,"machine"));
  gst_element_set_state(sink_bin, GST_STATE_READY);
  GstElement *sink=get_sink_element((GstBin *)sink_bin);
  
  /* act */
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
          "bpm=%3lu=%3lu, tpb=%"G_GUINT64_FORMAT"=%"G_GUINT64_FORMAT", stpb=%2lu, target-latency=%2u , latency-time=%6"G_GINT64_FORMAT"=%6"G_GINT64_FORMAT", delta=%+4d ",
          bpm,c_bpm,tpb,c_tpb,
          st,latency,
          latency_time,c_latency_time,
          (latency_time-((gint)latency*1000))/1000);
        
        /* assert */
        ck_assert_ulong_eq(c_bpm,bpm);
        ck_assert_ulong_eq(c_tpb,tpb);
        ck_assert_int64_eq(c_latency_time,latency_time);
      }
    }
  }

  /* cleanup */
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
  tcase_add_test(tc,test_btsinkmachine_actual_sink);
  tcase_add_test(tc,test_btsinkmachine_latency);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc,suite_setup, suite_teardown);
  return(tc);
}
