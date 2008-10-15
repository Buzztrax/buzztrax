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

/**
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

  // creating new empty app
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  mark_point();
  // creating new song
  song=bt_song_new(app);
  mark_point();
  // loading a song xml file
  loader=bt_song_io_new(check_get_test_song_path("test-simple1.xml"));
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

TCase *bt_song_info_example_case(void) {
  TCase *tc = tcase_create("BtSongInfoExamples");

  tcase_add_test(tc,test_btsonginfo_createdate);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
