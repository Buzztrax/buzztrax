/* $Id: t-sink-machine.c,v 1.1 2005-04-15 17:05:14 ensonic Exp $
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

// @todo change settings to test this now

/**
* try to create a machine with not exising plugin name
*/
START_TEST(test_btsinkmachine_obj1){
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSinkMachine *machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	
	/* try to create a source machine with wrong pluginname (not existing)*/
	//machine=bt_sink_machine_new(song,"id","nonsense");
	fail_unless(machine==NULL, NULL);
}
END_TEST

/**
* try to create a machine which is a source machine and not a sink machine
* here we mean the plugin-name from gst
*/
START_TEST(test_btsinkmachine_obj2){
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSinkMachine *machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	
	/* try to create a source machine with wrong plugin type (source instead of sink) */
	//machine=bt_sink_machine_new(song,"id","sinesource");
	fail_unless(machine==NULL, NULL);
}
END_TEST


TCase *bt_sink_machine_test_case(void) {
  TCase *tc = tcase_create("BtSinkMachineTests");

  //tcase_add_test(tc,test_btsinkmachine_obj1);
	//tcase_add_test(tc,test_btsinkmachine_obj2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
