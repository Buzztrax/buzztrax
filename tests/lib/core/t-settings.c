/* $Id: t-settings.c,v 1.5 2005-08-05 09:36:19 ensonic Exp $ */

#include "m-bt-core.h"

//-- globals

// this counts the number of runs, to provide different implementations for each
static int variant=0;

BtSettings *get_settings(void) {
  BtSettings *settings=NULL;
  
  switch(variant) {
    case 0:
      settings=BT_SETTINGS(bt_gconf_settings_new());
      break;
    case 1:
      settings=BT_SETTINGS(bt_plainfile_settings_new());
      break;
  }
  return(settings);
}

//-- fixtures

static void test_setup(void) {
  bt_init(NULL,NULL,NULL);
  gst_debug_category_set_threshold(bt_core_debug,GST_LEVEL_DEBUG);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
  variant++;
}

//-- tests
START_TEST(test_btsettings_get_audiosink1) {
  GST_INFO("--------------------------------------------------------------------------------");
  BtSettings *settings=get_settings();
  gchar *saved_audiosink_name,*test_audiosink_name;
  
  g_object_get(settings,"audiosink",&saved_audiosink_name,NULL);
  
  g_object_set(settings,"audiosink","fakesink",NULL);
  
  g_object_get(settings,"audiosink",&test_audiosink_name,NULL);
  
  fail_unless(!strcmp(test_audiosink_name,"fakesink"),"sink is %s",test_audiosink_name);
  
  g_object_set(settings,"audiosink",saved_audiosink_name,NULL);
  
  g_object_unref(settings);
  g_free(saved_audiosink_name);
  g_free(test_audiosink_name);
}
END_TEST;


TCase *bt_gconf_settings_test_case(void) {
  TCase *tc = tcase_create("BtSettingsTests");

  tcase_add_test(tc,test_btsettings_get_audiosink1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
