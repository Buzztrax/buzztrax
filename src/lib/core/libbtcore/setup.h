/* $Id: setup.h,v 1.6 2004-09-02 17:43:30 ensonic Exp $
 * class for machine and setup setup
 */

#ifndef BT_SETUP_H
#define BT_SETUP_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETUP		         (bt_setup_get_type ())
#define BT_SETUP(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETUP, BtSetup))
#define BT_SETUP_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETUP, BtSetupClass))
#define BT_IS_SETUP(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETUP))
#define BT_IS_SETUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETUP))
#define BT_SETUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETUP, BtSetupClass))

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
  GObjectClass parent_class;
};

/* used by SETUP_TYPE */
GType bt_setup_get_type(void);


#endif // BT_SETUP_H

