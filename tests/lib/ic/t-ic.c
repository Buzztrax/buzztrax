/* Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-ic.h"

//-- globals


//-- tests

// test if the normal init call works with commandline arguments (no args)
START_TEST(test_btic_init0) {
  btic_init(&test_argc,&test_argvptr);
}
END_TEST

// test if the init call handles correct null pointers
START_TEST(test_btic_init1) {
  btic_init(NULL,NULL);
}
END_TEST

// test if the normal init call works with commandline arguments
START_TEST(test_btic_init2) {
  /* arrange */
  gchar *test_argv[] = { "check_buzzard", "--btic-version" };
  gchar **test_argvptr = test_argv;
  gint test_argc=G_N_ELEMENTS(test_argv);

  btic_init(&test_argc,&test_argvptr);
}
END_TEST

/*
 * Test nonsense args.
 *
 * This unfortunately exits the test app.
 */
#ifdef __CHECK_DISABLED__
// test if the normal init call works with commandline arguments
START_TEST(test_btic_init3) {
  // this shadows the global vars of the same name
  gint test_argc=2;
  gchar *test_argv[test_argc];
  gchar **test_argvptr;

  test_argv[0]="check_buzzard";
  test_argv[1]="--btic-non-sense";
  test_argvptr=test_argv;

  btic_init(&test_argc,&test_argvptr);
}
END_TEST
#endif

TCase *bt_ic_test_case(void) {
  TCase *tc = tcase_create("BtICTests");

  tcase_add_test(tc,test_btic_init0);
  tcase_add_test(tc,test_btic_init1);
  tcase_add_test(tc,test_btic_init2);
  //tcase_add_test(tc,test_btic_init3);
  return(tc);
}
