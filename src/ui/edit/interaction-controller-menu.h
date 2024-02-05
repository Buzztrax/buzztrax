/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_INTERACTION_CONTROLLER_MENU_H
#define BT_INTERACTION_CONTROLLER_MENU_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_INTERACTION_CONTROLLER_MENU            (bt_interaction_controller_menu_get_type ())
#define BT_INTERACTION_CONTROLLER_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_INTERACTION_CONTROLLER_MENU, BtInteractionControllerMenu))
#define BT_INTERACTION_CONTROLLER_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_INTERACTION_CONTROLLER_MENU, BtInteractionControllerMenuClass))
#define BT_IS_INTERACTION_CONTROLLER_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_INTERACTION_CONTROLLER_MENU))
#define BT_IS_INTERACTION_CONTROLLER_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_INTERACTION_CONTROLLER_MENU))
#define BT_INTERACTION_CONTROLLER_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_INTERACTION_CONTROLLER_MENU, BtInteractionControllerMenuClass))

/* type macros */

typedef struct _BtInteractionControllerMenu BtInteractionControllerMenu;
typedef struct _BtInteractionControllerMenuClass BtInteractionControllerMenuClass;
typedef struct _BtInteractionControllerMenuPrivate BtInteractionControllerMenuPrivate;

/**
 * BtInteractionControllerMenu:
 *
 * the machine selection sub-menu for the canvas page context menu
 */
struct _BtInteractionControllerMenu {
  GObject parent;

  /*< private >*/
  BtInteractionControllerMenuPrivate *priv;
};

struct _BtInteractionControllerMenuClass {
  GObjectClass parent;
};

/**
 * BtInteractionControllerMenuType:
 * @BT_INTERACTION_CONTROLLER_RANGE_MENU: range controllers
 * @BT_INTERACTION_CONTROLLER_TRIGGER_MENU: trigger controllers
 *
 * #BtInteractionControllerMenu can generate a menu showing different controller
 * types.
 */
typedef enum {
  BT_INTERACTION_CONTROLLER_RANGE_MENU=0,
  BT_INTERACTION_CONTROLLER_TRIGGER_MENU,
} BtInteractionControllerMenuType;

#define BT_TYPE_INTERACTION_CONTROLLER_MENU_TYPE (bt_interaction_controller_menu_type_get_type())

GType bt_interaction_controller_menu_type_get_type(void) G_GNUC_CONST;

GType bt_interaction_controller_menu_get_type(void) G_GNUC_CONST;


BtInteractionControllerMenu *bt_interaction_controller_menu_new(BtInteractionControllerMenuType type, BtMachine *machine);

GMenu* bt_interaction_controller_menu_get_menu(BtInteractionControllerMenu* self);
GtkPopoverMenu* bt_interaction_controller_menu_get_popover(BtInteractionControllerMenu* self);

#endif // BT_INTERACTION_CONTROLLER_MENU_H
