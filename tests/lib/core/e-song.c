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
  g_object_checked_unref (app);
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
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, gen);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (gen, "machine");

  g_object_set (sequence, "length", 64L, NULL);
  bt_sequence_add_track (sequence, gen, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);
  bt_sequence_set_pattern (sequence, 16, 0, (BtCmdPattern *) pattern);
  bt_sequence_set_pattern (sequence, 32, 0, (BtCmdPattern *) pattern);
  bt_sequence_set_pattern (sequence, 48, 0, (BtCmdPattern *) pattern);
  g_object_set (element, "wave", /* silence */ 4, NULL);

  gst_object_unref (element);
  g_object_unref (pattern);
  g_object_unref (sequence);
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
test_bt_song_obj1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtSong *song = bt_song_new (app);

  /* assert */
  fail_unless (song != NULL, NULL);
  ck_assert_gobject_object_eq (song, "master", NULL);

  /* cleanup */
  g_object_checked_unref (song);
  BT_TEST_END;
}

// test, if a newly created song contains empty setup, sequence, song-info and
// wavetable
static void
test_bt_song_obj2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);

  /* act */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtSongInfo *songinfo =
      (BtSongInfo *) check_gobject_get_object_property (song, "song-info");
  BtWavetable *wavetable =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");

  /* assert */
  ck_assert_gobject_object_eq (song, "app", app);
  fail_unless (setup != NULL, NULL);
  ck_assert_gobject_object_eq (setup, "song", song);
  fail_unless (sequence != NULL, NULL);
  ck_assert_gobject_object_eq (sequence, "song", song);
  fail_unless (songinfo != NULL, NULL);
  ck_assert_gobject_object_eq (songinfo, "song", song);
  fail_unless (wavetable != NULL, NULL);
  ck_assert_gobject_object_eq (wavetable, "song", song);

  /* cleanup */
  g_object_unref (setup);
  g_object_unref (sequence);
  g_object_unref (songinfo);
  g_object_unref (wavetable);
  g_object_checked_unref (song);
  BT_TEST_END;
}

static void
test_bt_song_master (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = make_new_song ();

  /* assert */
  ck_assert_gobject_object_ne (song, "master", NULL);

  /* cleanup */
  g_object_checked_unref (song);
  BT_TEST_END;
}

// test if the song play routine works without failure
static void
test_bt_song_play_single (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = make_new_song ();
  g_signal_connect (G_OBJECT (song), "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), NULL);

  /* act */
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  /* assert */
  fail_unless (play_signal_invoked, NULL);
  ck_assert_gobject_boolean_eq (song, "is-playing", TRUE);

  /* act */
  bt_song_stop (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);

  /* assert */
  ck_assert_gobject_boolean_eq (song, "is-playing", FALSE);

  /* cleanup */
  g_object_checked_unref (song);
  BT_TEST_END;
}

// play, wait a little, stop, play again
static void
test_bt_song_play_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = make_new_song ();
  g_signal_connect (G_OBJECT (song), "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), NULL);
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);
  bt_song_stop (song);
  play_signal_invoked = FALSE;
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);

  /* act && assert */
  fail_unless (bt_song_play (song), NULL);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);
  fail_unless (play_signal_invoked, NULL);

  /* cleanup */
  bt_song_stop (song);
  g_object_checked_unref (song);
  BT_TEST_END;
}

// load a new song, play, change audiosink to fakesink
static void
test_bt_song_play_and_change_sink (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSettings *settings = bt_settings_make ();
  BtSong *song = make_new_song ();

  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  /* act */
  g_object_set (settings, "audiosink", "fakesink", NULL);

  /* assert */
  mark_point ();

  /* cleanup */
  bt_song_stop (song);
  g_object_checked_unref (song);
  BT_TEST_END;
}

// change audiosink to NULL, load and play a song
static void
test_bt_song_play_fallback_sink (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSettings *settings = bt_settings_make ();
  g_object_set (settings, "audiosink", NULL, "system-audiosink", NULL, NULL);
  BtSong *song = make_new_song ();

  /* act */
  fail_unless (bt_song_play (song), NULL);

  /* cleanup */
  bt_song_stop (song);
  g_object_checked_unref (song);
  BT_TEST_END;
}

// test the idle looper
static void
test_bt_song_idle1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = make_new_song ();

  /* act */
  g_object_set (G_OBJECT (song), "is-idle", TRUE, NULL);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);
  g_object_set (G_OBJECT (song), "is-idle", FALSE, NULL);

  /* assert */
  mark_point ();

  /* cleanup */
  g_object_checked_unref (song);
  BT_TEST_END;
}

// test the idle looper and playing transition
static void
test_bt_song_idle2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = make_new_song ();

  /* act */
  g_object_set (G_OBJECT (song), "is-idle", TRUE, NULL);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);
  // start regular playback, this should stop the idle loop
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);
  GST_INFO ("playing");

  /* assert */
  ck_assert_gobject_boolean_eq (song, "is-playing", TRUE);
  ck_assert_gobject_boolean_eq (song, "is-idle", TRUE);

  /* act */
  bt_song_stop (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 10);
  GST_INFO ("stopped");

  /* assert */
  ck_assert_gobject_boolean_eq (song, "is-playing", FALSE);
  ck_assert_gobject_boolean_eq (song, "is-idle", TRUE);

  /* cleanup */
  g_object_set (G_OBJECT (song), "is-idle", FALSE, NULL);
  g_object_checked_unref (song);
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
  /* arrange */
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

  /* cleanup */
  gst_object_unref (element1);
  gst_object_unref (element2);
  g_object_unref (sequence);
  g_object_checked_unref (song);
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
  /* arrange */
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
  g_object_checked_unref (song);
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
  /* arrange */
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

  /* cleanup */
  gst_object_unref (element1);
  gst_object_unref (element2);
  g_object_unref (setup);
  g_object_unref (sequence);
  g_object_checked_unref (song);
  BT_TEST_END;
}

/*
* check if we can connect a src machine to a sink while playing
*/
static void
test_bt_song_dynamic_add_src (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");

  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    GST_DEBUG ("song plays");

    BtMachine *gen2 =
        BT_MACHINE (bt_source_machine_new (song, "gen2", "audiotestsrc", 0L,
            NULL));
    GstElement *element2 =
        (GstElement *) check_gobject_get_object_property (gen2, "machine");
    g_object_set (element2, "wave", /* silence */ 4, NULL);
    bt_wire_new (song, gen2, sink, NULL);
    gst_object_unref (element2);

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing of song failed");
  }

  /* cleanup */
  g_object_unref (sequence);
  g_object_checked_unref (song);
  BT_TEST_END;
}

/*
* check if we can disconnect a src machine from a sink while playing.
*/
static void
test_bt_song_dynamic_rem_src (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
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
    GST_DEBUG ("song plays");

    /* unlink machines */
    bt_setup_remove_wire (setup, wire2);
    GST_DEBUG ("wire removed");

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing of song failed");
  }

  /* cleanup */
  g_object_unref (setup);
  g_object_unref (sequence);
  g_object_checked_unref (song);
  BT_TEST_END;
}

/*
 * check if we can connect a processor machine to a src and sink while playing
 */
static void
test_bt_song_dynamic_add_proc (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");

  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    GST_DEBUG ("song plays");

    BtMachine *proc =
        BT_MACHINE (bt_processor_machine_new (song, "proc", "volume", 0, NULL));
    bt_wire_new (song, gen1, proc, NULL);
    bt_wire_new (song, proc, sink, NULL);

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing song failed");
  }

  /* cleanup */
  g_object_unref (sequence);
  g_object_checked_unref (song);
  BT_TEST_END;
}

/*
 * check if we can disconnect a processor machine to a src and sink while playing
 */
static void
test_bt_song_dynamic_rem_proc (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen1", "audiotestsrc", 0L,
          NULL));
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (song, "proc", "volume", 0, NULL));
  bt_wire_new (song, gen1, proc, NULL);
  BtWire *wire2 = bt_wire_new (song, gen1, proc, NULL);
  BtWire *wire3 = bt_wire_new (song, proc, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");

  g_object_set (sequence, "length", 16L, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    g_usleep (G_USEC_PER_SEC / 10);
    GST_DEBUG ("song plays");

    /* unlink machines */
    bt_setup_remove_wire (setup, wire2);
    GST_DEBUG ("wire 2 removed");

    bt_setup_remove_wire (setup, wire3);
    GST_DEBUG ("wire 3 removed");

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    fail ("playing song failed");
  }

  /* cleanup */
  g_object_unref (setup);
  g_object_unref (sequence);
  g_object_checked_unref (song);
  BT_TEST_END;
}

/* should we have variants, where we remove the machines instead of the wires? */
TCase *
bt_song_example_case (void)
{
  TCase *tc = tcase_create ("BtSongExamples");

  tcase_add_test (tc, test_bt_song_obj1);
  tcase_add_test (tc, test_bt_song_obj2);
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
  tcase_add_test (tc, test_bt_song_dynamic_add_src);
  tcase_add_test (tc, test_bt_song_dynamic_rem_src);
  tcase_add_test (tc, test_bt_song_dynamic_add_proc);
  tcase_add_test (tc, test_bt_song_dynamic_rem_proc);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
