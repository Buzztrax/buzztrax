/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-ic.h"

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

// test init with wrong arg usage
START_TEST (test_btic_core_init_bad_arg_value)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *test_argv[] = { "check_buzztrax", "--btic-version=5" };
  gchar **test_argvptr = test_argv;
  gint test_argc = G_N_ELEMENTS (test_argv);

  GST_INFO ("-- act --");
  btic_init (&test_argc, &test_argvptr);

  GST_INFO ("-- assert --");
  BT_TEST_END;
}
END_TEST

// test init with nonsense args
START_TEST (test_btic_core_init_bad_arg)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *test_argv[] = { "check_buzztrax", "--btic-non-sense" };
  gchar **test_argvptr = test_argv;
  gint test_argc = G_N_ELEMENTS (test_argv);

  GST_INFO ("-- arrange --");
  GOptionContext *ctx = g_option_context_new (NULL);
  g_option_context_add_group (ctx, btic_init_get_option_group ());
  g_option_context_set_ignore_unknown_options (ctx, TRUE);

  /* act & assert */
  fail_unless (g_option_context_parse (ctx, &test_argc, &test_argvptr, NULL));
  ck_assert_int_eq (test_argc, 2);

  GST_INFO ("-- cleanup --");
  g_option_context_free (ctx);
  BT_TEST_END;
}
END_TEST

START_TEST (test_btic_core_init_bad_arg_exits)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *test_argv[] = { "check_buzztrax", "--btic-non-sense" };
  gchar **test_argvptr = test_argv;
  gint test_argc = G_N_ELEMENTS (test_argv);

  GST_INFO ("-- act --");
  btic_init (&test_argc, &test_argvptr);

  GST_INFO ("-- assert --");
  BT_TEST_END;
}
END_TEST

TCase *
bt_ic_test_case (void)
{
  TCase *tc = tcase_create ("BticTests");

  tcase_add_test (tc, test_btic_core_init_bad_arg_value);
  tcase_add_test (tc, test_btic_core_init_bad_arg);
  tcase_add_exit_test (tc, test_btic_core_init_bad_arg_exits, 1);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
