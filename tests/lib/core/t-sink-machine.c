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

// test attribute handling in sink names
BT_START_TEST(test_btsinkmachine_settings1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;
  //gchar *saved_audiosink_name;

  /* create a dummy app, song and get settings */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  song=bt_song_new(app);
  settings=bt_settings_new();
  mark_point();

  //g_object_get(settings,"audiosink",&saved_audiosink_name,NULL);
  g_object_set(settings,"audiosink","osssink sync=false",NULL);
  mark_point();

  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);

  //g_object_set(settings,"audiosink",saved_audiosink_name,NULL);

  //g_free(saved_audiosink_name);
  g_object_unref(machine);
  g_object_unref(settings);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST;

/*
 * Try to create a sink machine, if we set the sink property with the gconf
 * properties to the string "audioconvert ! osssink sync=false". This string
 * should be replaced by the sink machine to "ossink" and the machine should be
 * instantiable.
 */
BT_START_TEST(test_btsinkmachine_settings2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;

  /* create a dummy app, song and get settings */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);
  settings=bt_settings_new();
  mark_point();

  g_object_set(settings,"audiosink","audioconvert ! osssink sync=false",NULL);
  mark_point();

  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);

  g_object_unref(settings);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST;

// test attribute handling in sink names
BT_START_TEST(test_btsinkmachine_settings3) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;

  /* create a dummy app, song and get settings */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  song=bt_song_new(app);
  settings=bt_settings_new();
  mark_point();

  g_object_set(settings,"audiosink","xvimagsink",NULL);
  mark_point();

  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);

  g_object_unref(settings);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST;

// test attribute handling in sink names
BT_START_TEST(test_btsinkmachine_settings4) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;

  /* create a dummy app, song and get settings */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  song=bt_song_new(app);
  settings=bt_settings_new();
  mark_point();

  g_object_set(settings,"audiosink","alsasink device=invalid:666",NULL);
  mark_point();

  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);

  g_object_unref(settings);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST;

// test if the song play routine works with fakesink
BT_START_TEST(test_btsinkmachine_play1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  BtSettings *settings=NULL;
  gboolean load_ret = FALSE;
  gboolean res;

  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  settings=bt_settings_new();
  g_object_set(settings,"audiosink","fakesink",NULL);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_make(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  res=bt_song_play(song);
  fail_unless(res, NULL);
  sleep(1);
  bt_song_stop(song);

  g_object_unref(settings);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// test if the song play routine handles sink with wrong parameters
BT_START_TEST(test_btsinkmachine_play2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  BtSettings *settings=NULL;
  gboolean load_ret = FALSE;
  gboolean res;

  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  settings=bt_settings_new();
  g_object_set(settings,"audiosink","alsasink device=invalid:666",NULL);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_make(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  res=bt_song_play(song);
  fail_unless(res, NULL);
  sleep(1);
  bt_song_stop(song);

  g_object_unref(settings);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_sink_machine_test_case(void) {
  TCase *tc = tcase_create("BtSinkMachineTests");

  tcase_add_test(tc,test_btsinkmachine_settings1);
  tcase_add_test(tc,test_btsinkmachine_settings2);
  tcase_add_test(tc,test_btsinkmachine_settings3);
  tcase_add_test(tc,test_btsinkmachine_settings4);
  tcase_add_test(tc,test_btsinkmachine_play1);
  tcase_add_test(tc,test_btsinkmachine_play2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
