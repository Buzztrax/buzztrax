/* $Id: setup-methods.h,v 1.2 2004-05-07 15:16:04 ensonic Exp $
* defines all public methods of the setup class
*/

#ifndef BT_SETUP_METHODS_H
#define BT_SETUP_METHODS_H

#include "setup.h"

void bt_setup_add_machine(const BtSetup *self, const BtMachine *machine);

void bt_setup_add_wire(const BtSetup *self, const BtWire *wire);

#endif // BT_SETUP_METHDOS_H
