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

#include "m-bt-ic.h"

//-- globals

static BtIcRegistry *registry;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  registry = btic_registry_new ();
  btic_registry_add_device ((BtIcDevice *) btic_test_device_new ("test"));
}

static void
test_teardown (void)
{
  g_object_checked_unref (registry);
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_btic_device_lookup (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GList *devices =
      (GList *) check_gobject_get_ptr_property (registry, "devices");
  BtIcDevice *device = (BtIcDevice *) devices->data;

  GST_INFO ("-- assert --");
  fail_unless (device != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

TCase *
bt_device_example_case (void)
{
  TCase *tc = tcase_create ("BticDeviceExamples");

  tcase_add_test (tc, test_btic_device_lookup);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
