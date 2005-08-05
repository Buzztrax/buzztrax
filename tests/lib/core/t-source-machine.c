/* $Id: t-source-machine.c,v 1.3 2005-08-05 09:36:19 ensonic Exp $
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

/**
* try to create a machine with not exising plugin name
*/
START_TEST(test_btsourcemachine_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  /* try to create a source machine with wrong pluginname (not existing)*/
  machine=bt_source_machine_new(song,"id","nonsense",1);
  fail_unless(machine==NULL, NULL);
}
END_TEST

/**
* try to create a machine which is a sink machine and not a source machine
* here we mean the plugin-name from gst
*/
START_TEST(test_btsourcemachine_obj2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  /* try to create a source machine with wrong plugin type (sink instead of source) */
  machine=bt_source_machine_new(song,"id","esdsink",1);
  fail_unless(machine==NULL, NULL);
}
END_TEST

START_TEST(test_btsourcemachine_obj3){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  gulong testIdx=0;
  GError *error=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");
  
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
END_TEST


TCase *bt_source_machine_test_case(void) {
  TCase *tc = tcase_create("BtSourceMachineTests");

  tcase_add_test(tc,test_btsourcemachine_obj1);
  tcase_add_test(tc,test_btsourcemachine_obj2);
  tcase_add_test(tc,test_btsourcemachine_obj3);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
