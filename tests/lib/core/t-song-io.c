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

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
}

static void
test_teardown (void)
{
}

static void
case_teardown (void)
{
}


//-- tests

// try to create a SongIO object with NULL pointer
START_TEST (test_bt_song_io_new_null)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file (NULL, NULL);

  /*assert */
  ck_assert (song_io == NULL);
  BT_TEST_END;
}
END_TEST

// try to create a SongIO object with empty string
START_TEST (test_bt_song_io_new_empty_filename)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file ("", NULL);

  /*assert */
  ck_assert (song_io == NULL);
  BT_TEST_END;
}
END_TEST

// try to create a SongIO object from song name without extension
START_TEST (test_bt_song_io_new_wrong_filename)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file ("test", NULL);

  /*assert */
  ck_assert (song_io == NULL);
  BT_TEST_END;
}
END_TEST

// try to create a SongIO object from song name with unknown extension
START_TEST (test_bt_song_io_new_inexisting_filename)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file ("test.unk", NULL);

  /*assert */
  ck_assert (song_io == NULL);
  BT_TEST_END;
}
END_TEST

TCase *
bt_song_io_test_case (void)
{
  TCase *tc = tcase_create ("BtSongIOTests");

  tcase_add_test (tc, test_bt_song_io_new_null);
  tcase_add_test (tc, test_bt_song_io_new_empty_filename);
  tcase_add_test (tc, test_bt_song_io_new_wrong_filename);
  tcase_add_test (tc, test_bt_song_io_new_inexisting_filename);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
