/* $Id: persistence-location.h,v 1.2 2006-08-24 20:00:52 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_PERSISTENCE_LOCATION_H
#define BT_PERSISTENCE_LOCATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PERSISTENCE_LOCATION            (bt_persistence_location_get_type ())
#define BT_PERSISTENCE_LOCATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PERSISTENCE_LOCATION, BtPersistenceLocation))
#define BT_PERSISTENCE_LOCATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PERSISTENCE_LOCATION, BtPersistenceLocationClass))
#define BT_IS_PERSISTENCE_LOCATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PERSISTENCE_LOCATION))
#define BT_IS_PERSISTENCE_LOCATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PERSISTENCE_LOCATION))
#define BT_PERSISTENCE_LOCATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PERSISTENCE_LOCATION, BtPersistenceLocationClass))

/* type macros */

typedef struct _BtPersistenceLocation BtPersistenceLocation;
typedef struct _BtPersistenceLocationClass BtPersistenceLocationClass;

/**
 * BtPersistenceSelection:
 */
struct _BtPersistenceLocation {
  GObject parent;
};

struct _BtPersistenceLocationClass {
  GObjectClass parent;
};

GType bt_persistence_location_get_type(void) G_GNUC_CONST;


#endif // BT_PERSISTENCE_LOCATION_H
