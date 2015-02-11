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
  machine = NULL;
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

BtParameterGroup *
get_mono_parameter_group (void)
{
  machine =
      BT_MACHINE (bt_source_machine_new (song, "id",
          "buzztrax-test-mono-source", 0, NULL));
  return bt_machine_get_global_param_group (machine);
}


//-- tests

static void
test_bt_parameter_group_param (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  /* act && assert */
  ck_assert_int_eq (bt_parameter_group_get_param_index (pg, "g-double"), 1);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_parameter_group_size (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  /* act && assert */
  ck_assert_gobject_glong_eq (pg, "num-params", 5);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

/* try describe on a machine that does not implement the interface */
static void
test_bt_parameter_group_describe (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GValue val = { 0, };
  g_value_init (&val, G_TYPE_ULONG);
  g_value_set_ulong (&val, 1L);

  GST_INFO ("-- act --");
  gchar *str = bt_parameter_group_describe_param_value (pg, 0, &val);

  GST_INFO ("-- assert --");
  fail_unless (str == NULL, NULL);

  GST_INFO ("-- cleanup --");
  g_value_unset (&val);
  BT_TEST_END;
}

static void
test_bt_parameter_group_get_trigger (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  GST_INFO ("-- act --");
  glong ix = bt_parameter_group_get_trigger_param_index (pg);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ix, 3);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

TCase *
bt_param_group_example_case (void)
{
  TCase *tc = tcase_create ("BtParamGroupExamples");

  tcase_add_test (tc, test_bt_parameter_group_param);
  tcase_add_test (tc, test_bt_parameter_group_size);
  tcase_add_test (tc, test_bt_parameter_group_describe);
  tcase_add_test (tc, test_bt_parameter_group_get_trigger);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
