/* Buzztard
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

// test init with wrong arg usage
START_TEST(test_btcore_init0  ) {
  /* arrange */
  gchar *test_argv[] = { "check_buzzard", "--bt-version=5" };
  gchar **test_argvptr = test_argv;
  gint test_argc=G_N_ELEMENTS(test_argv) - 1;

  /* act */
  bt_init(&test_argc,&test_argvptr);
}
END_TEST

/*
 * Test nonsense args.
 *
 * This unfortunately exits the test app.
 */
#ifdef __CHECK_DISABLED__
// test init with nonsense args
START_TEST(test_btcore_init1) {
  /* arrange */
  gchar *test_argv[] = { "check_buzzard", "--bt-non-sense" };
  gchar **test_argvptr = test_argv;
  gint test_argc=G_N_ELEMENTS(test_argv) - 1;

  /* act */
  bt_init(&test_argc,&test_argvptr);
}
END_TEST
#endif

TCase *bt_core_test_case(void) {
  TCase *tc = tcase_create("BtCoreTests");

  tcase_add_test(tc,test_btcore_init0);
  //tcase_add_test(tc,test_btcore_init1);
  return(tc);
}
