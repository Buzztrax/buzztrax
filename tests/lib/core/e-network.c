/** $Id: e-network.c,v 1.1 2004-09-24 11:50:17 waffel Exp $
 */

#include "t-core.h"

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
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
	GstElement *bin=NULL;
	// wires
  BtWire *wire1=NULL;
	// song setup 
	BtSetup *setup=NULL;
	
	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));
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
  
  /* try to link machines */
	wire1=bt_wire_new(song, BT_MACHINE(gen1), BT_MACHINE(sink));
	// @todo why the BT_MACHINE function cannot be found in the documentation?
  fail_unless(wire1!=NULL, NULL);
	
	/* get the song setup */
	setup=bt_song_get_setup(song);
  fail_unless(setup!=NULL, NULL);
	
	/* add wire to song setup */
	bt_setup_add_wire(setup, wire1);
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
	g_object_unref(G_OBJECT(song));
	fail_unless(G_IS_OBJECT(song) == FALSE, NULL);
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
	BtWire *wire1=NULL, *wire2=NULL;
	// setup
	BtSetup *setup=NULL;
  
	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));  
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
  
	/* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song, BT_MACHINE(gen1), BT_MACHINE(sink));
	fail_unless(wire1!=NULL, NULL);
	
	/* try to create a wire from gen2 to sink */ 
	wire2=bt_wire_new(song, BT_MACHINE(gen2), BT_MACHINE(sink));
	fail_unless(wire2!=NULL, NULL);
	
	/* get the (should be empty) setup from the song */
	setup=bt_song_get_setup(song);
	
	/* try to add both wires to the setup (and impliziet to the song) */
	bt_setup_add_wire(setup, wire1);
	bt_setup_add_wire(setup, wire2);
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
	g_object_unref(G_OBJECT(song));
	fail_unless(G_IS_OBJECT(song) == FALSE, NULL);
}
END_TEST


TCase *bt_network_example_tcase(void) {
  TCase *tc = tcase_create("Network");

  tcase_add_test(tc,test_btcore_net_example1);
  tcase_add_test(tc,test_btcore_net_example2);
  return(tc);
}

Suite *bt_network_example_suite(void) { 
  Suite *s=suite_create("BtNetworkExamples"); 

  suite_add_tcase(s,bt_network_example_tcase());
  return(s);
}

