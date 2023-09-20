/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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

/* create a machine with not exising plugin name */
START_TEST (test_bt_processor_machine_wrong_name)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtProcessorMachine *machine =
      bt_processor_machine_new (&cparams, "nonsense", 1, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

/* create a machine which is a sink machine and not a processor machine */
START_TEST (test_bt_processor_machine_wrong_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  /*act */
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
      "autoaudiosink", 1, &err));

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  BT_TEST_END;
}
END_TEST

TCase *
bt_processor_machine_test_case (void)
{
  TCase *tc = tcase_create ("BtProcessorMachineTests");

  tcase_add_test (tc, test_bt_processor_machine_wrong_name);
  tcase_add_test (tc, test_bt_processor_machine_wrong_type);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
