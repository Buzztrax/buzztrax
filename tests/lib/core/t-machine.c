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

START_TEST (test_bt_machine_add_null_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  check_init_error_trapp ("", "BT_IS_CMD_PATTERN (pattern)");

  GST_INFO ("-- act --");
  bt_machine_add_pattern (gen1, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_add_pattern_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, gen1);

  GST_INFO ("-- act --");
  bt_machine_add_pattern (gen1, (BtCmdPattern *) pattern);

  GST_INFO ("-- assert --");
  GList *list = (GList *) check_gobject_get_ptr_property (gen1, "patterns");
  ck_assert_int_eq (g_list_length (list), 3 + 1);       /* break+mute+solo */

  GST_INFO ("-- cleanup --");
  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_remove_null_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  check_init_error_trapp ("", "BT_IS_CMD_PATTERN (pattern)");

  GST_INFO ("-- act --");
  bt_machine_remove_pattern (gen1, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_state_bypass_on_source)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));

  cparams.id = "vol";
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, src, proc, NULL);
  bt_wire_new (song, proc, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (src, "state", BT_MACHINE_STATE_BYPASS, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (proc, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (sink, "state", BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_state_solo_on_sink)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));

  cparams.id = "vol";
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));

  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, src, proc, NULL);
  bt_wire_new (song, proc, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (sink, "state", BT_MACHINE_STATE_SOLO, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (proc, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (sink, "state", BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

TCase *
bt_machine_test_case (void)
{
  TCase *tc = tcase_create ("BtMachineTests");

  tcase_add_test (tc, test_bt_machine_add_null_pattern);
  tcase_add_test (tc, test_bt_machine_add_pattern_twice);
  tcase_add_test (tc, test_bt_machine_remove_null_pattern);
  tcase_add_test (tc, test_bt_machine_state_bypass_on_source);
  tcase_add_test (tc, test_bt_machine_state_solo_on_sink);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
