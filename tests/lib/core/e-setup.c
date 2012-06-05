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

static void suite_setup(void) {
  bt_core_setup();
}

static void test_setup(void) {
  app=bt_test_application_new();
  song=bt_song_new(app);
}

static void test_teardown(void) {
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}

static void suite_teardown(void) {
  bt_core_teardown();
}


//-- tests

BT_START_TEST(test_btsetup_new){
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));

  /* act */
  GList *machines,*wires;
  g_object_get(G_OBJECT(setup),"machines",&machines,"wires",&wires,NULL);

  /* assert */
  fail_unless(machines==NULL, NULL);
  fail_unless(wires==NULL, NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_machine_add_id) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  
  /* act */
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_setup_get_machine_by_id(setup, "src"), source);

  /* cleanup */
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_machine_rem_id) {
  /* arrange */
  BtSetup *setup;
  g_object_get(song,"setup",&setup,NULL);
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));

  /* act */
  bt_setup_remove_machine(setup, source);

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_setup_get_machine_by_id(setup, "src"), NULL);

  /* cleanup */
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_machine_add_list) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));

  /* act */
  GList *list;
  g_object_get(setup,"machines",&list,NULL);

  /* assert */
  fail_unless(list!=NULL, NULL);
  ck_assert_int_eq(g_list_length(list),1);
  fail_unless((BtMachine *)list->data==source, NULL);

  /* cleanup */
  g_list_free(list);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_wire_add_machine_id) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));
  BtMachine *sink = BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));

  /* act */
  BtWire *wire = bt_wire_new(song,source,sink,NULL);

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_setup_get_wire_by_src_machine(setup, source), wire);

  /* cleanup */
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_wire_rem_machine_id) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));
  BtMachine *sink = BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire = bt_wire_new(song,source,sink,NULL);

  /* act */
  bt_setup_remove_wire(setup, wire);

  /* assert */
  ck_assert_gobject_eq_and_unref(bt_setup_get_wire_by_src_machine(setup, source), NULL);

  /* cleanup */
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_wire_add_src_list) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));
  BtMachine *sink = BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire = bt_wire_new(song,source,sink,NULL);
  
  /* act */
  GList *list=bt_setup_get_wires_by_src_machine(setup,source);

  /* assert */
  fail_unless(list!=NULL, NULL);
  ck_assert_int_eq(g_list_length(list),1);
  ck_assert_gobject_eq_and_unref(BT_WIRE(g_list_first(list)->data), wire);

  /* cleanup */
  g_list_free(list);
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


BT_START_TEST(test_btsetup_wire_add_dst_list) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));
  BtMachine *sink = BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire = bt_wire_new(song,source,sink,NULL);
  
  /* act */
  GList *list=bt_setup_get_wires_by_dst_machine(setup,sink);

  /* assert */
  fail_unless(list!=NULL, NULL);
  ck_assert_int_eq(g_list_length(list),1);
  ck_assert_gobject_eq_and_unref(BT_WIRE(g_list_first(list)->data), wire);

  /* cleanup */
  g_list_free(list);
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


/*
* In this example you can see, how we get a source machine back by its type.
*/
BT_START_TEST(test_btsetup_machine_type) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));

  /* act */
  BtMachine *machine=bt_setup_get_machine_by_type(setup, BT_TYPE_SOURCE_MACHINE);
  
  /* assert */
  fail_unless(machine==source,NULL);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


/*
* In this test case we check the _unique_id function.
*/
BT_START_TEST(test_btsetup_unique_id1) {
  /* arrange */
  BtSetup *setup = BT_SETUP(check_gobject_get_object_property(song, "setup"));
  BtMachine *source = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0,NULL));

  /* act */
  gchar *id=bt_setup_get_unique_machine_id(setup,"src");

  /* assert */
  fail_unless(id!=NULL, NULL);
  ck_assert_gobject_eq_and_unref(bt_setup_get_machine_by_id(setup,id),NULL);
  ck_assert_str_ne(id,"src");

  /* cleanup */
  g_free(id);
  g_object_unref(source);
  g_object_unref(setup);
}
BT_END_TEST


TCase *bt_setup_example_case(void) {
  TCase *tc = tcase_create("BtSetupExamples");

  tcase_add_test(tc,test_btsetup_new);
  tcase_add_test(tc,test_btsetup_machine_add_id);
  tcase_add_test(tc,test_btsetup_machine_rem_id);
  tcase_add_test(tc,test_btsetup_machine_add_list);
  tcase_add_test(tc,test_btsetup_wire_add_machine_id);
  tcase_add_test(tc,test_btsetup_wire_rem_machine_id);
  tcase_add_test(tc,test_btsetup_wire_add_src_list);
  tcase_add_test(tc,test_btsetup_wire_add_dst_list);
  tcase_add_test(tc,test_btsetup_machine_type);
  tcase_add_test(tc,test_btsetup_unique_id1);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc,suite_setup, suite_teardown);
  return(tc);
}
