/** $Id: t-setup.c,v 1.6 2004-10-22 16:15:58 ensonic Exp $
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
* Try to add the same machine twice to the setup
*/
START_TEST(test_btsetup_obj1){
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	BtSourceMachine *machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	/* try to create the machine */
	machine=bt_source_machine_new(song,"generator1","sinesrc",1);
	
	/* try to add the machine to the setup */
	bt_setup_add_machine(setup,BT_MACHINE(machine));
	
	/* try to add the same machine again */
	bt_setup_add_machine(setup, BT_MACHINE(machine));
  
  /* try to free the ressources */
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
END_TEST

/**
* try to add the same wire twice
*/
START_TEST(test_btsetup_obj2){
  BtApplication *app=NULL;
  BtSong *song=NULL;
	BtSetup *setup=NULL;
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	BtWire *wire=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	/* try to create the source machine */
	source=bt_source_machine_new(song,"generator1","sinesrc",1);
	
	/*  try to create the sink machine */
	sink=bt_sink_machine_new(song,"sink1");
	
	/* try to add the machines to the setup */
	bt_setup_add_machine(setup,BT_MACHINE(source));
	bt_setup_add_machine(setup,BT_MACHINE(sink));
	
	/* try to create the wire */
	wire=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup,wire);
	
	/* try to add again the same wire */
	bt_setup_add_wire(setup,wire);

  /* try to free the ressources */
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
END_TEST

TCase *bt_setup_obj_tcase(void) {
  TCase *tc = tcase_create("bt_setup case");

  tcase_add_test(tc,test_btsetup_obj1);
	tcase_add_test(tc,test_btsetup_obj2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_suite(void) { 
  Suite *s=suite_create("BtSetup"); 

  suite_add_tcase(s,bt_setup_obj_tcase());
  return(s);
}
