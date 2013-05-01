/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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

struct _BtMachineMenuClass {
  GtkMenuClass parent;
  
};

GType bt_machine_menu_get_type(void) G_GNUC_CONST;

BtMachineMenu *bt_machine_menu_new(const BtMainPageMachines *main_page_machines);

#endif // BT_MACHINE_MENU_H
