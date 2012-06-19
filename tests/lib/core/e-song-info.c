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

static void case_setup(void) {
  GST_INFO("================================================================================");
}

static void test_setup(void) {
  app=bt_test_application_new();
  song=bt_song_new(app);
}

static void test_teardown(void) {
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}

static void case_teardown(void) {
}


//-- tests

BT_START_TEST(test_bt_song_info_date_stamps) {
  /* arrange */
  BtSongInfo *song_info=BT_SONG_INFO(check_gobject_get_object_property(song, "song-info"));

  /* act */
  gchar *create_dts=check_gobject_get_str_property(song_info,"create-dts");

  /* assert */
  fail_unless(create_dts != NULL, NULL);
  ck_assert_gobject_str_eq(song_info,"change-dts",create_dts);

  /* cleanup */
  g_free(create_dts);
  g_object_unref(song_info);
}
BT_END_TEST

/*
* Test changing the tempo
*/
BT_START_TEST(test_bt_song_info_tempo) {
  /* arrange */
  BtSequence *sequence=BT_SEQUENCE(check_gobject_get_object_property(song, "sequence"));
  BtSongInfo *song_info=BT_SONG_INFO(check_gobject_get_object_property(song, "song-info"));
  g_object_set(song_info,"bpm",120,NULL);
  GstClockTime t1=bt_sequence_get_bar_time(sequence);

  /* act */
  g_object_set(song_info,"bpm",60,NULL);
  GstClockTime t2=bt_sequence_get_bar_time(sequence);

  /* assert */
  ck_assert_uint64_gt(t2,t1);
  ck_assert_uint64_eq((t2/2),t1);
  
  /* cleanup */
  g_object_unref(song_info);
  g_object_unref(sequence);
}
BT_END_TEST

TCase *bt_song_info_example_case(void) {
  TCase *tc = tcase_create("BtSongInfoExamples");

  tcase_add_test(tc,test_bt_song_info_date_stamps);
  tcase_add_test(tc,test_bt_song_info_tempo);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
