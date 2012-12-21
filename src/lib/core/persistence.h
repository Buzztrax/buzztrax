/* Buzztard
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_PERSISTENCE_H
#define BT_PERSISTENCE_H

#include <glib.h>
#include <glib-object.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
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
 * @save: virtual method to serialize an object to an xml node
 * @load: virtual method to deserialze an object from an xml node
 *
 * #BtPersistence interface
 */
struct _BtPersistenceInterface {
  /*< private >*/
  const GTypeInterface parent;

  /*< public >*/
  /* virtual methods */
  xmlNodePtr (*save)(const BtPersistence * const self, xmlNodePtr const node);
  BtPersistence* (*load)(const GType type, const BtPersistence * const self, xmlNodePtr node, GError **err, va_list var_args);
};

GType bt_persistence_get_type(void) G_GNUC_CONST;

// string formatting helper
extern const gchar *bt_persistence_strfmt_uchar(const guchar val);
extern const gchar *bt_persistence_strfmt_long(const glong val);
extern const gchar *bt_persistence_strfmt_ulong(const gulong val);
extern const gchar *bt_persistence_strfmt_double(const gdouble val);
extern const gchar *bt_persistence_strfmt_enum(GType enum_type,gint value);

// string parsing helper
extern gint bt_persistence_parse_enum(GType enum_type,const gchar *str);

// list helper
extern gboolean bt_persistence_save_list(const GList *list, xmlNodePtr const node);

// hashtable helper
extern void bt_persistence_collect_hashtable_entries(gpointer const key, gpointer const value, gpointer const user_data);
extern gboolean bt_persistence_save_hashtable(GHashTable *hashtable, xmlNodePtr const node);
extern gboolean bt_persistence_load_hashtable(GHashTable *hashtable, xmlNodePtr node);

// gvalue helper
extern gboolean bt_persistence_set_value(GValue* const gvalue, const gchar * const svalue);
extern gchar *bt_persistence_get_value(GValue * const gvalue);

// wrapper
extern xmlNodePtr bt_persistence_save(const BtPersistence * const self, xmlNodePtr const parent_node);
extern BtPersistence *bt_persistence_load(const GType type, const BtPersistence * const self, xmlNodePtr node, GError **err, ...);

#endif // BT_PERSISTENCE_H
