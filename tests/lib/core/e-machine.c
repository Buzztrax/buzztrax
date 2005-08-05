/* $Id: e-machine.c,v 1.2 2005-08-05 08:59:48 ensonic Exp $
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

START_TEST(test_btmachine_insert_input_level1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine;
  gboolean res;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  
  /* create a machine */
  machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0));
  fail_unless(machine != NULL, NULL);
  
  res=bt_machine_enable_input_level(machine);
  fail_unless(res == TRUE, NULL);
  
  g_object_try_unref(machine);
  g_object_try_unref(song);
  g_object_checked_unref(app);
}
END_TEST

START_TEST(test_btmachine_insert_input_level2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine1,*machine2;
  BtWire *wire;
  gboolean res;

  GST_INFO("--------------------------------------------------------------------------------");

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  
  /* create two machines */
  machine1=BT_MACHINE(bt_processor_machine_new(song,"vol1","volume",0));
  fail_unless(machine1 != NULL, NULL);
  machine2=BT_MACHINE(bt_processor_machine_new(song,"vol2","volume",0));
  fail_unless(machine2 != NULL, NULL);
  
  /* connect them */
  wire=bt_wire_new(song,machine1,machine2);
  fail_unless(wire != NULL, NULL);
  
  res=bt_machine_enable_input_level(machine2);
  fail_unless(res == TRUE, NULL);
  
  g_object_try_unref(wire);
  g_object_try_unref(machine1);
  g_object_try_unref(machine2);
  g_object_try_unref(song);
  g_object_checked_unref(app);
}
END_TEST
  
TCase *bt_machine_example_case(void) {
  TCase *tc = tcase_create("BtMachineExamples");

  tcase_add_test(tc,test_btmachine_insert_input_level1);
  tcase_add_test(tc,test_btmachine_insert_input_level2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
