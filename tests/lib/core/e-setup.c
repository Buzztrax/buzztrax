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

static void
test_bt_setup_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));

  /* act */
  GList *machines, *wires;
  g_object_get (G_OBJECT (setup), "machines", &machines, "wires", &wires, NULL);

  /* assert */
  fail_unless (machines == NULL, NULL);
  fail_unless (wires == NULL, NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_machine_add_id (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));

  /* act */
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_setup_get_machine_by_id (setup, "src"),
      source);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_machine_rem_id (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));

  /* act */
  bt_setup_remove_machine (setup, source);

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_setup_get_machine_by_id (setup, "src"),
      NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_machine_add_updates_list (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));

  /* act */
  GList *list = (GList *) check_gobject_get_ptr_property (setup, "machines");

  /* assert */
  fail_unless (list != NULL, NULL);
  ck_assert_int_eq (g_list_length (list), 1);
  fail_unless ((BtMachine *) list->data == source, NULL);

  /* cleanup */
  g_list_free (list);
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_wire_add_machine_id (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));

  /* act */
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_setup_get_wire_by_src_machine (setup,
          source), wire);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_wire_rem_machine_id (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  /* act */
  bt_setup_remove_wire (setup, wire);

  /* assert */
  ck_assert_gobject_eq_and_unref (bt_setup_get_wire_by_src_machine (setup,
          source), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_wire_add_src_list (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  /* act */
  GList *list = bt_setup_get_wires_by_src_machine (setup, source);

  /* assert */
  fail_unless (list != NULL, NULL);
  ck_assert_int_eq (g_list_length (list), 1);
  ck_assert_gobject_eq_and_unref (BT_WIRE (g_list_first (list)->data), wire);

  /* cleanup */
  g_list_free (list);
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_setup_wire_add_dst_list (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  /* act */
  GList *list = bt_setup_get_wires_by_dst_machine (setup, sink);

  /* assert */
  fail_unless (list != NULL, NULL);
  ck_assert_int_eq (g_list_length (list), 1);
  ck_assert_gobject_eq_and_unref (BT_WIRE (g_list_first (list)->data), wire);

  /* cleanup */
  g_list_free (list);
  g_object_unref (setup);
  BT_TEST_END;
}

/*
* In this example you can see, how we get a source machine back by its type.
*/
static void
test_bt_setup_machine_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0, NULL));

  /* act */
  BtMachine *machine =
      bt_setup_get_machine_by_type (setup, BT_TYPE_SOURCE_MACHINE);

  /* assert */
  fail_unless (machine == source, NULL);

  /* cleanup */
  g_object_unref (machine);
  g_object_unref (setup);
  BT_TEST_END;
}

/*
* In this test case we check the _unique_id function.
*/
static void
test_bt_setup_unique_machine_id1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  bt_source_machine_new (song, "src", "buzztrax-test-mono-source", 0, NULL);

  /* act */
  gchar *id = bt_setup_get_unique_machine_id (setup, "src");

  /* assert */
  fail_unless (id != NULL, NULL);
  ck_assert_gobject_eq_and_unref (bt_setup_get_machine_by_id (setup, id), NULL);
  ck_assert_str_ne (id, "src");

  /* cleanup */
  g_free (id);
  g_object_unref (setup);
  BT_TEST_END;
}

/*
* check if we can connect a src machine to a sink while playing
*/
static void
test_bt_setup_dynamic_add_src (BT_TEST_ARGS)
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
test_bt_setup_dynamic_rem_src (BT_TEST_ARGS)
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
test_bt_setup_dynamic_add_proc (BT_TEST_ARGS)
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
test_bt_setup_dynamic_rem_proc (BT_TEST_ARGS)
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

TCase *
bt_setup_example_case (void)
{
  TCase *tc = tcase_create ("BtSetupExamples");

  tcase_add_test (tc, test_bt_setup_new);
  tcase_add_test (tc, test_bt_setup_machine_add_id);
  tcase_add_test (tc, test_bt_setup_machine_rem_id);
  tcase_add_test (tc, test_bt_setup_machine_add_updates_list);
  tcase_add_test (tc, test_bt_setup_wire_add_machine_id);
  tcase_add_test (tc, test_bt_setup_wire_rem_machine_id);
  tcase_add_test (tc, test_bt_setup_wire_add_src_list);
  tcase_add_test (tc, test_bt_setup_wire_add_dst_list);
  tcase_add_test (tc, test_bt_setup_machine_type);
  tcase_add_test (tc, test_bt_setup_unique_machine_id1);
  tcase_add_test (tc, test_bt_setup_dynamic_add_src);
  tcase_add_test (tc, test_bt_setup_dynamic_rem_src);
  tcase_add_test (tc, test_bt_setup_dynamic_add_proc);
  tcase_add_test (tc, test_bt_setup_dynamic_rem_proc);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
