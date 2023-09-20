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

/* apply generic property tests to song-info */
START_TEST (test_bt_song_info_properties)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GObject *song_info = check_gobject_get_object_property (song, "song-info");

  /* act & assert */
  ck_assert (check_gobject_properties (song_info));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* create a new song-info with a NULL song object */
START_TEST (test_bt_song_info_null_song)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSongInfo *song_info = bt_song_info_new (NULL);

  GST_INFO ("-- assert --");
  ck_assert (song_info != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

TCase *
bt_song_info_test_case (void)
{
  TCase *tc = tcase_create ("BtSongInfoTests");

  tcase_add_test (tc, test_bt_song_info_properties);
  tcase_add_test (tc, test_bt_song_info_null_song);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
