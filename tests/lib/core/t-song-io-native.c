/* $Id: t-song-io-native.c,v 1.3 2006-08-24 20:00:55 ensonic Exp $
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
  //puts(__FILE__":teardown");
}

//-- tests

// try to create a songIO object from file with different format
BT_START_TEST(test_btsong_io_native_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *song_io;
  gboolean res;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  song_io=bt_song_io_new(check_get_test_song_path("broken1.xml"));
  fail_unless(song_io != NULL, NULL);
  
  res=bt_song_io_load(song_io,song);
  fail_unless(res == FALSE, NULL);

  g_object_checked_unref(song_io);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_song_io_native_test_case(void) {
  TCase *tc = tcase_create("BtSongIONativeTests");

  tcase_add_test(tc,test_btsong_io_native_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
