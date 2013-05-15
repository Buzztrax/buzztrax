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

static void
test_bt_settings_singleton (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSettings *settings1 = bt_settings_make ();

  /* act */
  BtSettings *settings2 = bt_settings_make ();

  /* assert */
  fail_unless (settings1 == settings2, NULL);

  /* cleanup */
  g_object_unref (settings2);
  g_object_checked_unref (settings1);
  BT_TEST_END;
}

static void
test_bt_settings_get_audiosink1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSettings *settings = bt_settings_make ();
  gchar *saved_audiosink_name =
      check_gobject_get_str_property (settings, "audiosink");

  /* act */
  g_object_set (settings, "audiosink", "fakesink", NULL);

  /* assert */
  ck_assert_gobject_str_eq (settings, "audiosink", "fakesink");

  /* cleanup */
  g_object_set (settings, "audiosink", saved_audiosink_name, NULL);
  g_free (saved_audiosink_name);
  g_object_unref (settings);
  BT_TEST_END;
}


TCase *
bt_settings_example_case (void)
{
  TCase *tc = tcase_create ("BtSettingsExamples");

  tcase_add_test (tc, test_bt_settings_singleton);
  tcase_add_test (tc, test_bt_settings_get_audiosink1);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
