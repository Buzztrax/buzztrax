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

/* apply generic property tests to song-info */
static void
test_bt_song_info_properties (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  GObject *song_info = check_gobject_get_object_property (song, "song-info");

  /* act & assert */
  fail_unless (check_gobject_properties (song_info), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* create a new song-info with a NULL song object */
static void
test_bt_song_info_null_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  BtSongInfo *song_info = bt_song_info_new (NULL);

  /* assert */
  fail_unless (song_info != NULL, NULL);

  /* cleanup */
  g_object_unref (song_info);
  BT_TEST_END;
}

TCase *
bt_song_info_test_case (void)
{
  TCase *tc = tcase_create ("BtSongInfoTests");

  tcase_add_test (tc, test_bt_song_info_properties);
  tcase_add_test (tc, test_bt_song_info_null_song);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
