/** $Id: t-setup.c,v 1.8 2004-12-18 16:12:39 waffel Exp $
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

/**
* try to create a new setup with a NULL song object
*/
START_TEST(test_btsetup_obj3) {
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	check_init_error_trapp("bt_setup_new","BT_IS_SONG(song)");
	setup=bt_setup_new(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_add_machine with NULL object for self
*/
START_TEST(test_btsetup_obj4) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);

	check_init_error_trapp("bt_setup_add_machine","BT_IS_SETUP(self)");
	bt_setup_add_machine(NULL,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_add_machine with NULL object for machine
*/
START_TEST(test_btsetup_obj5) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);

	check_init_error_trapp("bt_setup_add_machine","BT_IS_MACHINE(machine)");
	bt_setup_add_machine(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_add_wire with NULL object for self
*/
START_TEST(test_btsetup_obj6) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);

	check_init_error_trapp("bt_setup_add_wire","BT_IS_SETUP(self)");
	bt_setup_add_wire(NULL,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_add_wire with NULL object for wire
*/
START_TEST(test_btsetup_obj7) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);

	check_init_error_trapp("bt_setup_add_wire","BT_IS_WIRE(wire)");
	bt_setup_add_wire(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_get_machine_by_id with NULL object for self
*/
START_TEST(test_btsetup_obj8) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);

	check_init_error_trapp("bt_setup_get_machine_by_id","BT_IS_SETUP(self)");
	bt_setup_get_machine_by_id(NULL,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_get_machine_by_id with NULL object for id
*/
START_TEST(test_btsetup_obj9) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);

	check_init_error_trapp("bt_setup_get_machine_by_id",NULL);
	bt_setup_get_machine_by_id(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call get_machine_by_index with NULL for setup parameter
*/
START_TEST(test_btsetup_obj10) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_get_machine_by_index","BT_IS_SETUP(self)");
	bt_setup_get_machine_by_index(NULL,0);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_get_wire_by_src_machine with NULL for setup parameter 
*/
START_TEST(test_btsetup_obj11) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_get_wire_by_src_machine","BT_IS_SETUP(self)");
	bt_setup_get_wire_by_src_machine(NULL,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_get_wire_by_src_machine with NULL for machine parameter 
*/
START_TEST(test_btsetup_obj12) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_get_wire_by_src_machine","BT_IS_MACHINE(src)");
	bt_setup_get_wire_by_src_machine(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_machine_iterator_new with NULL for self parameter 
*/
START_TEST(test_btsetup_obj13) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_machine_iterator_new","BT_IS_SETUP(self)");
	bt_setup_machine_iterator_new(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_machine_iterator_next with NULL for iterator parameter 
*/
START_TEST(test_btsetup_obj14) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_machine_iterator_next",NULL);
	bt_setup_machine_iterator_next(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_machine_iterator_get_machine with NULL for iterator parameter 
*/
START_TEST(test_btsetup_obj15) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_machine_iterator_get_machine",NULL);
	bt_setup_machine_iterator_get_machine(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_wire_iterator_new with NULL for self parameter 
*/
START_TEST(test_btsetup_obj16) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_wire_iterator_new","BT_IS_SETUP(self)");
	bt_setup_wire_iterator_new(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_wire_iterator_next with NULL for iterator parameter 
*/
START_TEST(test_btsetup_obj17) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_wire_iterator_next",NULL);
	bt_setup_wire_iterator_next(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to call bt_setup_wire_iterator_get_wire with NULL for iterator parameter 
*/
START_TEST(test_btsetup_obj18) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  
	check_init_error_trapp("bt_setup_wire_iterator_get_wire",NULL);
	bt_setup_wire_iterator_get_wire(NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a machine from setup with NULL pointer for setup
*/
START_TEST(test_btsetup_obj19) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  
	check_init_error_trapp("bt_setup_remove_machine",NULL);
	bt_setup_remove_machine(NULL,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a wire from setup with NULL pointer for setup
*/
START_TEST(test_btsetup_obj20) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  
	check_init_error_trapp("bt_setup_remove_wire",NULL);
	bt_setup_remove_wire(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST


/**
* try to remove a machine from setup with NULL pointer for machine
*/
START_TEST(test_btsetup_obj21) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_remove_machine",NULL);
	bt_setup_remove_machine(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a wire from setup with NULL pointer for wire
*/
START_TEST(test_btsetup_obj22) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	check_init_error_trapp("bt_setup_remove_wire",NULL);
	bt_setup_remove_wire(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a machine from setup with a machine witch is never added
*/
START_TEST(test_btsetup_obj23) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	BtSourceMachine *gen=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	/* try to craete generator1 with sinesrc */
  gen = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen!=NULL, NULL);
	
	check_init_error_trapp("bt_setup_remove_machine",
	   "trying to remove machine that is not in setup");
	bt_setup_remove_machine(setup,BT_MACHINE(gen));
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a wire from setup with a wire witch is never added
*/
START_TEST(test_btsetup_obj24) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	// wire
	BtWire *wire=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
	fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL,NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create sink machine with esd sink */
	sink = bt_sink_machine_new(song,"sink1");
	fail_unless(sink!=NULL, NULL);
	
	/* try to create the wire */
	wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
	fail_unless(wire!=NULL, NULL);
	
	check_init_error_trapp("bt_setup_remove_wire",
	   "trying to remove wire that is not in setup");
	bt_setup_remove_wire(setup,BT_WIRE(wire));
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

TCase *bt_setup_obj_tcase(void) {
  TCase *tc = tcase_create("bt_setup case");

  tcase_add_test(tc,test_btsetup_obj1);
	tcase_add_test(tc,test_btsetup_obj2);
	tcase_add_test(tc,test_btsetup_obj3);
	tcase_add_test(tc,test_btsetup_obj4);
	tcase_add_test(tc,test_btsetup_obj5);
	tcase_add_test(tc,test_btsetup_obj6);
	tcase_add_test(tc,test_btsetup_obj7);
	tcase_add_test(tc,test_btsetup_obj8);
	tcase_add_test(tc,test_btsetup_obj9);
	tcase_add_test(tc,test_btsetup_obj10);
	tcase_add_test(tc,test_btsetup_obj11);
	tcase_add_test(tc,test_btsetup_obj12);
	tcase_add_test(tc,test_btsetup_obj13);
	tcase_add_test(tc,test_btsetup_obj14);
	tcase_add_test(tc,test_btsetup_obj15);
	tcase_add_test(tc,test_btsetup_obj16);
	tcase_add_test(tc,test_btsetup_obj17);
	tcase_add_test(tc,test_btsetup_obj18);
	tcase_add_test(tc,test_btsetup_obj19);
	tcase_add_test(tc,test_btsetup_obj20);
	tcase_add_test(tc,test_btsetup_obj21);
	tcase_add_test(tc,test_btsetup_obj22);
	tcase_add_test(tc,test_btsetup_obj23);
	tcase_add_test(tc,test_btsetup_obj24);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_suite(void) { 
  Suite *s=suite_create("BtSetup"); 

  suite_add_tcase(s,bt_setup_obj_tcase());
  return(s);
}
