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

static void case_setup(void) {
  GST_INFO("================================================================================");
}

static void test_setup(void) {
  app=bt_test_application_new();
  song=bt_song_new(app);
}

static void test_teardown(void) {
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}

static void case_teardown(void) {
}


//-- tests

BT_START_TEST(test_btmachine_obj1) {
  /* arrange */

  /* act */
  GError *err=NULL;
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));

  /* assert */
  fail_unless(machine != NULL, NULL);
  fail_unless(err==NULL, NULL);
  fail_unless(!bt_machine_has_patterns(machine),NULL);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


/*
 * activate the input level meter in an unconnected machine
 */
BT_START_TEST(test_btmachine_enable_input_level1) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0,NULL));

  /* act */
  gboolean res=bt_machine_enable_input_pre_level(machine);
  
  /* assert */
  fail_unless(res == TRUE, NULL);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


/*
 * activate the input level meter in a connected machine
 */
BT_START_TEST(test_btmachine_enable_input_level2) {
  /* arrange */
  BtMachine *machine1=BT_MACHINE(bt_processor_machine_new(song,"vol1","volume",0,NULL));
  BtMachine *machine2=BT_MACHINE(bt_processor_machine_new(song,"vol2","volume",0,NULL));
  BtWire *wire=bt_wire_new(song,machine1,machine2,NULL);

  /* act */
  gboolean res=bt_machine_enable_input_pre_level(machine2);

  /* assert */
  fail_unless(res == TRUE, NULL);

  /* cleanup */
  g_object_unref(wire);
  g_object_unref(machine1);
  g_object_unref(machine2);
}
BT_END_TEST


/*
 * activate the input gain control in an unconnected machine
 */
BT_START_TEST(test_btmachine_enable_input_gain1) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0,NULL));

  /* act */
  gboolean res=bt_machine_enable_input_gain(machine);
  
  /* assert */
  fail_unless(res == TRUE, NULL);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


/*
 * activate the output gain control in an unconnected machine
 */
BT_START_TEST(test_btmachine_enable_output_gain1) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0,NULL));

  /* act */
  gboolean res=bt_machine_enable_output_gain(machine);
  
  /* assert */
  fail_unless(res == TRUE, NULL);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


/*
 * add pattern
 */
BT_START_TEST(test_btmachine_add_pattern) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",1L,NULL));

  /* act */
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);
  
  /* assert */
  fail_unless(bt_machine_has_patterns(machine),NULL);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(pattern);
}             
BT_END_TEST


BT_START_TEST(test_btmachine_pattern_names) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",1L,NULL));
  BtPattern *pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,machine);

  /* act */
  gchar *pattern_name=bt_machine_get_unique_pattern_name(machine);
  
  /* assert */
  ck_assert_str_ne(pattern_name,"pattern-name");

  /* cleanup */
  g_free(pattern_name);
  g_object_unref(machine);
  g_object_unref(pattern);

}             
BT_END_TEST


BT_START_TEST(test_btmachine_check_voices) {
  /* arrange */
  
  /* act */
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",2L,NULL));

  /* assert */
  GstChildProxy *element=(GstChildProxy*)check_gobject_get_object_property(machine,"machine");
  ck_assert_int_eq(gst_child_proxy_get_children_count(element),2);
  //ck_assert_gobject_gulong_eq(machine,"voices",2);

  /* cleanup */
  gst_object_unref(element);
  g_object_unref(machine);
}
BT_END_TEST


/*
 * change voices and verify that voices in machine and patetrn are in sync
 */
BT_START_TEST(test_btmachine_change_voices) {
  /* arrange */
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-poly-source",1L,NULL));
  BtPattern *p1=bt_pattern_new(song,"pattern-id1","pattern-name1",8L,machine);
  BtPattern *p2=bt_pattern_new(song,"pattern-id2","pattern-name2",8L,machine);

  /* act */
  g_object_set(machine,"voices",2,NULL);
  
  /* assert */
  ck_assert_gobject_gulong_eq(p1,"voices",2);
  ck_assert_gobject_gulong_eq(p2,"voices",2);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(p1);
  g_object_unref(p2);
}
BT_END_TEST


BT_START_TEST(test_btmachine_state_mute_no_sideeffects) {
  /* arrange */
  BtMachine *src=BT_MACHINE(bt_source_machine_new(song,"gen","audiotestsrc",0L,NULL));
  BtMachine *proc=BT_MACHINE(bt_processor_machine_new(song,"vol","volume",0L,NULL));
  BtMachine *sink=BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire1=bt_wire_new(song,src,proc,NULL);
  BtWire *wire2=bt_wire_new(song,proc,sink,NULL);

  /* act */
  g_object_set(src,"state",BT_MACHINE_STATE_MUTE,NULL);

  /* assert */
  ck_assert_gobject_guint_eq(src, "state", BT_MACHINE_STATE_MUTE);
  ck_assert_gobject_guint_eq(proc, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq(sink, "state", BT_MACHINE_STATE_NORMAL);

  /* cleanup */
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(sink);
  g_object_unref(proc);
  g_object_unref(src);
}
BT_END_TEST


BT_START_TEST(test_btmachine_state_solo_unmutes_others) {
  /* arrange */
  BtMachine *src1=BT_MACHINE(bt_source_machine_new(song,"gen1","audiotestsrc",0L,NULL));
  BtMachine *src2=BT_MACHINE(bt_source_machine_new(song,"gen2","audiotestsrc",0L,NULL));
  BtMachine *sink=BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire1=bt_wire_new(song,src1,sink,NULL);
  BtWire *wire2=bt_wire_new(song,src2,sink,NULL);

  /* act */
  g_object_set(src1,"state",BT_MACHINE_STATE_SOLO,NULL);
  g_object_set(src2,"state",BT_MACHINE_STATE_SOLO,NULL);

  /* assert */
  ck_assert_gobject_guint_eq(src1, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq(src2, "state", BT_MACHINE_STATE_SOLO);

  /* cleanup */
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(sink);
  g_object_unref(src2);
  g_object_unref(src1);
}
BT_END_TEST


TCase *bt_machine_example_case(void) {
  TCase *tc = tcase_create("BtMachineExamples");

  tcase_add_test(tc,test_btmachine_obj1);
  tcase_add_test(tc,test_btmachine_enable_input_level1);
  tcase_add_test(tc,test_btmachine_enable_input_level2);
  tcase_add_test(tc,test_btmachine_enable_input_gain1);
  tcase_add_test(tc,test_btmachine_enable_output_gain1);
  tcase_add_test(tc,test_btmachine_add_pattern);
  tcase_add_test(tc,test_btmachine_pattern_names);
  tcase_add_test(tc,test_btmachine_check_voices);
  tcase_add_test(tc,test_btmachine_change_voices);
  tcase_add_test(tc,test_btmachine_state_mute_no_sideeffects);
  tcase_add_test(tc,test_btmachine_state_solo_unmutes_others);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
