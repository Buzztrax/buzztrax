/* $Id: wavelevel.h,v 1.2 2005-01-11 16:50:48 ensonic Exp $
 * class for wavelevel
 */

#ifndef BT_WAVELEVEL_H
#define BT_WAVELEVEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVELEVEL		         (bt_wavelevel_get_type ())
#define BT_WAVELEVEL(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVELEVEL, BtWavelevel))
#define BT_WAVELEVEL_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVELEVEL, BtWavelevelClass))
#define BT_IS_WAVELEVEL(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVELEVEL))
#define BT_IS_WAVELEVEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVELEVEL))
#define BT_WAVELEVEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVELEVEL, BtWavelevelClass))

/* type macros */

typedef struct _BtWavelevel BtWavelevel;
typedef struct _BtWavelevelClass BtWavelevelClass;
typedef struct _BtWavelevelPrivate BtWavelevelPrivate;

/**
 * BtWavelevel:
 *
 * virtual hardware setup
 * (contains #BtMachine and #BtWire objects)
 */
struct _BtWavelevel {
  GObject parent;
  
  /*< private >*/
  BtWavelevelPrivate *priv;
};
/* structure of the setup class */
struct _BtWavelevelClass {
  GObjectClass parent_class;
};

/* used by WAVELEVEL_TYPE */
GType bt_wavelevel_get_type(void);


#endif // BT_WAVELEVEL_H
