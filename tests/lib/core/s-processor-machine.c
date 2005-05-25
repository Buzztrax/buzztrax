/* $Id: s-processor-machine.c,v 1.1 2005-05-25 13:29:27 ensonic Exp $
 */

#include "m-bt-core.h"

//extern TCase *bt_processor_machine_test_case(void);
extern TCase *bt_processor_machine_example_case(void);

Suite *bt_processor_machine_suite(void) { 
  Suite *s=suite_create("BtProcessorMachine"); 

  //suite_add_tcase(s,bt_processor_machine_test_case());
  suite_add_tcase(s,bt_processor_machine_example_case());
  return(s);
}
