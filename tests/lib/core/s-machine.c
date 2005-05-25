/* $Id: s-machine.c,v 1.2 2005-05-25 13:29:27 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_machine_test_case(void);
extern TCase *bt_machine_example_case(void);

Suite *bt_machine_suite(void) { 
  Suite *s=suite_create("BtMachine"); 

  suite_add_tcase(s,bt_machine_test_case());
  suite_add_tcase(s,bt_machine_example_case());
  return(s);
}
