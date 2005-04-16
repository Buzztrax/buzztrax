/* $Id: s-bt-cmd-application.c,v 1.1 2005-04-16 10:48:58 ensonic Exp $
 */

#include "m-bt-cmd.h"

extern TCase *bt_cmd_application_test_case(void);
extern TCase *bt_cmd_application_example_case(void);

Suite *bt_cmd_application_suite(void) { 
  Suite *s=suite_create("BtCmdApplication"); 

  suite_add_tcase(s,bt_cmd_application_test_case());
  suite_add_tcase(s,bt_cmd_application_example_case());
  return(s);
}
