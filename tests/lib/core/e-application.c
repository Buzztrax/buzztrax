/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#include "m-bt-core.h"

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
test_bt_application_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtApplication *app = bt_test_application_new ();

  /* act */
  GstElement *bin;
  g_object_get (app, "bin", &bin, NULL);

  /* assert */
  ck_assert_int_eq (GST_BIN_NUMCHILDREN (bin), 0);

  /* cleanup */
  gst_object_unref (bin);
  g_object_checked_unref (app);
  BT_TEST_END;
}

TCase *
bt_application_example_case (void)
{
  TCase *tc = tcase_create ("BtApplicationExamples");

  tcase_add_test (tc, test_bt_application_new);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
