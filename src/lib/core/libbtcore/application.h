/* $Id: application.h,v 1.4 2004-07-20 18:24:18 ensonic Exp $
 * base class for a buzztard based application
 */

#ifndef BT_APPLICATION_H
#define BT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_TYPE_APPLICATION:
 *
 * #GType for BtApplication instances
 */
#define BT_TYPE_APPLICATION		         (bt_application_get_type ())
#define BT_APPLICATION(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_APPLICATION, BtApplication))
#define BT_APPLICATION_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_APPLICATION, BtApplicationClass))
#define BT_IS_APPLICATION(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_APPLICATION))
#define BT_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_APPLICATION))
#define BT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_APPLICATION, BtApplicationClass))

/* type macros */

typedef struct _BtApplication BtApplication;
typedef struct _BtApplicationClass BtApplicationClass;
typedef struct _BtApplicationPrivate BtApplicationPrivate;

/**
 * BtApplication:
 *
 * base object for a buzztard based application
 */
struct _BtApplication {
  GObject parent;
  
  /* private */
  BtApplicationPrivate *private;
};
/* structure of the application class */
struct _BtApplicationClass {
  GObjectClass parent_class;
  
};

/* used by APPLICATION_TYPE */
GType bt_application_get_type(void);

#endif // BT_APPLICATION_H

