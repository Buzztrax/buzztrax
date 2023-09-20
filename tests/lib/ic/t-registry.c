/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-ic.h"


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

START_TEST (test_btic_registry_unknown_device)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtIcRegistry *registry = btic_registry_new ();
  btic_registry_add_device ((BtIcDevice *) btic_test_device_new ("test"));

  GST_INFO ("-- act --");
  BtIcDevice *device = btic_registry_get_device_by_name ("unknown-device");

  GST_INFO ("-- assert --");
  ck_assert (device == NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (registry);
  BT_TEST_END;
}
END_TEST


TCase *
bt_registry_test_case (void)
{
  TCase *tc = tcase_create ("BticRegistryTests");

  tcase_add_test (tc, test_btic_registry_unknown_device);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
