/* Buzztrax
 * Copyright (C) 2019 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-edit.h"

//-- globals

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
}

static void
test_teardown (void)
{
}

static void
case_teardown (void)
{
}


//-- helper

//-- tests

static void
test_bt_strjoin_list_with_empty_list (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  gchar *res = bt_strjoin_list (NULL);

  GST_INFO ("-- assert --");
  fail_unless (res == NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_strjoin_list_with_single_value_list (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GList *list = g_list_append (NULL, "first");

  GST_INFO ("-- act --");
  gchar *res = bt_strjoin_list (list);

  GST_INFO ("-- assert --");
  fail_unless (res != NULL, NULL);
  fail_unless (strcmp ("first", res) == 0, NULL);

  GST_INFO ("-- cleanup --");
  g_free (res);
  g_list_free (list);
  BT_TEST_END;
}

static void
test_bt_strjoin_list_with_two_value_list (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GList *list = g_list_append (NULL, "first");
  list = g_list_append (list, "second");

  GST_INFO ("-- act --");
  gchar *res = bt_strjoin_list (list);

  GST_INFO ("-- assert --");
  fail_unless (res != NULL, NULL);
  fail_unless (strcmp ("first\nsecond", res) == 0, NULL);

  GST_INFO ("-- cleanup --");
  g_free (res);
  g_list_free (list);
  BT_TEST_END;
}


TCase *
bt_tools_example_case (void)
{
  TCase *tc = tcase_create ("BtToolsExamples");

  tcase_add_test (tc, test_bt_strjoin_list_with_empty_list);
  tcase_add_test (tc, test_bt_strjoin_list_with_single_value_list);
  tcase_add_test (tc, test_bt_strjoin_list_with_two_value_list);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
