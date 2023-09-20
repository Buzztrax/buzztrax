/* Buzztrax
 * Copyright (C) 2015 Buzztrax team <buzztrax-devel@buzztrax.org>
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
static BtMachine *src1;
static BtMachine *src2;
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
  cparams.id = "gen1";
  cparams.song = song;
  
  src1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  
  cparams.id = "gen2";
  src2 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
}

static void
test_teardown (void)
{
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

START_TEST (test_bt_cmd_pattern_control_source_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstObject *m =
      GST_OBJECT (check_gobject_get_object_property (src1, "output-gain"));

  GST_INFO ("-- act --");
  BtCmdPatternControlSource *pcs = bt_cmd_pattern_control_source_new (m, "mute",
      sequence, song_info, src1);

  GST_INFO ("-- assert --");
  ck_assert (pcs != NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (pcs);
  gst_object_unref (m);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_pattern_control_source_normal_default_value)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 1L, src1);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, src1, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  GstObject *m =
      GST_OBJECT (check_gobject_get_object_property (src1, "output-gain"));

  GST_INFO ("-- act --");
  gst_object_sync_values (m, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_genum_eq (src1, "state", (gulong)BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  gst_object_unref (m);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_pattern_control_source_set_mute)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdPattern *pattern = bt_cmd_pattern_new (song, src1, BT_PATTERN_CMD_MUTE);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, src1, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);
  GstObject *m =
      GST_OBJECT (check_gobject_get_object_property (src1, "output-gain"));

  GST_INFO ("-- act --");
  gst_object_sync_values (m, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_genum_eq (src1, "state", (gulong)BT_MACHINE_STATE_MUTE);
  ck_assert_gobject_gboolean_eq (m, "mute", TRUE);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  gst_object_unref (m);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_pattern_control_source_set_solo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdPattern *pattern = bt_cmd_pattern_new (song, src1, BT_PATTERN_CMD_SOLO);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, src1, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);
  GstObject *m1 =
      GST_OBJECT (check_gobject_get_object_property (src1, "output-gain"));
  GstObject *m2 =
      GST_OBJECT (check_gobject_get_object_property (src2, "output-gain"));

  GST_INFO ("-- act --");
  gst_object_sync_values (m1, G_GUINT64_CONSTANT (0));

  GST_INFO ("-- assert --");
  ck_assert_gobject_genum_eq (src1, "state", (gulong)BT_MACHINE_STATE_SOLO);
  ck_assert_gobject_gboolean_eq (m1, "mute", FALSE);
  ck_assert_gobject_gboolean_eq (m2, "mute", TRUE);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  gst_object_unref (m1);
  gst_object_unref (m2);
  BT_TEST_END;
}
END_TEST

TCase *
bt_cmd_pattern_control_source_example_case (void)
{
  TCase *tc = tcase_create ("BtCmdPatternControlSourceExamples");

  tcase_add_test (tc, test_bt_cmd_pattern_control_source_new);
  tcase_add_test (tc, test_bt_cmd_pattern_control_source_normal_default_value);
  tcase_add_test (tc, test_bt_cmd_pattern_control_source_set_mute);
  tcase_add_test (tc, test_bt_cmd_pattern_control_source_set_solo);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
