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

#ifndef BT_PERSISTENCE_METHODS_H
#define BT_PERSISTENCE_METHODS_H

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include "persistence-selection.h"
#include "persistence-location.h"
#include "persistence.h"


// string formatting helper
extern const gchar *bt_persistence_strfmt_uchar(const guchar val);
extern const gchar *bt_persistence_strfmt_long(const glong val);
extern const gchar *bt_persistence_strfmt_ulong(const gulong val);
extern const gchar *bt_persistence_strfmt_double(const gdouble val);

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
extern xmlNodePtr bt_persistence_save(const BtPersistence * const self, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection);
extern gboolean bt_persistence_load(const BtPersistence * const self, xmlNodePtr node, const BtPersistenceLocation * const location);

#endif // BT_PERSISTENCE_METHDOS_H
