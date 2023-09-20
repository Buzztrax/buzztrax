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
  tcase_add_test (tc, test_bt_sequence_pattern1);
  tcase_add_test (tc, test_bt_sequence_pattern2);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
