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

/*
* try to create a new setup with a NULL song object
*/
BT_START_TEST(test_btsonginfo_obj1) {
  BtSongInfo *song_info=NULL;
  
  // we don't use a _constructed method there yet
  //check_init_error_trapp("bt_song_info_","BT_IS_SONG(self->priv->song)");
  song_info=bt_song_info_new(NULL);
  //fail_unless(check_has_error_trapped(), NULL);
  fail_unless(song_info != NULL, NULL);
  g_object_unref(song_info);
}
BT_END_TEST


TCase *bt_song_info_test_case(void) {
  TCase *tc = tcase_create("BtSongInfoTests");

  tcase_add_test(tc,test_btsonginfo_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
