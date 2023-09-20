/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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

START_TEST (test_bt_tools_element_check)
{
  BT_TEST_START;
  GList *list = bt_gst_check_elements (NULL);
  ck_assert (list == NULL);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_str_parse_enum_wrong_type)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  gint v = bt_str_parse_enum (G_TYPE_INVALID, "hello");

  /*assert */
  ck_assert_int_eq (v, -1);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_str_parse_enum_null_str)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  gint v = bt_str_parse_enum (BT_TYPE_MACHINE_STATE, NULL);

  /*assert */
  ck_assert_int_eq (v, -1);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_str_format_enum_wrong_type)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  const gchar *v = bt_str_format_enum (G_TYPE_INVALID, 1);

  /*assert */
  ck_assert (v == NULL);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_str_format_enum_wrong_value)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  const gchar *v = bt_str_format_enum (BT_TYPE_MACHINE_STATE,
      BT_MACHINE_STATE_COUNT);

  /*assert */
  ck_assert (v == NULL);
  BT_TEST_END;
}
END_TEST

static GstMessage *
make_msg (void)
{
  GstObject *src = (GstObject *) gst_element_factory_make ("fakesrc", NULL);
  GError *error = g_error_new (GST_CORE_ERROR, GST_CORE_ERROR_FAILED,
      "description");
  GstMessage *msg = gst_message_new_info (src, error, "debug");
  gst_object_unref (src);

  return msg;
}

START_TEST (test_bt_log_message_error_wrong_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstMessage *msg = make_msg ();

  GST_INFO ("-- act --");
  gboolean res = bt_gst_log_message_error (GST_CAT_DEFAULT, "", "", 0, msg,
      NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (!res);

  GST_INFO ("-- cleanup --");
  gst_message_unref (msg);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_log_message_warning_wrong_type)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstMessage *msg = make_msg ();

  GST_INFO ("-- act --");
  gboolean res = bt_gst_log_message_warning (GST_CAT_DEFAULT, "", "", 0, msg,
      NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (!res);

  GST_INFO ("-- cleanup --");
  gst_message_unref (msg);
  BT_TEST_END;
}
END_TEST


TCase *
bt_tools_test_case (void)
{
  TCase *tc = tcase_create ("BtToolsTests");

  tcase_add_test (tc, test_bt_tools_element_check);
  tcase_add_test (tc, test_bt_str_parse_enum_wrong_type);
  tcase_add_test (tc, test_bt_str_parse_enum_null_str);
  tcase_add_test (tc, test_bt_str_format_enum_wrong_type);
  tcase_add_test (tc, test_bt_str_format_enum_wrong_value);
  tcase_add_test (tc, test_bt_log_message_error_wrong_type);
  tcase_add_test (tc, test_bt_log_message_warning_wrong_type);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
