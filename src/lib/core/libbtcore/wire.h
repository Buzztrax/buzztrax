/* $Id: wire.h,v 1.5 2004-05-14 16:59:22 ensonic Exp $
 * class for a machine to machine connection
 */

#ifndef BT_WIRE_H
#define BT_WIRE_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_WIRE_TYPE:
 *
 * #GType for BtWire instances
 */
#define BT_WIRE_TYPE		        (bt_wire_get_type ())
#define BT_WIRE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_WIRE_TYPE, BtWire))
#define BT_WIRE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_WIRE_TYPE, BtWireClass))
#define BT_IS_WIRE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_WIRE_TYPE))
#define BT_IS_WIRE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_WIRE_TYPE))
#define BT_WIRE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_WIRE_TYPE, BtWireClass))

/* type macros */

typedef struct _BtWire BtWire;
typedef struct _BtWireClass BtWireClass;
typedef struct _BtWirePrivate BtWirePrivate;

/**
 * BtWire:
 *
 * connects two #BtMachine instances
 */
struct _BtWire {
  GObject parent;
  
  /* private */
  BtWirePrivate *private;
};
/* structure of the wire class */
struct _BtWireClass {
  GObjectClass parent;
};

/* used by WIRE_TYPE */
GType bt_wire_get_type(void);

#endif // BT_WIRE_H

