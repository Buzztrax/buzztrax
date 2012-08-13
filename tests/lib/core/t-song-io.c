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
static void
test_bt_song_io_new_null (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  BtSongIO *song_io = bt_song_io_from_file (NULL);

  /*assert */
  fail_unless (song_io == NULL, NULL);
  BT_TEST_END;
}

// try to create a SongIO object with empty string
static void
test_bt_song_io_new_empty_filename (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  BtSongIO *song_io = bt_song_io_from_file ("");

  /*assert */
  fail_unless (song_io == NULL, NULL);
  BT_TEST_END;
}

// try to create a SongIO object from song name without extension
static void
test_bt_song_io_new_wrong_filename (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  BtSongIO *song_io = bt_song_io_from_file ("test");

  /*assert */
  fail_unless (song_io == NULL, NULL);
  BT_TEST_END;
}

// try to create a SongIO object from song name with unknown extension
static void
test_bt_song_io_new_inexisting_filename (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  BtSongIO *song_io = bt_song_io_from_file ("test.unk");

  /*assert */
  fail_unless (song_io == NULL, NULL);
  BT_TEST_END;
}

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
  return (tc);
}
