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

START_TEST (test_bt_tools_element_check0)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GList *to_check = g_list_prepend (NULL, "__ploink__");

  GST_INFO ("-- act --");
  GList *missing = bt_gst_check_elements (to_check);

  GST_INFO ("-- assert --");
  ck_assert (missing != NULL);
  ck_assert_str_eq (missing->data, "__ploink__");
  ck_assert (g_list_next (missing) == NULL);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_tools_element_check1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GList *to_check = g_list_prepend (NULL, "__ploink__");
  to_check = g_list_prepend (to_check, "__bang__");

  GST_INFO ("-- act --");
  GList *missing = bt_gst_check_elements (to_check);

  GST_INFO ("-- assert --");
  ck_assert (missing != NULL);
  ck_assert (g_list_next (missing) != NULL);
  ck_assert (g_list_next (g_list_next (missing)) == NULL);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_str_parse_enum)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  gint v = bt_str_parse_enum (BT_TYPE_MACHINE_STATE, "normal");

  GST_INFO ("-- assert --");
  ck_assert_int_eq (v, BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_str_format_enum)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  const gchar *v = bt_str_format_enum (BT_TYPE_MACHINE_STATE,
      BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- assert --");
  ck_assert_str_eq (v, "normal");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_log_message_error)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstObject *src = (GstObject *) gst_element_factory_make ("fakesrc", NULL);
  GError *error = g_error_new (GST_CORE_ERROR, GST_CORE_ERROR_FAILED,
      "description");
  GstMessage *msg = gst_message_new_error (src, error, "debug");
  gchar *message, *description;

  GST_INFO ("-- act --");
  gboolean res = bt_gst_log_message_error (GST_CAT_DEFAULT, __FILE__, "",
      __LINE__, msg, &message, &description);

  GST_INFO ("-- assert --");
  ck_assert (res);
  ck_assert (description != NULL);
  ck_assert_str_eq (message, "description");

  GST_INFO ("-- cleanup --");
  g_free (message);
  g_free (description);
  gst_message_unref (msg);
  gst_object_unref (src);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_log_message_warning)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstObject *src = (GstObject *) gst_element_factory_make ("fakesrc", NULL);
  GError *error = g_error_new (GST_CORE_ERROR, GST_CORE_ERROR_FAILED,
      "description");
  GstMessage *msg = gst_message_new_warning (src, error, "debug");
  gchar *message, *description;

  GST_INFO ("-- act --");
  gboolean res = bt_gst_log_message_warning (GST_CAT_DEFAULT, __FILE__, "",
      __LINE__, msg, &message, &description);

  GST_INFO ("-- assert --");
  ck_assert (res);
  ck_assert (description != NULL);
  ck_assert_str_eq (message, "description");

  GST_INFO ("-- cleanup --");
  g_free (message);
  g_free (description);
  gst_message_unref (msg);
  gst_object_unref (src);
  BT_TEST_END;
}
END_TEST

TCase *
bt_tools_example_case (void)
{
  TCase *tc = tcase_create ("BtToolsExamples");

  tcase_add_test (tc, test_bt_tools_element_check0);
  tcase_add_test (tc, test_bt_tools_element_check1);
  tcase_add_test (tc, test_bt_str_parse_enum);
  tcase_add_test (tc, test_bt_str_format_enum);
  tcase_add_test (tc, test_bt_log_message_error);
  tcase_add_test (tc, test_bt_log_message_warning);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
