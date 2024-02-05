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

#ifndef BT_WIRE_CANVAS_ITEM_H
#define BT_WIRE_CANVAS_ITEM_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

typedef struct _BtMachineCanvasItem BtMachineCanvasItem;
typedef struct _BtMainPageMachines BtMainPageMachines;
typedef struct _BtWire BtWire;

/**
 * BtWireCanvasItem:
 *
 * the root window for the editor application
 */
G_DECLARE_FINAL_TYPE (BtWireCanvasItem, bt_wire_canvas_item, BT, WIRE_CANVAS_ITEM, GtkWidget);
#define BT_TYPE_WIRE_CANVAS_ITEM            (bt_wire_canvas_item_get_type ())

BtWireCanvasItem *bt_wire_canvas_item_new(const BtMainPageMachines *main_page_machines,BtWire *wire,BtMachineCanvasItem *src_machine_item,BtMachineCanvasItem *dst_machine_item);

void bt_wire_show_analyzer_dialog(BtWire *wire);

#endif // BT_WIRE_CANVAS_ITEM_H
