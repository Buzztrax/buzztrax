/* $Id: machine-canvas-item-methods.h,v 1.2 2004-12-15 09:07:34 ensonic Exp $
 * defines all public methods of the machine canvas item class
 */

#ifndef BT_MACHINE_CANVAS_ITEM_METHODS_H
#define BT_MACHINE_CANVAS_ITEM_METHODS_H

#include "machine-canvas-item.h"
#include "main-page-machines.h"

extern BtMachineCanvasItem *bt_machine_canvas_item_new(const BtMainPageMachines *main_page_machines,BtMachine *machine,gdouble xpos,gdouble ypos,gdouble zoom);

#endif // BT_MACHINE_CANVAS_ITEM_METHODS_H
