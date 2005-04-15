/* $Id: t-network.c,v 1.11 2005-04-15 17:05:14 ensonic Exp $
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
* try to check if we can create a network of NULL machines and NULL wires
*
* this is a negative test
*/

START_TEST(test_btcore_net1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	gboolean song_ret;
	
  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	
	/* get the setup for the song */
	g_object_get(G_OBJECT(song),"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
  GST_DEBUG("test");
  
	/* try to add a NULL wire to the setup */
	bt_setup_add_wire(setup, NULL);
	
	/* try to start playing the song */
	song_ret=bt_song_play(song);
	fail_unless(song_ret==TRUE, NULL);
	
	/* stop the song */
	bt_song_stop(song);
	
  g_object_checked_unref(setup);  
	g_object_checked_unref(song);
  g_object_checked_unref(app);
}
END_TEST

TCase *bt_network_test_case(void) {
  TCase *tc = tcase_create("BtNetworkTests");

	// @todo why is this uncommented
	//tcase_add_test(tc,test_btcore_net1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
