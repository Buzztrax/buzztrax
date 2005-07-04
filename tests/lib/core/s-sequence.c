/* $Id: s-sequence.c,v 1.2 2005-07-04 11:37:22 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_sequence_test_case(void);
extern TCase *bt_sequence_example_case(void);

Suite *bt_sequence_suite(void) { 
  Suite *s=suite_create("BtSequence"); 

  suite_add_tcase(s,bt_sequence_test_case());
  suite_add_tcase(s,bt_sequence_example_case());
  return(s);
}
