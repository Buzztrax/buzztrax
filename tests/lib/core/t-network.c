/** $Id: t-network.c,v 1.5 2004-09-29 11:23:23 ensonic Exp $
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
* try to check if we can create a network of NULL machines and NULL wires
*
* this is a negative test
*/

START_TEST(test_btcore_net1) {
	GstElement *bin=NULL;
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	gboolean song_ret;
	
  GST_INFO("--------------------------------------------------------------------------------");

	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));
	
	/* get the setup for the song */
	g_object_get(G_OBJECT(song),"setup",&setup,NULL);
	fail_unless(setup!=NULL, NULL);
	
  GST_DEBUG("test");

	/* try to add a NULL machine to the setup */
	bt_setup_add_machine(setup, NULL);
  
	/* try to add a NULL wire to the setup */
	bt_setup_add_wire(setup, NULL);
	
	/* try to start playing the song */
	song_ret=bt_song_play(song);
	fail_unless(song_ret==TRUE, NULL);
	
	/* stop the song */
	bt_song_stop(song);
	
  g_object_checked_unref(setup);  
	g_object_checked_unref(song);
}
END_TEST

/**
 * @todo test creating wires with args beeing NULL
 * @todo test creating wires with args beeing nonsense (not BtMachines)
 * @todo test creating machines with wrong names (not existing)
 * @todo test creating machines of the wrong typ (sink that is a source)
 * @todo test adding machines/wires twice
 */


TCase *bt_network_obj_tcase(void) {
  TCase *tc = tcase_create("Network");

	tcase_add_test(tc,test_btcore_net1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}

Suite *bt_network_suite(void) { 
  Suite *s=suite_create("BtNetwork"); 

  suite_add_tcase(s,bt_network_obj_tcase());
  return(s);
}

