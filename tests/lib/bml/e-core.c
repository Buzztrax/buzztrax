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

static void
test_bml_setup (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  int res = bml_setup ();
  GST_INFO ("-- assert --");
  fail_unless (res, NULL);
  BT_TEST_END;
}

static void
test_bml_finalize (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bml_setup ();
  GST_INFO ("-- act --");
  bml_finalize ();
  BT_TEST_END;
}

static void
test_bmln_master_info (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bml_setup ();
  GST_INFO ("-- act --");
  bmln_set_master_info (120, 4, 44100);
  BT_TEST_END;
}

TCase *
bml_core_example_case (void)
{
  TCase *tc = tcase_create ("BmlCoreExamples");

  tcase_add_test (tc, test_bml_setup);
  tcase_add_test (tc, test_bml_finalize);
  // TODO: extract into a separate suite
  tcase_add_test (tc, test_bmln_master_info);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
