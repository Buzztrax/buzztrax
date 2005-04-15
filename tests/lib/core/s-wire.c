/* $Id: s-wire.c,v 1.1 2005-04-15 17:05:14 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_wire_test_case(void);
//extern TCase *bt_wire_example_case(void);

Suite *bt_wire_suite(void) { 
  Suite *s=suite_create("BtWire"); 

  suite_add_tcase(s,bt_wire_test_case());
  //suite_add_tcase(s,bt_wire_example_case());
  return(s);
}
