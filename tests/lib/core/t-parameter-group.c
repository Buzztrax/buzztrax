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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_parameter_group_invalid_param)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtParameterGroup *pg = bt_machine_get_global_param_group (machine);

  GST_INFO ("-- act --");
  glong ix = bt_parameter_group_get_param_index (pg, "nonsense");

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ix, -1);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_no_trigger_param)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-no-arg-mono-source", 0, NULL));
  BtParameterGroup *pg = bt_machine_get_global_param_group (machine);

  GST_INFO ("-- act --");
  glong ix = bt_parameter_group_get_trigger_param_index (pg);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ix, -1);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_no_wave_param)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 0, NULL));
  BtParameterGroup *pg = bt_machine_get_global_param_group (machine);

  GST_INFO ("-- act --");
  glong ix = bt_parameter_group_get_wave_param_index (pg);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ix, -1);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_set_default_for_prefs)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtParameterGroup *pg = bt_machine_get_prefs_param_group (machine);

  GST_INFO ("-- act --");
  bt_parameter_group_set_param_default (pg, 0);

  GST_INFO ("-- assert --");
  check_log_contains ("param g-static is not controllable",
      "param not ignored");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

TCase *
bt_parameter_group_test_case (void)
{
  TCase *tc = tcase_create ("BtParameterGroupTests");

  tcase_add_test (tc, test_bt_parameter_group_invalid_param);
  tcase_add_test (tc, test_bt_parameter_group_no_trigger_param);
  tcase_add_test (tc, test_bt_parameter_group_no_wave_param);
  tcase_add_test (tc, test_bt_parameter_group_set_default_for_prefs);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
