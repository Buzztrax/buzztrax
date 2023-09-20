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

START_TEST (test_bmln_open_bad_path)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bml_setup ();
  bmln_set_master_info (120, 4, 44100);
  GST_INFO ("-- act --");
  BuzzMachineHandle *bmh = bmln_open ("/path/does/not.exist");
  GST_INFO ("-- assert --");
  fail_unless (bmh == NULL);
  BT_TEST_END;
}
END_TEST

TCase *
bml_core_test_case (void)
{
  TCase *tc = tcase_create ("BmlCoreTests");

  tcase_add_test (tc, test_bmln_open_bad_path);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
