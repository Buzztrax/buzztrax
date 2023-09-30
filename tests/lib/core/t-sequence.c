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

//-- helpers

/**
 * make_short_sequence:
 *
 * Add to the given song one machine having one pattern and one track in the
 * sequence.
 *
 * Set the sequence to length 8.
 *
 * Sequence entries are set to the new pattern from time 1 to 8.
 *
 * @song: A fresh, empty BtSong.
 * @seq_out: (out): The created sequence.
 * @patt_out: (out) (optional): The created pattern, or NULL if not required.
 */
static void
make_short_sequence (BtSong *song, BtSequence** seq_out, BtCmdPattern** patt_out) {
  *seq_out =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  g_object_set (*seq_out, "length", 8L, NULL);
  
  BtMachineConstructorParams cparams;
  cparams.id = "genm";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  BtCmdPattern *pattern =
    BT_CMD_PATTERN (bt_pattern_new (song, "pattern-name", 8L, machine));
  
  bt_sequence_add_track (*seq_out, machine, -1);
 
  for (gint i = 0; i < 8; ++i) {
    bt_sequence_set_pattern (*seq_out, i, 0, pattern);
  }

  if (patt_out) {
    *patt_out = pattern;
  } else {
    g_object_unref (pattern);
  }
}

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

/* apply generic property tests to sequence */
START_TEST (test_bt_sequence_properties)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GObject *sequence = check_gobject_get_object_property (song, "sequence");

  /* act & assert */
  ck_assert (check_gobject_properties (sequence));

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

/* create a new sequence with NULL for song object */
START_TEST (test_bt_sequence_new_null_song)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSequence *sequence = bt_sequence_new (NULL);

  GST_INFO ("-- assert --");
  ck_assert (sequence != NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

/* try to add a NULL machine to the sequence */
START_TEST (test_bt_sequence_add_track1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  check_init_error_trapp ("", "BT_IS_MACHINE (machine)");

  GST_INFO ("-- act --");
  bt_sequence_add_track (sequence, NULL, -1);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

/* try to add a new machine for the sequence with NULL for the sequence parameter */
START_TEST (test_bt_sequence_add_track2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  check_init_error_trapp ("", "BT_IS_SEQUENCE (self)");

  GST_INFO ("-- act --");
  bt_sequence_add_track (NULL, machine, -1);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* try to remove a NULL machine from the sequence */
START_TEST (test_bt_sequence_rem_track1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  check_init_error_trapp ("", "BT_IS_MACHINE (machine)");

  GST_INFO ("-- act --");
  bt_sequence_remove_track_by_machine (sequence, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

/* try to remove a machine from the sequence that has never added */
START_TEST (test_bt_sequence_rem_track2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  GST_INFO ("-- act --");
  bt_sequence_remove_track_by_machine (sequence, machine);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

/* try to add a label to the sequence beyond the sequence length */
START_TEST (test_bt_sequence_length1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  g_object_set (sequence, "length", 4L, NULL);
  check_init_error_trapp ("bt_sequence_set_label", "time < self->priv->length");

  GST_INFO ("-- act --");
  bt_sequence_set_label (sequence, 5, "test");

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

/**
 * test_bt_sequence_length_reduce_no_truncate:
 *
 * A sequence has both a "nominal" length and a "pattern" length. The former
 * dictates when the song stops playing, and the latter represents the index at
 * which the final non-null piece of sequence data is found. This case asserts
 * that when the nominal song length is set to be less than the pattern length,
 * i.e. when the user uses "CTRL-E" to set the song end to be halfway through an
 * existing song, that sequence data will not be truncated.
 */
START_TEST (test_bt_sequence_length_reduce_no_truncate)
{
  BT_TEST_START;

  BtSequence *sequence;
  BtCmdPattern *pattern;
  make_short_sequence (song, &sequence, &pattern);
  
  /* Set the nominal length to be half the pattern length, and check that length
     values are correctly updated and that no sequence data has been lost. */
  GST_INFO ("-- act --");
  g_object_set (sequence, "length", 4L, NULL);
  g_object_set (sequence, "len-patterns", 8L, NULL);

  GST_INFO ("-- assert --");

  glong length = -1;
  g_object_get (sequence, "length", &length, NULL);
  ck_assert_int_eq (length, 4);

  for (gint i = 0; i < 8; ++i) {
    BtCmdPattern *pattern_test = bt_sequence_get_pattern (sequence, i, 0);
    ck_assert_ptr_eq (pattern_test, pattern);
    g_object_unref (pattern_test);
  }

  GST_INFO ("-- cleanup --");
  
  // 10 refs = pattern * 8 + machine + 1 ref for sequence->priv->pattern_usage (bt_sequence_use_pattern)
  ck_g_object_remaining_unref (pattern, 10);
  ck_g_object_remaining_unref (sequence, 1);    // 1 ref = song

  BT_TEST_END;
}
END_TEST

/*
 * test_bt_sequence_length_reduce_and_persist:
 *
 * Related to test_bt_sequence_length_reduce_no_truncate, this test checks that
 * given the setup mentioned in that test no truncation occurs once the song
 * has been persisted and reloaded.
 */
START_TEST (test_bt_sequence_length_reduce_and_persist)
{
  BT_TEST_START;

  BtSequence *sequence;
  make_short_sequence (song, &sequence, NULL);

  GST_INFO ("-- act --");
  
  g_object_set (sequence, "length", 4L, NULL);
  g_object_set (sequence, "len-patterns", 8L, NULL);
  
  xmlDocPtr const doc = xmlNewDoc (XML_CHAR_PTR ("1.0"));
  xmlNodePtr node = bt_persistence_save (BT_PERSISTENCE (song), NULL, NULL);
  xmlDocSetRootElement (doc, node);

  // Dump the persisted result just to help those debugging the test.
  xmlChar* xml;
  xmlDocDumpFormatMemory (doc, &xml, NULL, TRUE);
  GST_INFO ("Persisted song XML:\n %s", xml);
  xmlFree (xml);

  bt_persistence_load (G_OBJECT_TYPE (song), BT_PERSISTENCE (song), node, NULL);
  
  // A new sequence has been created by the load operation, release the old one.
  ck_g_object_remaining_unref (sequence, 1);    // 1 ref = song
  
  sequence =
    BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));

  GST_INFO ("-- assert --");

  glong length;
  g_object_get (sequence, "length", &length, NULL);
  ck_assert_int_eq (length, 4);

  glong len_patterns = -1;
  g_object_get (sequence, "len-patterns", &len_patterns, NULL);
  ck_assert_int_eq (len_patterns, 8);
  
  for (gint i = 0; i < 8; ++i) {
    BtCmdPattern *pattern_test = bt_sequence_get_pattern (sequence, i, 0);
    ck_assert_msg (pattern_test != NULL, "Loaded sequence must have a pattern present at time %d", i);
    g_object_unref (pattern_test);
  }
  
  GST_INFO ("-- cleanup --");
  
  xmlFreeDoc (doc);
  
  ck_g_object_remaining_unref (sequence, 1);    // 1 ref = song
  
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_pattern1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_set (sequence, "length", 4L, NULL);
  check_init_error_trapp ("bt_sequence_set_pattern",
      "track < self->priv->tracks");

  GST_INFO ("-- act --");
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pattern);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  ck_g_object_logged_unref (pattern);
  ck_g_object_logged_unref (sequence);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sequence_pattern2)
{
  BT_TEST_START;
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtMachineConstructorParams cparams;
  cparams.id = "genm";
  cparams.song = song;
  
  BtMachine *machine1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  cparams.id = "genp";
  BtMachine *machine2 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  BtCmdPattern *pattern1 =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L, machine1);
  g_object_set (sequence, "length", 4L, NULL);
  bt_sequence_add_track (sequence, machine1, -1);
  bt_sequence_add_track (sequence, machine2, -1);
  check_init_error_trapp ("bt_sequence_set_pattern",
      "adding a pattern to a track with different machine!");

  GST_INFO ("-- act --");
  bt_sequence_set_pattern (sequence, 0, 1, pattern1);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  ck_assert_gobject_ref_ct (machine1, 2);       // song, sequence
  ck_assert_gobject_ref_ct (machine1, 2);       // song, sequence
  ck_g_object_remaining_unref (pattern1, 1);    // machine1
  ck_g_object_remaining_unref (sequence, 1);    // song
  BT_TEST_END;
}
END_TEST

TCase *
bt_sequence_test_case (void)
{
  TCase *tc = tcase_create ("BtSequenceTests");

  tcase_add_test (tc, test_bt_sequence_properties);
  tcase_add_test (tc, test_bt_sequence_new_null_song);
  tcase_add_test (tc, test_bt_sequence_add_track1);
  tcase_add_test (tc, test_bt_sequence_add_track2);
  tcase_add_test (tc, test_bt_sequence_rem_track1);
  tcase_add_test (tc, test_bt_sequence_rem_track2);
  tcase_add_test (tc, test_bt_sequence_length1);
  tcase_add_test (tc, test_bt_sequence_length_reduce_no_truncate);
  tcase_add_test (tc, test_bt_sequence_length_reduce_and_persist);
  tcase_add_test (tc, test_bt_sequence_pattern1);
  tcase_add_test (tc, test_bt_sequence_pattern2);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
