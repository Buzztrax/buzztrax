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

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
  //puts(__FILE__":teardown");
}

//-- tests

// try to create a SongIO object with NULL pointer
BT_START_TEST(test_btsong_io_obj1) {
  BtSongIO *song_io;
  
  song_io=bt_song_io_from_file(NULL);
  fail_unless(song_io == NULL, NULL);
}
BT_END_TEST

// try to create a SongIO object with empty string
BT_START_TEST(test_btsong_io_obj2) {
  BtSongIO *song_io;

  song_io=bt_song_io_from_file("");
  fail_unless(song_io==NULL, NULL);
}
BT_END_TEST

// try to create a SongIO object from song name without extension
BT_START_TEST(test_btsong_io_obj3) {
  BtSongIO *song_io;

  song_io=bt_song_io_from_file("test");
  fail_unless(song_io==NULL, NULL);
}
BT_END_TEST

// try to create a SongIO object from song name with unknown extension
BT_START_TEST(test_btsong_io_obj4) {
  BtSongIO *song_io;

  song_io=bt_song_io_from_file("test.unk");
  fail_unless(song_io==NULL, NULL);
}
BT_END_TEST

TCase *bt_song_io_test_case(void) {
  TCase *tc = tcase_create("BtSongIOTests");

  tcase_add_test(tc,test_btsong_io_obj1);
  tcase_add_test(tc,test_btsong_io_obj2);
  tcase_add_test(tc,test_btsong_io_obj3);
  tcase_add_test(tc,test_btsong_io_obj4);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
