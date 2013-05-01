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
test_btic_learn_register_control (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  GList *list = (GList *) check_gobject_get_ptr_property (registry, "devices");
  BtIcDevice *device = NULL;
  for (; list; list = g_list_next (list)) {
    if (BTIC_IS_TEST_DEVICE (list->data)) {
      device = (BtIcDevice *) list->data;
    }
  }

  /* act */
  BtIcControl *control =
      btic_learn_register_learned_control (BTIC_LEARN (device), "learn1");

  /* assert */
  fail_unless (control != NULL, NULL);

  /* cleanup */
  BT_TEST_END;
}

TCase *
bt_learn_example_case (void)
{
  TCase *tc = tcase_create ("BticLearnExamples");

  tcase_add_test (tc, test_btic_learn_register_control);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
