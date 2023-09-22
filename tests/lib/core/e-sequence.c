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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_sequence_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (sequence, "length", 0L);
  ck_assert_gobject_gulong_eq (sequence, "tracks", 0L);
  ck_assert_gobject_gboolean_eq (sequence, "loop", FALSE);
  ck_assert_gobject_glong_eq (sequence, "loop-start", -1L);
  ck_assert_gobject_glong_eq (sequence, "loop-end", -1L);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_labels)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 8L, NULL);

  GST_INFO ("-- act --");
  bt_sequence_set_label (sequence, 0, "test");
  bt_sequence_set_label (sequence, 7, "test");

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 0), "test");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 1), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 7), "test");

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_append_track)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  GST_INFO ("-- act --");
  bt_sequence_add_track (sequence, gen, -1);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0), gen);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_insert_track)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtMachine *gen2 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 0, NULL));
  bt_sequence_add_track (sequence, gen1, -1);

  GST_INFO ("-- act --");
  bt_sequence_add_track (sequence, gen2, 0);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0), gen2);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_move_track_left)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen1";
  
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  
  cparams.id = "gen2";
  BtMachine *gen2 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 0, NULL));
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_add_track (sequence, gen2, -1);

  GST_INFO ("-- act --");
  bt_sequence_move_track_left (sequence, 1);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0), gen2);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_move_track_right)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtMachine *gen2 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 0, NULL));
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_add_track (sequence, gen2, -1);

  GST_INFO ("-- act --");
  bt_sequence_move_track_right (sequence, 0);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0), gen2);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- act --");
  bt_sequence_set_pattern (sequence, 0, 0, pattern);
  bt_sequence_set_pattern (sequence, 7, 1, pattern);

  GST_INFO ("-- assert --");
  fail_unless (bt_sequence_is_pattern_used (sequence, (BtPattern *) pattern));
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      pattern);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 7, 1),
      pattern);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (pattern);
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_get_tick_by_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtCmdPattern *p1 = (BtCmdPattern *) bt_pattern_new (song, "p1", 4L, machine);
  BtCmdPattern *p2 = (BtCmdPattern *) bt_pattern_new (song, "p2", 4L, machine);
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- act --");
  bt_sequence_set_pattern (sequence, 0, 0, p1);
  bt_sequence_set_pattern (sequence, 3, 0, p2);
  bt_sequence_set_pattern (sequence, 6, 0, p1);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (bt_sequence_get_tick_by_pattern (sequence, 0, p1, 0), 0);
  ck_assert_int_eq (bt_sequence_get_tick_by_pattern (sequence, 0, p1, 2), 6);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (p1);
  g_object_try_unref (p2);
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarge_length)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 8L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (sequence, "length", 8L);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarge_length_check_labels)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 4L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 0), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 1), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 2), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 3), NULL);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarge_length_labels)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_set_label (sequence, 0, "test");
  bt_sequence_set_label (sequence, 7, "test");

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 16L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 0), "test");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 1), NULL);
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 7), "test");
  ck_assert_str_eq_and_free (bt_sequence_get_label (sequence, 8), NULL);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_shrink_length)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  bt_sequence_add_track (sequence, machine, -1);
  g_object_set (sequence, "length", 16L, NULL);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 8L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (sequence, "length", 8L);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarge_track)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen-m";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  GST_INFO ("-- act --");
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (sequence, "tracks", 2L);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarge_track_vals)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- act --");
  g_object_set (sequence, "tracks", 2L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (sequence, "tracks", 2L);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0),
      machine);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 1), NULL);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_shrink_track)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  g_object_set (sequence, "length", 1L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- act --");
  bt_sequence_remove_track_by_ix (sequence, 0);
  ck_assert_gobject_gulong_eq (sequence, "tracks", 1L);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_machine (sequence, 0),
      machine);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarge_both_vals)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);
  bt_sequence_set_pattern (sequence, 7, 1, pattern);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_add_track (sequence, machine, -1);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      pattern);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 7, 1),
      pattern);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (pattern);
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST

/* we moved these updates to the app, to give the undo/redo framework a chance
 * to backup the data
 */
#ifdef __CHECK_DISABLED__
// test that removing patterns updates the sequence
START_TEST (test_bt_sequence_update)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine, -1);
  bt_sequence_set_pattern (sequence, 0, 0, pattern);

  GST_INFO ("-- act --");
  bt_machine_remove_pattern (machine, pattern);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      NULL);

  GST_INFO ("-- cleanup --");
  g_object_try_unref (pattern);
  g_object_try_unref (sequence);
  BT_TEST_END;
}
END_TEST
#endif

#define TICK_CT 8L

typedef struct
{
  gint values[TICK_CT];
} BtSequenceTicksTestData;

static void
on_bt_sequence_ticks_notify (GstObject * machine, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceTicksTestData *data = (BtSequenceTicksTestData *) user_data;
  static GstClockTime last = G_GUINT64_CONSTANT (0);
  GstClockTime cur = gst_util_get_timestamp ();
  GstClockTime diff = GST_CLOCK_DIFF (last, cur);
  last = cur;
  gint val;

  g_object_get (machine, "wave", &val, NULL);
  GST_INFO ("notify %d (%" GST_TIME_FORMAT ")", val, GST_TIME_ARGS (diff));
  if (val <= TICK_CT) {
    // we'd like to also count the ticks (we did before), see GST_BUG_733031
    data->values[val] = val;
  }
}

START_TEST (test_bt_sequence_ticks)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequenceTicksTestData data = { {0,} };
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  g_object_set (song_info, "bpm", 250L, "tpb", 16L, NULL);
  /* need a real element that handles tempo and calls gst_object_sync */
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *src = BT_MACHINE (bt_source_machine_new (&cparams,
      "simsyn", 0, NULL));


  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, src, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", TICK_CT, src);
  GstObject *element =
      GST_OBJECT (check_gobject_get_object_property (src, "machine"));
  g_object_set (sequence, "length", 8L, NULL);
  bt_sequence_add_track (sequence, src, -1);
  gint i;
  gchar str[] = { '\0', '\0' };

  for (i = 0; i < 8; i++) {
    str[0] = 48 + i;
    bt_pattern_set_global_event (pattern, i, 1, str);
    data.values[i] = -1;
  }
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  g_signal_connect (G_OBJECT (element), "notify::wave",
      G_CALLBACK (on_bt_sequence_ticks_notify), &data);

  GST_INFO ("-- act --");
  bt_song_play (song);
  // FIXME: this would be racy, the song could finish after this, before we run
  // the next wait
  //check_run_main_loop_until_playing_or_error (song);
  //GST_INFO ("playing ...");
  check_run_main_loop_until_eos_or_error (song);
  GST_INFO ("stopped");

  GST_INFO ("-- assert --");
  for (i = 0; i < 8; i++) {
    ck_assert_int_eq (data.values[i], i);
  }

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  g_object_unref (pattern);
  g_object_unref (sequence);
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

#undef TICK_CT

START_TEST (test_bt_sequence_default_loop)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);

  GST_INFO ("-- act --");
  g_object_set (sequence, "loop", TRUE, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0L);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 16L);

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarging_length_enlarges_loop)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 24L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0L);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 24L);

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_enlarging_length_keeps_loop)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);
  g_object_set (sequence, "loop-end", 8L, NULL);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 12L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0L);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 8L);

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_shortening_length_truncates_loop)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 8L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_glong_eq (sequence, "loop-start", 0L);
  ck_assert_gobject_glong_eq (sequence, "loop-end", 8L);

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_shortening_length_disables_loop)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 24L, NULL);
  g_object_set (sequence, "loop", TRUE, NULL);
  g_object_set (sequence, "loop-start", 16L, NULL);

  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 12L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gboolean_eq (sequence, "loop", FALSE);

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_insert_rows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "melo", 8L, gen);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 4, 0, pattern);

  GST_INFO ("-- act --");
  bt_sequence_insert_rows (sequence, 4, 0, 4);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 8, 0),
      pattern);
  fail_unless (bt_sequence_get_pattern (sequence, 4, 0) == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_insert_rows_shifts_out)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 8L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "melo", 8L, gen);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 4, 0, pattern);

  GST_INFO ("-- act --");
  bt_sequence_insert_rows (sequence, 4, 0, 4);

  GST_INFO ("-- assert --");
  fail_if (bt_sequence_is_pattern_used (sequence, (BtPattern *) pattern));
  fail_unless (bt_sequence_get_pattern (sequence, 4, 0) == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_insert_full_rows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));
  
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "melo", 8L, gen);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 4, 0, pattern);
  bt_sequence_set_pattern (sequence, 4, 1, pattern);

  GST_INFO ("-- act --");
  bt_sequence_insert_full_rows (sequence, 4, 4);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 8, 0),
      pattern);
  fail_unless (bt_sequence_get_pattern (sequence, 4, 0) == NULL);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 8, 1),
      pattern);
  fail_unless (bt_sequence_get_pattern (sequence, 4, 1) == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_delete_rows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));

  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "melo", 8L, gen);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 4, 0, pattern);

  GST_INFO ("-- act --");
  bt_sequence_delete_rows (sequence, 0, 0, 4);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      pattern);
  fail_unless (bt_sequence_get_pattern (sequence, 4, 0) == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_delete_full_rows)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));

  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "melo", 8L, gen);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 4, 0, pattern);
  bt_sequence_set_pattern (sequence, 4, 1, pattern);

  GST_INFO ("-- act --");
  bt_sequence_delete_full_rows (sequence, 0, 4);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 0),
      pattern);
  fail_unless (bt_sequence_get_pattern (sequence, 4, 0) == NULL);
  ck_assert_gobject_eq_and_unref (bt_sequence_get_pattern (sequence, 0, 1),
      pattern);
  fail_unless (bt_sequence_get_pattern (sequence, 4, 1) == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST


START_TEST (test_bt_sequence_duration)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));

  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  GstElement *sink_bin =
      GST_ELEMENT (check_gobject_get_object_property (sink, "machine"));
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  GST_INFO ("-- act --");
  gint64 duration;
  gboolean res =
      gst_element_query_duration (sink_bin, GST_FORMAT_TIME, &duration);

  GST_INFO ("-- assert --");
  ck_assert (res);
  ck_assert_int64_ne (duration, G_GINT64_CONSTANT (-1));
  ck_assert_uint64_eq (duration, 16L * tick_time);

  GST_INFO ("-- cleanup --");
  gst_object_unref (sink_bin);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_duration_play)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 16L, NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));

  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, gen, sink, NULL);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (gen, "machine");
  g_object_set (element, "wave", /* silence */ 4, NULL);
  GstElement *sink_bin =
      GST_ELEMENT (check_gobject_get_object_property (sink, "machine"));
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  GST_INFO ("-- act --");
  gint64 duration;
  gboolean res =
      gst_element_query_duration (sink_bin, GST_FORMAT_TIME, &duration);

  GST_INFO ("-- assert --");
  ck_assert (res);
  ck_assert_int64_ne (duration, G_GINT64_CONSTANT (-1));
  ck_assert_uint64_eq (duration, 16L * tick_time);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  gst_object_unref (element);
  gst_object_unref (sink_bin);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST


TCase *
bt_sequence_example_case (void)
{
  TCase *tc = tcase_create ("BtSequenceExamples");

  tcase_add_test (tc, test_bt_sequence_new);
  tcase_add_test (tc, test_bt_sequence_labels);
  tcase_add_test (tc, test_bt_sequence_append_track);
  tcase_add_test (tc, test_bt_sequence_insert_track);
  tcase_add_test (tc, test_bt_sequence_move_track_left);
  tcase_add_test (tc, test_bt_sequence_move_track_right);
  tcase_add_test (tc, test_bt_sequence_pattern);
  tcase_add_test (tc, test_bt_sequence_get_tick_by_pattern);
  tcase_add_test (tc, test_bt_sequence_enlarge_length);
  tcase_add_test (tc, test_bt_sequence_enlarge_length_check_labels);
  tcase_add_test (tc, test_bt_sequence_enlarge_length_labels);
  tcase_add_test (tc, test_bt_sequence_shrink_length);
  tcase_add_test (tc, test_bt_sequence_enlarge_track);
  tcase_add_test (tc, test_bt_sequence_enlarge_track_vals);
  tcase_add_test (tc, test_bt_sequence_shrink_track);
  tcase_add_test (tc, test_bt_sequence_enlarge_both_vals);
  //tcase_add_test(tc,test_bt_sequence_update);

  // See https://github.com/Buzztrax/buzztrax/issues/111
  //tcase_add_test (tc, test_bt_sequence_ticks);
  
  tcase_add_test (tc, test_bt_sequence_default_loop);
  tcase_add_test (tc, test_bt_sequence_enlarging_length_enlarges_loop);
  tcase_add_test (tc, test_bt_sequence_enlarging_length_keeps_loop);
  tcase_add_test (tc, test_bt_sequence_shortening_length_truncates_loop);
  tcase_add_test (tc, test_bt_sequence_shortening_length_disables_loop);
  tcase_add_test (tc, test_bt_sequence_insert_rows);
  tcase_add_test (tc, test_bt_sequence_insert_rows_shifts_out);
  tcase_add_test (tc, test_bt_sequence_insert_full_rows);
  tcase_add_test (tc, test_bt_sequence_delete_rows);
  tcase_add_test (tc, test_bt_sequence_delete_full_rows);
  tcase_add_test (tc, test_bt_sequence_duration);
  tcase_add_test (tc, test_bt_sequence_duration_play);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
