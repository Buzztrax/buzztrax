/* $Id: t-sink-machine.c,v 1.5 2005-09-13 18:51:00 ensonic Exp $
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

//-- tests

START_TEST(test_btsinkmachine_settings1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;
  gchar *saved_audiosink_name;
  
  GST_INFO("--------------------------------------------------------------------------------");
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  settings=BT_SETTINGS(bt_gconf_settings_new());
  
  g_object_get(settings,"audiosink",&saved_audiosink_name,NULL);
  
  g_object_set(settings,"audiosink","osssink sync=false",NULL);
  
  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);
  
  
  g_object_set(settings,"audiosink",saved_audiosink_name,NULL);
  
  g_object_unref(settings);
  g_free(saved_audiosink_name);
  g_object_unref(machine);
  g_object_unref(song);
  g_object_unref(app);
}
END_TEST;

/**
* Try to create a sink machine, if we set the sink property with the gconf 
* properties to the string "audioconvert ! osssink sync=false". This string 
* should be replaced by the sink machine to "ossink" and the machine should be 
* instantiable.
*/
START_TEST(test_btsinkmachine_settings2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;
  gchar *saved_audiosink_name;
  
  GST_INFO("--------------------------------------------------------------------------------");
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  settings=BT_SETTINGS(bt_gconf_settings_new());
  
  g_object_get(settings,"audiosink",&saved_audiosink_name,NULL);
  
  g_object_set(settings,"audiosink","audioconvert ! osssink sync=false",NULL);
  
  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);
  
  
  g_object_set(settings,"audiosink",saved_audiosink_name,NULL);
  
  g_object_unref(settings);
  g_free(saved_audiosink_name);
  g_object_unref(machine);
  g_object_unref(song);
  g_object_unref(app);
}
END_TEST;

TCase *bt_sink_machine_test_case(void) {
  TCase *tc = tcase_create("BtSinkMachineTests");

  tcase_add_test(tc,test_btsinkmachine_settings1);
  tcase_add_test(tc,test_btsinkmachine_settings2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
