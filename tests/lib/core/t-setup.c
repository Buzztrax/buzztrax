/** $Id: t-setup.c,v 1.3 2004-10-04 17:04:48 waffel Exp $
**/

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
* Try to add a NULL machine to a empty setup. We have no return value. But in
* this case no assertion should be thrown.
*
*/
START_TEST(test_btsetup_obj1){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	bt_setup_add_machine(setup, NULL);
	
}
END_TEST

/**
* Try to add a NULL wire to a empty setup. We have no return value. But in
* this case no assertion should be thrown.
*
*/
START_TEST(test_btsetup_obj2){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	bt_setup_add_wire(setup, NULL);
	
}
END_TEST

/**
* Try to add the same machine twice to the setup
*/
START_TEST(test_btsetup_obj3){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	BtSourceMachine *machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	
	/* try to create the machine */
	machine=bt_source_machine_new(song,"generator1","sinesource",0);
	
	/* try to add the machine to the setup */
	bt_setup_add_machine(setup,BT_MACHINE(machine));
	
	/*  try to add the same machine again */
	bt_setup_add_machine(setup, BT_MACHINE(machine));
}
END_TEST

/**
* try to add the same wire twice
*/
START_TEST(test_btsetup_obj4){
  BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	BtWire *wire=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	
	/* try to create the source machine */
	source=bt_source_machine_new(song,"generator1","sinesource",0);
	
	/*  try to create the sink machine */
	sink=bt_sink_machine_new(song,"sink1","esdsink");
	
	/* try to add the machines to the setup */
	bt_setup_add_machine(setup,BT_MACHINE(source));
	bt_setup_add_machine(setup,BT_MACHINE(sink));
	
	/* try to create the wire */
	bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup,wire);
	
	/* try to add again the same wire */
	bt_setup_add_wire(setup,wire);
}
END_TEST

TCase *bt_setup_obj_tcase(void) {
  TCase *tc = tcase_create("bt_setup case");

  tcase_add_test(tc,test_btsetup_obj1);
	tcase_add_test(tc,test_btsetup_obj2);
	tcase_add_test(tc,test_btsetup_obj3);
	tcase_add_test(tc,test_btsetup_obj4);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_suite(void) { 
  Suite *s=suite_create("BtSetup"); 

  suite_add_tcase(s,bt_setup_obj_tcase());
  return(s);
}
