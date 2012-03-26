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

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
}

//-- tests

BT_START_TEST(test_bttools_element_check0) {
  GList *to_check=g_list_prepend(NULL,"__ploink__");
  GList *missing=bt_gst_check_elements(to_check);
  fail_unless(missing!=NULL,NULL);
  fail_unless(!strcmp(missing->data,"__ploink__"),NULL);
  fail_unless(g_list_next(missing)==NULL,NULL);
}
BT_END_TEST


BT_START_TEST(test_bttools_element_check1) {
  GList *to_check=g_list_prepend(NULL,"__ploink__");
  to_check=g_list_prepend(to_check,"__bang__");

  GList *missing=bt_gst_check_elements(to_check);
  fail_unless(missing!=NULL,NULL);
  fail_unless(g_list_next(missing)!=NULL,NULL);
  fail_unless(g_list_next(g_list_next(missing))==NULL,NULL);
}
BT_END_TEST


TCase *bt_tools_example_case(void) {
  TCase *tc = tcase_create("BtToolsExamples");

  tcase_add_test(tc,test_bttools_element_check0);
  tcase_add_test(tc,test_bttools_element_check1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
