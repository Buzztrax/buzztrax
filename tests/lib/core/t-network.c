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
  //puts(__FILE__":teardown");
}

//-- tests

/*
* try to check if we can create a network of NULL machines and NULL wires
*
* this is a negative test
*/
BT_START_TEST(test_btcore_net1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  gboolean song_ret;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* get the setup for the song */
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to add a NULL wire to the setup */
  check_init_error_trapp("bt_setup_add_wire","BT_IS_WIRE");
  bt_setup_add_wire(setup, NULL);
  fail_unless(check_has_error_trapped(), NULL);

  /* try to start playing the song */
  song_ret=bt_song_play(song);
  fail_unless(song_ret==TRUE, NULL);
  bt_song_stop(song);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_network_test_case(void) {
  TCase *tc = tcase_create("BtNetworkTests");

  tcase_add_test(tc,test_btcore_net1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
