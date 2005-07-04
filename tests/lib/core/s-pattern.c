/* $Id: s-pattern.c,v 1.1 2005-07-04 14:05:11 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_pattern_test_case(void);
extern TCase *bt_pattern_example_case(void);

Suite *bt_pattern_suite(void) { 
  Suite *s=suite_create("BtPattern"); 

  suite_add_tcase(s,bt_pattern_test_case());
  suite_add_tcase(s,bt_pattern_example_case());
  return(s);
}
