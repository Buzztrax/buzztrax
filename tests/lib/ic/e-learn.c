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
  ck_g_object_final_unref (registry);
}

static void
case_teardown (void)
{
}

//-- tests

START_TEST (test_btic_learn_register_control)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtIcDevice *device = btic_registry_get_device_by_name ("test");

  GST_INFO ("-- act --");
  BtIcControl *control =
      btic_learn_register_learned_control (BTIC_LEARN (device), "learn1");

  GST_INFO ("-- assert --");
  ck_assert (control != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (device);
  BT_TEST_END;
}
END_TEST

START_TEST (test_btic_learn_store_controls)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtIcDevice *device = btic_registry_get_device_by_name ("test");
  btic_learn_register_learned_control (BTIC_LEARN (device), "learn1");

  GST_INFO ("-- act --");
  btic_learn_store_controller_map (BTIC_LEARN (device));

  GST_INFO ("-- assert --");
  ck_assert (g_file_test ("./buzztrax/controller-maps/test",
          G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));

  GST_INFO ("-- cleanup --");
  g_object_unref (device);
  BT_TEST_END;
}
END_TEST

TCase *
bt_learn_example_case (void)
{
  TCase *tc = tcase_create ("BticLearnExamples");

  tcase_add_test (tc, test_btic_learn_register_control);
  tcase_add_test (tc, test_btic_learn_store_controls);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
