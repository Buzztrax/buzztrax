/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
#include <gtk/gtk.h>

/**
 * BtMachineMenu:
 *
 * the machine selection sub-menu for the canvas page context menu
 */
G_DECLARE_FINAL_TYPE(BtMachineMenu, bt_machine_menu, BT, MACHINE_MENU, GObject);
#define BT_TYPE_MACHINE_MENU            (bt_machine_menu_get_type ())

typedef struct _BtMainPageMachines BtMainPageMachines;

BtMachineMenu *bt_machine_menu_new(const BtMainPageMachines *main_page_machines);
GMenu* bt_machine_menu_get_menu(BtMachineMenu* self);

#endif // BT_MACHINE_MENU_H
