/* $Id: t-machine.c,v 1.5 2005-06-14 07:19:55 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

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

/* this is an abstract base class, which should not be instantiable
 * unfortunately glib manages again to error out here in a fashion that exits the app :(
 */
/*
START_TEST(test_btmachine_abstract) {
	BtMachine *machine;
	
	machine=g_object_new(BT_TYPE_MACHINE,NULL);
	fail_unless(machine==NULL,NULL);
}
END_TEST
*/

/*
* sinesrc | volume | audio_sink
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

/*
* sinesrc1, sinesrc2 | audio_sink
* 1) solo sinesrc1
* 2) solo sinesrc2
* 3) test that sinesrc1 is not solo
*/
START_TEST(test_btmachine_state2) {
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
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	/* create a new sine source machine */
	sine1=bt_source_machine_new(song,"sine1","sinesrc",0);
	bt_setup_add_machine(setup,BT_MACHINE(sine1));
	
	/* create a new sine source machine */
	sine2=bt_source_machine_new(song,"sine2","sinesrc",0);
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
	
	/* start setting the states */
	g_object_set(sine1,"state",BT_MACHINE_STATE_SOLO,NULL);
	g_object_set(sine2,"state",BT_MACHINE_STATE_SOLO,NULL);
	g_object_get(sine1,"state",&state_ref,NULL);
	fail_unless(state_ref!=BT_MACHINE_STATE_SOLO,NULL);
}
END_TEST

TCase *bt_machine_test_case(void) {
  TCase *tc = tcase_create("BtMachineTests");
	
	//tcase_add_test(tc, test_btmachine_abstract);
	tcase_add_test(tc, test_btmachine_state1);
	tcase_add_test(tc, test_btmachine_state2);
	tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
