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

// show machine properties dialog
static gchar *element_names[] = {
  "buzztrax-test-no-arg-mono-source", "buzztrax-test-mono-source",
  "buzztrax-test-poly-source", "buzztrax-test-poly-source"
};
static gulong element_voices[] = { 0, 0, 0, 1 };

static void
test_bt_machine_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          element_names[_i], element_voices[_i], &err));

  GST_INFO ("-- assert --");
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);
  fail_unless (!bt_machine_has_patterns (machine), NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

/*
 * activate the input level meter in an unconnected machine
 */
static void
test_bt_machine_enable_input_level1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (song, "vol", "volume", 0, NULL));

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_input_pre_level (machine);

  GST_INFO ("-- assert --");
  fail_unless (res == TRUE, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

/*
 * activate the input level meter in a connected machine
 */
static void
test_bt_machine_enable_input_level2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine1 =
      BT_MACHINE (bt_processor_machine_new (song, "vol1", "volume", 0, NULL));
  BtMachine *machine2 =
      BT_MACHINE (bt_processor_machine_new (song, "vol2", "volume", 0, NULL));
  bt_wire_new (song, machine1, machine2, NULL);

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_input_pre_level (machine2);

  GST_INFO ("-- assert --");
  fail_unless (res == TRUE, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

/*
 * activate the input gain control in an unconnected machine
 */
static void
test_bt_machine_enable_input_gain1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (song, "vol", "volume", 0, NULL));

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_input_gain (machine);

  GST_INFO ("-- assert --");
  fail_unless (res == TRUE, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

/*
 * activate the output gain control in an unconnected machine
 */
static void
test_bt_machine_enable_output_gain1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (song, "vol", "volume", 0, NULL));

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_output_gain (machine);

  GST_INFO ("-- assert --");
  fail_unless (res == TRUE, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

/* add pattern */
static void
test_bt_machine_add_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));

  GST_INFO ("-- act --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  GST_INFO ("-- assert --");
  fail_unless (bt_machine_has_patterns (machine), NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_machine_rem_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  GST_INFO ("-- act --");
  bt_machine_remove_pattern (machine, (BtCmdPattern *) pattern);

  GST_INFO ("-- assert --");
  fail_if (bt_machine_has_patterns (machine), NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_machine_unique_pattern_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  GST_INFO ("-- act --");
  gchar *pattern_name = bt_machine_get_unique_pattern_name (machine);

  GST_INFO ("-- assert --");
  ck_assert_str_ne (pattern_name, "pattern-name");

  GST_INFO ("-- cleanup --");
  g_free (pattern_name);
  g_object_unref (pattern);

  BT_TEST_END;
}

static void
test_bt_machine_next_pattern_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "00", 8L, machine);

  GST_INFO ("-- act --");
  gchar *pattern_name = bt_machine_get_unique_pattern_name (machine);

  GST_INFO ("-- assert --");
  ck_assert_str_eq (pattern_name, "01");

  GST_INFO ("-- cleanup --");
  g_free (pattern_name);
  g_object_unref (pattern);

  BT_TEST_END;
}

static void
test_bt_machine_check_voices (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 2L, NULL));

  GST_INFO ("-- assert --");
  GstChildProxy *element =
      (GstChildProxy *) check_gobject_get_object_property (machine, "machine");
  ck_assert_int_eq (gst_child_proxy_get_children_count (element), 2);
  //ck_assert_gobject_gulong_eq(machine,"voices",2);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}

/*
 * change voices and verify that voices in machine and patetrn are in sync
 */
static void
test_bt_machine_change_voices (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *p1 = bt_pattern_new (song, "pattern-name1", 8L, machine);
  BtPattern *p2 = bt_pattern_new (song, "pattern-name2", 8L, machine);

  GST_INFO ("-- act --");
  g_object_set (machine, "voices", 2, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (p1, "voices", 2);
  ck_assert_gobject_gulong_eq (p2, "voices", 2);

  GST_INFO ("-- cleanup --");
  g_object_unref (p1);
  g_object_unref (p2);
  BT_TEST_END;
}

static void
test_bt_machine_state_mute_no_sideeffects (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (song, "vol", "volume", 0L, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  bt_wire_new (song, src, proc, NULL);
  bt_wire_new (song, proc, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (src, "state", BT_MACHINE_STATE_MUTE, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src, "state", BT_MACHINE_STATE_MUTE);
  ck_assert_gobject_guint_eq (proc, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (sink, "state", BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_machine_state_solo_unmutes_others (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *src1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  BtMachine *src2 =
      BT_MACHINE (bt_source_machine_new (song, "gen2", "audiotestsrc", 0L,
          NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  bt_wire_new (song, src1, sink, NULL);
  bt_wire_new (song, src2, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (src1, "state", BT_MACHINE_STATE_SOLO, NULL);
  g_object_set (src2, "state", BT_MACHINE_STATE_SOLO, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src1, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (src2, "state", BT_MACHINE_STATE_SOLO);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_machine_pretty_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (song, "audiotestsrc", "audiotestsrc",
          0L, NULL));

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (src, "pretty-name", "audiotestsrc");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_machine_pretty_name_with_detail (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (src, "pretty-name", "gen (audiotestsrc)");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

TCase *
bt_machine_example_case (void)
{
  TCase *tc = tcase_create ("BtMachineExamples");

  tcase_add_loop_test (tc, test_bt_machine_create, 0,
      G_N_ELEMENTS (element_names));
  tcase_add_test (tc, test_bt_machine_enable_input_level1);
  tcase_add_test (tc, test_bt_machine_enable_input_level2);
  tcase_add_test (tc, test_bt_machine_enable_input_gain1);
  tcase_add_test (tc, test_bt_machine_enable_output_gain1);
  tcase_add_test (tc, test_bt_machine_add_pattern);
  tcase_add_test (tc, test_bt_machine_rem_pattern);
  tcase_add_test (tc, test_bt_machine_unique_pattern_name);
  tcase_add_test (tc, test_bt_machine_next_pattern_name);
  tcase_add_test (tc, test_bt_machine_check_voices);
  tcase_add_test (tc, test_bt_machine_change_voices);
  tcase_add_test (tc, test_bt_machine_state_mute_no_sideeffects);
  tcase_add_test (tc, test_bt_machine_state_solo_unmutes_others);
  tcase_add_test (tc, test_bt_machine_pretty_name);
  tcase_add_test (tc, test_bt_machine_pretty_name_with_detail);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
