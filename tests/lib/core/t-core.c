/* $Id: t-core.c,v 1.9 2005-10-13 15:52:24 ensonic Exp $
 */

#include "m-bt-core.h"

//-- globals


// test if the normal init call works with commandline arguments
START_TEST(test_btcore_init0) {
  bt_init(&test_argc,&test_argvptr);
}
END_TEST

// test if the init call handles correct null pointers
START_TEST(test_btcore_init1) {
  bt_init(NULL,NULL);
}
END_TEST

TCase *bt_core_test_case(void) {
  TCase *tc = tcase_create("BtCoreTests");

  tcase_add_test(tc,test_btcore_init0);
  tcase_add_test(tc,test_btcore_init1);
  return(tc);
}
