/** $Id: t-setup.c,v 1.2 2004-10-01 16:01:47 ensonic Exp $
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
* Try to add a NULL machine to a empty setup. We have no return value. But in
* this case no assertion should be thrown.
*
*/
START_TEST(test_btsetup_obj1){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	bt_setup_add_machine(setup, NULL);
	
}
END_TEST

/**
* Try to add a NULL wire to a empty setup. We have no return value. But in
* this case no assertion should be thrown.
*
*/
START_TEST(test_btsetup_obj2){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	bt_setup_add_wire(setup, NULL);
	
}
END_TEST

TCase *bt_setup_obj_tcase(void) {
  TCase *tc = tcase_create("bt_setup case");

  tcase_add_test(tc,test_btsetup_obj1);
	tcase_add_test(tc,test_btsetup_obj2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_suite(void) { 
  Suite *s=suite_create("BtSetup"); 

  suite_add_tcase(s,bt_setup_obj_tcase());
  return(s);
}
