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

/* this is an abstract base class, which should not be instantiable
 * unfortunately glib crashes here: http://bugzilla.gnome.org/show_bug.cgi?id=677835
 */
#ifdef __CHECK_DISABLED__
BT_START_TEST(test_btmachine_abstract) {
  /* arrage */
  check_init_error_trapp(NULL, "cannot create instance of abstract (non-instantiatable) type");
  
  /* act */
  BtMachine *machine=g_object_new(BT_TYPE_MACHINE,NULL);
  
  /* assert */
  fail_unless(machine==NULL,NULL);
  fail_unless(check_has_error_trapped());
}
BT_END_TEST
#endif

// FIXME(ensonic): is this really testing something?
BT_START_TEST(test_btmachine_names) {
  GError *err=NULL;
  // machines
  BtSourceMachine *sine1=NULL;
  BtSourceMachine *sine2=NULL;
  BtSinkMachine *sink=NULL;
  // wires
  BtWire *wire_sine1_sink=NULL;
  BtWire *wire_sine2_sink=NULL;

  /* arrange */
  sine1=bt_source_machine_new(song,"sine1","audiotestsrc",0L,NULL);
  sine2=bt_source_machine_new(song,"sine2","audiotestsrc",0L,NULL);
  sink=bt_sink_machine_new(song,"sink",&err);
  mark_point();

  /* create wires */
  g_object_set(sine1, "id", "beep1", NULL);
  g_object_set(sine2, "id", "beep2", NULL);
  wire_sine2_sink=bt_wire_new(song,BT_MACHINE(sine2),BT_MACHINE(sink),&err);
  wire_sine1_sink=bt_wire_new(song,BT_MACHINE(sine1),BT_MACHINE(sink),&err);
  mark_point();

  g_object_unref(wire_sine1_sink);
  g_object_unref(wire_sine2_sink);
  g_object_unref(sink);
  g_object_unref(sine2);
  g_object_unref(sine1);
}
BT_END_TEST

TCase *bt_machine_test_case(void) {
  TCase *tc = tcase_create("BtMachineTests");

#ifdef __CHECK_DISABLED__
  tcase_add_test(tc, test_btmachine_abstract);
#endif
  tcase_add_test(tc, test_btmachine_names);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
