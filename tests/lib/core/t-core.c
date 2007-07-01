/* $Id: t-core.c,v 1.12 2007-07-01 12:34:14 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#include "m-bt-core.h"

//-- globals


//-- tests

// test if the normal init call works with commandline arguments (no args)
START_TEST(test_btcore_init0) {
  bt_init(&test_argc,&test_argvptr);
}
END_TEST

// test if the init call handles correct null pointers
START_TEST(test_btcore_init1) {
  bt_init(NULL,NULL);
}
END_TEST

// test if the normal init call works with commandline arguments
START_TEST(test_btcore_init2) {
  // this shadows the global vars of the same name
  gint test_argc=2;
  gchar *test_argv[test_argc];
  gchar **test_argvptr;

  test_argv[0]="check_buzzard";
  test_argv[1]="--bt-version";
  test_argvptr=test_argv;

  bt_init(&test_argc,&test_argvptr);
}
END_TEST

/*
 * Test nonsense args.
 *
 * This unfortunately exits the test app.
 */
#ifdef __CHECK_DISABLED__
// test if the normal init call works with commandline arguments
START_TEST(test_btcore_init3) {
  // this shadows the global vars of the same name
  gint test_argc=2;
  gchar *test_argv[test_argc];
  gchar **test_argvptr;

  test_argv[0]="check_buzzard";
  test_argv[1]="--bt-non-sense";
  test_argvptr=test_argv;

  bt_init(&test_argc,&test_argvptr);
}
END_TEST
#endif

TCase *bt_core_test_case(void) {
  TCase *tc = tcase_create("BtCoreTests");

  tcase_add_test(tc,test_btcore_init0);
  tcase_add_test(tc,test_btcore_init1);
  tcase_add_test(tc,test_btcore_init2);
  //tcase_add_test(tc,test_btcore_init3);
  return(tc);
}
