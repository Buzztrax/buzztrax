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

#ifndef BT_MACHINE_CANVAS_ITEM_H
#define BT_MACHINE_CANVAS_ITEM_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define BT_TYPE_MACHINE_CANVAS_ITEM             (bt_machine_canvas_item_get_type ())
#define BT_MACHINE_CANVAS_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE_CANVAS_ITEM, BtMachineCanvasItem))
#define BT_MACHINE_CANVAS_ITEM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE_CANVAS_ITEM, BtMachineCanvasItemClass))
#define BT_IS_MACHINE_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE_CANVAS_ITEM))
#define BT_IS_MACHINE_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE_CANVAS_ITEM))
#define BT_MACHINE_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE_CANVAS_ITEM, BtMachineCanvasItemClass))

/* type macros */

typedef struct _BtMachineCanvasItem BtMachineCanvasItem;
typedef struct _BtMachineCanvasItemClass BtMachineCanvasItemClass;
typedef struct _BtMachineCanvasItemPrivate BtMachineCanvasItemPrivate;

/**
 * BtMachineCanvasItem:
 *
 * the root window for the editor application
 */
struct _BtMachineCanvasItem {
  GtkWidget parent;
  
  /*< private >*/
  BtMachineCanvasItemPrivate *priv;
};

struct _BtMachineCanvasItemClass {
  GtkWidgetClass parent;
};

GType bt_machine_canvas_item_get_type(void) G_GNUC_CONST;

#include "main-page-machines.h"

BtMachineCanvasItem *bt_machine_canvas_item_new(const BtMainPageMachines *main_page_machines,BtMachine *machine,gdouble zoom);

void bt_machine_show_properties_dialog(BtMachine *machine);
void bt_machine_show_preferences_dialog(BtMachine *machine);
void bt_machine_show_analyzer_dialog(BtMachine *machine);

#endif // BT_MACHINE_CANVAS_ITEM_H
