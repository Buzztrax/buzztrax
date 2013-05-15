/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

// test init with wrong arg usage
static void
test_bt_core_init0 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  gchar *test_argv[] = { "check_buzzard", "--bt-version=5" };
  gchar **test_argvptr = test_argv;
  gint test_argc = G_N_ELEMENTS (test_argv);

  /* act */
  bt_init (&test_argc, &test_argvptr);

  /* assert */
  mark_point ();
  BT_TEST_END;
}

// test init with nonsense args
static void
test_bt_core_init1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  gchar *test_argv[] = { "check_buzzard", "--bt-non-sense" };
  gchar **test_argvptr = test_argv;
  gint test_argc = G_N_ELEMENTS (test_argv);

  /* arrange */
  GOptionContext *ctx = g_option_context_new (NULL);
  g_option_context_add_group (ctx, bt_init_get_option_group ());
  g_option_context_set_ignore_unknown_options (ctx, TRUE);

  /* act & assert */
  fail_unless (g_option_context_parse (ctx, &test_argc, &test_argvptr, NULL));
  ck_assert_int_eq (test_argc, 2);

  /* cleanup */
  g_option_context_free (ctx);
  BT_TEST_END;
}

TCase *
bt_core_test_case (void)
{
  TCase *tc = tcase_create ("BtCoreTests");

  tcase_add_test (tc, test_bt_core_init0);
  tcase_add_test (tc, test_bt_core_init1);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
