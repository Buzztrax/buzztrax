/* $Id: persistence.h,v 1.2 2006-03-08 15:30:27 ensonic Exp $
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

  xmlNodePtr (*save)(BtPersistence *self, xmlDocPtr doc, xmlNodePtr node, BtPersistenceSelection *selection);
  gboolean (*load)(BtPersistence *self, xmlDocPtr doc, xmlNodePtr node, BtPersistenceLocation *location);
};

GType bt_persistence_get_type(void) G_GNUC_CONST;

#endif // BT_PERSISTENCE_H
