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

#ifndef BT_PERSISTENCE_H
#define BT_PERSISTENCE_H

#include <glib.h>
#include <glib-object.h>
#include <libxml/tree.h>

#define BT_TYPE_PERSISTENCE               (bt_persistence_get_type())
#define BT_PERSISTENCE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PERSISTENCE, BtPersistence))
#define BT_IS_PERSISTENCE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PERSISTENCE))
#define BT_PERSISTENCE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BT_TYPE_PERSISTENCE, BtPersistenceInterface))

/* type macros */

/**
 * BtPersistence:
 *
 * Opaque interface handle.
 */
typedef struct _BtPersistence BtPersistence;
typedef struct _BtPersistenceInterface BtPersistenceInterface;

/**
 * BtPersistenceInterface:
 * @save: virtual method to serialize an object to an xml node.
 *        "userdata" may be used to pass subclass-specific inputs to the save
 *        operation. The meaning of this data differs from subclass to subclass.
 * @load: virtual method to deserialze an object from an xml node
 *
 * #BtPersistence interface
 */
struct _BtPersistenceInterface {
  /*< private >*/
  const GTypeInterface parent;

  /*< public >*/
  /* virtual methods */
  xmlNodePtr (*save)(const BtPersistence * const self, xmlNodePtr const node, gpointer const userdata);
  BtPersistence* (*load)(const GType type, BtPersistence * const self, xmlNodePtr node, GError **err, va_list var_args);
};

GType bt_persistence_get_type(void) G_GNUC_CONST;

// list helper
gboolean bt_persistence_save_list(const GList *list, xmlNodePtr const node, gpointer const userdata);
gboolean bt_persistence_save_list_model(GListModel *list, xmlNodePtr const node, gpointer const userdata);

// hashtable helper
void bt_persistence_collect_hashtable_entries(gpointer const key, gpointer const value, gpointer const user_data);
gboolean bt_persistence_save_hashtable(GHashTable *hashtable, xmlNodePtr const node);
gboolean bt_persistence_load_hashtable(GHashTable *hashtable, xmlNodePtr node);

// wrapper
xmlNodePtr bt_persistence_save(const BtPersistence * const self, xmlNodePtr const parent_node, gpointer const userdata);
BtPersistence *bt_persistence_load(const GType type, BtPersistence * const self, xmlNodePtr node, GError **err, ...);

#endif // BT_PERSISTENCE_H
