/* $Id: main-page-machines-methods.h,v 1.2 2004-10-15 15:39:33 ensonic Exp $
 * defines all public methods of the main machines page class
 */

#ifndef BT_MAIN_PAGE_MACHINES_METHODS_H
#define BT_MAIN_PAGE_MACHINES_METHODS_H

#include "main-page-machines.h"
#include "edit-application.h"

extern BtMainPageMachines *bt_main_page_machines_new(const BtEditApplication *app);

extern void machine_view_get_machine_position(GHashTable *properties, gdouble *pos_x,gdouble *pos_y);

#endif // BT_MAIN_PAGE_MACHINES_METHDOS_H
