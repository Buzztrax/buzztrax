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

static void test_setup(void) {
  bt_core_setup();
  app=bt_test_application_new();
  song=bt_song_new(app);
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  g_object_checked_unref(song);
  g_object_checked_unref(app);
  bt_core_teardown();
}


//-- tests

BT_START_TEST(test_btprocessormachine_obj1) {
  /* arrange */

  /* act */
  GError *err=NULL;
  BtProcessorMachine *machine=bt_processor_machine_new(song,"vol","volume",0,&err);
  
  /* assert */
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btprocessormachine_pattern) {
  /* arrange */
  BtProcessorMachine *machine=bt_processor_machine_new(song,"vol","volume",0,NULL);

  /* act */
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  
  /* assert */
  ck_assert_gobject_gulong_eq(pattern,"voices",0);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btprocessormachine_pattern_by_id) {
  /* arrange */
  BtProcessorMachine *machine=bt_processor_machine_new(song,"vol","volume",0,NULL);

  /* act */
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  
  /* assert */
  ck_assert_gobject_eq_and_unref(bt_machine_get_pattern_by_id(BT_MACHINE(machine),"pattern-id"),pattern);
  
  /* cleanup */
  g_object_unref(pattern);
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btprocessormachine_pattern_by_list) {
  GList *list,*node;

  /* arrange */
  BtProcessorMachine *machine=bt_processor_machine_new(song,"vol","volume",0,NULL);
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,BT_MACHINE(machine));
  g_object_get(G_OBJECT(machine),"patterns",&list,NULL);

  /* act */
  node=g_list_last(list);

  /* assert */
  fail_unless(node->data==pattern, NULL);

  /* cleanup */
  g_list_foreach(list,(GFunc)g_object_unref,NULL);
  g_list_free(list);
  g_object_unref(pattern);
  g_object_unref(machine);
}
BT_END_TEST


BT_START_TEST(test_btprocessormachine_def_patterns) {
  /* arrange */
  BtProcessorMachine *machine=bt_processor_machine_new(song,"vol","volume",0,NULL);

  /* act */
  GList *list;
  g_object_get(machine,"patterns",&list,NULL);

  /* assert */
  fail_unless(list!=NULL, NULL);
  ck_assert_int_eq(g_list_length(list),3);

  /* cleanup */
  g_list_foreach(list,(GFunc)g_object_unref,NULL);
  g_list_free(list);
  g_object_unref(machine);
}
BT_END_TEST


TCase *bt_processor_machine_example_case(void) {
  TCase *tc = tcase_create("BtProcessorMachineExamples");

  tcase_add_test(tc,test_btprocessormachine_obj1);
  tcase_add_test(tc,test_btprocessormachine_pattern);
  tcase_add_test(tc,test_btprocessormachine_pattern_by_id);
  tcase_add_test(tc,test_btprocessormachine_pattern_by_list);
  tcase_add_test(tc,test_btprocessormachine_def_patterns);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
