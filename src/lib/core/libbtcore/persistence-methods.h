/* $Id: persistence-methods.h,v 1.10 2006-08-24 20:00:52 ensonic Exp $
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
extern const gchar *bt_persistence_strfmt_uchar(guchar val);
extern const gchar *bt_persistence_strfmt_long(glong val);
extern const gchar *bt_persistence_strfmt_ulong(gulong val);
extern const gchar *bt_persistence_strfmt_double(gdouble val);

// list helper
extern gboolean bt_persistence_save_list(const GList *list, xmlNodePtr node);

// hashtable helper
extern gboolean bt_persistence_save_hashtable(const GHashTable *hashtable, xmlNodePtr node);
extern gboolean bt_persistence_load_hashtable(GHashTable *hashtable, xmlNodePtr node);

// gvalue helper
extern gboolean bt_persistence_set_value(GValue *gvalue, const gchar *svalue);
extern gchar *bt_persistence_get_value(GValue *gvalue);

// wrapper
extern xmlNodePtr bt_persistence_save(BtPersistence *self, xmlNodePtr parent_node, BtPersistenceSelection *selection);
extern gboolean bt_persistence_load(BtPersistence *self, xmlNodePtr node, BtPersistenceLocation *location);

#endif // BT_PERSISTENCE_METHDOS_H
