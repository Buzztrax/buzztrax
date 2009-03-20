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

//-- tests

BT_START_TEST(test_btmachine_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine;
  GError *err=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);

  /* create a machine */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* should have no patterns */
  fail_unless(!bt_machine_has_patterns(machine),NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(machine);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * activate the input level meter in an unconnected machine
 */
BT_START_TEST(test_btmachine_enable_input_level1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine;
  GError *err=NULL;
  gboolean res;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);

  /* create a machine */
  machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0,&err));
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  res=bt_machine_enable_input_pre_level(machine);
  fail_unless(res == TRUE, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(machine);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * activate the input level meter in a connected machine
 */
BT_START_TEST(test_btmachine_enable_input_level2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine1,*machine2;
  BtWire *wire;
  GError *err=NULL;
  gboolean res;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);

  /* create two machines */
  machine1=BT_MACHINE(bt_processor_machine_new(song,"vol1","volume",0,&err));
  fail_unless(machine1 != NULL, NULL);
  fail_unless(err==NULL, NULL);
  machine2=BT_MACHINE(bt_processor_machine_new(song,"vol2","volume",0,&err));
  fail_unless(machine2 != NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* connect them */
  wire=bt_wire_new(song,machine1,machine2,&err);
  fail_unless(wire != NULL, NULL);
  fail_unless(err==NULL, NULL);

  res=bt_machine_enable_input_pre_level(machine2);
  fail_unless(res == TRUE, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(wire);
  g_object_checked_unref(machine1);
  g_object_checked_unref(machine2);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * activate the input gain control in an unconnected machine
 */
BT_START_TEST(test_btmachine_enable_input_gain1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine;
  GError *err=NULL;
  gboolean res;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);

  /* create a machine */
  machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0,&err));
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  res=bt_machine_enable_input_gain(machine);
  fail_unless(res == TRUE, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(machine);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * activate the output gain control in an unconnected machine
 */
BT_START_TEST(test_btmachine_enable_output_gain1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtMachine *machine;
  GError *err=NULL;
  gboolean res;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);

  /* create a machine */
  machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0,&err));
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  res=bt_machine_enable_output_gain(machine);
  fail_unless(res == TRUE, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(machine);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_machine_example_case(void) {
  TCase *tc = tcase_create("BtMachineExamples");

  tcase_add_test(tc,test_btmachine_obj1);
  tcase_add_test(tc,test_btmachine_enable_input_level1);
  tcase_add_test(tc,test_btmachine_enable_input_level2);
  tcase_add_test(tc,test_btmachine_enable_input_gain1);
  tcase_add_test(tc,test_btmachine_enable_output_gain1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
