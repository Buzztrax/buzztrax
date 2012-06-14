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

BT_START_TEST(test_btsetup_properties) {
  /* arrange */
  GObject *setup=check_gobject_get_object_property(song,"setup");

  /* act & assert */
  fail_unless(check_gobject_properties(setup), NULL);

  /* cleanup */
}
BT_END_TEST


/* create a new setup with NULL for song object */
BT_START_TEST(test_btsetup_new_null_song) {
  /* act */
  BtSetup *setup=bt_setup_new(NULL);

  /* assert */
  fail_unless(setup != NULL, NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* add the same machine twice to the setup */
BT_START_TEST(test_btsetup_add_machine_twice) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0,NULL));

  /* act & assert */
  fail_if(bt_setup_add_machine(setup,machine), NULL);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(setup);
}
BT_END_TEST


/* add the same wire twice */
BT_START_TEST(test_btsetup_add_wire_twice) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *source=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,NULL));
  BtMachine *sink=BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire=bt_wire_new(song,source,sink,NULL);

  /* act & assert */
  fail_if(bt_setup_add_wire(setup,wire), NULL);

  /* cleanup */
  g_object_unref(wire);
  g_object_unref(source);
  g_object_unref(sink);
  g_object_unref(setup);
}
BT_END_TEST


/* call bt_setup_add_machine with NULL object for self */
BT_START_TEST(test_btsetup_obj4) {
  /* arrange */
  check_init_error_trapp("bt_setup_add_machine","BT_IS_SETUP(self)");

  /* act */
  bt_setup_add_machine(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
}
BT_END_TEST


/* call bt_setup_add_machine with NULL object for machine */
BT_START_TEST(test_btsetup_obj5) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_add_machine","BT_IS_MACHINE(machine)");

  /* act */
  bt_setup_add_machine(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* call bt_setup_add_wire with NULL object for self */
BT_START_TEST(test_btsetup_obj6) {
  /* arrange */
  check_init_error_trapp("bt_setup_add_wire","BT_IS_SETUP(self)");

  /* act */
  bt_setup_add_wire(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
}
BT_END_TEST


/* call bt_setup_add_wire with NULL object for wire */
BT_START_TEST(test_btsetup_obj7) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_add_wire","BT_IS_WIRE(wire)");

  /* act */
  bt_setup_add_wire(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* call bt_setup_get_machine_by_id with NULL object for self */
BT_START_TEST(test_btsetup_obj8) {
  /* arrange */
  check_init_error_trapp("bt_setup_get_machine_by_id","BT_IS_SETUP(self)");

  /* act */
  bt_setup_get_machine_by_id(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
}
BT_END_TEST


/* call bt_setup_get_machine_by_id with NULL object for id */
BT_START_TEST(test_btsetup_obj9) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_get_machine_by_id",NULL);

  /* act */
  bt_setup_get_machine_by_id(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/*
* call bt_setup_get_wire_by_src_machine with NULL for setup parameter
*/
BT_START_TEST(test_btsetup_get_wire_by_src_machine1) {
  /* arrange */
  check_init_error_trapp("bt_setup_get_wire_by_src_machine","BT_IS_SETUP(self)");

  /* act */
  bt_setup_get_wire_by_src_machine(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
}
BT_END_TEST


/* call bt_setup_get_wire_by_src_machine with NULL for machine parameter */
BT_START_TEST(test_btsetup_get_wire_by_src_machine2) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_get_wire_by_src_machine","BT_IS_MACHINE(src)");

  /* act */
  bt_setup_get_wire_by_src_machine(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* get wires by source machine with NULL for setup */
BT_START_TEST(test_btsetup_get_wires_by_src_machine1) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtSourceMachine *src_machine=bt_source_machine_new(song,"src","buzztard-test-mono-source",0L,NULL);
  check_init_error_trapp("bt_setup_get_wires_by_src_machine","BT_IS_SETUP(self)");

  /* act */
  bt_setup_get_wires_by_src_machine(NULL,BT_MACHINE(src_machine));

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(src_machine);
  g_object_unref(setup);
}
BT_END_TEST


/* get wires by source machine with NULL for machine */
BT_START_TEST(test_btsetup_get_wires_by_src_machine2) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_get_wires_by_src_machine","BT_IS_MACHINE(src)");

  /* act */
  bt_setup_get_wires_by_src_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* get wires by source machine with a not added machine */
BT_START_TEST(test_btsetup_get_wires_by_src_machine3) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *machine=BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0L,NULL));
  bt_setup_remove_machine(setup, machine);

  /* act & assert */
  fail_if(bt_setup_get_wires_by_src_machine(setup,machine), NULL);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(setup);
}
BT_END_TEST


/* call bt_setup_get_wire_by_dst_machine with NULL for setup parameter */
BT_START_TEST(test_btsetup_get_wire_by_dst_machine1) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_get_wire_by_dst_machine","BT_IS_SETUP(self)");

  /* act */
  bt_setup_get_wire_by_dst_machine(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* call bt_setup_get_wire_by_dst_machine with NULL for machine parameter */
BT_START_TEST(test_btsetup_get_wire_by_dst_machine2) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_get_wire_by_dst_machine","BT_IS_MACHINE(dst)");

  /* act */
  bt_setup_get_wire_by_dst_machine(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* get wires by destination machine with NULL for setup */
BT_START_TEST(test_btsetup_get_wires_by_dst_machine1) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *machine=BT_MACHINE(bt_sink_machine_new(song,"dst",NULL));
  check_init_error_trapp("bt_setup_get_wires_by_dst_machine","BT_IS_SETUP(self)");

  /* act */
  bt_setup_get_wires_by_dst_machine(NULL,machine);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(setup);
}
BT_END_TEST


/* get wires by sink machine with NULL for machine */
BT_START_TEST(test_btsetup_get_wires_by_dst_machine2) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_get_wires_by_dst_machine","BT_IS_MACHINE(dst)");

  /* act */
  bt_setup_get_wires_by_dst_machine(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* get wires by sink machine with a not added machine */
BT_START_TEST(test_btsetup_get_wires_by_dst_machine3) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *machine=BT_MACHINE(bt_sink_machine_new(song,"dst",NULL));
  bt_setup_remove_machine(setup, machine);

  /* act & assert */
  fail_if(bt_setup_get_wires_by_dst_machine(setup,machine), NULL);

  /* cleanup */
  g_object_unref(machine);
  g_object_unref(setup);
}
BT_END_TEST


/* remove a machine from setup with NULL pointer for setup */
BT_START_TEST(test_btsetup_remove_machine_null_setup) {
  /* arrange */
  check_init_error_trapp("bt_setup_remove_machine",NULL);

  /* act */
  bt_setup_remove_machine(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
}
BT_END_TEST


/* remove a wire from setup with NULL pointer for setup */
BT_START_TEST(test_btsetup_remove_wire_null_setup) {
  /* arrange */
  check_init_error_trapp("bt_setup_remove_wire",NULL);

  /* act */
  bt_setup_remove_wire(NULL,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
}
BT_END_TEST


/* remove a machine from setup with NULL pointer for machine */
BT_START_TEST(test_btsetup_remove_null_machine) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_remove_machine",NULL);

  /* act */
  bt_setup_remove_machine(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* remove a wire from setup with NULL pointer for wire */
BT_START_TEST(test_btsetup_remove_null_wire) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  check_init_error_trapp("bt_setup_remove_wire",NULL);

  /* act */
  bt_setup_remove_wire(setup,NULL);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(setup);
}
BT_END_TEST


/* remove a machine from setup with a machine witch is never added */
BT_START_TEST(test_btsetup_remove_machine_twice) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *gen = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0L,NULL));
  bt_setup_remove_machine(setup,gen);
  check_init_error_trapp("bt_setup_remove_machine", "trying to remove machine that is not in setup");

  /* act */
  bt_setup_remove_machine(setup,gen);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(gen);
  g_object_unref(setup);
}
BT_END_TEST


/* remove a wire from setup with a wire which is not added */
BT_START_TEST(test_btsetup_remove_wire_twice) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *gen = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0L,NULL));
  BtMachine *sink=BT_MACHINE(bt_sink_machine_new(song,"dst",NULL));
  BtWire *wire = bt_wire_new(song,gen,sink,NULL);
  bt_setup_remove_wire(setup,wire);
  check_init_error_trapp("bt_setup_remove_wire", "trying to remove wire that is not in setup");

  /* act */
  bt_setup_remove_wire(setup,wire);

  /* assert */
  fail_unless(check_has_error_trapped(), NULL);

  /* cleanup */
  g_object_unref(gen);
  g_object_unref(sink);
  g_object_unref(wire);
  g_object_unref(setup);
}
BT_END_TEST


/* add wire(src,dst) and wire(dst,src) to setup. This should fail (cycle). */
BT_START_TEST(test_btsetup_wire_cycle1) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *src = BT_MACHINE(bt_processor_machine_new(song,"src","volume",0L,NULL));
  BtMachine *dst = BT_MACHINE(bt_processor_machine_new(song,"src","volume",0L,NULL));
  BtWire *wire1 = bt_wire_new(song,src,dst,NULL);

  /* act */
  GError *err=NULL;
  BtWire *wire2 = bt_wire_new(song,dst,src,&err);

  /* assert */
  fail_unless(wire2!=NULL,NULL);
  fail_unless(err!=NULL, NULL);

  /* cleanup */
  g_object_unref(src);
  g_object_unref(dst);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(setup);
}
BT_END_TEST


// test graph cycles
BT_START_TEST(test_btsetup_wire_cycle2) {
  /* arrange */
  BtSetup *setup=(BtSetup *)check_gobject_get_object_property(song,"setup");
  BtMachine *elem1 = BT_MACHINE(bt_processor_machine_new(song,"src1","volume",0L,NULL));
  BtMachine *elem2 = BT_MACHINE(bt_processor_machine_new(song,"src2","volume",0L,NULL));
  BtMachine *elem3 = BT_MACHINE(bt_processor_machine_new(song,"src3","volume",0L,NULL));
  BtMachine *sink=BT_MACHINE(bt_sink_machine_new(song,"sink",NULL));
  BtWire *wire0 = bt_wire_new(song, elem3, sink, NULL);
  BtWire *wire1 = bt_wire_new(song, elem1, elem2, NULL);
  BtWire *wire2 = bt_wire_new(song, elem2, elem3, NULL);

  /* act */
  GError *err=NULL;
  BtWire *wire3 = bt_wire_new(song, elem3, elem1, &err);

  /* assert */
  fail_unless(wire3!=NULL,NULL);
  fail_unless(err!=NULL, NULL);

  /* cleanup */
  g_object_unref(elem1);
  g_object_unref(elem2);
  g_object_unref(elem3);
  g_object_unref(wire0);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(wire3);
  g_object_unref(setup);
}
BT_END_TEST


TCase *bt_setup_test_case(void) {
  TCase *tc = tcase_create("BtSetupTests");

  tcase_add_test(tc,test_btsetup_properties);
  tcase_add_test(tc,test_btsetup_new_null_song);
  tcase_add_test(tc,test_btsetup_add_machine_twice);
  tcase_add_test(tc,test_btsetup_add_wire_twice);
  tcase_add_test(tc,test_btsetup_obj4);
  tcase_add_test(tc,test_btsetup_obj5);
  tcase_add_test(tc,test_btsetup_obj6);
  tcase_add_test(tc,test_btsetup_obj7);
  tcase_add_test(tc,test_btsetup_obj8);
  tcase_add_test(tc,test_btsetup_obj9);
  tcase_add_test(tc,test_btsetup_get_wire_by_src_machine1);
  tcase_add_test(tc,test_btsetup_get_wire_by_src_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_src_machine1);
  tcase_add_test(tc,test_btsetup_get_wires_by_src_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_src_machine3);
  tcase_add_test(tc,test_btsetup_get_wire_by_dst_machine1);
  tcase_add_test(tc,test_btsetup_get_wire_by_dst_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_dst_machine1);
  tcase_add_test(tc,test_btsetup_get_wires_by_dst_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_dst_machine3);
  tcase_add_test(tc,test_btsetup_remove_machine_null_setup);
  tcase_add_test(tc,test_btsetup_remove_wire_null_setup);
  tcase_add_test(tc,test_btsetup_remove_null_machine);
  tcase_add_test(tc,test_btsetup_remove_null_wire);
  tcase_add_test(tc,test_btsetup_remove_machine_twice);
  tcase_add_test(tc,test_btsetup_remove_wire_twice);
  tcase_add_test(tc,test_btsetup_wire_cycle1);
  tcase_add_test(tc,test_btsetup_wire_cycle2);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
