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
* In this example we show how to create a source machine and adding a
* newly created pattern to it. Then we try to get the pattern back from
* the machine by its id.
*
* We check also, if we can create a pattern iterator and getting the added
* pattern back from the newly created iterator.
*/
BT_START_TEST(test_btsourcemachine_obj1){
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a source machine */
  machine=bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this example we show how to create a mono source machine and adding a
* newly created pattern to it. Then we try to get the pattern back from
* the machine by its id.
*
* We check also, if we can create a pattern iterator and getting the added
* pattern back from the newly created iterator.
*/
BT_START_TEST(test_btsourcemachine_obj2){
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  BtPattern *pattern=NULL;
  BtPattern *ref_pattern=NULL;
  GList *list,*node;
  gulong voices;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a source machine */
  machine=bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);

  /* verify number of voices */
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==0, NULL);

  /* try to get the same pattern back per id */
  ref_pattern=bt_machine_get_pattern_by_id(BT_MACHINE(machine),"pattern-id");
  fail_unless(ref_pattern==pattern, NULL);
  g_object_try_unref(ref_pattern);

  g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
  /* the list should not be null */
  fail_unless(list!=NULL, NULL);
  /* source machine has 3 default pattern (break+mute+solo) */
  fail_unless(g_list_length(list)==4, NULL);
  node=g_list_last(list);

  /* the returned pointer should point to the same pattern, that we added
  to the machine before */
  ref_pattern=node->data;
  fail_unless(ref_pattern==pattern, NULL);

  /* cleanup */
  g_list_free(list);

  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
* In this example we show how to create a poly source machine and adding a
* newly created pattern to it. The we change the number of voices in the machine
* and check back the voices in the pattern.
*/
BT_START_TEST(test_btsourcemachine_obj3){
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSourceMachine *machine=NULL;
  BtPattern *pattern=NULL;
  GstElement *element;
  gulong voices;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* try to create a source machine */
  machine=bt_source_machine_new(song,"gen","buzztard-test-poly-source",1,&err);
  fail_unless(machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* verify the number of voices */
  g_object_get(machine,"machine",&element,NULL);
  voices=gst_child_proxy_get_children_count(GST_CHILD_PROXY(element));
  g_object_unref(element);
  fail_unless(voices==1, NULL);

  /* try to create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  fail_unless(pattern!=NULL, NULL);

  /* verify number of voices */
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==1, NULL);

  /* change number of voices in the machine and verify again */
  g_object_set(machine,"voices",4,NULL);
  g_object_get(pattern,"voices",&voices,NULL);
  fail_unless(voices==4, NULL);

  /* cleanup */
  g_object_unref(pattern);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_source_machine_example_case(void) {
  TCase *tc = tcase_create("BtSourceMachineExamples");

  tcase_add_test(tc,test_btsourcemachine_obj1);
  tcase_add_test(tc,test_btsourcemachine_obj2);
  tcase_add_test(tc,test_btsourcemachine_obj3);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
