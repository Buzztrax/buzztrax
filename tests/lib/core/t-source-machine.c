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

/* create a machine with not exising plugin name */
BT_START_TEST(test_btsourcemachine_wrong_name) {
  /* arrange */

  /* act */
  GError *err=NULL;
  BtSourceMachine *machine=bt_source_machine_new(song,"id","nonsense",1,&err);

  /* assert */
  fail_unless(machine!=NULL, NULL);
  fail_unless(err!=NULL, NULL);

  /* cleanup */
}
BT_END_TEST


/* create a machine which is a sink machine and not a source machine */
BT_START_TEST(test_btsourcemachine_wrong_type) {
  /* arrange */

  /*act */
  GError *err=NULL;
  BtSourceMachine *machine=bt_source_machine_new(song,"id","autoaudiosink",1,&err);

  /* assert */
  fail_unless(machine!=NULL, NULL);
  fail_unless(err!=NULL, NULL);

  /* cleanup */
}
BT_END_TEST


// FIXME(ensonic): move to parame group tests */
BT_START_TEST(test_btsourcemachine_invalid_param) {
  /* arrange */
  BtSourceMachine *machine=bt_source_machine_new(song,"id","audiotestsrc",0,NULL);
  BtParameterGroup *pg=bt_machine_get_global_param_group(BT_MACHINE(machine));

  /* act && assert */
  ck_assert_int_eq(bt_parameter_group_get_param_index(pg,"nonsense"), -1);

  /* cleanup */
  g_object_unref(machine);
}
BT_END_TEST


TCase *bt_source_machine_test_case(void) {
  TCase *tc = tcase_create("BtSourceMachineTests");

  tcase_add_test(tc,test_btsourcemachine_wrong_name);
  tcase_add_test(tc,test_btsourcemachine_wrong_type);
  tcase_add_test(tc,test_btsourcemachine_invalid_param);
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture(tc, case_setup, case_teardown);
  return(tc);
}
