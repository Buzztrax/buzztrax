/* $Id: persistence-methods.h,v 1.9 2006-05-25 16:29:18 ensonic Exp $
 * defines all public methods of the io interface
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
