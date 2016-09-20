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
static gboolean play_signal_invoked = FALSE;

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
  play_signal_invoked = FALSE;
}

static void
test_teardown (void)
{
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

static BtSong *
make_new_song (void)
{
  BtSong *song = bt_song_new (app);
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, gen);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (gen, "machine");

  /* duration: 0:00:00.480000000 */
  g_object_set (song_info, "bpm", 250L, "tpb", 16L, NULL);
  g_object_set (sequence, "length", 32L, NULL);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_sequence_set_pattern (sequence, 16, 0, (BtCmdPattern *) pattern);
  g_object_set (element, "wave", /* silence */ 4, NULL);
  bt_machine_set_param_defaults (gen);

  gst_object_unref (element);
  g_object_unref (pattern);
  g_object_unref (sequence);
  g_object_unref (song_info);
  GST_INFO ("  song created");
  return song;
}

// helper method to test the play signal
static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  play_signal_invoked = TRUE;
  GST_INFO ("got signal");
}

//-- tests

// test if the default constructor works as expected
static void
test_bt_song_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtSong *song = bt_song_new (app);

  GST_INFO ("-- assert --");
  fail_unless (song != NULL, NULL);
  ck_assert_gobject_object_eq (song, "master", NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// test, if a newly created song contains setup, sequence, song-info and
// wavetable and they point back to the song
static void
test_bt_song_members (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);

  GST_INFO ("-- act --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtSongInfo *songinfo =
      (BtSongInfo *) check_gobject_get_object_property (song, "song-info");
  BtWavetable *wavetable =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");

  GST_INFO ("-- assert --");
  ck_assert_gobject_object_eq (song, "app", app);
  fail_unless (setup != NULL, NULL);
  ck_assert_gobject_object_eq (setup, "song", song);
  fail_unless (sequence != NULL, NULL);
  ck_assert_gobject_object_eq (sequence, "song", song);
  fail_unless (songinfo != NULL, NULL);
  ck_assert_gobject_object_eq (songinfo, "song", song);
  fail_unless (wavetable != NULL, NULL);
  ck_assert_gobject_object_eq (wavetable, "song", song);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  g_object_unref (sequence);
  g_object_unref (songinfo);
  g_object_unref (wavetable);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_master (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- assert --");
  ck_assert_gobject_object_ne (song, "master", NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// test if the song play routine works without failure
static void
test_bt_song_play_single (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();
  g_signal_connect (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), NULL);

  GST_INFO ("-- act --");
  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);

  GST_INFO ("-- assert --");
  fail_unless (play_signal_invoked, NULL);
  ck_assert_gobject_gboolean_eq (song, "is-playing", TRUE);

  GST_INFO ("-- act --");
  bt_song_stop (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gboolean_eq (song, "is-playing", FALSE);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// play, wait a little, stop, play again
static void
test_bt_song_play_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();
  g_signal_connect (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), NULL);

  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);
  bt_song_stop (song);
  play_signal_invoked = FALSE;
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);

  /* act && assert */
  fail_unless (bt_song_play (song), NULL);
  check_run_main_loop_until_playing_or_error (song);
  fail_unless (play_signal_invoked, NULL);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// load a new song, play, change audiosink to fakesink
static void
test_bt_song_play_and_change_sink (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSettings *settings = bt_settings_make ();
  BtSong *song = make_new_song ();

  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  GST_INFO ("-- act --");
  g_object_set (settings, "audiosink", "fakesink", NULL);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// change audiosink to NULL, load and play a song
static void
test_bt_song_play_fallback_sink (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSettings *settings = bt_settings_make ();
  g_object_set (settings, "audiosink", NULL,
      /* TODO(ensonic): this is not writable!
       * - subclass settings and override the prop?
       * - test app will probably also need to override to be able to return this
       "system-audiosink", NULL,
       */
      NULL);
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  fail_unless (bt_song_play (song), NULL);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// test the idle looper
static void
test_bt_song_idle1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  g_object_set (G_OBJECT (song), "is-idle", TRUE, NULL);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);
  g_object_set (G_OBJECT (song), "is-idle", FALSE, NULL);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

// test the idle looper and playing transition
static void
test_bt_song_idle2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  g_object_set (G_OBJECT (song), "is-idle", TRUE, NULL);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);
  // start regular playback, this should stop the idle loop
  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);
  GST_INFO ("playing");

  GST_INFO ("-- assert --");
  ck_assert_gobject_gboolean_eq (song, "is-playing", TRUE);
  ck_assert_gobject_gboolean_eq (song, "is-idle", TRUE);

  GST_INFO ("-- act --");
  bt_song_stop (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);
  GST_INFO ("stopped");

  GST_INFO ("-- assert --");
  ck_assert_gobject_gboolean_eq (song, "is-playing", FALSE);
  ck_assert_gobject_gboolean_eq (song, "is-idle", TRUE);

  GST_INFO ("-- cleanup --");
  g_object_set (G_OBJECT (song), "is-idle", FALSE, NULL);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

/*
 * check if we can connect two sine machines to one sink. Also try to play after
 * connecting the machines.
 */
static void
test_bt_song_play_two_sources (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  BtMachine *gen2 =
      BT_MACHINE (bt_source_machine_new (song, "gen2", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  bt_wire_new (song, gen2, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");
  GstElement *element2 =
      (GstElement *) check_gobject_get_object_property (gen2, "machine");

  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_add_track (sequence, gen2, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  g_object_set (element2, "wave", /* silence */ 4, NULL);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing song failed");
  }

  GST_INFO ("-- cleanup --");
  gst_object_unref (element1);
  gst_object_unref (element2);
  g_object_unref (sequence);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

/*
 * check if we can connect two sine machines to one effect and this to the
 * sink. Also try to start play after connecting the machines.
 */
static void
test_bt_song_play_two_sources_and_one_fx (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  BtMachine *gen2 =
      BT_MACHINE (bt_source_machine_new (song, "gen2", "audiotestsrc", 0L,
          NULL));
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (song, "proc", "volume", 0, NULL));
  bt_wire_new (song, gen1, proc, NULL);
  bt_wire_new (song, gen2, proc, NULL);
  bt_wire_new (song, proc, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");
  GstElement *element2 =
      (GstElement *) check_gobject_get_object_property (gen2, "machine");

  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_add_track (sequence, gen2, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  g_object_set (element2, "wave", /* silence */ 4, NULL);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing song failed");
  }

  gst_object_unref (element1);
  gst_object_unref (element2);
  g_object_unref (sequence);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

/*
 * check if we can connect two sine machines to one sink, then play() and
 * stop(). After stopping remove one machine and play again.
 */
static void
test_bt_song_play_change_replay (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  BtMachine *gen2 =
      BT_MACHINE (bt_source_machine_new (song, "gen2", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  BtWire *wire2 = bt_wire_new (song, gen2, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");
  GstElement *element2 =
      (GstElement *) check_gobject_get_object_property (gen2, "machine");

  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_add_track (sequence, gen2, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  g_object_set (element2, "wave", /* silence */ 4, NULL);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing of network song failed");
  }

  /* remove one machine */
  bt_setup_remove_wire (setup, wire2);
  fail_unless (bt_sequence_remove_track_by_machine (sequence, gen2), NULL);
  bt_setup_remove_machine (setup, gen2);
  mark_point ();

  /* play the song again */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing song failed again");
  }

  GST_INFO ("-- cleanup --");
  gst_object_unref (element1);
  gst_object_unref (element2);
  g_object_unref (setup);
  g_object_unref (sequence);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_play_pos (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);
  g_usleep (G_USEC_PER_SEC / 5);
  bt_song_update_playback_position (song);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_gt (song, "play-pos", 0L);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_play_pos_on_eos (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  bt_song_play (song);
  check_run_main_loop_until_eos_or_error (song);
  bt_song_update_playback_position (song);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_ge (song, "play-pos", 32L);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_play_pos_after_initial_seek (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  g_object_set (song, "play-pos", 16L, NULL);
  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);
  bt_song_update_playback_position (song);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_ge (song, "play-pos", 16L);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_play_again_should_restart (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  g_object_set (song, "play-pos", 48L, NULL);
  bt_song_play (song);
  check_run_main_loop_until_eos_or_error (song);
  bt_song_play (song);
  check_run_main_loop_until_playing_or_error (song);
  bt_song_update_playback_position (song);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_lt (song, "play-pos", 63L);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_play_loop (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  g_object_set (sequence, "loop", TRUE, NULL);

  GST_INFO ("-- act --");
  bt_song_play (song);

  gboolean res =
      check_run_main_loop_until_msg_or_error (song, "message::segment-done");

  GST_INFO ("-- assert --");
  fail_unless (res);

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  g_object_unref (sequence);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_persistence (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = make_new_song ();

  GST_INFO ("-- act --");
  xmlNodePtr node = bt_persistence_save (BT_PERSISTENCE (song), NULL);

  GST_INFO ("-- assert --");
  fail_unless (node != NULL, NULL);
  ck_assert_str_eq ((gchar *) node->name, "buzztrax");
  fail_unless (node->children != NULL, NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_tempo_update_set_context (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtTestMonoSource *e =
      (BtTestMonoSource *) check_gobject_get_object_property (gen, "machine");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  bt_wire_new (song, gen, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (song_info, "bpm", 250L, "tpb", 16L, NULL);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (e->bpm, 250);
  ck_assert_int_eq (e->tpb, 16);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}



/* should we have variants, where we remove the machines instead of the wires? */
TCase *
bt_song_example_case (void)
{
  TCase *tc = tcase_create ("BtSongExamples");

  tcase_add_test (tc, test_bt_song_new);
  tcase_add_test (tc, test_bt_song_members);
  tcase_add_test (tc, test_bt_song_master);
  tcase_add_test (tc, test_bt_song_play_single);
  tcase_add_test (tc, test_bt_song_play_twice);
  tcase_add_test (tc, test_bt_song_play_and_change_sink);
  tcase_add_test (tc, test_bt_song_play_fallback_sink);
  tcase_add_test (tc, test_bt_song_idle1);
  tcase_add_test (tc, test_bt_song_idle2);
  tcase_add_test (tc, test_bt_song_play_two_sources);
  tcase_add_test (tc, test_bt_song_play_two_sources_and_one_fx);
  tcase_add_test (tc, test_bt_song_play_change_replay);
  tcase_add_test (tc, test_bt_song_play_pos);
  tcase_add_test (tc, test_bt_song_play_pos_on_eos);
  tcase_add_test (tc, test_bt_song_play_pos_after_initial_seek);
  tcase_add_test (tc, test_bt_song_play_again_should_restart);
  tcase_add_test (tc, test_bt_song_play_loop);
  tcase_add_test (tc, test_bt_song_persistence);
  tcase_add_test (tc, test_bt_song_tempo_update_set_context);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
