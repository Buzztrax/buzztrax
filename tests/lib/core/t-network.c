/** $Id: t-network.c,v 1.2 2004-09-24 11:48:10 waffel Exp $
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
* try to check if we can create a network of NULL wires
*
* this is a negative test
*/

START_TEST(test_btcore_net1) {
	GstElement *bin=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	gboolean song_ret;
	
	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));
	
	/* get the setup for the song */
	setup=bt_song_get_setup(song);
	fail_unless(setup!=NULL, NULL);
	
	/* try to add a NULL wire to the setup */
	bt_setup_add_wire(setup, NULL);
	// @todo why this function have no return value?
	
	/* try to start playing the song */
	song_ret=bt_song_play(song);
	fail_unless(song_ret!=FALSE, NULL);
	
	/* stop the song */
	bt_song_stop(song);
	
	g_object_unref(G_OBJECT(song));
	fail_unless(G_IS_OBJECT(song) == FALSE, NULL);
	
}
END_TEST


TCase *bt_network_obj_tcase(void) {
  TCase *tc = tcase_create("Network");

	tcase_add_test(tc,test_btcore_net1);
  return(tc);
}

Suite *bt_network_suite(void) { 
  Suite *s=suite_create("BtNetwork"); 

  suite_add_tcase(s,bt_network_obj_tcase());
  return(s);
}

