/* $Id: persistence.h,v 1.1 2006-02-24 19:51:49 ensonic Exp $
 * interface for object io
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
  GTypeInterface parent;

  gboolean (*save)(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection);
  gboolean (*load)(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceLocation *location);
};

GType bt_persistence_get_type(void) G_GNUC_CONST;

#endif // BT_PERSISTENCE_H
