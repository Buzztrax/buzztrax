/** $Id: t-setup.c,v 1.13 2005-02-16 19:10:27 waffel Exp $
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

START_TEST(test_btsetup_properties) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	gboolean check_prop_ret=FALSE;
	
	GST_INFO("--------------------------------------------------------------------------------");
  
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	check_prop_ret=check_gobject_properties(G_OBJECT(setup));
	fail_unless(check_prop_ret==TRUE,NULL);
}
END_TEST

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
	
	/* try to create the wire */
	wire=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink));
	
	/* try to add again the same wire */
	wire=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink));

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
* try to remove a machine from setup with NULL pointer for setup
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
  
	check_init_error_trapp("bt_setup_remove_machine",NULL);
	bt_setup_remove_machine(NULL,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a wire from setup with NULL pointer for setup
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
  
	check_init_error_trapp("bt_setup_remove_wire",NULL);
	bt_setup_remove_wire(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST


/**
* try to remove a machine from setup with NULL pointer for machine
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
	
	check_init_error_trapp("bt_setup_remove_machine",NULL);
	bt_setup_remove_machine(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a wire from setup with NULL pointer for wire
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
	
	check_init_error_trapp("bt_setup_remove_wire",NULL);
	bt_setup_remove_wire(setup,NULL);
	fail_unless(check_has_error_trapped(), NULL);
}
END_TEST

/**
* try to remove a machine from setup with a machine witch is never added
*/
START_TEST(test_btsetup_obj17) {
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
START_TEST(test_btsetup_obj18) {
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

/**
* try to add wire(src,dst) and wire(dst,src) to setup. This should fail (cycle).
*/
START_TEST(test_btsetup_wire1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtProcessorMachine *source=NULL;
	BtProcessorMachine *dst=NULL;
	// wire
	BtWire *wire_one=NULL;
	BtWire *wire_two=NULL;
	gboolean ret=FALSE;
	
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
	
	/* try to craete volume machine */
  source = bt_processor_machine_new(song,"src","volume",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create volume machine */
	dst = bt_processor_machine_new(song,"dst","volume",0);
	fail_unless(dst!=NULL, NULL);
	
	/* try to create the wire one */
	wire_one = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(dst));
	fail_unless(wire_one!=NULL, NULL);
	
	/* this should fail */
	wire_two = bt_wire_new(song, BT_MACHINE(dst), BT_MACHINE(source));
	fail_unless(wire_two!=NULL,NULL);
	
}
END_TEST

/**
* try to add wire(dst,src) and wire(src,dst) to setup. This should fail (cycle).
*/
START_TEST(test_btsetup_wire2) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtProcessorMachine *source=NULL;
	BtProcessorMachine *dst=NULL;
	// wire
	BtWire *wire_one=NULL;
	BtWire *wire_two=NULL;
	gboolean ret=FALSE;
	
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
	
	/* try to craete volume machine */
  source = bt_processor_machine_new(song,"src","volume",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create volume machine */
	dst = bt_processor_machine_new(song,"dst","volume",0);
	fail_unless(dst!=NULL, NULL);
	
	/* try to create the wire one */
	wire_one = bt_wire_new(song, BT_MACHINE(dst), BT_MACHINE(source));
	fail_unless(wire_one!=NULL, NULL);
	
	/* this should fail */	
	wire_two = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(dst));
	fail_unless(wire_two!=NULL,NULL);
	
}
END_TEST

/**
* try to add wire(src1,dst), wire(dst,src2) and wire(src2,scr1) to setup. This 
* should fail (cycle).
*/
START_TEST(test_btsetup_wire3) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtProcessorMachine *src1=NULL;
	BtProcessorMachine *src2=NULL;
	BtProcessorMachine *dst=NULL;
	// wire
	BtWire *wire_one=NULL;
	BtWire *wire_two=NULL;
	BtWire *wire_three=NULL;
	gboolean ret=FALSE;
	
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
	
		/* try to craete volume machine */
  src1 = bt_processor_machine_new(song,"src1","volume",0);
  fail_unless(src1!=NULL, NULL);
	
	/* try to craete volume machine */
  src2 = bt_processor_machine_new(song,"src2","volume",0);
  fail_unless(src2!=NULL, NULL);

	/* try to create volume machine */
	dst = bt_processor_machine_new(song,"dst","volume",0);
	fail_unless(dst!=NULL, NULL);
	
	/* try to create the wire one */
	wire_one = bt_wire_new(song, BT_MACHINE(src1), BT_MACHINE(dst));
	fail_unless(wire_one!=NULL, NULL);
	
	wire_two = bt_wire_new(song, BT_MACHINE(dst), BT_MACHINE(src2));
	fail_unless(wire_two!=NULL,NULL);
	
	/* this should fail */
	wire_three = bt_wire_new(song, BT_MACHINE(src2), BT_MACHINE(src1));
	fail_unless(wire_three!=NULL,NULL);
	
}
END_TEST

START_TEST(test_btsetup_get1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	// wire
	BtWire *wire=NULL;
	BtWire *ref_wire=NULL;
	/* wire list */
	GList* wire_list=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	
}
END_TEST

TCase *bt_setup_obj_tcase(void) {
  TCase *tc = tcase_create("bt_setup case");

	tcase_add_test(tc,test_btsetup_properties);
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
	tcase_add_test(tc,test_btsetup_wire1);
	tcase_add_test(tc,test_btsetup_wire2);
	tcase_add_test(tc,test_btsetup_wire3);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_suite(void) { 
  Suite *s=suite_create("BtSetup"); 

  suite_add_tcase(s,bt_setup_obj_tcase());
  return(s);
}
