/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

static void
case_setup (void)
{
  GST_INFO
      ("================================================================================");
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
test_bt_persistence_parse_enum_wrong_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  gint v = bt_persistence_parse_enum (G_TYPE_INVALID, "hello");

  /*assert */
  ck_assert_int_eq (v, -1);
  BT_TEST_END;
}

static void
test_bt_persistence_parse_enum_null_str (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  gint v = bt_persistence_parse_enum (BT_TYPE_MACHINE_STATE, NULL);

  /*assert */
  ck_assert_int_eq (v, -1);
  BT_TEST_END;
}

static void
test_bt_persistence_strfmt_enum_wrong_type (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  const gchar *v = bt_persistence_strfmt_enum (G_TYPE_INVALID, 1);

  /*assert */
  fail_unless (v == NULL, NULL);
  BT_TEST_END;
}

static void
test_bt_persistence_strfmt_enum_wrong_value (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  const gchar *v = bt_persistence_strfmt_enum (BT_TYPE_MACHINE_STATE,
      BT_MACHINE_STATE_COUNT);

  /*assert */
  fail_unless (v == NULL, NULL);
  BT_TEST_END;
}

TCase *
bt_persistence_test_case (void)
{
  TCase *tc = tcase_create ("BtPersistenceTests");

  tcase_add_test (tc, test_bt_persistence_parse_enum_wrong_type);
  tcase_add_test (tc, test_bt_persistence_parse_enum_null_str);
  tcase_add_test (tc, test_bt_persistence_strfmt_enum_wrong_type);
  tcase_add_test (tc, test_bt_persistence_strfmt_enum_wrong_value);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
