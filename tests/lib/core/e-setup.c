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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_setup_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));

  GST_INFO ("-- act --");
  GList *machines, *wires;
  g_object_get (G_OBJECT (setup), "machines", &machines, "wires", &wires, NULL);

  GST_INFO ("-- assert --");
  ck_assert (machines == NULL);
  ck_assert (wires == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_machine_add_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));

  GST_INFO ("-- act --");
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_setup_get_machine_by_id (setup, "src"),
      source);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_machine_rem_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  GST_INFO ("-- act --");
  bt_setup_remove_machine (setup, source);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_setup_get_machine_by_id (setup, "src"),
      NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_machine_add_updates_list)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  GST_INFO ("-- act --");
  GList *list = (GList *) check_gobject_get_ptr_property (setup, "machines");

  GST_INFO ("-- assert --");
  ck_assert (list != NULL);
  ck_assert_int_eq (g_list_length (list), 1);
  ck_assert ((BtMachine *) list->data == source);

  GST_INFO ("-- cleanup --");
  g_list_free (list);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_wire_add_machine_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  GST_INFO ("-- act --");
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_setup_get_wire_by_src_machine (setup,
          source), wire);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_wire_rem_machine_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  GST_INFO ("-- act --");
  bt_setup_remove_wire (setup, wire);

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_setup_get_wire_by_src_machine (setup,
          source), NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_wire_add_src_list)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  GST_INFO ("-- act --");
  GList *list = bt_setup_get_wires_by_src_machine (setup, source);

  GST_INFO ("-- assert --");
  ck_assert (list != NULL);
  ck_assert_int_eq (g_list_length (list), 1);
  ck_assert_gobject_eq_and_unref (BT_WIRE (g_list_first (list)->data), wire);

  GST_INFO ("-- cleanup --");
  g_list_free (list);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_setup_wire_add_dst_list)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  GST_INFO ("-- act --");
  GList *list = bt_setup_get_wires_by_dst_machine (setup, sink);

  GST_INFO ("-- assert --");
  ck_assert (list != NULL);
  ck_assert_int_eq (g_list_length (list), 1);
  ck_assert_gobject_eq_and_unref (BT_WIRE (g_list_first (list)->data), wire);

  GST_INFO ("-- cleanup --");
  g_list_free (list);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/*
* In this example you can see, how we get a source machine back by its type.
*/
START_TEST (test_bt_setup_machine_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  GST_INFO ("-- act --");
  BtMachine *machine =
      bt_setup_get_machine_by_type (setup, BT_TYPE_SOURCE_MACHINE);

  GST_INFO ("-- assert --");
  ck_assert (machine == source);

  GST_INFO ("-- cleanup --");
  g_object_unref (machine);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/*
* In this test case we check the _unique_id function.
*/
START_TEST (test_bt_setup_unique_machine_id1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup = BT_SETUP (check_gobject_get_object_property (song, "setup"));
  BtMachineConstructorParams cparams;
  cparams.id = "src";
  cparams.song = song;
  
  bt_source_machine_new (&cparams, "buzztrax-test-mono-source", 0, NULL);

  GST_INFO ("-- act --");
  gchar *id = bt_setup_get_unique_machine_id (setup, "src");

  GST_INFO ("-- assert --");
  ck_assert (id != NULL);
  ck_assert_gobject_eq_and_unref (bt_setup_get_machine_by_id (setup, id), NULL);
  ck_assert_str_ne (id, "src");

  GST_INFO ("-- cleanup --");
  g_free (id);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/*
* check if we can connect a src machine to a sink while playing
*/
START_TEST (test_bt_setup_dynamic_add_src)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  cparams.id = "gen1";
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");

  g_object_set (sequence, "length", 64L, "loop", TRUE, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  bt_machine_set_param_defaults (gen1);

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    check_run_main_loop_until_playing_or_error (song);
    GST_DEBUG ("song plays");

    cparams.id = "gen2";
  
    BtMachine *gen2 =
        BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
            NULL));
    GstElement *element2 =
        (GstElement *) check_gobject_get_object_property (gen2, "machine");
    g_object_set (element2, "wave", /* silence */ 4, NULL);
    bt_machine_set_param_defaults (gen2);
    bt_wire_new (song, gen2, sink, NULL);
    gst_object_unref (element2);

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    ck_assert_msg (FALSE, "playing of song failed");
  }

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

/*
* check if we can disconnect a src machine from a sink while playing.
*/
START_TEST (test_bt_setup_dynamic_rem_src)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  cparams.id = "gen1";
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  
  cparams.id = "gen2";
  BtMachine *gen2 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  BtWire *wire2 = bt_wire_new (song, gen2, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");
  GstElement *element2 =
      (GstElement *) check_gobject_get_object_property (gen2, "machine");

  g_object_set (sequence, "length", 64L, "loop", TRUE, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_add_track (sequence, gen2, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  g_object_set (element2, "wave", /* silence */ 4, NULL);
  bt_machine_set_param_defaults (gen1);
  bt_machine_set_param_defaults (gen2);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    check_run_main_loop_until_playing_or_error (song);
    GST_DEBUG ("song plays");

    /* unlink machines */
    bt_setup_remove_wire (setup, wire2);
    GST_DEBUG ("wire removed");

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    ck_assert_msg (FALSE, "playing of song failed");
  }

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

/*
 * check if we can connect a processor machine to a src and sink while playing
 */
START_TEST (test_bt_setup_dynamic_add_proc)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  cparams.id = "gen1";
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen1, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");

  g_object_set (sequence, "length", 64L, "loop", TRUE, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  bt_machine_set_param_defaults (gen1);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    check_run_main_loop_until_playing_or_error (song);
    GST_DEBUG ("song plays");

  
    cparams.id = "proc";
  
    BtMachine *proc =
        BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));
    bt_wire_new (song, gen1, proc, NULL);
    bt_wire_new (song, proc, sink, NULL);

    g_usleep (G_USEC_PER_SEC / 10);

    /* stop the song */
    bt_song_stop (song);
  } else {
    ck_assert_msg (FALSE, "playing song failed");
  }

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence);;
  BT_TEST_END;
}
END_TEST

/*
 * check if we can disconnect a processor machine from a src and sink while
 * playing
 */
START_TEST (test_bt_setup_dynamic_rem_proc)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  cparams.id = "gen1";
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  
  cparams.id = "proc";
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));
  bt_wire_new (song, gen1, sink, NULL);
  BtWire *wire2 = bt_wire_new (song, gen1, proc, NULL);
  BtWire *wire3 = bt_wire_new (song, proc, sink, NULL);
  GstElement *element1 =
      (GstElement *) check_gobject_get_object_property (gen1, "machine");

  g_object_set (sequence, "length", 64L, "loop", TRUE, NULL);
  bt_sequence_add_track (sequence, gen1, -1);
  g_object_set (element1, "wave", /* silence */ 4, NULL);
  bt_machine_set_param_defaults (gen1);
  mark_point ();

  /* play the song */
  if (bt_song_play (song)) {
    mark_point ();
    check_run_main_loop_until_playing_or_error (song);
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
    ck_assert_msg (FALSE, "playing song failed");
  }

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  g_object_unref (sequence);
  BT_TEST_END;
}
END_TEST

/*
// We can't implement these below, as songs without atleast 1 src..sink chain
// wont play. Also when we disconnect the last, the song stops.
// Maybe we should consider to have a permanent silent-src -> master:
// - We can't do it in sink-bin as this has a static sink-pad
// - We could do it in bt_sink_machine_constructed. After the internal elements
//   including adder are activated, we can request a pad on adder and link
//   a "audiotestsrc wave=silence "to it.

// initially only master
test_bt_setup_dynamic_add_only_src
test_bt_setup_dynamic_rem_only_src
// initialy only master
test_bt_setup_dynamic_add_src_and_proc
test_bt_setup_dynamic_rem_src_and_proc
*/

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
  return tc;
}
