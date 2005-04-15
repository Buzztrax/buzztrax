/** $Id: t-core.c,v 1.3 2005-04-15 15:13:12 ensonic Exp $
 */

#include "t-core.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  //puts(__FILE__":teardown");
}

// test if the normal init call works with commandline arguments
START_TEST(test_btcore_init0) {
	bt_init(&test_argc,&test_argvptr,NULL);
}
END_TEST

// test if the init call handles correct null pointers
START_TEST(test_btcore_init1) {
	bt_init(NULL,NULL,NULL);
}
END_TEST

TCase *bt_core_tcase(void) {
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc,test_btcore_init0);
	tcase_add_test(tc,test_btcore_init1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}

Suite *bt_core_suite(void) { 
  Suite *s=suite_create("BtCore"); 

  suite_add_tcase(s,bt_core_tcase());
  return(s);
}
