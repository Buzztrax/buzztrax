/* Buzztrax
 * Copyright (C) 2016 Buzztrax team <buzztrax-devel@buzztrax.org>
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

static BtApplication *app;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
}

static void
test_teardown (void)
{
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

static void
test_bt_child_proxy_get (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gulong length;

  GST_INFO ("-- act --");
  bt_child_proxy_get (app, "song::sequence::length", &length, NULL);

  GST_INFO ("-- assert --");
  ck_assert_int_gt (length, 0);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_child_proxy_set (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gboolean loop1, loop2;
  bt_child_proxy_get (app, "song::sequence::loop", &loop1, NULL);

  GST_INFO ("-- act --");
  bt_child_proxy_set (app, "song::sequence::loop", !loop1, NULL);

  GST_INFO ("-- assert --");
  bt_child_proxy_get (app, "song::sequence::loop", &loop2, NULL);
  ck_assert_int_ne (loop1, loop2);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

TCase *
bt_child_proxy_example_case (void)
{
  TCase *tc = tcase_create ("BtChildProxyExamples");

  tcase_add_test (tc, test_bt_child_proxy_get);
  tcase_add_test (tc, test_bt_child_proxy_set);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
