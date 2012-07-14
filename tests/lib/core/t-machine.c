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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-core.h"

//-- globals

static BtApplication *app;
static BtSong *song;

//-- fixtures

static void
case_setup (void)
{
  GST_INFO
      ("================================================================================");
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

BT_START_TEST (test_bt_machine_add_pattern)
{
  /* arrange */
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  check_init_error_trapp ("", "BT_IS_CMD_PATTERN (pattern)");

  /* act */
  bt_machine_add_pattern (gen1, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_try_unref (gen1);
}

BT_END_TEST
// FIXME(ensonic): is this really testing something?
BT_START_TEST (test_bt_machine_names)
{
  /* arrange */
  BtMachine *gen1 =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  BtMachine *gen2 =
      BT_MACHINE (bt_source_machine_new (song, "gen2",
          "buzztard-test-mono-source", 0L, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));

  /* act */
  g_object_set (gen1, "id", "beep1", NULL);
  g_object_set (gen2, "id", "beep2", NULL);
  BtWire *wire2 = bt_wire_new (song, gen1, sink, NULL);
  BtWire *wire1 = bt_wire_new (song, gen2, sink, NULL);

  /* assert */
  mark_point ();

  /* cleanup */
  g_object_unref (wire1);
  g_object_unref (wire2);
  g_object_unref (sink);
  g_object_unref (gen2);
  g_object_unref (gen1);
}

BT_END_TEST TCase * bt_machine_test_case (void)
{
  TCase *tc = tcase_create ("BtMachineTests");

  tcase_add_test (tc, test_bt_machine_add_pattern);
  tcase_add_test (tc, test_bt_machine_names);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
