/* $Id: application.h,v 1.1 2004-05-12 09:35:14 ensonic Exp $
 * base class for a buzztard based application
 */

#ifndef BT_APPLICATION_H
#define BT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_APPLICATION_TYPE		        (bt_application_get_type ())
#define BT_APPLICATION(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_APPLICATION_TYPE, BtApplication))
#define BT_APPLICATION_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_APPLICATION_TYPE, BtApplicationClass))
#define BT_IS_APPLICATION(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_APPLICATION_TYPE))
#define BT_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_APPLICATION_TYPE))
#define BT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_APPLICATION_TYPE, BtApplicationClass))

/* type macros */

typedef struct _BtApplication BtApplication;
typedef struct _BtApplicationClass BtApplicationClass;
typedef struct _BtApplicationPrivate BtApplicationPrivate;

struct _BtApplication {
  GObject parent;
  
  /* private */
  BtApplicationPrivate *private;
};
/* structure of the application class */
struct _BtApplicationClass {
  GObjectClass parent;
  
};

/* used by APPLICATION_TYPE */
GType bt_application_get_type(void);

#endif // BT_APPLICATION_H

