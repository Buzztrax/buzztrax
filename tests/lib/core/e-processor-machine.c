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

START_TEST (test_bt_processor_machine_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_processor_machine_def_patterns)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);

  GST_INFO ("-- act --");
  GList *list = (GList *) check_gobject_get_ptr_property (machine, "patterns");

  GST_INFO ("-- assert --");
  ck_assert (list != NULL);
  ck_assert_int_eq (g_list_length (list), 3);   /* break+mute+bypass */

  GST_INFO ("-- cleanup --");
  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_processor_machine_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);

  GST_INFO ("-- act --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L,
      BT_MACHINE (machine));

  GST_INFO ("-- assert --");
  ck_assert (pattern != NULL);
  ck_assert_gobject_gulong_eq (pattern, "voices", 0L);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_processor_machine_pattern_by_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);

  GST_INFO ("-- act --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L,
      BT_MACHINE (machine));

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_machine_get_pattern_by_name (BT_MACHINE
          (machine), "pattern-name"), pattern);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_processor_machine_pattern_by_index)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);

  GST_INFO ("-- act --");
  BtCmdPattern *pattern = bt_machine_get_pattern_by_index (BT_MACHINE (machine),
      BT_PROCESSOR_MACHINE_PATTERN_INDEX_MUTE);

  GST_INFO ("-- assert --");
  ck_assert (pattern != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_processor_machine_pattern_by_list)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L,
      BT_MACHINE (machine));
  GList *list = (GList *) check_gobject_get_ptr_property (machine, "patterns");

  GST_INFO ("-- act --");
  GList *node = g_list_last (list);

  GST_INFO ("-- assert --");
  ck_assert (node->data == pattern);

  GST_INFO ("-- cleanup --");
  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_processor_machine_ref)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "volume", 0, NULL);
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  gst_object_ref (machine);

  GST_INFO ("-- act --");
  bt_setup_remove_machine (setup, BT_MACHINE (machine));

  GST_INFO ("-- assert --");
  ck_assert_int_eq (G_OBJECT_REF_COUNT (machine), 1);

  GST_INFO ("-- cleanup --");
  gst_object_unref (machine);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

TCase *
bt_processor_machine_example_case (void)
{
  TCase *tc = tcase_create ("BtProcessorMachineExamples");

  tcase_add_test (tc, test_bt_processor_machine_new);
  tcase_add_test (tc, test_bt_processor_machine_def_patterns);
  tcase_add_test (tc, test_bt_processor_machine_pattern);
  tcase_add_test (tc, test_bt_processor_machine_pattern_by_id);
  tcase_add_test (tc, test_bt_processor_machine_pattern_by_index);
  tcase_add_test (tc, test_bt_processor_machine_pattern_by_list);
  tcase_add_test (tc, test_bt_processor_machine_ref);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
