/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

static void suite_setup(void) {
  bt_init(&test_argc,&test_argvptr);
  bt_core_setup();
}

static void suite_teardown(void) {
  bt_core_teardown();
}


//-- tests

BT_START_TEST(test_btapplication_new) {
  /* arrange */
  BtApplication *app=bt_test_application_new();

  /* act */
  GstElement *bin;
  g_object_get(app,"bin",&bin,NULL);

  /* assert */
  ck_assert_int_eq(GST_BIN_NUMCHILDREN(bin),0);
  
  /* cleanup */
  gst_object_unref(bin);
  g_object_checked_unref(app);
}
BT_END_TEST



TCase *bt_application_example_case(void) {
  TCase *tc = tcase_create("BtApplicationExamples");

  tcase_add_test(tc,test_btapplication_new);
  tcase_add_unchecked_fixture(tc,suite_setup, suite_teardown);
  return(tc);
}
