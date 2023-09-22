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
#if 0
static gboolean play_signal_invoked = FALSE;
#endif

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  BtSettings *settings;

  app = bt_test_application_new ();
  // no beeps please
  settings = bt_settings_make ();
  g_object_set (settings, "audiosink", "fakesink", NULL);
  g_object_unref (settings);
}

static void
test_teardown (void)
{
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_song_properties)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GObject *song = (GObject *) bt_song_new (app);

  /* act & assert */
  fail_unless (check_gobject_properties (song));

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song);
  BT_TEST_END;
}
END_TEST

// test if the default constructor handles NULL
START_TEST (test_bt_song_new_null_app)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_song_", "BT_IS_APPLICATION (self->priv->app)");

  GST_INFO ("-- act --");
  BtSong *song = bt_song_new (NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());
  ck_assert (song != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

// we need the apps bin to actually play
START_TEST (test_bt_song_play_empty)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  check_init_error_trapp ("bt_song_play",
      "GST_IS_BIN (self->priv->master_bin)");

  GST_INFO ("-- act --");
  bt_song_play (song);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}
END_TEST

// song is null
START_TEST (test_bt_song_play_null)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  check_init_error_trapp ("bt_song_play", "BT_IS_SONG (self)");

  GST_INFO ("-- act --");
  bt_song_play (NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}
END_TEST

// load a new song while the first plays
//
// dbeswick: this test has been observed to succeed when run individually,
// but segfault during a suite run.
START_TEST (test_bt_song_play_and_load_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song = bt_song_new (app);
  BtSongIO *loader =
      bt_song_io_from_file (check_get_test_song_path ("test-simple1.xml"),
      NULL);
  bt_song_io_load (loader, song, NULL);
  ck_g_object_final_unref (loader);
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  GST_INFO ("-- act --");
  loader =
      bt_song_io_from_file (check_get_test_song_path ("test-simple2.xml"),
      NULL);

  GST_INFO ("-- assert --");
  ck_assert (bt_song_io_load (loader, song, NULL));

  GST_INFO ("-- cleanup --");
  bt_song_stop (song);
  ck_g_object_final_unref (loader);
  ck_g_object_final_unref (song);
  BT_TEST_END;
}
END_TEST

TCase *
bt_song_test_case (void)
{
  TCase *tc = tcase_create ("BtSongTests");

  tcase_add_test (tc, test_bt_song_properties);
  tcase_add_test (tc, test_bt_song_new_null_app);
  tcase_add_test (tc, test_bt_song_play_empty);
  tcase_add_test (tc, test_bt_song_play_null);
  tcase_add_test (tc, test_bt_song_play_and_load_new);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
