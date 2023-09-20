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

START_TEST (test_bt_song_info_date_stamps)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));

  GST_INFO ("-- act --");
  gchar *create_dts = check_gobject_get_str_property (song_info, "create-dts");

  GST_INFO ("-- assert --");
  fail_unless (create_dts != NULL, NULL);
  ck_assert_gobject_str_eq (song_info, "change-dts", create_dts);

  GST_INFO ("-- cleanup --");
  g_free (create_dts);
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

/* Test changing the tempo */
START_TEST (test_bt_song_info_update_bpm)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstClockTime t1, t2;
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  g_object_set (song_info, "bpm", 120, NULL);
  g_object_get (song_info, "tick-duration", &t1, NULL);

  GST_INFO ("-- act --");
  g_object_set (song_info, "bpm", 60, NULL);
  g_object_get (song_info, "tick-duration", &t2, NULL);

  GST_INFO ("-- assert --");
  ck_assert_uint64_eq (t2, t1 + t1);

  GST_INFO ("-- cleanup --");
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_info_update_tpb)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstClockTime t1, t2;
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  g_object_set (song_info, "tpb", 8, NULL);
  g_object_get (song_info, "tick-duration", &t1, NULL);

  GST_INFO ("-- act --");
  g_object_set (song_info, "tpb", 4, NULL);
  g_object_get (song_info, "tick-duration", &t2, NULL);

  GST_INFO ("-- assert --");
  ck_assert_uint64_eq (t2, t1 + t1);

  GST_INFO ("-- cleanup --");
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST


START_TEST (test_bt_song_info_seconds_since_last_saved)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));

  GST_INFO ("-- act --");
  // TODO: waiting one sec makes this the slowest test :/
  g_usleep (G_USEC_PER_SEC);
  gint ts = bt_song_info_get_seconds_since_last_saved (song_info);

  GST_INFO ("-- assert --");
  ck_assert_int_gt (ts, 0);

  GST_INFO ("-- cleanup --");
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_info_tick_to_time)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  g_object_set (song_info, "bpm", 250L, "tpb", 16L, NULL);

  GST_INFO ("-- act --");
  GstClockTime ts = bt_song_info_tick_to_time (song_info, 8);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ts, 120 * GST_MSECOND);

  GST_INFO ("-- cleanup --");
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

TCase *
bt_song_info_example_case (void)
{
  TCase *tc = tcase_create ("BtSongInfoExamples");

  tcase_add_test (tc, test_bt_song_info_date_stamps);
  tcase_add_test (tc, test_bt_song_info_update_bpm);
  tcase_add_test (tc, test_bt_song_info_update_tpb);
  tcase_add_test (tc, test_bt_song_info_seconds_since_last_saved);
  tcase_add_test (tc, test_bt_song_info_tick_to_time);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}

