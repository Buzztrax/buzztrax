/** $Id: e-setup.c,v 1.1 2004-10-01 13:25:03 waffel Exp $
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
* Try to create a bt_machine_iterator with a real setup parameter.
* This case demonstrates, how to create a machine iterator from the setup.
* The returned pointer to the iterator should not be null.
*
*/
START_TEST(test_btsetup_obj1){
	BtSong *song=NULL;
	BtSetup *setup=NULL;
	GstElement *bin=NULL;
	gpointer *iter_ptr;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	bin = gst_thread_new("thread");

	song=bt_song_new(GST_BIN(bin));
	setup=bt_setup_new(song);
	fail_unless(setup!=NULL, NULL);
	iter_ptr=bt_setup_machine_iterator_new(setup);
	// the iterator should not be null
	fail_unless(iter_ptr!=NULL, NULL);
	// the next element of the pointer should be null
	fail_unless(bt_setup_machine_iterator_next(iter_ptr)==NULL, NULL);
}
END_TEST

TCase *bt_setup_example_tcase(void) {
  TCase *tc = tcase_create("bt_setup example case");

  tcase_add_test(tc,test_btsetup_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}


Suite *bt_setup_example_suite(void) { 
  Suite *s=suite_create("BtSetupExample"); 

  suite_add_tcase(s,bt_setup_example_tcase());
  return(s);
}
