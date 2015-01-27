/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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
/**
 * SECTION:bttestapplication
 * @short_description: unit test buzztrax application
 *
 * Dummy application subclass.
 */

#define BT_CORE
#define BT_TEST_APPLICATION_C

#include "bt-check.h"

#include "core/core.h"
#include "bt-test-application.h"

//-- the class

G_DEFINE_TYPE (BtTestApplication, bt_test_application, BT_TYPE_APPLICATION);

//-- constructor methods

/**
 * bt_test_application_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance
 */
BtApplication *
bt_test_application_new (void)
{
  return BT_APPLICATION (g_object_new (BT_TYPE_TEST_APPLICATION, NULL));
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_test_application_init (BtTestApplication * self)
{
}

static void
bt_test_application_class_init (BtTestApplicationClass * klass)
{
}
