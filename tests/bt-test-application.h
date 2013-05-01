/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#ifndef BT_TEST_APPLICATION_H
#define BT_TEST_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_TEST_APPLICATION            (bt_test_application_get_type ())
#define BT_TEST_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_APPLICATION, BtTestApplication))
#define BT_TEST_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_APPLICATION, BtTestApplicationClass))
#define BT_IS_TEST_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_APPLICATION))
#define BT_IS_TEST_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_APPLICATION))
#define BT_TEST_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_APPLICATION, BtTestApplicationClass))

/* type macros */

typedef struct _BtTestApplication BtTestApplication;
typedef struct _BtTestApplicationClass BtTestApplicationClass;

/**
 * BtTestApplication:
 *
 * gconf based implementation object for a buzztrax based settings
 */
struct _BtTestApplication {
  const BtApplication parent;
};
/* structure of the gconf-settings class */
struct _BtTestApplicationClass {
  const BtApplicationClass parent;
};

BtApplication *bt_test_application_new(void);

GType bt_test_application_get_type(void) G_GNUC_CONST;

#endif // BT_TEST_APPLICATION_H
