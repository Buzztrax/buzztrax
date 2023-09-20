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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

static void
make_test_song (void)
{
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  cparams.id = "gen";
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
      "audiotestsrc", 0L, NULL));
  bt_wire_new (song, gen, sink, NULL);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (gen, "machine");
  g_object_set (element, "wave", /* silence */ 4, NULL);
  bt_machine_set_param_defaults (gen);

  gst_object_unref (element);
}


//-- tests

// test attribute handling in sink names
START_TEST (test_bt_sink_machine_settings_name_with_parameter)
{
  BT_TEST_START;

  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "osssink sync=false", NULL);
  mark_point ();

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtMachine *machine = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


/* Check if we handle a sink setting of "audioconvert ! osssink sync=false".
 * This string should be replaced by the sink machine to "ossink" and the
 * machine should be instantiable.
 */
START_TEST (test_bt_sink_machine_settings_name_is_launch_snippet)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "audioconvert ! osssink sync=false",
      NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


// test attribute handling in sink names
START_TEST (test_bt_sink_machine_settings_wrong_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "xvimagsink", NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


// test attribute handling in sink names
START_TEST (test_bt_sink_machine_settings_wrong_parameters)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "alsasink device=invalid:666", NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


// test attribute handling in sink names
START_TEST (test_bt_sink_machine_settings_inexistent_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "doesnotexistssink", NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


// test if the song play routine works with fakesink
START_TEST (test_bt_sink_machine_play_fakesink)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "fakesink", NULL);
  make_test_song ();

  /* act & assert */
  ck_assert (bt_song_play (song));
  g_usleep (G_USEC_PER_SEC / 10);
  bt_song_stop (song);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// the sink machine should fallback on the default device
START_TEST (test_bt_sink_machine_play_wrong_parameters)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "alsasink device=invalid:666", NULL);
  make_test_song ();

  /* act & assert */
  ck_assert (bt_song_play (song));
  g_usleep (G_USEC_PER_SEC / 10);
  bt_song_stop (song);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// the sink machine should fallback to another sink
START_TEST (test_bt_sink_machine_play_inexistent_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "doesnotexistssink", NULL);

  make_test_song ();

  /* act & assert */
  ck_assert (bt_song_play (song));
  g_usleep (G_USEC_PER_SEC / 10);
  bt_song_stop (song);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

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
  return tc;
}
