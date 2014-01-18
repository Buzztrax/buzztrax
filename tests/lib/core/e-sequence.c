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
  BtSettings *settings;

  app = bt_test_application_new ();
  // no beeps please
  settings = bt_settings_make ();
  g_object_set (settings, "audiosink", "fakesink", NULL);
  g_object_unref (settings);
  song = bt_song_new (app);
  bt_child_proxy_get ((gpointer) song, "song-info::tick-duration", &tick_time,
      NULL);
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
test_bt_sequence_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));

  /* assert */
  ck_assert_gobject_gulong_eq (sequence, "length", 0);
  ck_assert_gobject_gulong_eq (sequence, "tracks", 0);
  ck_assert_gobject_boolean_eq (sequence, "loop", FALSE);
  ck_assert_gobject_glong_eq (sequence, "loop-start", -1);
  ck_assert_gobject_glong_eq (sequence, "loop-end", -1);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_labels (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 8L, NULL);

  /* act */
  bt_sequence_set_label (sequence, 0, "test");
  bt_sequence_set_label (sequence, 7, "test");

  /* assert */
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 0), "test");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 1), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 7), "test");

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_tracks (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));

  /* act */
  bt_sequence_add_track (sequence, machine, -1);

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0),
      machine);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  /* act */
  bt_sequence_set_pattern (sequence, 0, 0, pattern);
  bt_sequence_set_pattern (sequence, 7, 1, pattern);

  /* assert */
  fail_unless (bt_sequence_is_pattern_used (sequence, (BtPattern *) pattern),
      NULL);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      pattern);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 7, 1),
      pattern);

  /* cleanup */
  g_object_try_unref (pattern);
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarge_length (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));

  /* act */
  g_object_set (sequence, "length", 8L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (sequence, "length", 8);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarge_length_check_labels (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));

  /* act */
  g_object_set (sequence, "length", 4L, NULL);

  /* assert */
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 0), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 1), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 2), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 3), NULL);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarge_length_labels (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_set_label (sequence, 0, "test");
  bt_sequence_set_label (sequence, 7, "test");

  /* act */
  g_object_set (sequence, "length", 16L, NULL);

  /* assert */
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 0), "test");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 1), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 7), "test");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 8), NULL);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_shrink_length (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);

  /* act */
  g_object_set (sequence, "length", 8L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (sequence, "length", 8);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarge_track (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen-m",
          "buzztrax-test-mono-source", 0, NULL));

  /* act */
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  /* assert */
  ck_assert_gobject_gulong_eq (sequence, "tracks", 2);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarge_track_vals (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));
  bt_sequence_add_track (sequence, machine, -1);

  /* act */
  g_object_set (sequence, "tracks", 2L, NULL);

  /* assert */
  ck_assert_gobject_gulong_eq (sequence, "tracks", 2);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0),
      machine);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 1), NULL);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_shrink_track (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));
  g_object_set (sequence, "length", 1L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  /* act */
  bt_sequence_remove_track_by_ix (sequence, 0);
  ck_assert_gobject_gulong_eq (sequence, "tracks", 1);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0),
      machine);

  /* cleanup */
  g_object_try_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarge_both_vals (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);
  bt_sequence_set_pattern (sequence, 7, 1, pattern);

  /* act */
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      pattern);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 7, 1),
      pattern);

  /* cleanup */
  g_object_try_unref (pattern);
  g_object_try_unref (sequence);
  BT_TEST_END;
}

/* we moved these updates to the app, to give the undo/redo framework a chance
 * to backup the data
 */
#ifdef __CHECK_DISABLED__
// test that removing patterns updates the sequence
static void
test_bt_sequence_update (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);

  /* act */
  bt_machine_remove_pattern (machine, pattern);

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      NULL);

  /* cleanup */
  g_object_try_unref (pattern);
  g_object_try_unref (sequence);
  BT_TEST_END;
}
#endif

typedef struct
{
  gint ct;
  gint values[8];
} BtSequenceTicksTestData;

static void
on_btsequence_ticks_notify (GstObject * machine, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceTicksTestData *data = (BtSequenceTicksTestData *) user_data;
  static GstClockTime last = G_GUINT64_CONSTANT (0);
  GstClockTime cur = gst_util_get_timestamp ();
  GstClockTime diff = GST_CLOCK_DIFF (last, cur);
  last = cur;

  GST_INFO ("notify %d (%" GST_TIME_FORMAT ")", data->ct, GST_TIME_ARGS (diff));
  if (data->ct < 9) {
    g_object_get (machine, "wave", &data->values[data->ct], NULL);
  }
  data->ct++;
}

static void
test_bt_sequence_ticks (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequenceTicksTestData data = { 0, };
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  g_object_set (song_info, "bpm", 250L, "tpb", 16L, NULL);
  /* need a real element that handles tempo and calls gst_object_sync */
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (song, "gen", "simsyn", 0, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  bt_wire_new (song, src, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, src);
  GstObject *element =
      GST_OBJECT (check_gobject_get_object_property (src, "machine"));
  g_object_set (sequence, "length", 8L, NULL);
  GstClockTime play_time = bt_song_info_tick_to_time (song_info, 8L);
  bt_sequence_add_track (sequence, src, -1);
  bt_pattern_set_global_event (pattern, 0, 1, "0");
  bt_pattern_set_global_event (pattern, 1, 1, "1");
  bt_pattern_set_global_event (pattern, 2, 1, "2");
  bt_pattern_set_global_event (pattern, 3, 1, "3");
  bt_pattern_set_global_event (pattern, 4, 1, "4");
  bt_pattern_set_global_event (pattern, 5, 1, "5");
  bt_pattern_set_global_event (pattern, 6, 1, "6");
  bt_pattern_set_global_event (pattern, 7, 1, "7");
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  g_signal_connect (G_OBJECT (element), "notify::wave",
      G_CALLBACK (on_btsequence_ticks_notify), &data);

  /* act */
  GST_INFO ("play for %" GST_TIME_FORMAT, GST_TIME_ARGS (play_time));
  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);
  GST_INFO ("playing ...");
  // TODO(ensonic): this should not need to '*6' but there seems to be a lag in
  // the notify emmissions that we can handle
  // GST_DEBUG="audiosynth:5" BT_CHECKS="test_bt_sequence_*" make bt_core.check
  // egrep "(bt-check|audiosynth)" /tmp/bt_core/e-sequence/test_bt_sequence_ticks.0.log
  check_run_main_loop_for_usec (GST_TIME_AS_USECONDS (play_time * 6));
  GST_INFO ("stop");
  bt_song_stop (song);

  /* assert */
  ck_assert_int_ge (data.ct, 8);
  ck_assert_int_eq (data.values[0], 0);
  ck_assert_int_eq (data.values[1], 1);
  ck_assert_int_eq (data.values[2], 2);
  ck_assert_int_eq (data.values[3], 3);
  ck_assert_int_eq (data.values[4], 4);
  ck_assert_int_eq (data.values[5], 5);
  ck_assert_int_eq (data.values[6], 6);
  ck_assert_int_eq (data.values[7], 7);

  /* cleanup */
  gst_object_unref (element);
  g_object_unref (pattern);
  g_object_unref (sequence);
  g_object_unref (song_info);
  BT_TEST_END;
}

static void
test_bt_sequence_default_loop (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);

  /* act */
  g_object_set (sequence, "loop", TRUE, NULL);

  /* assert */
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 16);

  /* cleanup */
  g_object_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarging_length_enlarges_loop (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);

  /* act */
  g_object_set (sequence, "length", 24L, NULL);

  /* assert */
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 24);

  /* cleanup */
  g_object_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_enlarging_length_keeps_loop (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);
  g_object_set (sequence, "loop-end", 8L, NULL);

  /* act */
  g_object_set (sequence, "length", 12L, NULL);

  /* assert */
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 8);

  /* cleanup */
  g_object_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_shortening_length_truncates_loop (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);

  /* act */
  g_object_set (sequence, "length", 8L, NULL);

  /* assert */
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 8);

  /* cleanup */
  g_object_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_shortening_length_disables_loop (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 24L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);
  g_object_set (sequence, "loop-start", 16L, NULL);

  /* act */
  g_object_set (sequence, "length", 12L, NULL);

  /* assert */
  ck_assert_gobject_boolean_eq (sequence, "loop", FALSE);

  /* cleanup */
  g_object_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_duration (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  bt_wire_new (song, gen, sink, NULL);
  GstElement *sink_bin =
      GST_ELEMENT (check_gobject_get_object_property (sink, "machine"));
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  /* act */
  gint64 duration;
  gboolean res =
      gst_element_query_duration (sink_bin, GST_FORMAT_TIME, &duration);

  /* assert */
  fail_unless (res, NULL);
  ck_assert_int64_ne (duration, G_GINT64_CONSTANT (-1));
  ck_assert_uint64_eq (duration, 16L * tick_time);

  /* cleanup */
  gst_object_unref (sink_bin);
  g_object_unref (sequence);
  BT_TEST_END;
}

static void
test_bt_sequence_duration_play (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  bt_wire_new (song, gen, sink, NULL);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (gen, "machine");
  g_object_set (element, "wave", /* silence */ 4, NULL);
  GstElement *sink_bin =
      GST_ELEMENT (check_gobject_get_object_property (sink, "machine"));
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  /* act */
  gint64 duration;
  gboolean res =
      gst_element_query_duration (sink_bin, GST_FORMAT_TIME, &duration);

  /* assert */
  fail_unless (res, NULL);
  ck_assert_int64_ne (duration, -1);
  ck_assert_uint64_eq (duration, 16L * tick_time);

  /* cleanup */
  bt_song_stop (song);
  gst_object_unref (element);
  gst_object_unref (sink_bin);
  g_object_unref (sequence);
  BT_TEST_END;
}


TCase *
bt_sequence_example_case (void)
{
  TCase *tc = tcase_create ("BtSequenceExamples");

  tcase_add_test (tc, test_bt_sequence_new);
  tcase_add_test (tc, test_bt_sequence_labels);
  tcase_add_test (tc, test_bt_sequence_tracks);
  tcase_add_test (tc, test_bt_sequence_pattern);
  tcase_add_test (tc, test_bt_sequence_enlarge_length);
  tcase_add_test (tc, test_bt_sequence_enlarge_length_check_labels);
  tcase_add_test (tc, test_bt_sequence_enlarge_length_labels);
  tcase_add_test (tc, test_bt_sequence_shrink_length);
  tcase_add_test (tc, test_bt_sequence_enlarge_track);
  tcase_add_test (tc, test_bt_sequence_enlarge_track_vals);
  tcase_add_test (tc, test_bt_sequence_shrink_track);
  tcase_add_test (tc, test_bt_sequence_enlarge_both_vals);
  //tcase_add_test(tc,test_bt_sequence_update);
  tcase_add_test (tc, test_bt_sequence_ticks);
  tcase_add_test (tc, test_bt_sequence_default_loop);
  tcase_add_test (tc, test_bt_sequence_enlarging_length_enlarges_loop);
  tcase_add_test (tc, test_bt_sequence_enlarging_length_keeps_loop);
  tcase_add_test (tc, test_bt_sequence_shortening_length_truncates_loop);
  tcase_add_test (tc, test_bt_sequence_shortening_length_disables_loop);
  tcase_add_test (tc, test_bt_sequence_duration);
  tcase_add_test (tc, test_bt_sequence_duration_play);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
