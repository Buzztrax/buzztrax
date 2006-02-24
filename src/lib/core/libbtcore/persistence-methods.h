/* $Id: persistence-methods.h,v 1.1 2006-02-24 19:51:49 ensonic Exp $
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
 
extern gboolean bt_persistence_save(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection);
extern gboolean bt_persistence_load(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceLocation *location);

#endif // BT_PERSISTENCE_METHDOS_H
