/* $Id: e-source-machine.c,v 1.1 2005-04-15 17:05:13 ensonic Exp $
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

//-- tests

/**
* In this example we show how to create a source machine and adding a
* newly created pattern to it. Then we try to get the pattern back from
* the machine by its id.
*
* We check also, if we can create a pattern iterator and getting the added
* pattern back from the newly created iterator.
*/
START_TEST(test_btsourcemachine_obj1){
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtSourceMachine *machine=NULL;
	BtPattern *pattern=NULL;
	BtPattern *ref_pattern=NULL;
	GList *list,*node;
	
	GST_INFO("--------------------------------------------------------------------------------");
	
	/* create a dummy app */
	app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
	song=bt_song_new(app);
	
	/* try to create a source machine */
	machine=bt_source_machine_new(song,"id","sinesrc",1);
	fail_unless(machine!=NULL, NULL);
	
	/* try to create a pattern */
	pattern=bt_pattern_new(song,"pattern-id","pattern-name",8,1,BT_MACHINE(machine));
	fail_unless(pattern!=NULL, NULL);
	
	/* try to add a new pattern */
	bt_machine_add_pattern(BT_MACHINE(machine),pattern);
	
	/* try to get the same pattern back per id */
	ref_pattern=bt_machine_get_pattern_by_id(BT_MACHINE(machine),"pattern-id");
	fail_unless(ref_pattern==pattern, NULL);
	g_object_try_unref(ref_pattern);
	
	g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
	/* the list should not be null */
	fail_unless(list!=NULL, NULL);
	node=list;

	/* the returned pointer should be point to the same pattern, that we added
	to the machine before */
	ref_pattern=node->data;
	fail_unless(ref_pattern==pattern, NULL);

	/* the list should contain only one element */
	fail_unless(g_list_length(list)==1, NULL);
	
	g_list_free(list);
}
END_TEST

TCase *bt_source_machine_example_case(void) {
  TCase *tc = tcase_create("BtSourceMachineExamples");

  tcase_add_test(tc,test_btsourcemachine_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
