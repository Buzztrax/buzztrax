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
* try to create a wire with the same machine as source and dest
*/
BT_START_TEST(test_btwire_obj1){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtWire *wire=NULL;
  // machine
  BtProcessorMachine *machine=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);

  /* try to create a source machine */
  machine=bt_processor_machine_new(song,"id","volume",0);
  fail_unless(machine!=NULL,NULL);

  check_init_error_trapp("bt_wire_new","src_machine!=dst_machine");
  /* try to add the machine twice to the wire */
  wire=bt_wire_new(song,BT_MACHINE(machine),BT_MACHINE(machine));
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(wire==NULL,NULL);

  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwire_obj2){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtWire *wire1=NULL;
  BtWire *wire2=NULL;
  BtSourceMachine *source=NULL;
  BtProcessorMachine *sink1=NULL;
  BtProcessorMachine *sink2=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create a source machine */
  source=bt_source_machine_new(song,"id","audiotestsrc",0);
  fail_unless(source!=NULL,NULL);

  /* try to create a volume machine */
  sink1=bt_processor_machine_new(song,"volume1","volume",0);
  fail_unless(sink1!=NULL,NULL);

  /* try to create a volume machine */
  sink2=bt_processor_machine_new(song,"volume2","volume",0);
  fail_unless(sink2!=NULL,NULL);

  /* try to connect processor machine to volume1 */
  wire1=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink1));
  mark_point();
  fail_unless(wire1!=NULL,NULL);

  /* try to connect processor machine to volume2 */
  wire2=bt_wire_new(song,BT_MACHINE(source),BT_MACHINE(sink2));
  mark_point();
  fail_unless(wire2!=NULL,NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_wire_test_case(void) {
  TCase *tc = tcase_create("BtWireTests");

  tcase_add_test(tc,test_btwire_obj1);
  tcase_add_test(tc,test_btwire_obj2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
