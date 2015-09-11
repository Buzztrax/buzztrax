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

static BtApplication *app;
static BtSong *song;
static BtMachine *machine;
static BtPattern *pattern;

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
  g_object_unref (pattern);
  pattern = NULL;
  machine = NULL;
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

static BtValueGroup *
get_mono_value_group (void)
{
  machine =
      BT_MACHINE (bt_source_machine_new (song, "id",
          "buzztrax-test-mono-source", 0, NULL));
  pattern = bt_pattern_new (song, "pattern-name", 4L, machine);
  return bt_pattern_get_global_group (pattern);
}

//-- tests

static void
test_bt_value_group_get_beyond_size (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();

  /* act && assert */
  fail_unless (bt_value_group_get_event_data (vg, 100, 100) == NULL, NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_value_group_range_randomize_column_empty_end (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "10");

  GST_INFO ("-- act --");
  bt_value_group_range_randomize_column (vg, 0, 3, 0);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "10");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 2, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 3, 0), NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}


TCase *
bt_value_group_test_case (void)
{
  TCase *tc = tcase_create ("BtValueGroupTests");

  tcase_add_test (tc, test_bt_value_group_get_beyond_size);
  tcase_add_test (tc, test_bt_value_group_range_randomize_column_empty_end);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
