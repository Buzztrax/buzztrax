/* $Id: wire-canvas-item-methods.h,v 1.1 2004-12-13 07:52:27 ensonic Exp $
 * defines all public methods of the wire canvas item class
 */

#ifndef BT_WIRE_CANVAS_ITEM_METHODS_H
#define BT_WIRE_CANVAS_ITEM_METHODS_H

#include "wire-canvas-item.h"
#include "main-page-machines.h"

extern BtWireCanvasItem *bt_wire_canvas_item_new(BtMainPageMachines *main_page_machines,BtWire *wire,gdouble pos_xs,gdouble pos_ys,gdouble pos_xe,gdouble pos_ye,BtMachineCanvasItem *src_machine_item,BtMachineCanvasItem *dst_machine_item);

#endif // BT_WIRE_CANVAS_ITEM_METHODS_H
