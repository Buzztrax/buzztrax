/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "m-bt-core.h"

//-- globals

static FILE *saved_stdout = NULL;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  saved_stdout = stdout;
  stdout = tmpfile ();
}

static void
test_teardown (void)
{
  fclose (stdout);
  stdout = saved_stdout;
}

static void
case_teardown (void)
{
}


//-- tests

// test if the normal init call works with commandline arguments (no args)
static void
test_bt_core_init0 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  bt_init (&test_argc, &test_argvptr);
  BT_TEST_END;
}

// test if the init call handles correct null pointers
static void
test_bt_core_init1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  bt_init (NULL, NULL);
  BT_TEST_END;
}

// test if the normal init call works with commandline arguments
static void
test_bt_core_init2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *test_argv[] = { "check_buzztrax", "--bt-version" };
  gchar **test_argvptr = test_argv;
  gint test_argc = G_N_ELEMENTS (test_argv);

  GST_INFO ("-- act --");
  bt_init (&test_argc, &test_argvptr);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (test_argc, 1);
  fail_unless (check_file_contains_str (stdout, NULL,
          "libbuzztrax-core-" BT_VERSION " from " PACKAGE_STRING), NULL);
  BT_TEST_END;
}

TCase *
bt_core_example_case (void)
{
  TCase *tc = tcase_create ("BtCoreExamples");

  tcase_add_test (tc, test_bt_core_init0);
  tcase_add_test (tc, test_bt_core_init1);
  tcase_add_test (tc, test_bt_core_init2);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
