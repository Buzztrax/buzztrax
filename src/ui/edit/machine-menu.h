/* $Id: machine-menu.h,v 1.2 2005-10-08 18:12:13 ensonic Exp $
 * class for the machine selection sub-menu
 */

#ifndef BT_MACHINE_MENU_H
#define BT_MACHINE_MENU_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MACHINE_MENU            (bt_machine_menu_get_type ())
#define BT_MACHINE_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE_MENU, BtMachineMenu))
#define BT_MACHINE_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE_MENU, BtMachineMenuClass))
#define BT_IS_MACHINE_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE_MENU))
#define BT_IS_MACHINE_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE_MENU))
#define BT_MACHINE_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE_MENU, BtMachineMenuClass))

/* type macros */

typedef struct _BtMachineMenu BtMachineMenu;
typedef struct _BtMachineMenuClass BtMachineMenuClass;
typedef struct _BtMachineMenuPrivate BtMachineMenuPrivate;

/**
 * BtMachineMenu:
 *
 * the machine selection sub-menu for the canvas page context menu
 */
struct _BtMachineMenu {
  GtkMenu parent;
  
  /*< private >*/
  BtMachineMenuPrivate *priv;
};
/* structure of the machine-menu class */
struct _BtMachineMenuClass {
  GtkMenuClass parent;
  
};

/* used by MACHINE_MENU_TYPE */
GType bt_machine_menu_get_type(void) G_GNUC_CONST;

#endif // BT_MACHINE_MENU_H
