/* Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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
/**
 * SECTION:bttestapplication
 * @short_description: unit test buzztard application
 *
 * Dummy application subclass.
 */

#define BT_CORE
#define BT_TEST_APPLICATION_C

#include "bt-check.h"

#include "core.h"
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
BtApplication *bt_test_application_new(void) {
  return BT_APPLICATION(g_object_new(BT_TYPE_TEST_APPLICATION,NULL));
}

//-- methods

//-- wrapper

//-- class internals

static void bt_test_application_init(BtTestApplication *self) {
}

static void bt_test_application_class_init(BtTestApplicationClass *klass) {
}

