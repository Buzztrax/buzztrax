/* $Id: e-pattern.c,v 1.3 2005-07-12 11:44:43 ensonic Exp $
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
}

//-- tests

START_TEST(test_btpattern_copy) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtMachine *machine=NULL;
	BtPattern *pattern1=NULL,*pattern2=NULL;
	gulong length1,length2,voices1,voices2;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
	/* try to create a source machine */
	machine=BT_MACHINE(bt_source_machine_new(song,"gen","sinesrc",0L));
	fail_unless(machine!=NULL, NULL);
	
	/* try to create a pattern */
	pattern1=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
	fail_unless(pattern1!=NULL, NULL);

	/* create a copy */
	pattern2=bt_pattern_copy(pattern1);
	fail_unless(pattern2!=NULL, NULL);
	fail_unless(pattern2!=pattern1, NULL);
	
	/* compare */
	g_object_get(pattern1,"length",&length1,"voices",&voices1,NULL);
	g_object_get(pattern2,"length",&length2,"voices",&voices2,NULL);
	fail_unless(length1==length2, NULL);
	fail_unless(voices1==voices2, NULL);
	
	g_object_try_unref(pattern1);
	g_object_try_unref(pattern2);
	g_object_try_unref(machine);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

START_TEST(test_btpattern_enlarge_length) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtMachine *machine=NULL;
	BtPattern *pattern=NULL;
	gulong length;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
	/* try to create a source machine */
	// @todo try "bml-ErsKick" before and fall back to "sinesrc" as long as we don't have multi-voice machine in gst
	machine=BT_MACHINE(bt_source_machine_new(song,"gen","sinesrc",0L));
	fail_unless(machine!=NULL, NULL);
	
	/* try to create a pattern */
	pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
	fail_unless(pattern!=NULL, NULL);
  
  // we need gst-elements with controllable params
  //bt_pattern_set_global_event(pattern,4,1,"1.0");
	
	g_object_get(pattern,"length",&length,NULL);
	fail_unless(length==8, NULL);

	g_object_set(pattern,"length",16L,NULL);
	g_object_get(pattern,"length",&length,NULL);
	fail_unless(length==16, NULL);
	
	g_object_try_unref(pattern);
	g_object_try_unref(machine);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

START_TEST(test_btpattern_shrink_length) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtMachine *machine=NULL;
	BtPattern *pattern=NULL;
	gulong length;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
	/* try to create a source machine */
	machine=BT_MACHINE(bt_source_machine_new(song,"gen","sinesrc",0L));
	fail_unless(machine!=NULL, NULL);
	
	/* try to create a pattern */
	pattern=bt_pattern_new(song,"pattern-id","pattern-name",16L,BT_MACHINE(machine));
	fail_unless(pattern!=NULL, NULL);
	
	g_object_get(pattern,"length",&length,NULL);
	fail_unless(length==16, NULL);

	g_object_set(pattern,"length",8L,NULL);
	g_object_get(pattern,"length",&length,NULL);
	fail_unless(length==8, NULL);
	
	g_object_try_unref(pattern);
	g_object_try_unref(machine);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST

#ifdef __WE_NEED_A_MACHINE_IN_GST_THAT_CAN_DO_VOICES
START_TEST(test_btpattern_resize_voices1) {
	BtApplication *app=NULL;
	BtSong *song=NULL;
	BtMachine *machine=NULL;
	BtPattern *pattern=NULL;
	gulong voices;

  GST_INFO("--------------------------------------------------------------------------------");

	/* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
	/* create a new song */
	song=bt_song_new(app);
	/* try to create a source machine */
	machine=BT_MACHINE(bt_source_machine_new(song,"gen","sinesrc",0L));
	fail_unless(machine!=NULL, NULL);
	
	/* try to create a pattern */
	pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
	fail_unless(pattern!=NULL, NULL);
	
	g_object_get(pattern,"voices",&voices,NULL);
	fail_unless(voices==0, NULL);

	g_object_set(voices,"voices",2L,NULL);
	g_object_get(voices,"voices",&voices,NULL);
	fail_unless(length==2, NULL);
	
	g_object_try_unref(pattern);
	g_object_try_unref(machine);
  g_object_try_unref(song);
	g_object_checked_unref(app);
}
END_TEST
#endif

TCase *bt_pattern_example_case(void) {
  TCase *tc = tcase_create("BtPatternExamples");

	tcase_add_test(tc,test_btpattern_copy);
	tcase_add_test(tc,test_btpattern_enlarge_length);
	tcase_add_test(tc,test_btpattern_shrink_length);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
