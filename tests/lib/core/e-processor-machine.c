/* $Id: e-processor-machine.c,v 1.5 2005-09-14 10:16:34 ensonic Exp $
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

BT_START_TEST(test_btprocessormachine_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtProcessorMachine *machine;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  
  /* create a machine */
  machine=bt_processor_machine_new(song,"vol","volume",0);
  fail_unless(machine != NULL, NULL);

  g_object_try_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST
  
  
TCase *bt_processor_machine_example_case(void) {
  TCase *tc = tcase_create("BtProcessorMachineExamples");

  tcase_add_test(tc,test_btprocessormachine_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
