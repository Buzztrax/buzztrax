/* Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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

// test if the normal init call works with commandline arguments (no args)
static void test_btic_registry_create0 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtIcRegistry *registry = btic_registry_new ();

  /* assert */
  fail_unless (registry != NULL, NULL);

  /* cleanup */
  g_object_checked_unref (registry);
  BT_TEST_END;
}
 TCase * bt_registry_example_case (void)
{
  TCase *tc = tcase_create ("BticRegistryExamples");

  tcase_add_test (tc, test_btic_registry_create0);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
