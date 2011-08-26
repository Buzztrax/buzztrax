/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_WIRE_CANVAS_ITEM_H
#define BT_WIRE_CANVAS_ITEM_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WIRE_CANVAS_ITEM            (bt_wire_canvas_item_get_type ())
#define BT_WIRE_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItem))
#define BT_WIRE_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItemClass))
#define BT_IS_WIRE_CANVAS_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE_CANVAS_ITEM))
#define BT_IS_WIRE_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE_CANVAS_ITEM))
#define BT_WIRE_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItemClass))

/* type macros */

typedef struct _BtWireCanvasItem BtWireCanvasItem;
typedef struct _BtWireCanvasItemClass BtWireCanvasItemClass;
typedef struct _BtWireCanvasItemPrivate BtWireCanvasItemPrivate;

/**
 * BtWireCanvasItem:
 *
 * the root window for the editor application
 */
struct _BtWireCanvasItem {
  GnomeCanvasGroup parent;
  
  /*< private >*/
  BtWireCanvasItemPrivate *priv;
};

struct _BtWireCanvasItemClass {
  GnomeCanvasGroupClass parent;
  
};

GType bt_wire_canvas_item_get_type(void) G_GNUC_CONST;

BtWireCanvasItem *bt_wire_canvas_item_new(const BtMainPageMachines *main_page_machines,BtWire *wire,gdouble pos_xs,gdouble pos_ys,gdouble pos_xe,gdouble pos_ye,BtMachineCanvasItem *src_machine_item,BtMachineCanvasItem *dst_machine_item);

#endif // BT_WIRE_CANVAS_ITEM_H
