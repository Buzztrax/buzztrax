/** $Id: t-core.c,v 1.2 2004-09-24 22:42:16 ensonic Exp $
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

TCase *libbtcore_tcase(void) {
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc,test_btcore_init0);
	tcase_add_test(tc,test_btcore_init1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}

Suite *libbtcore_suite(void) { 
  Suite *s=suite_create("LibBtCore"); 

  suite_add_tcase(s,libbtcore_tcase());
  return(s);
}


