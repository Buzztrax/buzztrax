/* $Id: cmd-application.h,v 1.6 2004-09-29 16:56:46 ensonic Exp $
 * class for a commandline based buzztard tool application
 */

#ifndef BT_CMD_APPLICATION_H
#define BT_CMD_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_CMD_APPLICATION		         (bt_cmd_application_get_type ())
#define BT_CMD_APPLICATION(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CMD_APPLICATION, BtCmdApplication))
#define BT_CMD_APPLICATION_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_CMD_APPLICATION, BtCmdApplicationClass))
#define BT_IS_CMD_APPLICATION(obj)	       (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_CMD_APPLICATION))
#define BT_IS_CMD_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_CMD_APPLICATION))
#define BT_CMD_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_CMD_APPLICATION, BtCmdApplicationClass))

/* type macros */

typedef struct _BtCmdApplication BtCmdApplication;
typedef struct _BtCmdApplicationClass BtCmdApplicationClass;
typedef struct _BtCmdApplicationPrivate BtCmdApplicationPrivate;

/**
 * BtCmdApplication:
 *
 * #BtApplication subclass for the commandline application
 */
struct _BtCmdApplication {
  BtApplication parent;
  
  /* private */
  BtCmdApplicationPrivate *priv;
};
/* structure of the cmd-application class */
struct _BtCmdApplicationClass {
  BtApplicationClass parent;
  
};

/* used by CMD_APPLICATION_TYPE */
GType bt_cmd_application_get_type(void);

#endif // BT_CMD_APPLICATION_H

