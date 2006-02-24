/* $Id: persistence-selection.h,v 1.1 2006-02-24 19:51:49 ensonic Exp $
 * base-class for an object serialisation filters
 */

#ifndef BT_PERSISTENCE_SELECTION_H
#define BT_PERSISTENCE_SELECTION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PERSISTENCE_SELECTION            (bt_persistence_selection_get_type ())
#define BT_PERSISTENCE_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PERSISTENCE_SELECTION, BtPersistenceSelection))
#define BT_PERSISTENCE_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PERSISTENCE_SELECTION, BtPersistenceSelectionClass))
#define BT_IS_PERSISTENCE_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PERSISTENCE_SELECTION))
#define BT_IS_PERSISTENCE_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PERSISTENCE_SELECTION))
#define BT_PERSISTENCE_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PERSISTENCE_SELECTION, BtPersistenceSelectionClass))

/* type macros */

typedef struct _BtPersistenceSelection BtPersistenceSelection;
typedef struct _BtPersistenceSelectionClass BtPersistenceSelectionClass;

/**
 * BtPersistenceSelection:
 */
struct _BtPersistenceSelection {
  GObject parent;
};

struct _BtPersistenceSelectionClass {
  GObjectClass parent;
};

GType bt_persistence_selection_get_type(void) G_GNUC_CONST;


#endif // BT_PERSISTENCE_SELECTION_H
