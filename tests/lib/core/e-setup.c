/** $Id: e-setup.c,v 1.14 2005-01-28 19:34:53 waffel Exp $
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
* In this test case we demonstrate how to create a setup and adding a machine
* to the setup. 
* Then we use the iterator to get the current added machine back and looking
* if the machine was added nice.
*
*/
START_TEST(test_btsetup_obj1){
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machine
	BtSourceMachine *gen1=NULL;
	BtMachine *ref_machine=NULL;
	GList *list,*node;
	
	GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

	/* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	
	/* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);
	
	/* try to add the machine to the setup (and therewith to the song) */
	bt_setup_add_machine(setup,BT_MACHINE(gen1));
	
	g_object_get(G_OBJECT(setup),"machines",&list,NULL);
	/* the list should not be null */
	fail_unless(list!=NULL, NULL);
	node=list;
	
	/* the pointer should be pointing to the gen1 machine */
	ref_machine=node->data;
	fail_unless(ref_machine!=NULL, NULL);
	fail_unless(ref_machine==BT_MACHINE(gen1), NULL);
	fail_unless(BT_IS_SOURCE_MACHINE(ref_machine)==TRUE, NULL);
	
	/* the list should contain only one element */
	fail_unless(g_list_length(list)==1, NULL);
	
	g_list_free(list);
}
END_TEST

/**
* In this test case we demonstrate how to create a wire and adding the newly
* created wire to the setup. After that, we try to get the same wire back, if
* we give the source or dest machine.
*/
START_TEST(test_btsetup_obj2) {
  BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	// wire
	BtWire *wire=NULL;
	BtWire *ref_wire=NULL;
	GList *list,*node;
  	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create sink machine with esd sink */
	sink = bt_sink_machine_new(song,"sink1");
	fail_unless(sink!=NULL, NULL);
	
	/* try to create the wire */
	wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
	fail_unless(wire!=NULL, NULL);
	
	/* try to add the machines to the setup. We must do this. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	bt_setup_add_machine(setup, BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup, wire);
	/* try to get the list of wires in the setup */
  g_object_get(G_OBJECT(setup),"wires",&list,NULL);
  /* the list should not null */
  fail_unless(list!=NULL,NULL);
  
  /* look, of the added wire is in the list */
  ref_wire=list->data;
  fail_unless(ref_wire!=NULL,NULL);
  fail_unless(wire==BT_WIRE(ref_wire),NULL);

	/* the list should contain only one element */
	fail_unless(g_list_length(list)==1, NULL);
	
	g_list_free(list);

	/* try to get the current added wire by the source machine. In this case the 
	source of the wire is our source machine.*/
	ref_wire = bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
	fail_unless(ref_wire!=NULL, NULL);
	fail_unless(ref_wire==wire, NULL);
	g_object_try_unref(ref_wire);
	
	/* try to get the current added wire by the dest machine. In this case the
	destination of the wire is our sink machine. */
	ref_wire = bt_setup_get_wire_by_dst_machine(setup, BT_MACHINE(sink));
	fail_unless(ref_wire!=NULL, NULL);
	fail_unless(ref_wire==wire, NULL);
	g_object_try_unref(ref_wire);	
}
END_TEST

/**
* In this test we demonstrate how to remove a machine from the setup after the 
* same machine is added to the setup.
*/
START_TEST(test_btsetup_obj3) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtMachine *ref_machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to add the machine to the setup. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	
  /* try to get the machine back from the setup */
	ref_machine=bt_setup_get_machine_by_id(setup, "generator1");
	fail_unless(ref_machine!=NULL, NULL);
	g_object_unref(ref_machine);
	
	/* now we try to remove the same machine from the setup */
	bt_setup_remove_machine(setup, BT_MACHINE(source));
	
	/* try to get the machine back from the setup, the ref_machine should be null */
	ref_machine=bt_setup_get_machine_by_id(setup, "generator1");
	fail_unless(ref_machine==NULL, NULL);
}
END_TEST

/**
* In this test we demonstrate how to remove a wire from the setup after the 
* same wire is added to the setup.
*/
START_TEST(test_btsetup_obj4) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	// wire
	BtWire *wire=NULL;
	BtWire *ref_wire=NULL;
	
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create sink machine with esd sink */
	sink = bt_sink_machine_new(song,"sink1");
	fail_unless(sink!=NULL, NULL);
	
	/* try to create the wire */
	wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
	fail_unless(wire!=NULL, NULL);
	
	/* try to add the machines to the setup. We must do this. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	bt_setup_add_machine(setup, BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup, wire);
	
	/* check if we can get the wire from the setup */
	ref_wire=bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
	fail_unless(ref_wire!=NULL,NULL);
	g_object_try_unref(ref_wire);
	
	/* try to remove the wire from the setup */
	bt_setup_remove_wire(setup, wire);
	
  /* check if we can get the wire from the setup. The ref_wire should be null */
	ref_wire=bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
	fail_unless(ref_wire==NULL,NULL);
}
END_TEST

/**
* In this example we show you how to get wires by source machines.
*/
START_TEST(test_btsetup_wire1) {
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
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create sink machine with esd sink */
	sink = bt_sink_machine_new(song,"sink1");
	fail_unless(sink!=NULL, NULL);
	
	/* try to create the wire */
	wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
	fail_unless(wire!=NULL, NULL);
	
	/* try to add the machines to the setup. We must do this. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	bt_setup_add_machine(setup, BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup, wire);
	
	/* try to get the list of wires */
	wire_list=bt_setup_get_wires_by_src_machine(setup,BT_MACHINE(source));
	fail_unless(wire_list!=NULL,NULL);
	ref_wire=BT_WIRE(g_list_first(wire_list)->data);
	fail_unless(ref_wire!=NULL,NULL);
	fail_unless(ref_wire==wire,NULL);
	g_object_unref(ref_wire);
	g_list_free(wire_list);
	
}
END_TEST

/**
* In this example we show you how to get wires by a destination machine.
*/
START_TEST(test_btsetup_wire2) {
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
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create sink machine with esd sink */
	sink = bt_sink_machine_new(song,"sink1");
	fail_unless(sink!=NULL, NULL);
	
	/* try to create the wire */
	wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
	fail_unless(wire!=NULL, NULL);
	
	/* try to add the machines to the setup. We must do this. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	bt_setup_add_machine(setup, BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup, wire);
	
	/* try to get the list of wires */
	wire_list=bt_setup_get_wires_by_dst_machine(setup,BT_MACHINE(sink));
	fail_unless(wire_list!=NULL,NULL);
	ref_wire=BT_WIRE(g_list_first(wire_list)->data);
	fail_unless(ref_wire!=NULL,NULL);
	fail_unless(ref_wire==wire,NULL);
	g_object_unref(ref_wire);
	g_list_free(wire_list);
	
}
END_TEST

START_TEST(test_btsetup_machine1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtMachine *ref_machine=NULL;
	// Gtype of the machine
	GType machine_type;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to add the machine to the setup. We must do this. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	
	/* new try to get the machine back via bt_setup_get_machine_by_type */
	machine_type=G_OBJECT_TYPE(source);
	ref_machine=bt_setup_get_machine_by_type(setup, machine_type);
	fail_unless(BT_IS_SOURCE_MACHINE(ref_machine),NULL);
	fail_unless(ref_machine==BT_MACHINE(source),NULL);
	
}
END_TEST

TCase *bt_setup_example_tcase(void) {
  TCase *tc = tcase_create("bt_setup example case");

  tcase_add_test(tc,test_btsetup_obj1);
	tcase_add_test(tc,test_btsetup_obj2);
	tcase_add_test(tc,test_btsetup_obj3);
	tcase_add_test(tc,test_btsetup_obj4);
	tcase_add_test(tc,test_btsetup_wire1);
	tcase_add_test(tc,test_btsetup_wire2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_example_suite(void) { 
  Suite *s=suite_create("BtSetupExample"); 

  suite_add_tcase(s,bt_setup_example_tcase());
  return(s);
}
