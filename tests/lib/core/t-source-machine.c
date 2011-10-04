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
* try to create a machine with not exising plugin name
*/
BT_START_TEST(test_btsourcemachine_obj1) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a source machine with wrong pluginname (not existing)*/
  machine=bt_source_machine_new(song,"id","nonsense",1,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err!=NULL, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* try to create a machine which is a sink machine and not a source machine
*/
BT_START_TEST(test_btsourcemachine_obj2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a source machine with wrong plugin type (sink instead of source) */
  machine=bt_source_machine_new(song,"id","autoaudiosink",1,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err!=NULL, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btsourcemachine_obj3){
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  gulong ix=0;
  GError *error=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a normal sink machine */
  machine=bt_source_machine_new(song,"id","audiotestsrc",0,&err);
  fail_unless(machine!=NULL,NULL);
  fail_unless(err==NULL, NULL);
  /* try to get global param index from audiotestsrc */
  ix=bt_machine_get_global_param_index(BT_MACHINE(machine),"nonsense",&error);
  fail_unless(g_error_matches(error, g_quark_from_static_string("BtMachine"),0),NULL);
  g_error_free(error);
  fail_unless(ix==-1);

  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_source_machine_test_case(void) {
  TCase *tc = tcase_create("BtSourceMachineTests");

  tcase_add_test(tc,test_btsourcemachine_obj1);
  tcase_add_test(tc,test_btsourcemachine_obj2);
  tcase_add_test(tc,test_btsourcemachine_obj3);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
