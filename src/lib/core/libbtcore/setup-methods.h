/* $Id: setup-methods.h,v 1.5 2004-05-14 16:59:22 ensonic Exp $
 * defines all public methods of the setup class
 */

#ifndef BT_SETUP_METHODS_H
#define BT_SETUP_METHODS_H

#include "setup.h"

void bt_setup_add_machine(const BtSetup *self, const BtMachine *machine);

void bt_setup_add_wire(const BtSetup *self, const BtWire *wire);

BtMachine *bt_setup_get_machine_by_id(const BtSetup *self, const gchar *id);

BtWire *bt_setup_get_wire_by_src_machine(const BtSetup *self,const BtMachine *src);

BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup *self,const BtMachine *dst);

#endif // BT_SETUP_METHDOS_H
