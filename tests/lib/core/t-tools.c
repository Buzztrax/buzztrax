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
  bt_init(&test_argc,&test_argvptr);
  bt_core_setup();
}

static void test_teardown(void) {
  bt_core_teardown();
}

//-- tests

BT_START_TEST(test_bttools_element_check) {
  GList *list = bt_gst_check_elements(NULL);
  fail_unless(list==NULL,NULL);  
}
BT_END_TEST


TCase *bt_tools_test_case(void) {
  TCase *tc = tcase_create("BtToolsTests");

  tcase_add_test(tc,test_bttools_element_check);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
