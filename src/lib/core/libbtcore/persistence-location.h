/* $Id: persistence-location.h,v 1.1 2006-02-24 19:51:49 ensonic Exp $
 * base-class for an object serialisation filters
 */

#ifndef BT_PERSISTENCE_LOCATION_H
#define BT_PERSISTENCE_LOCATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PERSISTENCE_LOCATION            (bt_persistence_location_get_type ())
#define BT_PERSISTENCE_LOCATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PERSISTENCE_LOCATION, BtPersistenceLocation))
#define BT_PERSISTENCE_LOCATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PERSISTENCE_LOCATION, BtPersistenceLocationClass))
#define BT_IS_PERSISTENCE_LOCATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PERSISTENCE_LOCATION))
#define BT_IS_PERSISTENCE_LOCATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PERSISTENCE_LOCATION))
#define BT_PERSISTENCE_LOCATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PERSISTENCE_LOCATION, BtPersistenceLocationClass))

/* type macros */

typedef struct _BtPersistenceLocation BtPersistenceLocation;
typedef struct _BtPersistenceLocationClass BtPersistenceLocationClass;

/**
 * BtPersistenceSelection:
 */
struct _BtPersistenceLocation {
  GObject parent;
};

struct _BtPersistenceLocationClass {
  GObjectClass parent;
};

GType bt_persistence_location_get_type(void) G_GNUC_CONST;


#endif // BT_PERSISTENCE_LOCATION_H
