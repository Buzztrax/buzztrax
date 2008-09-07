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

/**
 * BT_PERSISTENCE_ERROR:
 * @label: label to jump to
 *
 * depending on the configuration aborts the loading by jumping to the given
 * @label.
 */
/* uncomment this to make loading files fail on warnings
#define BT_PERSISTENCE_ERROR(label) goto label
*/
/* this is a hack to avoid undefined label warnings */
#define BT_PERSISTENCE_ERROR(label) if(0) goto label

/* type macros */

typedef struct _BtPersistence BtPersistence; /* dummy object */
typedef struct _BtPersistenceInterface BtPersistenceInterface;

struct _BtPersistenceInterface {
  const GTypeInterface parent;

  xmlNodePtr (*save)(const BtPersistence * const self, xmlNodePtr const node, const BtPersistenceSelection * const selection);
  gboolean (*load)(const BtPersistence * const self, xmlNodePtr node, const BtPersistenceLocation * const location);
};

GType bt_persistence_get_type(void) G_GNUC_CONST;

#endif // BT_PERSISTENCE_H
