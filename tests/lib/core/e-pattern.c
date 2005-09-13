/* $Id: e-pattern.c,v 1.9 2005-09-13 22:12:13 ensonic Exp $
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
}

//-- tests

BT_START_TEST(test_btpattern_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine=NULL;
  BtPattern *pattern=NULL;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L));
  fail_unless(machine!=NULL, NULL);
  
  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);

  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btpattern_obj2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine=NULL;
  BtPattern *pattern=NULL;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  /* create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",2L));
  fail_unless(machine!=NULL, NULL);
  
  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);

  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btpattern_copy) {
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
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",2L));
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
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


BT_START_TEST(test_btpattern_enlarge_length) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine=NULL;
  BtPattern *pattern=NULL;
  gulong length;
  gchar *data;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  /* try to create a source machine */
  // @todo try "bml-ErsKick" before and fall back to "sinesrc" as long as we don't have multi-voice machine in gst
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L));
  fail_unless(machine!=NULL, NULL);
  
  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);
  
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"5");
  bt_pattern_set_global_event(pattern,4,0,"10");
  
  /* verify length */
  g_object_get(pattern,"length",&length,NULL);
  fail_unless(length==8, NULL);

  /* change and verify length */
  g_object_set(pattern,"length",16L,NULL);
  g_object_get(pattern,"length",&length,NULL);
  fail_unless(length==16, NULL);
  
  /* verify data */
  data=bt_pattern_get_global_event(pattern,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"5",1),"data is '%s' instead of '5'",data);
  g_free(data);
  data=bt_pattern_get_global_event(pattern,4,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"10",2),"data is '%s' instead of '10'",data);
  g_free(data);

  data=bt_pattern_get_global_event(pattern,10,0);
  fail_unless(data==NULL, "data is '%s' instead of ''",data);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btpattern_shrink_length) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine=NULL;
  BtPattern *pattern=NULL;
  gulong length;
  gchar *data;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  /* try to create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L));
  fail_unless(machine!=NULL, NULL);
  
  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",16L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);
  
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"5");
  bt_pattern_set_global_event(pattern,4,0,"10");

  /* verify length */
  g_object_get(pattern,"length",&length,NULL);
  fail_unless(length==16, NULL);

  /* change and verify length */
  g_object_set(pattern,"length",8L,NULL);
  g_object_get(pattern,"length",&length,NULL);
  fail_unless(length==8, NULL);
  
  /* verify data */
  data=bt_pattern_get_global_event(pattern,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"5",1),"data is '%s' instead of '5'",data);
  g_free(data);
  data=bt_pattern_get_global_event(pattern,4,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"10",2),"data is '%s' instead of '10'",data);
  g_free(data);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btpattern_enlarge_voices) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine=NULL;
  BtPattern *pattern=NULL;
  gulong voices;
  gchar *data;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  /* try to create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",1L));
  fail_unless(machine!=NULL, NULL);
  
  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);
  
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"5");
  bt_pattern_set_voice_event(pattern,4,0,0,"10");

  /* verify voices */
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==1, NULL);

  /* change and verify voices */
  g_object_set(pattern,"voices",2L,NULL);
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==2, NULL);

  /* verify data */
  data=bt_pattern_get_global_event(pattern,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"5",1),"data is '%s' instead of '5'",data);
  g_free(data);
  data=bt_pattern_get_voice_event(pattern,4,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"10",2),"data is '%s' instead of '10'",data);
  g_free(data);

  data=bt_pattern_get_voice_event(pattern,0,1,0);
  fail_unless(data==NULL, "data is '%s' instead of ''",data);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btpattern_shrink_voices) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine=NULL;
  BtPattern *pattern=NULL;
  gulong voices;
  gchar *data;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  /* try to create a source machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",2L));
  fail_unless(machine!=NULL, NULL);
  
  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);
  
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"5");
  bt_pattern_set_voice_event(pattern,4,0,0,"10");

  /* verify voices */
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==2, NULL);

  /* change and verify voices */
  g_object_set(pattern,"voices",1L,NULL);
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==1, NULL);
  
  /* verify data */
  data=bt_pattern_get_global_event(pattern,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"5",1),"data is '%s' instead of '5'",data);
  g_free(data);
  data=bt_pattern_get_voice_event(pattern,4,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"10",2),"data is '%s' instead of '10'",data);
  g_free(data);

  /* cleanup */
  g_object_try_unref(pattern);
  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_pattern_example_case(void) {
  TCase *tc = tcase_create("BtPatternExamples");

  tcase_add_test(tc,test_btpattern_obj1);
  tcase_add_test(tc,test_btpattern_obj2);
  tcase_add_test(tc,test_btpattern_copy);
  tcase_add_test(tc,test_btpattern_enlarge_length);
  tcase_add_test(tc,test_btpattern_shrink_length);
  tcase_add_test(tc,test_btpattern_enlarge_voices);
  tcase_add_test(tc,test_btpattern_shrink_voices);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
