/* $Id: main-statusbar.h,v 1.4 2004-09-29 16:56:47 ensonic Exp $
 * class for the editor main statusbar
 */

#ifndef BT_MAIN_STATUSBAR_H
#define BT_MAIN_STATUSBAR_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_STATUSBAR		        (bt_main_statusbar_get_type ())
#define BT_MAIN_STATUSBAR(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_STATUSBAR, BtMainStatusbar))
#define BT_MAIN_STATUSBAR_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_STATUSBAR, BtMainStatusbarClass))
#define BT_IS_MAIN_STATUSBAR(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_MAIN_STATUSBAR))
#define BT_IS_MAIN_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_STATUSBAR))
#define BT_MAIN_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_STATUSBAR, BtMainStatusbarClass))

/* type macros */

typedef struct _BtMainStatusbar BtMainStatusbar;
typedef struct _BtMainStatusbarClass BtMainStatusbarClass;
typedef struct _BtMainStatusbarPrivate BtMainStatusbarPrivate;

/**
 * BtMainStatusbar:
 *
 * the root window for the editor application
 */
struct _BtMainStatusbar {
  GtkHBox parent;
  
  /* private */
  BtMainStatusbarPrivate *priv;
};
/* structure of the main-menu class */
struct _BtMainStatusbarClass {
  GtkHBoxClass parent;
  
};

/* used by MAIN_STATUSBAR_TYPE */
GType bt_main_statusbar_get_type(void);

#endif // BT_MAIN_STATUSBAR_H

