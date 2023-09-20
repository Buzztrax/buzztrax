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
static BtSong *song;

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
  song = bt_song_new (app);
}

static void
test_teardown (void)
{
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_child_proxy_get)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gulong val = 0;

  GST_INFO ("-- act --");
  bt_child_proxy_get (song, "foo::bar", &val, NULL);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (val, 0);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_child_proxy_set)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_child_proxy_set (song, "foo::bar", 0, NULL);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_child_proxy_get_property)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GValue value = { 0, }
END_TEST;
  g_value_init (&value, G_TYPE_ULONG);
  g_value_set_ulong (&value, 0);

  GST_INFO ("-- act --");
  bt_child_proxy_get_property ((GObject *) song, "foo::bar", &value);
  gulong val = g_value_get_ulong (&value);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (val, 0);

  GST_INFO ("-- cleanup --");
  g_value_unset (&value);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_child_proxy_set_property)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GValue value = { 0, }
END_TEST;
  g_value_init (&value, G_TYPE_ULONG);
  g_value_set_ulong (&value, 0);

  GST_INFO ("-- act --");
  bt_child_proxy_set_property ((GObject *) song, "foo::bar", &value);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  g_value_unset (&value);
  BT_TEST_END;
}
END_TEST

TCase *
bt_child_proxy_test_case (void)
{
  TCase *tc = tcase_create ("BtChildProxytest");

  tcase_add_test (tc, test_bt_child_proxy_get);
  tcase_add_test (tc, test_bt_child_proxy_set);
  tcase_add_test (tc, test_bt_child_proxy_get_property);
  tcase_add_test (tc, test_bt_child_proxy_set_property);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
