/* $Id: main-toolbar.h,v 1.3 2004-09-29 16:56:47 ensonic Exp $
 * class for the editor main toolbar
 */

#ifndef BT_MAIN_TOOLBAR_H
#define BT_MAIN_TOOLBAR_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_TOOLBAR		        (bt_main_toolbar_get_type ())
#define BT_MAIN_TOOLBAR(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_TOOLBAR, BtMainToolbar))
#define BT_MAIN_TOOLBAR_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_TOOLBAR, BtMainToolbarClass))
#define BT_IS_MAIN_TOOLBAR(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_MAIN_TOOLBAR))
#define BT_IS_MAIN_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_TOOLBAR))
#define BT_MAIN_TOOLBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_TOOLBAR, BtMainToolbarClass))

/* type macros */

typedef struct _BtMainToolbar BtMainToolbar;
typedef struct _BtMainToolbarClass BtMainToolbarClass;
typedef struct _BtMainToolbarPrivate BtMainToolbarPrivate;

/**
 * BtMainToolbar:
 *
 * the root window for the editor application
 */
struct _BtMainToolbar {
  GtkHandleBox parent;
  
  /* private */
  BtMainToolbarPrivate *priv;
};
/* structure of the main-menu class */
struct _BtMainToolbarClass {
  GtkHandleBoxClass parent;
  
};

/* used by MAIN_TOOLBAR_TYPE */
GType bt_main_toolbar_get_type(void);

#endif // BT_MAIN_TOOLBAR_H

