/* $Id: setup-methods.h,v 1.3 2004-05-07 18:04:15 ensonic Exp $
* defines all public methods of the setup class
*/

#ifndef BT_SETUP_METHODS_H
#define BT_SETUP_METHODS_H

#include "setup.h"

void bt_setup_add_machine(const BtSetup *self, const BtMachine *machine);

void bt_setup_add_wire(const BtSetup *self, const BtWire *wire);

BtMachine *bt_setup_get_machine_by_id(const BtSetup *self, const gchar *id);

#endif // BT_SETUP_METHDOS_H
