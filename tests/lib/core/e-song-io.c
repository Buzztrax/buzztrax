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
#include "song-io-native.h"

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

static void
test_bt_song_io_module_info (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  const GList *mi = bt_song_io_get_module_info_list ();

  GST_INFO ("-- assert --");
  fail_unless (mi != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_song_io_file (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("simple2.xml"));

  GST_INFO ("-- assert --");
  fail_unless (song_io != NULL, NULL);
  fail_unless (BT_IS_SONG_IO_NATIVE (song_io), NULL);

  GST_INFO ("-- cleanup --");
  g_object_checked_unref (song_io);
  BT_TEST_END;
}

static void
test_bt_song_io_data (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_data (NULL, 0, "audio/x-bzt-xml");

  GST_INFO ("-- assert --");
  fail_unless (song_io != NULL, NULL);
  fail_unless (BT_IS_SONG_IO_NATIVE (song_io), NULL);

  GST_INFO ("-- cleanup --");
  g_object_checked_unref (song_io);
  BT_TEST_END;
}

TCase *
bt_song_io_example_case (void)
{
  TCase *tc = tcase_create ("BtSongIOExamples");

  tcase_add_test (tc, test_bt_song_io_module_info);
  tcase_add_test (tc, test_bt_song_io_file);
  tcase_add_test (tc, test_bt_song_io_data);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
