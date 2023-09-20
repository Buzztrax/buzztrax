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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_cmd_pattern_obj_mono1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  GST_INFO ("-- act --");
  BtCmdPattern *pattern =
      bt_cmd_pattern_new (song, machine, BT_PATTERN_CMD_MUTE);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (pattern, "command", BT_PATTERN_CMD_MUTE);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_pattern_obj_poly1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 2L, NULL));

  GST_INFO ("-- act --");
  BtCmdPattern *pattern =
      bt_cmd_pattern_new (song, machine, BT_PATTERN_CMD_MUTE);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (pattern, "command", BT_PATTERN_CMD_MUTE);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

TCase *
bt_cmd_pattern_example_case (void)
{
  TCase *tc = tcase_create ("BtCmdPatternExamples");

  tcase_add_test (tc, test_bt_cmd_pattern_obj_mono1);
  tcase_add_test (tc, test_bt_cmd_pattern_obj_poly1);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
