/* $Id$
 *
 * Buzztard
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

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
}

/*
* In this test we show, how to get the creation date of an song from the
* song info class. We load a example song and try to retrive the creation date
* from it.
*/
BT_START_TEST(test_btsonginfo_createdate) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  BtSongInfo *song_info=NULL;
  gchar *create_dts=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  // loading a song xml file
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  mark_point();
  bt_song_io_load(loader,song);
  mark_point();
  // get the song info class from the song property
  g_object_get(song,"song-info",&song_info,NULL);
  mark_point();
  // get the creating date property from the song info
  g_object_get(song_info,"create-dts",&create_dts,NULL);
  fail_unless(create_dts!=NULL,NULL);
  g_free(create_dts);

  g_object_unref(song_info);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);

}
BT_END_TEST

/*
* Test changing the tempo
*/
BT_START_TEST(test_btsonginfo_tempo) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  BtSongInfo *song_info;
  BtSequence *sequence;
  GstClockTime t1, t2;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  // loading a song xml file
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  mark_point();
  bt_song_io_load(loader,song);
  mark_point();
  // get the song info class from the song property
  g_object_get(song,"song-info",&song_info,"sequence",&sequence,NULL);
  mark_point();
  // set a new bpm
  g_object_set(song_info,"bpm",120,NULL);
  t1=bt_sequence_get_bar_time(sequence);

  // set a new bpm
  g_object_set(song_info,"bpm",60,NULL);
  t2=bt_sequence_get_bar_time(sequence);
  
  fail_unless(t2>t1,NULL);
  fail_unless((t2/2)==t1,NULL);
  
  // wait a bit for the change to happen
  g_usleep(G_USEC_PER_SEC/10);

  g_object_unref(song_info);
  g_object_unref(sequence);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);

}
BT_END_TEST

TCase *bt_song_info_example_case(void) {
  TCase *tc = tcase_create("BtSongInfoExamples");

  tcase_add_test(tc,test_btsonginfo_createdate);
  tcase_add_test(tc,test_btsonginfo_tempo);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
