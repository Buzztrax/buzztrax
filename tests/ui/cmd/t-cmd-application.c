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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "m-bt-cmd.h"

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

//-- tests

// tests if the play method works without exceptions if we put NULL as filename.
static void
test_bt_cmd_application_play_null_as_filename (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  check_init_error_trapp ("bt_cmd_application_play",
      "BT_IS_STRING (input_file_name)");

  /* act */
  gboolean ret = bt_cmd_application_play (app, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (ret == FALSE, NULL);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

// file not found test. this is a negative test
static void
test_bt_cmd_application_play_non_existing_file (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  check_init_error_trapp ("bt_cmd_application_play",
      "BT_IS_STRING (input_file_name)");

  /* act */
  gboolean ret = bt_cmd_application_play (app, "");

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (ret == FALSE, NULL);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

// test if the info method works with NULL argument for the filename,
static void
test_bt_cmd_application_info_null_as_filename (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  check_init_error_trapp ("bt_cmd_application_info",
      "BT_IS_STRING (input_file_name)");

  /* act */
  gboolean ret = bt_cmd_application_info (app, NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (ret == FALSE, NULL);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

// test if the info method works with a empty filename.
static void
test_bt_cmd_application_info_non_existing_file (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  check_init_error_trapp ("bt_cmd_application_info",
      "BT_IS_STRING (input_file_name)");

  /* act */
  gboolean ret = bt_cmd_application_info (app, "", NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (ret == FALSE, NULL);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

TCase *
bt_cmd_application_test_case (void)
{
  TCase *tc = tcase_create ("BtCmdApplicationTests");

  tcase_add_test (tc, test_bt_cmd_application_play_null_as_filename);
  tcase_add_test (tc, test_bt_cmd_application_play_non_existing_file);
  tcase_add_test (tc, test_bt_cmd_application_info_null_as_filename);
  tcase_add_test (tc, test_bt_cmd_application_info_non_existing_file);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
