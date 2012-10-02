/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-core.h"

//-- globals
static BtApplication *app;
static BtSong *song;
static BtSequence *sequence;
static BtMachine *machine;
static BtParameterGroup *pg;
static GstObject *element;

//-- fixtures

static void
case_setup (void)
{
  GST_INFO
      ("================================================================================");
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  song = bt_song_new (app);
  sequence = BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0, NULL));
  element = GST_OBJECT (check_gobject_get_object_property (machine, "machine"));
  pg = bt_machine_get_global_param_group (machine);
}

static void
test_teardown (void)
{
  gst_object_unref (element);
  g_object_unref (machine);
  g_object_unref (sequence);
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_bt_pattern_control_source_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtPatternControlSource *pcs = bt_pattern_control_source_new (sequence,
      machine, pg);

  /* assert */
  fail_unless (pcs != NULL, NULL);

  /* cleanup */
  g_object_unref (pcs);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_normal_default_value (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  g_object_set (element, "g-ulong", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  /* act */
  gst_object_sync_values (G_OBJECT (element), G_GUINT64_CONSTANT (0));

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 10);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_trigger_default_value (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  /* act */
  gst_object_sync_values (G_OBJECT (element), G_GUINT64_CONSTANT (0));

  /* assert */
  ck_assert_gobject_genum_eq (element, "g-note", GSTBT_NOTE_NONE);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_override_default_value (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  g_object_set (element, "g-ulong", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  bt_pattern_set_global_event (pattern, 0, 0, "100");

  /* act */
  gst_object_sync_values (G_OBJECT (element), G_GUINT64_CONSTANT (0));

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 100);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_restore_default_value (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  g_object_set (element, "g-ulong", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  bt_pattern_set_global_event (pattern, 0, 0, "100");
  bt_pattern_set_global_event (pattern, 0, 0, NULL);

  /* act */
  gst_object_sync_values (G_OBJECT (element), G_GUINT64_CONSTANT (0));

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 10);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_change_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 0, "100");

  /* act */
  gst_object_sync_values (G_OBJECT (element), G_GUINT64_CONSTANT (0));

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 100);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_hold_normal (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  GstClockTime tick_time = bt_sequence_get_bar_time (sequence);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 0, "50");
  bt_pattern_set_global_event (pattern, 4, 0, "100");
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);

  /* act */
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (1) * tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 50);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_release_trigger (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  GstClockTime tick_time = bt_sequence_get_bar_time (sequence);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 3, "c-4");
  bt_pattern_set_global_event (pattern, 4, 3, "c-5");
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);

  /* act */
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (1) * tick_time);

  /* assert */
  ck_assert_gobject_genum_eq (element, "g-note", GSTBT_NOTE_NONE);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_combine_pattern_shadows (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern1 =
      bt_pattern_new (song, "pattern1", "pattern1", 8L, machine);
  BtPattern *pattern2 =
      bt_pattern_new (song, "pattern2", "pattern2", 8L, machine);
  GstClockTime tick_time = bt_sequence_get_bar_time (sequence);
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern1);
  bt_sequence_set_pattern (sequence, 4, 0, (BtCmdPattern *) pattern2);
  bt_pattern_set_global_event (pattern1, 0, 0, "50");
  bt_pattern_set_global_event (pattern1, 4, 0, "100");
  bt_pattern_set_global_event (pattern2, 0, 0, "200");  /* value shadows above */

  /* act */
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (4) * tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 200);

  /* cleanup */
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_combine_pattern_unshadows (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern1 =
      bt_pattern_new (song, "pattern1", "pattern1", 8L, machine);
  BtPattern *pattern2 =
      bt_pattern_new (song, "pattern2", "pattern2", 8L, machine);
  GstClockTime tick_time = bt_sequence_get_bar_time (sequence);
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern1);
  bt_sequence_set_pattern (sequence, 4, 0, (BtCmdPattern *) pattern2);
  bt_pattern_set_global_event (pattern1, 0, 0, "50");
  bt_pattern_set_global_event (pattern1, 4, 0, "100");
  bt_pattern_set_global_event (pattern2, 0, 0, "200");  /* value shadows above */
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (4) * tick_time);

  /* act */
  bt_sequence_set_pattern (sequence, 4, 0, NULL);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (4) * tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 100);

  /* cleanup */
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_combine_value_unshadows (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern1 =
      bt_pattern_new (song, "pattern1", "pattern1", 8L, machine);
  BtPattern *pattern2 =
      bt_pattern_new (song, "pattern2", "pattern2", 8L, machine);
  GstClockTime tick_time = bt_sequence_get_bar_time (sequence);
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern1);
  bt_sequence_set_pattern (sequence, 4, 0, (BtCmdPattern *) pattern2);
  bt_pattern_set_global_event (pattern1, 0, 0, "50");
  bt_pattern_set_global_event (pattern1, 4, 0, "100");
  bt_pattern_set_global_event (pattern2, 0, 0, "200");  /* value shadows above */
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (4) * tick_time);

  /* act */
  bt_pattern_set_global_event (pattern2, 0, 0, NULL);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (4) * tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 50);

  /* cleanup */
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}

static void
test_bt_pattern_control_source_combine_two_tracks (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtPattern *pattern =
      bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  GstClockTime tick_time = bt_sequence_get_bar_time (sequence);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_sequence_set_pattern (sequence, 1, 1, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 0, "50");
  bt_pattern_set_global_event (pattern, 1, 0, "100");
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (1) * tick_time);

  /* act */
  bt_sequence_set_pattern (sequence, 1, 1, NULL);
  gst_object_sync_values (G_OBJECT (element),
      G_GUINT64_CONSTANT (1) * tick_time);

  /* assert */
  ck_assert_gobject_gulong_eq (element, "g-ulong", 100);

  /* cleanup */
  g_object_unref (pattern);
  BT_TEST_END;
}

TCase *
bt_pattern_control_source_example_case (void)
{
  TCase *tc = tcase_create ("BtPatternControlSourceExamples");

  tcase_add_test (tc, test_bt_pattern_control_source_new);
  tcase_add_test (tc, test_bt_pattern_control_source_normal_default_value);
  tcase_add_test (tc, test_bt_pattern_control_source_trigger_default_value);
  tcase_add_test (tc, test_bt_pattern_control_source_override_default_value);
  tcase_add_test (tc, test_bt_pattern_control_source_restore_default_value);
  tcase_add_test (tc, test_bt_pattern_control_source_change_pattern);
  tcase_add_test (tc, test_bt_pattern_control_source_hold_normal);
  tcase_add_test (tc, test_bt_pattern_control_source_release_trigger);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_pattern_shadows);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_pattern_unshadows);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_value_unshadows);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_two_tracks);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
