/* $Id: edit-application.h,v 1.3 2004-08-13 18:58:11 ensonic Exp $
 * class for a gtk based buzztard editor application
 */

#ifndef BT_EDIT_APPLICATION_H
#define BT_EDIT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_TYPE_EDIT_APPLICATION:
 *
 * #GType for BtEditApplication instances
 */
#define BT_TYPE_EDIT_APPLICATION		        (bt_edit_application_get_type ())
#define BT_EDIT_APPLICATION(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplication))
#define BT_EDIT_APPLICATION_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))
#define BT_IS_EDIT_APPLICATION(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_EDIT_APPLICATION))
#define BT_IS_EDIT_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_EDIT_APPLICATION))
#define BT_EDIT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))

/* type macros */

typedef struct _BtEditApplication BtEditApplication;
typedef struct _BtEditApplicationClass BtEditApplicationClass;
typedef struct _BtEditApplicationPrivate BtEditApplicationPrivate;

/**
 * BtEditApplication:
 *
 * #BtApplication subclass for the gtk editor application
 */
struct _BtEditApplication {
  BtApplication parent;
  
  /* private */
  BtEditApplicationPrivate *private;
};
/* structure of the edit-application class */
struct _BtEditApplicationClass {
  BtApplicationClass parent;

   guint song_changed_signal_id; 
};

/* used by EDIT_APPLICATION_TYPE */
GType bt_edit_application_get_type(void);

#endif // BT_EDIT_APPLICATION_H

