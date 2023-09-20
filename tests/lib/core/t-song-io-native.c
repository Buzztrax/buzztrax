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

static gchar *bad_songs[] = {
  "broken1.xml",                // wrong xml type
  "broken3.xml",                // not well formed (truncated)
  "broken5.xml",                // not well formed (not start tag)
};

// load files with errors
START_TEST (test_bt_song_io_native_bad_songs)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path (bad_songs[_i]), NULL);

  GST_INFO ("-- act & assert --");
  GError *err = NULL;
  ck_assert (!bt_song_io_load (song_io, song, &err));
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_error_free (err);
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

// load file into non empty song
START_TEST (test_bt_song_io_native_load_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple1.xml"),
      NULL);
  bt_song_io_load (song_io, song, NULL);
  ck_g_object_final_unref (song_io);
  song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple2.xml"),
      NULL);

  /* act & assert */
  ck_assert (bt_song_io_load (song_io, song, NULL));

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

// not_a_song.abc

TCase *
bt_song_io_native_test_case (void)
{
  TCase *tc = tcase_create ("BtSongIONativeTests");

  tcase_add_loop_test (tc, test_bt_song_io_native_bad_songs, 0,
      G_N_ELEMENTS (bad_songs));
  tcase_add_test (tc, test_bt_song_io_native_load_twice);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
