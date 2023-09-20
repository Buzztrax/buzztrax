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
  bt_child_proxy_set (song, "sequence::length", 16, "sequence::loop", FALSE,
      NULL);
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
  gulong length;

  GST_INFO ("-- act --");
  bt_child_proxy_get (song, "sequence::length", &length, NULL);

  GST_INFO ("-- assert --");
  ck_assert_int_gt (length, 0);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_child_proxy_set)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gboolean loop;

  GST_INFO ("-- act --");
  bt_child_proxy_set (song, "sequence::loop", TRUE, NULL);

  GST_INFO ("-- assert --");
  bt_child_proxy_get (song, "sequence::loop", &loop, NULL);
  ck_assert_int_eq (TRUE, loop);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_child_proxy_get_property)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GValue value = { 0, };
  g_value_init (&value, G_TYPE_ULONG);

  GST_INFO ("-- act --");
  bt_child_proxy_get_property ((GObject *) song, "sequence::length", &value);
  gulong length = g_value_get_ulong (&value);

  GST_INFO ("-- assert --");
  ck_assert_int_gt (length, 0);

  GST_INFO ("-- cleanup --");
  g_value_unset (&value);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_child_proxy_set_property)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gboolean loop;
  GValue value = { 0, };
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, TRUE);

  GST_INFO ("-- act --");
  bt_child_proxy_set_property ((GObject *) song, "sequence::loop", &value);

  GST_INFO ("-- assert --");
  bt_child_proxy_get (song, "sequence::loop", &loop, NULL);
  ck_assert_int_eq (TRUE, loop);

  GST_INFO ("-- cleanup --");
  g_value_unset (&value);
  BT_TEST_END;
}
END_TEST

TCase *
bt_child_proxy_example_case (void)
{
  TCase *tc = tcase_create ("BtChildProxyExamples");

  tcase_add_test (tc, test_bt_child_proxy_get);
  tcase_add_test (tc, test_bt_child_proxy_set);
  tcase_add_test (tc, test_bt_child_proxy_get_property);
  tcase_add_test (tc, test_bt_child_proxy_set_property);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
