/* $Id: edit-application.h,v 1.7 2004-10-08 13:50:04 ensonic Exp $
 * class for a gtk based buzztard editor application
 */

#ifndef BT_EDIT_APPLICATION_H
#define BT_EDIT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_EDIT_APPLICATION		        (bt_edit_application_get_type ())
#define BT_EDIT_APPLICATION(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplication))
#define BT_EDIT_APPLICATION_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))
#define BT_IS_EDIT_APPLICATION(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_EDIT_APPLICATION))
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
  BtEditApplicationPrivate *priv;
};
/* structure of the edit-application class */
struct _BtEditApplicationClass {
  BtApplicationClass parent;

  void (*song_changed)(const BtEditApplication *app, gpointer user_data);
};

/* used by EDIT_APPLICATION_TYPE */
GType bt_edit_application_get_type(void);

#endif // BT_EDIT_APPLICATION_H

