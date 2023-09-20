/* Buzztrax
 * Copyright (C) 2016 Stefan Sauer <ensonic@users.sf.net>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "m-bml.h"

//-- globals


//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
  bml_setup ();
  bmln_set_master_info (120, 4, 44100);
}

static void
test_setup (void)
{
}

static void
test_teardown (void)
{
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bmln_get_machine_info)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BuzzMachineHandle *bmh = bmln_open (".libs/libTestBmGenerator.so");

  GST_INFO ("-- act --");
  int val;
  int ret = bmln_get_machine_info (bmh, BM_PROP_TYPE, &val);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ret, 1);
  ck_assert_int_eq (val, 1);    // generator

  GST_INFO ("-- cleanup --");
  bmln_close (bmh);
  BT_TEST_END;
}
END_TEST

TCase *
bml_class_example_case (void)
{
  TCase *tc = tcase_create ("BmlClassExamples");

  tcase_add_test (tc, test_bmln_get_machine_info);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
