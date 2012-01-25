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

/* this is an abstract base class, which should not be instantiable
 * unfortunately glib manages again to error out here in a fashion that exits the app :(
 */
/*
BT_START_TEST(test_btmachine_abstract) {
  BtMachine *machine;

  machine=g_object_new(BT_TYPE_MACHINE,NULL);
  fail_unless(machine==NULL,NULL);
}
BT_END_TEST
*/

/*
 * audiotestsrc | volume | audio_sink
 * mute audiotestsrc, mute volume, unmute volume, test if volume still is muted
 */
BT_START_TEST(test_btmachine_state1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  GError *err=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtProcessorMachine *volume=NULL;
  BtSinkMachine *sink=NULL;
  // wires
  BtWire *wire_src_proc=NULL;
  BtWire *wire_proc_sink=NULL;
  // machine states
  BtMachineState state_ref;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);

  /* create a new source machine */
  source=bt_source_machine_new(song,"source","audiotestsrc",0,&err);

  /* create a new processor machine */
  volume=bt_processor_machine_new(song,"volume","volume",0,&err);

  /* create a new sink machine */
  sink=bt_sink_machine_new(song, "alsasink",&err);

  /* create wire (src,proc) */
  wire_src_proc=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(volume),&err);

  /* create wire (proc,sink) */
  wire_proc_sink=bt_wire_new(song,BT_MACHINE(volume),BT_MACHINE(sink),&err);

  /* start setting the states */
  g_object_set(source,"state",BT_MACHINE_STATE_MUTE,NULL);
  g_object_get(source,"state",&state_ref,NULL);
  fail_unless(state_ref==BT_MACHINE_STATE_MUTE,NULL);

  g_object_set(volume,"state",BT_MACHINE_STATE_MUTE,NULL);
  g_object_get(volume,"state",&state_ref,NULL);
  fail_unless(state_ref==BT_MACHINE_STATE_MUTE,NULL);

  g_object_set(source,"state",BT_MACHINE_STATE_NORMAL,NULL);
  g_object_get(source,"state",&state_ref,NULL);
  fail_unless(state_ref==BT_MACHINE_STATE_NORMAL,NULL);

  g_object_get(volume,"state",&state_ref,NULL);
  fail_unless(state_ref==BT_MACHINE_STATE_MUTE,NULL);

  g_object_unref(wire_proc_sink);
  g_object_unref(wire_src_proc);
  g_object_unref(sink);
  g_object_unref(volume);
  g_object_unref(source);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * audiotestsrc1, audiotestsrc2 | audio_sink
 * solo audiotestsrc1, solo audiotestsrc2, test that audiotestsrc1 is not solo
 */
BT_START_TEST(test_btmachine_state2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  GError *err=NULL;
  // machines
  BtSourceMachine *sine1=NULL;
  BtSourceMachine *sine2=NULL;
  BtSinkMachine *sink=NULL;
  // wires
  BtWire *wire_sine1_sink=NULL;
  BtWire *wire_sine2_sink=NULL;
  // machine states
  BtMachineState state_ref;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);

  /* create a new sine source machine */
  sine1=bt_source_machine_new(song,"sine1","audiotestsrc",0,&err);

  /* create a new sine source machine */
  sine2=bt_source_machine_new(song,"sine2","audiotestsrc",0,&err);

  /* create a new sink machine */
  sink=bt_sink_machine_new(song,"alsasink",&err);

  /* create wire (sine1,src) */
  wire_sine1_sink=bt_wire_new(song,BT_MACHINE(sine1),BT_MACHINE(sink),&err);

  /* create wire (sine2,src) */
  wire_sine2_sink=bt_wire_new(song,BT_MACHINE(sine2),BT_MACHINE(sink),&err);
  mark_point();

  /* start setting the states */
  g_object_set(sine1,"state",BT_MACHINE_STATE_SOLO,NULL);
  g_object_set(sine2,"state",BT_MACHINE_STATE_SOLO,NULL);
  g_object_get(sine1,"state",&state_ref,NULL);
  fail_unless(state_ref!=BT_MACHINE_STATE_SOLO,NULL);

  g_object_unref(wire_sine1_sink);
  g_object_unref(wire_sine2_sink);
  g_object_unref(sink);
  g_object_unref(sine2);
  g_object_unref(sine1);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * create two machines from the same type, rename them and then link them in
 * checks that we ensure unique names
 */
BT_START_TEST(test_btmachine_names) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  GError *err=NULL;
  // machines
  BtSourceMachine *sine1=NULL;
  BtSourceMachine *sine2=NULL;
  BtSinkMachine *sink=NULL;
  // wires
  BtWire *wire_sine1_sink=NULL;
  BtWire *wire_sine2_sink=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);

  /* create a new sine source machine */
  sine1=bt_source_machine_new(song,"sine1","audiotestsrc",0,&err);
  g_object_set(sine1, "id", "beep1", NULL);

  /* create a new sine source machine */
  sine2=bt_source_machine_new(song,"sine2","audiotestsrc",0,&err);
  g_object_set(sine2, "id", "beep2", NULL);

  /* create a new sink machine */
  sink=bt_sink_machine_new(song,"alsasink",&err);
  mark_point();

  /* create wires */
  wire_sine2_sink=bt_wire_new(song,BT_MACHINE(sine2),BT_MACHINE(sink),&err);
  wire_sine1_sink=bt_wire_new(song,BT_MACHINE(sine1),BT_MACHINE(sink),&err);
  mark_point();

  g_object_unref(wire_sine1_sink);
  g_object_unref(wire_sine2_sink);
  g_object_unref(sink);
  g_object_unref(sine2);
  g_object_unref(sine1);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_machine_test_case(void) {
  TCase *tc = tcase_create("BtMachineTests");

  // TODO(ensonic): try catching the critical log
  //tcase_add_test(tc, test_btmachine_abstract);
  tcase_add_test(tc, test_btmachine_state1);
  tcase_add_test(tc, test_btmachine_state2);
  tcase_add_test(tc, test_btmachine_names);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
