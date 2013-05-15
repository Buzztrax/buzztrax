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
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}

//-- tests

// load file with errors
static void
test_bt_song_io_native_broken_file (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("broken1.xml"));

  /* act & assert */
  fail_if (bt_song_io_load (song_io, song), NULL);

  /* cleanup */
  g_object_checked_unref (song_io);
  BT_TEST_END;
}

// load file into non empty song
static void
test_bt_song_io_native_load_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple1.xml"));
  bt_song_io_load (song_io, song);
  g_object_checked_unref (song_io);
  song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple2.xml"));

  /* act & assert */
  fail_unless (bt_song_io_load (song_io, song), NULL);

  /* cleanup */
  g_object_checked_unref (song_io);
  BT_TEST_END;
}

TCase *
bt_song_io_native_test_case (void)
{
  TCase *tc = tcase_create ("BtSongIONativeTests");

  tcase_add_test (tc, test_bt_song_io_native_broken_file);
  tcase_add_test (tc, test_bt_song_io_native_load_twice);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
