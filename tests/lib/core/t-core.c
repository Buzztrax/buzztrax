/** $Id: t-core.c,v 1.1 2004-08-17 17:02:17 waffel Exp $
 */

#include "t-core.h"

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
  return(tc);
}

Suite *libbtcore_suite(void) { 
  Suite *s=suite_create("LibBtCore"); 

  suite_add_tcase(s,libbtcore_tcase());
  return(s);
}


