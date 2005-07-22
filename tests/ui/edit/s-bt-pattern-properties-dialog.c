/* $Id: s-bt-pattern-properties-dialog.c,v 1.1 2005-07-22 13:26:03 ensonic Exp $
 */

#include "m-bt-edit.h"

//extern TCase *bt_pattern_properties_dialog_test_case(void);
extern TCase *bt_pattern_properties_dialog_example_case(void);

Suite *bt_pattern_properties_dialog_suite(void) { 
  Suite *s=suite_create("BtPatternPropertiesDialog"); 

  //suite_add_tcase(s,bt_pattern_properties_dialog_test_case());
  suite_add_tcase(s,bt_pattern_properties_dialog_example_case());
  return(s);
}
