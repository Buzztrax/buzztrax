/* $Id: s-settings.c,v 1.3 2006-01-16 21:39:26 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_gconf_settings_test_case(void);

Suite *bt_settings_suite(void) { 
  Suite *s=suite_create("BtSettings"); 

  // setup/teardown provides different environments for each run
  suite_add_tcase(s,bt_gconf_settings_test_case());
  // @todo: run it again when we have an implementation for the plainfile_settings
  //suite_add_tcase(s,bt_gconf_settings_test_case());
  return(s);
}
