/* $Id: t-network.c,v 1.15 2005-09-13 18:51:00 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
	bt_core_teardown();
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
	/* @todo even though the bin is empty one can set it to play */
  song_ret=bt_song_play(song);
  //fail_unless(song_ret==FALSE, NULL);
	fail_unless(song_ret==TRUE, NULL);
    
  g_object_unref(setup);  
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
END_TEST

TCase *bt_network_test_case(void) {
  TCase *tc = tcase_create("BtNetworkTests");

  tcase_add_test(tc,test_btcore_net1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
