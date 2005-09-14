/* $Id: t-source-machine.c,v 1.6 2005-09-14 10:16:34 ensonic Exp $
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

/**
* try to create a machine with not exising plugin name
*/
BT_START_TEST(test_btsourcemachine_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  /* try to create a source machine with wrong pluginname (not existing)*/
  machine=bt_source_machine_new(song,"id","nonsense",1);
  fail_unless(machine==NULL, NULL);
}
BT_END_TEST

/**
* try to create a machine which is a sink machine and not a source machine
* here we mean the plugin-name from gst
*/
BT_START_TEST(test_btsourcemachine_obj2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  /* try to create a source machine with wrong plugin type (sink instead of source) */
  machine=bt_source_machine_new(song,"id","esdsink",1);
  fail_unless(machine==NULL, NULL);
}
BT_END_TEST

BT_START_TEST(test_btsourcemachine_obj3){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  gulong testIdx=0;
  GError *error=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  /* try to create a normal sink machine */
  machine=bt_source_machine_new(song,"id","sinesrc",0);
  fail_unless(machine!=NULL,NULL);
  /* try to get global param index from sinesrc */
  testIdx=bt_machine_get_global_param_index(BT_MACHINE(machine),"nonsense",&error);
  fail_unless(g_error_matches(error,
                              g_quark_from_static_string("BtMachine"),
                              0),
                              NULL);
  g_error_free(error);
}
BT_END_TEST


TCase *bt_source_machine_test_case(void) {
  TCase *tc = tcase_create("BtSourceMachineTests");

  tcase_add_test(tc,test_btsourcemachine_obj1);
  tcase_add_test(tc,test_btsourcemachine_obj2);
  tcase_add_test(tc,test_btsourcemachine_obj3);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
