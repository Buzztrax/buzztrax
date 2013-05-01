/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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

/* create a machine with not exising plugin name */
static void
test_bt_source_machine_wrong_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  GError *err = NULL;
  BtSourceMachine *machine =
      bt_source_machine_new (song, "id", "nonsense", 1, &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err != NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}

/* create a machine which is a sink machine and not a source machine */
static void
test_bt_source_machine_wrong_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /*act */
  GError *err = NULL;
  BtSourceMachine *machine =
      bt_source_machine_new (song, "id", "autoaudiosink", 1, &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err != NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}

TCase *
bt_source_machine_test_case (void)
{
  TCase *tc = tcase_create ("BtSourceMachineTests");

  tcase_add_test (tc, test_bt_source_machine_wrong_name);
  tcase_add_test (tc, test_bt_source_machine_wrong_type);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
