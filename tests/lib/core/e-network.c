/* $Id: e-network.c,v 1.11 2005-04-15 17:05:12 ensonic Exp $
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

/**
* try to check if we can simple connect a sine machine to an sink. The sink can be
* a esdsink or an alsasink. We also try after connecting the machines to start
* play.
*/
START_TEST(test_btcore_net_example1) {
	BtApplication *app=NULL;
  // the song
	BtSong *song=NULL;
	// machines
	BtSourceMachine *gen1=NULL;
	BtSinkMachine *sink=NULL;
  BtMachine *machine;
	// wires
  BtWire *wire, *wire1=NULL;
	// song setup 
	BtSetup *setup=NULL;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  
	/* get the song setup */
	g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
	sink=bt_sink_machine_new(song,"master");
  fail_unless(sink!=NULL, NULL);
  
  /* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);
  
  /* check if we can retrieve the machine via the id */
  machine=bt_setup_get_machine_by_id(setup,"master");
  fail_unless(machine==BT_MACHINE(sink), NULL);
	g_object_unref(machine);
  machine=bt_setup_get_machine_by_id(setup,"generator1");
  fail_unless(machine==BT_MACHINE(gen1), NULL);
	g_object_unref(machine);
  
  /* try to link machines */
	wire1=bt_wire_new(song, BT_MACHINE(gen1), BT_MACHINE(sink));
  fail_unless(wire1!=NULL, NULL);

  /* check if we can retrieve the wire via the source machine */
  wire=bt_setup_get_wire_by_src_machine(setup,BT_MACHINE(gen1));
  fail_unless(wire==wire1, NULL);
	g_object_try_unref(wire);

  /* check if we can retrieve the wire via the dest machine */
  wire=bt_setup_get_wire_by_dst_machine(setup,BT_MACHINE(sink));
  fail_unless(wire==wire1, NULL);
	g_object_try_unref(wire);
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
  
  g_object_unref(setup);
	g_object_checked_unref(song);
  g_object_checked_unref(app);
}
END_TEST

/**
* try to check if we can connect two sine machines to one sink. The sink can be
* a esdsink or an alsasink. We also try after connecting the machines to start
* play
*/
START_TEST(test_btcore_net_example2) {
  BtApplication *app=NULL;
  // the song
	BtSong *song=NULL;
	// machines
	BtSourceMachine *gen1=NULL,*gen2=NULL;
	BtSinkMachine *sink=NULL;
	// wires
	BtWire *wire, *wire1=NULL, *wire2=NULL;
	// setup
	BtSetup *setup=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
  
	/* get the song setup */
	g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
	sink=bt_sink_machine_new(song,"master");
	fail_unless(sink!=NULL, NULL);
  
  /* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);
  
  /* try to craete generator2 with sinesrc */
  gen2 = bt_source_machine_new(song,"generator2","sinesrc",0);
  fail_unless(gen2!=NULL, NULL);
  
	/* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song, BT_MACHINE(gen1), BT_MACHINE(sink));
	fail_unless(wire1!=NULL, NULL);
	
	/* try to create a wire from gen2 to sink */ 
	wire2=bt_wire_new(song, BT_MACHINE(gen2), BT_MACHINE(sink));
	fail_unless(wire2!=NULL, NULL);
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
  
  g_object_unref(setup);
	g_object_checked_unref(song);
	g_object_checked_unref(app);
}
END_TEST


TCase *bt_network_example_case(void) {
  TCase *tc = tcase_create("BtNetworkExamples");

  tcase_add_test(tc,test_btcore_net_example1);
  tcase_add_test(tc,test_btcore_net_example2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
