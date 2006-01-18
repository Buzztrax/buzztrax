/* $Id: t-machine.c,v 1.14 2006-01-18 13:11:55 ensonic Exp $
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

/**
 * audiotestsrc | volume | audio_sink
 * mute audiotestsrc, mute volume, unmute volume, test if volume still is muted
 *
 * needs song in playing state
 */
#ifdef __CHECK_DISABLED__
BT_START_TEST(test_btmachine_state1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtProcessorMachine *volume=NULL;
  BtSinkMachine *sink=NULL;
  // wires
  BtWire *wire_src_proc=NULL;
  BtWire *wire_proc_sink=NULL;
  // machine states
  BtMachineState state_ref;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* create a new source machine */
  source=bt_source_machine_new(song,"source","audiotestsrc",0);
  bt_setup_add_machine(setup,BT_MACHINE(source));
  
  /* create a new processor machine */
  volume=bt_processor_machine_new(song,"volume","volume",0);
  bt_setup_add_machine(setup,BT_MACHINE(volume));
  
  /* create a new sink machine */
  sink=bt_sink_machine_new(song, "alsasink");
  bt_setup_add_machine(setup,BT_MACHINE(sink));
  
  /* create wire (src,proc) */
  wire_src_proc=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(volume));
  bt_setup_add_wire(setup,wire_src_proc);
  
  /* create wire (proc,sink) */
  wire_proc_sink=bt_wire_new(song,BT_MACHINE(volume),BT_MACHINE(sink));
  bt_setup_add_wire(setup,wire_proc_sink);
  
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
#endif

/**
 * audiotestsrc1, audiotestsrc2 | audio_sink
 * solo audiotestsrc1, solo audiotestsrc2, test that audiotestsrc1 is not solo
 *
 * needs song in playing state
 */
#ifdef __CHECK_DISABLED__
BT_START_TEST(test_btmachine_state2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *sine1=NULL;
  BtSourceMachine *sine2=NULL;
  BtSinkMachine *sink=NULL;
  // wires
  BtWire *wire_sine1_sink=NULL;
  BtWire *wire_sine2_sink=NULL;
  // machine states
  BtMachineState state_ref;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* create a new sine source machine */
  sine1=bt_source_machine_new(song,"sine1","audiotestsrc",0);
  bt_setup_add_machine(setup,BT_MACHINE(sine1));
  
  /* create a new sine source machine */
  sine2=bt_source_machine_new(song,"sine2","audiotestsrc",0);
  bt_setup_add_machine(setup,BT_MACHINE(sine2));
  
  /* create a new sink machine */
  sink=bt_sink_machine_new(song,"alsasink");
  bt_setup_add_machine(setup,BT_MACHINE(sink));
  
  /* create wire (sine1,src) */
  wire_sine1_sink=bt_wire_new(song,BT_MACHINE(sine1),BT_MACHINE(sink));
  bt_setup_add_wire(setup, wire_sine1_sink);
  
  /* create wire (sine2,src) */
  wire_sine2_sink=bt_wire_new(song,BT_MACHINE(sine2),BT_MACHINE(sink));
  bt_setup_add_wire(setup, wire_sine2_sink);
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
#endif

TCase *bt_machine_test_case(void) {
  TCase *tc = tcase_create("BtMachineTests");
  
  // @todo try catching the critical log
  //tcase_add_test(tc, test_btmachine_abstract);
  /* @todo state change tests need the song to be in playing state,
     otherwise the calls block
  tcase_add_test(tc, test_btmachine_state1);
  tcase_add_test(tc, test_btmachine_state2);
  */
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  // we need to raise the default timeout of 3 seconds
  tcase_set_timeout(tc, 10);
  return(tc);
}
