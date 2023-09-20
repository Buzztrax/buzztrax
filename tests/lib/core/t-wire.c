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

START_TEST (test_bt_wire_properties)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *src = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  cparams.id = "proc";
  BtMachine *dst =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, dst, NULL);

  /* act & assert */
  ck_assert (check_gobject_properties ((GObject *) wire));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* create a new wire with NULL for song object */
START_TEST (test_bt_wire_new_null_song)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *src = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  cparams.id = "proc";
  BtMachine *dst =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (NULL, src, dst, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (wire);        // there is no setup to take ownership :/
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

/* create a new wire with NULL for the dst machine */
START_TEST (test_bt_wire_new_null_machine)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *src = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, src, NULL, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

/* create a new wire with the wrong machine type for src */
START_TEST (test_bt_wire_new_wrong_src_machine_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *src = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtMachine *dst = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, src, dst, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

/* create a new wire with the wrong machine type for dst */
START_TEST (test_bt_wire_new_wrong_dst_machine_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  BtMachine *src = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  BtMachine *dst = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, src, dst, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

/* create a wire with the same machine as source and dest */
START_TEST (test_bt_wire_same_src_and_dst)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "id";
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, machine, machine, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wire_new_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "audiotestsrc";
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc",
          0L, NULL));
  cparams.id = "volume";
  BtMachine *proc = BT_MACHINE (bt_processor_machine_new (&cparams,
          "volume", 0L, NULL));
  bt_wire_new (song, src, proc, NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, src, proc, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

TCase *
bt_wire_test_case (void)
{
  TCase *tc = tcase_create ("BtWireTests");

  tcase_add_test (tc, test_bt_wire_properties);
  tcase_add_test (tc, test_bt_wire_new_null_song);
  tcase_add_test (tc, test_bt_wire_new_null_machine);
  tcase_add_test (tc, test_bt_wire_new_wrong_src_machine_type);
  tcase_add_test (tc, test_bt_wire_new_wrong_dst_machine_type);
  tcase_add_test (tc, test_bt_wire_same_src_and_dst);
  tcase_add_test (tc, test_bt_wire_new_twice);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
