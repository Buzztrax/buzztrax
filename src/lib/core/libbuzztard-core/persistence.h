/* $Id$
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

#ifndef BT_PERSISTENCE_H
#define BT_PERSISTENCE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PERSISTENCE               (bt_persistence_get_type())
#define BT_PERSISTENCE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PERSISTENCE, BtPersistence))
#define BT_IS_PERSISTENCE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PERSISTENCE))
#define BT_PERSISTENCE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BT_TYPE_PERSISTENCE, BtPersistenceInterface))

/* type macros */

typedef struct _BtPersistence BtPersistence; /* dummy object */
typedef struct _BtPersistenceInterface BtPersistenceInterface;

struct _BtPersistenceInterface {
  const GTypeInterface parent;

  xmlNodePtr (*save)(const BtPersistence * const self, xmlNodePtr const node);
  BtPersistence* (*load)(const GType type, const BtPersistence * const self, xmlNodePtr node, GError **err, va_list var_args);
};

GType bt_persistence_get_type(void) G_GNUC_CONST;

#endif // BT_PERSISTENCE_H
