/** $Id: e-setup.c,v 1.3 2004-10-04 16:09:47 waffel Exp $
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
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	// machine
	BtSourceMachine *gen1=NULL;
	BtMachine *ref_machine=NULL;
	
	gpointer *iter_ptr;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);
	
	/* try to add the machine to the setup (and therewith to the song) */
	bt_setup_add_machine(setup,BT_MACHINE(gen1));
	
	iter_ptr=bt_setup_machine_iterator_new(setup);
	/* the iterator should not be null */
	fail_unless(iter_ptr!=NULL, NULL);
	
	/* the pointer should be point to the gen1 machine */
	ref_machine=bt_setup_machine_iterator_get_machine(iter_ptr);
	fail_unless(ref_machine!=NULL, NULL);
	fail_unless(ref_machine==BT_MACHINE(gen1), NULL);
	fail_unless(BT_IS_SOURCE_MACHINE(ref_machine)==TRUE, NULL);
	
	/* the next element of the pointer should be null */
	fail_unless(bt_setup_machine_iterator_next(iter_ptr)==NULL, NULL);
}
END_TEST

/**
* In this test case we demonstrate how to create a wire and adding the newly
* created wire to the setup. After that, we try to get the same wire back, if
* we give the source or dest machine.
*/
START_TEST(test_btsetup_obj2) {
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	// machines
	BtSourceMachine *source=NULL;
	BtSinkMachine *sink=NULL;
	// wire
	BtWire *wire=NULL;
	BtWire *ref_wire=NULL;
	
	gpointer *iter_ptr;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	fail_unless(setup!=NULL, NULL);
	
	/* try to craete generator1 with sinesrc */
  source = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(source!=NULL, NULL);
	
	/* try to create sink machine with esd sink */
	sink = bt_sink_machine_new(song,"sink1","esdsink");
	fail_unless(sink!=NULL, NULL);
	
	/* try to create the wire */
	wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
	fail_unless(wire!=NULL, NULL);
	
	/* try to add the machines to the setup. We must do this. */
	bt_setup_add_machine(setup, BT_MACHINE(source));
	bt_setup_add_machine(setup, BT_MACHINE(sink));
	
	/* try to add the wire to the setup */
	bt_setup_add_wire(setup, wire);
	
	/* try to get the current added wire by the source machine. In this case the 
	source of the wire is our source machine.*/
	ref_wire = bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
	fail_unless(ref_wire!=NULL, NULL);
	fail_unless(ref_wire==wire, NULL);
	
	/* setting the ref wire back to NULL for next check. */
	ref_wire = NULL;
	
	/* try to get the current added wire by the dest machine. In this case the
	destination of the wire is our sink machine. */
	ref_wire = bt_setup_get_wire_by_dst_machine(setup, BT_MACHINE(sink));
	fail_unless(ref_wire!=NULL, NULL);
	fail_unless(ref_wire==wire, NULL);
	
}
END_TEST

TCase *bt_setup_example_tcase(void) {
  TCase *tc = tcase_create("bt_setup example case");

  tcase_add_test(tc,test_btsetup_obj1);
	tcase_add_test(tc,test_btsetup_obj2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_example_suite(void) { 
  Suite *s=suite_create("BtSetupExample"); 

  suite_add_tcase(s,bt_setup_example_tcase());
  return(s);
}
