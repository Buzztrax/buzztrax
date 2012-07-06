/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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
static gfloat minv,maxv;

//-- fixtures

static void case_setup(void) {
  GST_INFO("================================================================================");
}

static void test_setup(void) {
  app=bt_test_application_new();
  settings=bt_settings_make();
  song=bt_song_new(app);
  minv=G_MAXFLOAT;
  maxv=-G_MAXFLOAT;
}

static void test_teardown(void) {
  g_object_checked_unref(song);
  g_object_unref(settings);
  g_object_checked_unref(app);
}

static void case_teardown(void) {
}

//-- helper

static void make_new_song(void) {
  BtSequence *sequence=(BtSequence *)check_gobject_get_object_property(song,"sequence");
  BtMachine *sink=BT_MACHINE(bt_sink_machine_new(song,"master",NULL));
  BtMachine *gen=BT_MACHINE(bt_source_machine_new(song,"gen","audiotestsrc",0L,NULL));
  BtWire *wire=bt_wire_new(song, gen, sink,NULL);
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(gen));
  GstElement *element=(GstElement *)check_gobject_get_object_property(gen,"machine");

  g_object_set(sequence,"length",64L,NULL);
  bt_sequence_add_track(sequence,gen,-1);
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  bt_sequence_set_pattern(sequence,16,0,(BtCmdPattern *)pattern);
  bt_sequence_set_pattern(sequence,32,0,(BtCmdPattern *)pattern);
  bt_sequence_set_pattern(sequence,48,0,(BtCmdPattern *)pattern);
  g_object_set(element,"wave",/* square */ 1,"volume",1.0,NULL);

  gst_object_unref(element);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_unref(gen);
  g_object_unref(sink);
  g_object_unref(sequence);
  GST_INFO("  song created");
}

static GstElement *get_sink_element(GstBin *bin) {
  GstElement *e;
  GList *node;
  
  GST_INFO_OBJECT(bin,"looking for audio_sink");
  for(node=GST_BIN_CHILDREN(bin);node;node=g_list_next(node)) {
    e=(GstElement *)node->data;
    if(GST_IS_BIN(e) && GST_OBJECT_FLAG_IS_SET(e,GST_ELEMENT_IS_SINK)) {
      return get_sink_element((GstBin *)e);
    }
    if(GST_OBJECT_FLAG_IS_SET(e,GST_ELEMENT_IS_SINK)) {
      return e;
    }
  }
  return NULL;
}
 
static void
handoff_buffer_cb (GstElement * fakesink, GstBuffer * buffer, GstPad * pad,
    gpointer user_data)
{
  gint i,num_samples;
  gfloat *data=(gfloat *)GST_BUFFER_DATA(buffer);
  GST_DEBUG_OBJECT(fakesink,"got buffer %p, caps=%"GST_PTR_FORMAT,buffer,GST_BUFFER_CAPS(buffer));
  num_samples=GST_BUFFER_SIZE(buffer)/sizeof(gfloat);
  for(i=0;i<num_samples;i++) {
    if(data[i]<minv) minv=data[i];
    else if(data[i]>maxv) maxv=data[i];
  }
}

//-- tests

BT_START_TEST(test_bt_sink_bin_new) {
  /* arrange */

  /* act */
  GstElement *bin=gst_element_factory_make("bt-sink-bin",NULL);

  /* assert */
  fail_unless(bin != NULL, NULL);

  /* cleanup */
  gst_object_unref(bin);
}
BT_END_TEST

/* test playback */
/* test recording */
/* test playback + recording */

/* test master volume, using appsink? */
BT_START_TEST(test_bt_sink_bin_master_volume) {
  /* arrange */
  gdouble volume=1.0/(gdouble)_i;
  g_object_set(settings,"audiosink","fakesink",NULL);
  make_new_song();
  BtSetup *setup=BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *machine=bt_setup_get_machine_by_type(setup,BT_TYPE_SINK_MACHINE);
  GstElement *sink_bin=GST_ELEMENT(check_gobject_get_object_property(machine,"machine"));
  gst_element_set_state(sink_bin, GST_STATE_READY);
  GstElement *fakesink=get_sink_element((GstBin *)sink_bin);
  g_object_set(fakesink,"signal-handoffs",TRUE,NULL);
  g_signal_connect(fakesink, "handoff", (GCallback) handoff_buffer_cb, NULL);

  /* act */
  g_object_set(sink_bin,"master-volume",volume,NULL);
  bt_song_play(song);
  check_run_main_loop_for_usec(G_USEC_PER_SEC/10);

  /* assert */
  GST_INFO("minv=%7.4lf, maxv=%7.4lf",minv,maxv);
  ck_assert_float_eq(maxv,+volume);
  ck_assert_float_eq(minv,-volume);

  /* cleanup */
  bt_song_stop(song);
  gst_object_unref(sink_bin);
  g_object_try_unref(machine);
  g_object_unref(setup);
}
BT_END_TEST

/* insert analyzers */
BT_START_TEST(test_bt_sink_bin_analyzers) {
  /* arrange */
  make_new_song();
  BtSetup *setup=BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *machine=bt_setup_get_machine_by_type(setup,BT_TYPE_SINK_MACHINE);
  GstElement *sink_bin=GST_ELEMENT(check_gobject_get_object_property(machine,"machine"));
  GstElement *fakesink=gst_element_factory_make("fakesink",NULL);
  GstElement *queue=gst_element_factory_make("queue",NULL);
  GList *analyzers_list=g_list_prepend(g_list_prepend(NULL,fakesink),queue);
  g_object_set(sink_bin,"analyzers",analyzers_list,NULL);
  g_object_set(fakesink,"signal-handoffs",TRUE,NULL);
  g_signal_connect(fakesink, "handoff", (GCallback) handoff_buffer_cb, NULL);

  /* act */
  bt_song_play(song);
  check_run_main_loop_for_usec(G_USEC_PER_SEC/10);
  bt_song_write_to_lowlevel_dot_file(song);

  /* assert */
  GST_INFO("minv=%7.4lf, maxv=%7.4lf",minv,maxv);
  ck_assert_float_eq(maxv,+1.0);
  ck_assert_float_eq(minv,-1.0);

  /* cleanup */
  bt_song_stop(song);
  gst_object_unref(sink_bin);
  g_object_try_unref(machine);
}
BT_END_TEST


TCase *bt_sink_bin_example_case(void) {
  TCase *tc = tcase_create("BtSinkBinExamples");

  tcase_add_test(tc,test_bt_sink_bin_new);
  tcase_add_loop_test(tc,test_bt_sink_bin_master_volume,1,3);
  tcase_add_test(tc,test_bt_sink_bin_analyzers);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
