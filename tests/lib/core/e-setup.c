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
* In this test case we demonstrate how to create a setup and adding a machine
* to the setup.
* Then we use the iterator to get the current added machine back and check
* if the machine was added correctly.
*/
BT_START_TEST(test_btsetup_obj1){
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machine
  BtSourceMachine *source=NULL;
  BtMachine *ref_machine=NULL;
  GList *list,*node;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);

  /* try to create machine */
  source = bt_source_machine_new(song,"src","buzztard-test-mono-source",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
  /* the list should not be null */
  fail_unless(list!=NULL, NULL);
  node=list;

  /* the pointer should be pointing to the gen1 machine */
  ref_machine=node->data;
  fail_unless(ref_machine!=NULL, NULL);
  fail_unless(ref_machine==BT_MACHINE(source), NULL);
  fail_unless(BT_IS_SOURCE_MACHINE(ref_machine)==TRUE, NULL);

  /* the list should contain only one element */
  fail_unless(g_list_length(list)==1, NULL);

  g_list_free(list);

  /* clean up */
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this test case we demonstrate how to create a wire and adding the newly
* created wire to the setup. After that, we try to get the same wire back, if
* we give the source or dest machine.
*/
BT_START_TEST(test_btsetup_obj2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtSinkMachine *sink=NULL;
  // wire
  BtWire *wire=NULL;
  BtWire *ref_wire=NULL;
  GList *list;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","audiotestsrc",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create sink machine (default audio sink) */
  sink = bt_sink_machine_new(song,"sink",&err);
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create the wire */
  wire = bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink),&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to get the list of wires in the setup */
  g_object_get(G_OBJECT(setup),"wires",&list,NULL);
  /* the list should not null */
  fail_unless(list!=NULL,NULL);

  /* look, of the added wire is in the list */
  ref_wire=list->data;
  fail_unless(ref_wire!=NULL,NULL);
  fail_unless(wire==BT_WIRE(ref_wire),NULL);

  /* the list should contain only one element */
  fail_unless(g_list_length(list)==1, NULL);

  g_list_free(list);

  /* try to get the current added wire by the source machine. In this case the
  source of the wire is our source machine.*/
  ref_wire = bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
  fail_unless(ref_wire!=NULL, NULL);
  fail_unless(ref_wire==wire, NULL);
  g_object_try_unref(ref_wire);

  /* try to get the current added wire by the dest machine. In this case the
  destination of the wire is our sink machine. */
  ref_wire = bt_setup_get_wire_by_dst_machine(setup, BT_MACHINE(sink));
  fail_unless(ref_wire!=NULL, NULL);
  fail_unless(ref_wire==wire, NULL);
  g_object_try_unref(ref_wire);

  /* clean up */
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this test we demonstrate how to remove a machine from the setup after the
* same machine is added to the setup.
*/
BT_START_TEST(test_btsetup_obj3) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtMachine *ref_machine=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","buzztard-test-mono-source",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to get the machine back from the setup */
  ref_machine=bt_setup_get_machine_by_id(setup, "src");
  fail_unless(ref_machine!=NULL, NULL);
  g_object_unref(ref_machine);

  /* now we try to remove the same machine from the setup */
  bt_setup_remove_machine(setup, BT_MACHINE(source));

  /* try to get the machine back from the setup, the ref_machine should be null */
  ref_machine=bt_setup_get_machine_by_id(setup, "src");
  fail_unless(ref_machine==NULL, NULL);

  /* clean up */
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this test we demonstrate how to remove a wire from the setup after the
* same wire is added to the setup.
*/
BT_START_TEST(test_btsetup_obj4) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtSinkMachine *sink=NULL;
  // wire
  BtWire *wire=NULL;
  BtWire *ref_wire=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","audiotestsrc",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create sink machine (default audio sink) */
  sink = bt_sink_machine_new(song,"sink",&err);
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create the wire */
  wire = bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink),&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* check if we can get the wire from the setup */
  ref_wire=bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
  fail_unless(ref_wire!=NULL,NULL);
  g_object_try_unref(ref_wire);

  /* try to remove the wire from the setup */
  bt_setup_remove_wire(setup, wire);

  /* check if we can get the wire from the setup. The ref_wire should be null */
  ref_wire=bt_setup_get_wire_by_src_machine(setup, BT_MACHINE(source));
  fail_unless(ref_wire==NULL,NULL);

  /* clean up */
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this example we show you how to get wires by source machines.
*/
BT_START_TEST(test_btsetup_wire1) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtSinkMachine *sink=NULL;
  // wire
  BtWire *wire=NULL;
  BtWire *ref_wire=NULL;
  /* wire list */
  GList* wire_list=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","audiotestsrc",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create sink machine (default audio sink) */
  sink = bt_sink_machine_new(song,"sink",&err);
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create the wire */
  wire = bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink),&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to get the list of wires */
  wire_list=bt_setup_get_wires_by_src_machine(setup,BT_MACHINE(source));
  fail_unless(wire_list!=NULL,NULL);
  ref_wire=BT_WIRE(g_list_first(wire_list)->data);
  fail_unless(ref_wire!=NULL,NULL);
  fail_unless(ref_wire==wire,NULL);
  g_object_unref(ref_wire);
  g_list_free(wire_list);

  /* clean up */
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this example we show you how to get wires by a destination machine.
*/
BT_START_TEST(test_btsetup_wire2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtSinkMachine *sink=NULL;
  // wire
  BtWire *wire=NULL;
  BtWire *ref_wire=NULL;
  /* wire list */
  GList* wire_list=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","audiotestsrc",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create sink machine (default audio sink) */
  sink = bt_sink_machine_new(song,"sink",&err);
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create the wire */
  wire = bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink),&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to get the list of wires */
  wire_list=bt_setup_get_wires_by_dst_machine(setup,BT_MACHINE(sink));
  fail_unless(wire_list!=NULL,NULL);
  ref_wire=BT_WIRE(g_list_first(wire_list)->data);
  fail_unless(ref_wire!=NULL,NULL);
  fail_unless(ref_wire==wire,NULL);
  g_object_unref(ref_wire);
  g_list_free(wire_list);

  /* clean up */
  g_object_unref(wire);
  g_object_unref(sink);
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this example you can see, how we get a source machine back by its type.
*/
BT_START_TEST(test_btsetup_machine1) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtMachine *ref_machine=NULL;
  // Gtype of the machine
  GType machine_type;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","buzztard-test-mono-source",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* now try to get the machine back via bt_setup_get_machine_by_type */
  machine_type=G_OBJECT_TYPE(source);
  ref_machine=bt_setup_get_machine_by_type(setup, machine_type);
  fail_unless(BT_IS_SOURCE_MACHINE(ref_machine),NULL);
  fail_unless(ref_machine==BT_MACHINE(source),NULL);
  g_object_unref(ref_machine);

  /* clean up */
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this test case we check the _unique_id function.
*/
BT_START_TEST(test_btsetup_unique_id1){
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machine
  BtSourceMachine *source=NULL;
  BtMachine *ref_machine=NULL;
  gchar *id;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);

  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);

  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","buzztard-test-mono-source",0,&err);
  fail_unless(source!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* get an id for a new machine, with the same name */
  id=bt_setup_get_unique_machine_id(setup,"src");
  fail_unless(id!=NULL, NULL);
  fail_unless(strcmp(id,"src"), NULL);

  /* there should be no machine using this id */
  ref_machine=bt_setup_get_machine_by_id(setup,id);
  fail_unless(ref_machine==NULL, NULL);

  /* clean up */
  g_free(id);
  g_object_unref(source);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_setup_example_case(void) {
  TCase *tc = tcase_create("BtSetupExamples");

  tcase_add_test(tc,test_btsetup_obj1);
  tcase_add_test(tc,test_btsetup_obj2);
  tcase_add_test(tc,test_btsetup_obj3);
  tcase_add_test(tc,test_btsetup_obj4);
  tcase_add_test(tc,test_btsetup_wire1);
  tcase_add_test(tc,test_btsetup_wire2);
  tcase_add_test(tc,test_btsetup_machine1);
  tcase_add_test(tc,test_btsetup_unique_id1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
