/* $Id: s-settings.c,v 1.2 2005-08-05 09:36:19 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_gconf_settings_test_case(void);

Suite *bt_settings_suite(void) { 
  Suite *s=suite_create("BtSettings"); 

  // setup/teardown provides different environments for each run
  suite_add_tcase(s,bt_gconf_settings_test_case());
  suite_add_tcase(s,bt_gconf_settings_test_case());
  return(s);
}
