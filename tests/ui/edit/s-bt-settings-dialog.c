/* $Id: s-bt-settings-dialog.c,v 1.1 2005-04-16 10:48:58 ensonic Exp $
 */

#include "m-bt-edit.h"

//extern TCase *bt_settings_dialog_test_case(void);
extern TCase *bt_settings_dialog_example_case(void);

Suite *bt_settings_dialog_suite(void) { 
  Suite *s=suite_create("BtSettingsDialog"); 

  //suite_add_tcase(s,bt_settings_dialog_test_case());
  suite_add_tcase(s,bt_settings_dialog_example_case());
  return(s);
}
