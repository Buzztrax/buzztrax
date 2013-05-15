/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_APPLICATION_H
#define BT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_APPLICATION             (bt_application_get_type ())
#define BT_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_APPLICATION, BtApplication))
#define BT_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_APPLICATION, BtApplicationClass))
#define BT_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_APPLICATION))
#define BT_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_APPLICATION))
#define BT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_APPLICATION, BtApplicationClass))

/* type macros */

typedef struct _BtApplication BtApplication;
typedef struct _BtApplicationClass BtApplicationClass;
typedef struct _BtApplicationPrivate BtApplicationPrivate;

/**
 * BtApplication:
 *
 * base object for a buzztrax based application
 */
struct _BtApplication {
  const GObject parent;
  
  /*< private >*/
  BtApplicationPrivate *priv;
};

struct _BtApplicationClass {
  const GObjectClass parent;
  
};

GType bt_application_get_type(void) G_GNUC_CONST;

#endif // BT_APPLICATION_H
