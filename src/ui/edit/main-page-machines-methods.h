/* $Id: main-page-machines-methods.h,v 1.3 2004-12-11 15:07:53 ensonic Exp $
 * defines all public methods of the main machines page class
 */

#ifndef BT_MAIN_PAGE_MACHINES_METHODS_H
#define BT_MAIN_PAGE_MACHINES_METHODS_H

#include "main-page-machines.h"
#include "edit-application.h"

extern BtMainPageMachines *bt_main_page_machines_new(const BtEditApplication *app);

extern void machine_view_get_machine_position(GHashTable *properties, gdouble *pos_x,gdouble *pos_y);

extern void bt_main_page_machines_remove_machine_item(const BtMainPageMachines *self, BtMachineCanvasItem *item);
extern void bt_main_page_machines_remove_wire_item(const BtMainPageMachines *self, BtMachineCanvasItem *item);

#endif // BT_MAIN_PAGE_MACHINES_METHDOS_H
