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
test_bt_wire_properties (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *src = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  BtMachine *dst =
      BT_MACHINE (bt_processor_machine_new (song, "proc", "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, dst, NULL);

  /* act & assert */
  fail_unless (check_gobject_properties ((GObject *) wire), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* create a new wire with NULL for song object */
static void
test_bt_wire_new_null_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *src = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  BtMachine *dst =
      BT_MACHINE (bt_processor_machine_new (song, "proc", "volume", 0L, NULL));

  /* act */
  BtWire *wire = bt_wire_new (NULL, src, dst, NULL);

  /* assert */
  fail_unless (wire != NULL, NULL);

  /* cleanup */
  g_object_unref (wire);        // there is no setup to take ownership :/
  BT_TEST_END;
}

/* create a new wire with NULL for song object */
static void
test_bt_wire_new_null_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *src = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));

  /* act */
  BtWire *wire = bt_wire_new (NULL, src, NULL, NULL);

  /* assert */
  fail_unless (wire != NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}

/* create a wire with the same machine as source and dest */
static void
test_bt_wire_same_src_and_dst (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (song, "id", "volume", 0, NULL));

  /* act */
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, machine, machine, &err);
  fail_unless (wire != NULL, NULL);
  fail_unless (err != NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}

TCase *
bt_wire_test_case (void)
{
  TCase *tc = tcase_create ("BtWireTests");

  tcase_add_test (tc, test_bt_wire_properties);
  tcase_add_test (tc, test_bt_wire_new_null_song);
  tcase_add_test (tc, test_bt_wire_new_null_machine);
  tcase_add_test (tc, test_bt_wire_same_src_and_dst);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
