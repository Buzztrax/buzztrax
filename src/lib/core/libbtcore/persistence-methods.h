/* $Id: persistence-methods.h,v 1.2 2006-02-28 22:26:46 ensonic Exp $
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
extern const gchar *bt_persistence_strfmt_long(glong val);
extern const gchar *bt_persistence_strfmt_ulong(gulong val);

// list helper
extern gboolean bt_persistence_save_list(const GList *list,xmlDocPtr doc, xmlNodePtr parent_node);

// wrapper
extern gboolean bt_persistence_save(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection);
extern gboolean bt_persistence_load(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceLocation *location);

#endif // BT_PERSISTENCE_METHDOS_H
