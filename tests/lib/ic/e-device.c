/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

#include "m-bt-ic.h"

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
test_btic_device_lookup (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtIcRegistry *registry = btic_registry_new ();

  /* act */
  GList *devices =
      (GList *) check_gobject_get_ptr_property (registry, "devices");
  BtIcDevice *device = (BtIcDevice *) devices->data;

  /* assert */
  fail_unless (device != NULL, NULL);

  /* cleanup */
  g_object_checked_unref (registry);
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
