/** $Id: e-network.c,v 1.4 2004-09-26 01:50:08 ensonic Exp $
 */

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

/**
* try to check if we can simple connect a sine machine to an sink. The sink can be
* a esdsink or an alsasink. We also try after connecting the machines to start
* play.
*/
START_TEST(test_btcore_net_example1) {
  // the song
	BtSong *song=NULL;
	// machines
	BtSourceMachine *gen1=NULL;
	BtSinkMachine *sink=NULL;
  BtMachine *machine;
	GstElement *bin=NULL;
	// wires
  BtWire *wire, *wire1=NULL;
	// song setup 
	BtSetup *setup=NULL;

  GST_INFO("--------------------------------------------------------------------------------");

	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));
	/* get the song setup */
	g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the esd sink */
	sink=bt_sink_machine_new(song,"master","esdsink");
  if (sink == NULL) {
    /* try to create an alsa sink */
    sink=bt_sink_machine_new(song,"master","alsasink");
  }
  fail_unless(sink!=NULL, NULL);
  
  /* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);

	/* try to add all machines to the setup (and therewith to the song) */
  bt_setup_add_machine(setup,BT_MACHINE(sink));
  bt_setup_add_machine(setup,BT_MACHINE(gen1));
  
  /* check if we can retrieve the machine via the id */
  machine=bt_setup_get_machine_by_id(setup,"master");
  fail_unless(machine==sink, NULL);
  machine=bt_setup_get_machine_by_id(setup,"generator1");
  fail_unless(machine==gen1, NULL);
  
  /* try to link machines */
	wire1=bt_wire_new(song, BT_MACHINE(gen1), BT_MACHINE(sink));
  fail_unless(wire1!=NULL, NULL);
  
	/* add wire to song setup */
	bt_setup_add_wire(setup, wire1);

  /* check if we can retrieve the wire via the source machine */
  wire=bt_setup_get_wire_by_src_machine(setup,BT_MACHINE(gen1));
  fail_unless(wire==wire1, NULL);

  /* check if we can retrieve the wire via the dest machine */
  wire=bt_setup_get_wire_by_dst_machine(setup,BT_MACHINE(sink));
  fail_unless(wire==wire1, NULL);
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
  
  g_object_checked_unref(setup);
	g_object_checked_unref(song);
}
END_TEST

/**
* try to check if we can connect two sine machines to one sink. The sink can be
* a esdsink or an alsasink. We also try after connecting the machines to start
* play
*/
START_TEST(test_btcore_net_example2) {
  // the song
	BtSong *song=NULL;
	// machines
	BtSourceMachine *gen1=NULL,*gen2=NULL;
	BtSinkMachine *sink=NULL;
	GstElement *bin=NULL;
	// wires
	BtWire *wire, *wire1=NULL, *wire2=NULL;
	// setup
	BtSetup *setup=NULL;
  
  GST_INFO("--------------------------------------------------------------------------------");

	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));  
	/* get the song setup */
	g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the esd sink */
	sink=bt_sink_machine_new(song,"master","esdsink");
  if (sink == NULL) {
    /* try to create an alsa sink */
    sink=bt_sink_machine_new(song,"master","alsasink");
  }
	fail_unless(sink!=NULL, NULL);
  
  /* try to craete generator1 with sinesrc */
  gen1 = bt_source_machine_new(song,"generator1","sinesrc",0);
  fail_unless(gen1!=NULL, NULL);
  
  /* try to craete generator2 with sinesrc */
  gen2 = bt_source_machine_new(song,"generator2","sinesrc",0);
  fail_unless(gen2!=NULL, NULL);
  
	/* try to add all machines to the setup (and therewith to the song) */
  bt_setup_add_machine(setup,BT_MACHINE(sink));
  bt_setup_add_machine(setup,BT_MACHINE(gen1));
  bt_setup_add_machine(setup,BT_MACHINE(gen2));
  
	/* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song, BT_MACHINE(gen1), BT_MACHINE(sink));
	fail_unless(wire1!=NULL, NULL);

	/* try to add the wire to the setup (and therewith to the song) */
	bt_setup_add_wire(setup, wire1);
	
	/* try to create a wire from gen2 to sink */ 
	wire2=bt_wire_new(song, BT_MACHINE(gen2), BT_MACHINE(sink));
	fail_unless(wire2!=NULL, NULL);
	
	/* try to add the wire to the setup (and therewith to the song) */
	bt_setup_add_wire(setup, wire2);
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
  
  g_object_checked_unref(setup);
	g_object_checked_unref(song);
}
END_TEST


TCase *bt_network_example_tcase(void) {
  TCase *tc = tcase_create("Network");

  tcase_add_test(tc,test_btcore_net_example1);
  tcase_add_test(tc,test_btcore_net_example2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}

Suite *bt_network_example_suite(void) { 
  Suite *s=suite_create("BtNetworkExamples"); 

  suite_add_tcase(s,bt_network_example_tcase());
  return(s);
}

