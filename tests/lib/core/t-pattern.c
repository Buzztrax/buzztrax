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

static BtApplication *app;
static BtSong *song;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  song = bt_song_new (app);
}

static void
test_teardown (void)
{
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

static void
test_bt_pattern_properties (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  /* act & assert */
  fail_unless (check_gobject_properties ((GObject *) pattern), NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_new_null_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_pattern_", "BT_IS_MACHINE (self->priv->machine)");

  /* act */
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 1L, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (pattern != NULL, NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_new_null_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "id",
          "buzztrax-test-mono-source", 0, NULL));
  check_init_error_trapp ("bt_cmd_pattern_", "BT_IS_STRING (self->priv->name)");

  /* act */
  BtPattern *pattern = bt_pattern_new (song, NULL, 1L, machine);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (pattern != NULL, NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_get_group_by_null_paramgroup (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "id",
          "buzztrax-test-mono-source", 0, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 1L, machine);

  /* act && assert */
  fail_unless (bt_pattern_get_group_by_parameter_group (pattern, NULL) == NULL,
      NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

TCase *
bt_pattern_test_case (void)
{
  TCase *tc = tcase_create ("BtPatternTests");

  tcase_add_test (tc, test_bt_pattern_properties);
  tcase_add_test (tc, test_bt_pattern_new_null_machine);
  tcase_add_test (tc, test_bt_pattern_new_null_name);
  tcase_add_test (tc, test_bt_pattern_get_group_by_null_paramgroup);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
