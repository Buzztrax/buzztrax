/** $Id: t-wire.c,v 1.2 2004-10-08 13:50:04 ensonic Exp $
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
* try to create a wire with the same machine as source and dest 
*/
START_TEST(test_btwire_obj1){
	GstElement *bin=NULL;
	BtSong *song=NULL;
	BtWire *wire=NULL;
	// machine
	BtSourceMachine *machine=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin=gst_thread_new("thread");
  /* create a new song */
  song=bt_song_new(GST_BIN(bin));
	
	/* try to create a source machine */
	machine=bt_source_machine_new(song,"genrator1","sinesource",1);
	
	/* try to add the machine twice to the wire */
	wire=bt_wire_new(song,BT_MACHINE(machine),BT_MACHINE(machine));
	// this should fail and the wire should be null
	fail_unless(wire==NULL, NULL);
}
END_TEST

TCase *bt_wire_obj_tcase(void) {
  TCase *tc = tcase_create("bt_wire case");

  //tcase_add_test(tc,test_btwire_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_wire_suite(void) { 
  Suite *s=suite_create("BtWire"); 

  suite_add_tcase(s,bt_wire_obj_tcase());
  return(s);
}
