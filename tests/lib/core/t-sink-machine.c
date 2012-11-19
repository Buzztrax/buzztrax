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
static BtSettings *settings;

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
  settings = bt_settings_make ();
}

static void
test_teardown (void)
{
  g_object_unref (settings);
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

static void
make_test_song (void)
{
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  bt_wire_new (song, gen, sink, NULL);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (gen, "machine");
  g_object_set (element, "wave", /* silence */ 4, NULL);

  gst_object_unref (element);
}


//-- tests

// test attribute handling in sink names
static void
test_bt_sink_machine_settings_name_with_parameter (BT_TEST_ARGS)
{
  BT_TEST_START;

  /* arrange */
  g_object_set (settings, "audiosink", "osssink sync=false", NULL);
  mark_point ();

  /* act */
  GError *err = NULL;
  BtSinkMachine *machine = bt_sink_machine_new (song, "master", &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}


/* Check if we handle a sink setting of "audioconvert ! osssink sync=false".
 * This string should be replaced by the sink machine to "ossink" and the
 * machine should be instantiable.
 */
static void
test_bt_sink_machine_settings_name_is_launch_snippet (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "audioconvert ! osssink sync=false",
      NULL);

  /* act */
  GError *err = NULL;
  BtSinkMachine *machine = bt_sink_machine_new (song, "master", &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}


// test attribute handling in sink names
static void
test_bt_sink_machine_settings_wrong_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "xvimagsink", NULL);

  /* act */
  GError *err = NULL;
  BtSinkMachine *machine = bt_sink_machine_new (song, "master", &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}


// test attribute handling in sink names
static void
test_bt_sink_machine_settings_wrong_parameters (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "alsasink device=invalid:666", NULL);

  /* act */
  GError *err = NULL;
  BtSinkMachine *machine = bt_sink_machine_new (song, "master", &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}


// test attribute handling in sink names
static void
test_bt_sink_machine_settings_inexistent_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "doesnotexistssink", NULL);

  /* act */
  GError *err = NULL;
  BtSinkMachine *machine = bt_sink_machine_new (song, "master", &err);

  /* assert */
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}


// test if the song play routine works with fakesink
static void
test_bt_sink_machine_play_fakesink (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "fakesink", NULL);
  make_test_song ();

  /* act & assert */
  fail_unless (bt_song_play (song), NULL);
  g_usleep (G_USEC_PER_SEC / 10);
  bt_song_stop (song);

  /* cleanup */
  BT_TEST_END;
}

// test if the song play routine handles sink with wrong parameters
static void
test_bt_sink_machine_play_wrong_parameters (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "alsasink device=invalid:666", NULL);
  make_test_song ();

  /* act & assert */
  fail_unless (bt_song_play (song), NULL);
  g_usleep (G_USEC_PER_SEC / 10);
  bt_song_stop (song);

  /* cleanup */
  BT_TEST_END;
}

// test if the song play routine handles sink with wrong parameters
static void
test_bt_sink_machine_play_inexistent_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  g_object_set (settings, "audiosink", "doesnotexistssink", NULL);

  make_test_song ();

  /* act & assert */
  fail_unless (bt_song_play (song), NULL);
  g_usleep (G_USEC_PER_SEC / 10);
  bt_song_stop (song);

  /* cleanup */
  BT_TEST_END;
}

TCase *
bt_sink_machine_test_case (void)
{
  TCase *tc = tcase_create ("BtSinkMachineTests");

  tcase_add_test (tc, test_bt_sink_machine_settings_name_with_parameter);
  tcase_add_test (tc, test_bt_sink_machine_settings_name_is_launch_snippet);
  tcase_add_test (tc, test_bt_sink_machine_settings_wrong_type);
  tcase_add_test (tc, test_bt_sink_machine_settings_wrong_parameters);
  tcase_add_test (tc, test_bt_sink_machine_settings_inexistent_type);
  tcase_add_test (tc, test_bt_sink_machine_play_fakesink);
  tcase_add_test (tc, test_bt_sink_machine_play_wrong_parameters);
  tcase_add_test (tc, test_bt_sink_machine_play_inexistent_type);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
