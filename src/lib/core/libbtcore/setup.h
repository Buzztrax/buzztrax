/* $Id: setup.h,v 1.3 2004-05-14 16:59:22 ensonic Exp $
 * class for machine and setup setup
 */

#ifndef BT_SETUP_H
#define BT_SETUP_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_SETUP_TYPE:
 *
 * #GType for BtSetup instances
 */
#define BT_SETUP_TYPE		        (bt_setup_get_type ())
#define BT_SETUP(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SETUP_TYPE, BtSetup))
#define BT_SETUP_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SETUP_TYPE, BtSetupClass))
#define BT_IS_SETUP(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SETUP_TYPE))
#define BT_IS_SETUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SETUP_TYPE))
#define BT_SETUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SETUP_TYPE, BtSetupClass))

/* type macros */

typedef struct _BtSetup BtSetup;
typedef struct _BtSetupClass BtSetupClass;
typedef struct _BtSetupPrivate BtSetupPrivate;

/**
 * BtSetup:
 *
 * virtual hardware setup
 * (contains #BtMachine and #BtWire objects)
 */
struct _BtSetup {
  GObject parent;
  
  /* private */
  BtSetupPrivate *private;
};
/* structure of the setup class */
struct _BtSetupClass {
  GObjectClass parent;
};

/* used by SETUP_TYPE */
GType bt_setup_get_type(void);


#endif // BT_SETUP_H

