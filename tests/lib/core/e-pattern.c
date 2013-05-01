/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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
test_bt_pattern_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));

  /* act */
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  /* assert */
  ck_assert_gobject_str_eq (pattern, "name", "pattern-name");

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}


static gchar *element_names[] = {
  "buzztrax-test-no-arg-mono-source", "buzztrax-test-mono-source",
  "buzztrax-test-poly-source", "buzztrax-test-poly-source"
};
static gulong element_voices[] = { 0, 0, 0, 1 };

static void
test_bt_pattern_obj_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          element_names[_i], element_voices[_i], NULL));

  /* act */
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  /* assert */
  ck_assert_gobject_gulong_eq (pattern, "voices", element_voices[_i]);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_obj_mono (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  /* act */
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 0, 1, "2.5");
  bt_pattern_set_global_event (pattern, 0, 2, "1");

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), "5");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 1),
      "2.5");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 2), "1");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 6, 0), NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_obj_poly (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 2L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  /* act */
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 4, 0, "10");
  bt_pattern_set_voice_event (pattern, 0, 0, 0, "5");
  bt_pattern_set_voice_event (pattern, 4, 0, 0, "10");

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), "5");
  ck_assert_str_eq_and_free (bt_pattern_get_voice_event (pattern, 4, 0, 0),
      "10");
  ck_assert_str_eq_and_free (bt_pattern_get_voice_event (pattern, 6, 0, 0),
      NULL);

  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_obj_wire1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *src_machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtMachine *sink_machine =
      BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  BtWire *wire = bt_wire_new (song, src_machine, sink_machine, NULL);

  /* act */
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, sink_machine);

  /* assert */
  fail_unless (bt_pattern_get_wire_group (pattern, wire) != NULL, NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_obj_wire2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *src_machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtMachine *sink_machine =
      BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  BtWire *wire = bt_wire_new (song, src_machine, sink_machine, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, sink_machine);

  /* act */
  bt_pattern_set_wire_event (pattern, 0, wire, 0, "100");

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_wire_event (pattern, 0, wire, 0),
      "100");

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_copy (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 2L, NULL));
  BtPattern *pattern1 = bt_pattern_new (song, "pattern-name", 8L, machine);
  bt_pattern_set_global_event (pattern1, 0, 0, "5");
  bt_pattern_set_voice_event (pattern1, 4, 0, 0, "10");

  /* act */
  BtPattern *pattern2 = bt_pattern_copy (pattern1);

  /* assert */
  fail_unless (pattern2 != NULL, NULL);
  fail_unless (pattern2 != pattern1, NULL);
  ck_assert_gobject_gulong_eq (pattern2, "voices", 2);
  ck_assert_gobject_gulong_eq (pattern2, "length", 8);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern2, 0, 0), "5");

  /* cleanup */
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}

static void
test_bt_pattern_has_data (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  /* act */
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 4, 0, "10");

  /* assert */
  fail_unless (bt_pattern_test_tick (pattern, 0) == TRUE, NULL);
  fail_unless (bt_pattern_test_tick (pattern, 4) == TRUE, NULL);
  fail_unless (bt_pattern_test_tick (pattern, 1) == FALSE, NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_enlarge_length (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 4, 0, "10");

  /* act */
  g_object_set (pattern, "length", 16L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (pattern, "length", 16);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), "5");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 4, 0), "10");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 10, 0),
      NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_shrink_length (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 16L, machine);
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 12, 0, "10");

  /* act */
  g_object_set (pattern, "length", 8L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (pattern, "length", 8);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), "5");

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_enlarge_voices (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_voice_event (pattern, 4, 0, 0, "10");

  /* act */
  g_object_set (machine, "voices", 2L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (pattern, "voices", 2);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), "5");
  ck_assert_str_eq_and_free (bt_pattern_get_voice_event (pattern, 4, 0, 0),
      "10");
  ck_assert_str_eq_and_free (bt_pattern_get_voice_event (pattern, 0, 1, 0),
      NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_shrink_voices (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 2L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_voice_event (pattern, 4, 0, 0, "10");

  /* act */
  g_object_set (machine, "voices", 1L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (pattern, "voices", 1);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), "5");
  ck_assert_str_eq_and_free (bt_pattern_get_voice_event (pattern, 4, 0, 0),
      "10");

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_insert_row (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 4, 0, "10");

  /* act */
  bt_value_group_insert_row (bt_pattern_get_global_group (pattern), 0, 0);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), NULL);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 1, 0), "5");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 4, 0), NULL);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 5, 0), "10");

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_delete_row (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  bt_pattern_set_global_event (pattern, 0, 0, "5");
  bt_pattern_set_global_event (pattern, 4, 0, "10");

  /* act */
  bt_value_group_delete_row (bt_pattern_get_global_group (pattern), 0, 0);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 0), NULL);
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 3, 0), "10");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 4, 0), NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_mono_get_global_vg (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "id",
          "buzztrax-test-mono-source", 0, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 1L, machine);

  /* act && assert */
  fail_unless (bt_pattern_get_global_group (pattern) != NULL, NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_mono_get_voice_vg (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "id",
          "buzztrax-test-mono-source", 0, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 1L, machine);

  /* act && assert */
  fail_unless (bt_pattern_get_voice_group (pattern, 1) == NULL, NULL);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

TCase *
bt_pattern_example_case (void)
{
  TCase *tc = tcase_create ("BtPatternExamples");

  tcase_add_test (tc, test_bt_pattern_name);
  tcase_add_loop_test (tc, test_bt_pattern_obj_create, 0,
      G_N_ELEMENTS (element_names));
  tcase_add_test (tc, test_bt_pattern_obj_mono);
  tcase_add_test (tc, test_bt_pattern_obj_poly);
  tcase_add_test (tc, test_bt_pattern_obj_wire1);
  tcase_add_test (tc, test_bt_pattern_obj_wire2);
  tcase_add_test (tc, test_bt_pattern_copy);
  tcase_add_test (tc, test_bt_pattern_has_data);
  tcase_add_test (tc, test_bt_pattern_enlarge_length);
  tcase_add_test (tc, test_bt_pattern_shrink_length);
  tcase_add_test (tc, test_bt_pattern_enlarge_voices);
  tcase_add_test (tc, test_bt_pattern_shrink_voices);
  tcase_add_test (tc, test_bt_pattern_insert_row);
  tcase_add_test (tc, test_bt_pattern_delete_row);
  tcase_add_test (tc, test_bt_pattern_mono_get_global_vg);
  tcase_add_test (tc, test_bt_pattern_mono_get_voice_vg);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
