/* $Id: main-menu.h,v 1.4 2004-10-08 13:50:04 ensonic Exp $
 * class for the editor main menu
 */

#ifndef BT_MAIN_MENU_H
#define BT_MAIN_MENU_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_MENU		         (bt_main_menu_get_type ())
#define BT_MAIN_MENU(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_MENU, BtMainMenu))
#define BT_MAIN_MENU_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_MENU, BtMainMenuClass))
#define BT_IS_MAIN_MENU(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_MENU))
#define BT_IS_MAIN_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_MENU))
#define BT_MAIN_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_MENU, BtMainMenuClass))

/* type macros */

typedef struct _BtMainMenu BtMainMenu;
typedef struct _BtMainMenuClass BtMainMenuClass;
typedef struct _BtMainMenuPrivate BtMainMenuPrivate;

/**
 * BtMainMenu:
 *
 * the root window for the editor application
 */
struct _BtMainMenu {
  GtkMenuBar parent;
  
  /* private */
  BtMainMenuPrivate *priv;
};
/* structure of the main-menu class */
struct _BtMainMenuClass {
  GtkMenuBarClass parent;
  
};

/* used by MAIN_MENU_TYPE */
GType bt_main_menu_get_type(void);

#endif // BT_MAIN_MENU_H

