/** $Id: t-machine.c,v 1.1 2005-01-31 19:05:11 waffel Exp $ **/

#include "t-core.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
}

//-- tests

/**
* test following scenario:
* * sinesrc | volume | audio_sink
* 1) mute sinesrc
* 2) mute volume
* 3) unmute volume
* 4) test if volume still is muted
*/
START_TEST(test_btmachine_state1) {
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
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	/* create a new source machine */
	source=bt_source_machine_new(song,"source","sinesrc",0);
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
	g_object_set(volume,"state",BT_MACHINE_STATE_MUTE,NULL);
	g_object_set(source,"state",BT_MACHINE_STATE_NORMAL,NULL);
	g_object_get(volume,"state",&state_ref,NULL);
	fail_unless(state_ref==BT_MACHINE_STATE_MUTE,NULL);
	
}
END_TEST

TCase *bt_machine_obj_tcase(void) {
  TCase *tc = tcase_create("bt_machine case");
	
	tcase_add_test(tc, test_btmachine_state1);
  return(tc);
}

Suite *bt_machine_suite(void) { 
  Suite *s=suite_create("BtMachine"); 

  suite_add_tcase(s,bt_machine_obj_tcase());
  return(s);
}
