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
static BtSongInfo *song_info;
static BtSequence *sequence;
static BtMachine *machine;
static BtParameterGroup *pg;
static GstObject *element;
static GstClockTime tick_time;

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
  bt_child_proxy_get ((gpointer) song, "sequence", &sequence, "song-info",
      &song_info, "song-info::tick-duration", &tick_time, NULL);
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  element = GST_OBJECT (check_gobject_get_object_property (machine, "machine"));
  pg = bt_machine_get_global_param_group (machine);
}

static void
test_teardown (void)
{
  gst_object_unref (element);
  g_object_unref (song_info);
  g_object_unref (sequence);
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- tests

START_TEST (test_bt_pattern_control_source_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstObject *m =
      GST_OBJECT (check_gobject_get_object_property (machine, "machine"));

  GST_INFO ("-- act --");
  BtPatternControlSource *pcs = bt_pattern_control_source_new (m, "g-uint",
      sequence, song_info, machine, pg);

  GST_INFO ("-- assert --");
  ck_assert (pcs != NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (pcs);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_normal_default_for_empty_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  g_object_set (element, "g-uint", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 10);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_normal_default_for_cmd_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdPattern *pattern = bt_cmd_pattern_new (song, machine,
      BT_PATTERN_CMD_MUTE);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);

  g_object_set (element, "g-uint", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 10);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_trigger_default_for_empty_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_int_eq (((BtTestMonoSource *) element)->note_val, GSTBT_NOTE_NONE);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_override_default_from_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  g_object_set (element, "g-uint", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  bt_pattern_set_global_event (pattern, 0, 0, "100");

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 100);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_restore_default_on_pattern_cleared)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  g_object_set (element, "g-uint", 10, NULL);
  bt_parameter_group_set_param_default (pg, 0);

  bt_pattern_set_global_event (pattern, 0, 0, "100");
  bt_pattern_set_global_event (pattern, 0, 0, NULL);

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 10);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_change_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 0, "100");

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 100);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_hold_normal)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 0, "50");
  bt_pattern_set_global_event (pattern, 4, 0, "100");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (1) * tick_time);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 50);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_release_trigger)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 3, "c-4");
  bt_pattern_set_global_event (pattern, 4, 3, "c-5");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (1) * tick_time);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (((BtTestMonoSource *) element)->note_val, GSTBT_NOTE_NONE);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_combine_pattern_shadows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern1 = bt_pattern_new (song, "pattern1", 8L, machine);
  BtPattern *pattern2 = bt_pattern_new (song, "pattern2", 8L, machine);
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern1);
  bt_sequence_set_pattern (sequence, 4, 0, (BtCmdPattern *) pattern2);
  bt_pattern_set_global_event (pattern1, 0, 0, "50");
  bt_pattern_set_global_event (pattern1, 4, 0, "100");
  bt_pattern_set_global_event (pattern2, 0, 0, "200");  /* value shadows above */

  GST_INFO ("-- act --");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (4) * tick_time);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 200);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_combine_pattern_unshadows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern1 = bt_pattern_new (song, "pattern1", 8L, machine);
  BtPattern *pattern2 = bt_pattern_new (song, "pattern2", 8L, machine);
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern1);
  bt_sequence_set_pattern (sequence, 4, 0, (BtCmdPattern *) pattern2);
  bt_pattern_set_global_event (pattern1, 0, 0, "50");
  bt_pattern_set_global_event (pattern1, 4, 0, "100");
  bt_pattern_set_global_event (pattern2, 0, 0, "200");  /* value shadows above */
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (4) * tick_time);

  GST_INFO ("-- act --");
  bt_sequence_set_pattern (sequence, 4, 0, NULL);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (4) * tick_time);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 100);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_combine_value_unshadows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern1 = bt_pattern_new (song, "pattern1", 8L, machine);
  BtPattern *pattern2 = bt_pattern_new (song, "pattern2", 8L, machine);
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern1);
  bt_sequence_set_pattern (sequence, 4, 0, (BtCmdPattern *) pattern2);
  bt_pattern_set_global_event (pattern1, 0, 0, "50");
  bt_pattern_set_global_event (pattern1, 4, 0, "100");
  bt_pattern_set_global_event (pattern2, 0, 0, "200");  /* value shadows above */
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (4) * tick_time);

  GST_INFO ("-- act --");
  bt_pattern_set_global_event (pattern2, 0, 0, NULL);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (4) * tick_time);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 50);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_pattern_control_source_combine_two_tracks)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_sequence_set_pattern (sequence, 1, 1, (BtCmdPattern *) pattern);
  bt_pattern_set_global_event (pattern, 0, 0, "50");
  bt_pattern_set_global_event (pattern, 1, 0, "100");
  gst_object_sync_values (element, G_GUINT64_CONSTANT (0) * tick_time);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (1) * tick_time);

  GST_INFO ("-- act --");
  bt_sequence_set_pattern (sequence, 1, 1, NULL);
  gst_object_sync_values (element, G_GUINT64_CONSTANT (1) * tick_time);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 100);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

TCase *
bt_pattern_control_source_example_case (void)
{
  TCase *tc = tcase_create ("BtPatternControlSourceExamples");

  tcase_add_test (tc, test_bt_pattern_control_source_new);
  tcase_add_test (tc,
      test_bt_pattern_control_source_normal_default_for_empty_pattern);
  tcase_add_test (tc,
      test_bt_pattern_control_source_normal_default_for_cmd_pattern);
  tcase_add_test (tc,
      test_bt_pattern_control_source_trigger_default_for_empty_pattern);
  tcase_add_test (tc,
      test_bt_pattern_control_source_override_default_from_pattern);
  tcase_add_test (tc,
      test_bt_pattern_control_source_restore_default_on_pattern_cleared);
  tcase_add_test (tc, test_bt_pattern_control_source_change_pattern);
  tcase_add_test (tc, test_bt_pattern_control_source_hold_normal);
  tcase_add_test (tc, test_bt_pattern_control_source_release_trigger);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_pattern_shadows);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_pattern_unshadows);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_value_unshadows);
  tcase_add_test (tc, test_bt_pattern_control_source_combine_two_tracks);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
