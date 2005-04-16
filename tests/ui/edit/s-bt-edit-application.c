/* $Id: s-bt-edit-application.c,v 1.1 2005-04-16 10:48:58 ensonic Exp $
 */

#include "m-bt-edit.h"

//extern TCase *bt_edit_application_test_case(void);
extern TCase *bt_edit_application_example_case(void);

Suite *bt_edit_application_suite(void) { 
  Suite *s=suite_create("BtEditApplication"); 

  //suite_add_tcase(s,bt_edit_application_test_case());
  suite_add_tcase(s,bt_edit_application_example_case());
  return(s);
}
