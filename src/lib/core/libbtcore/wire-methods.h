/* $Id: wire-methods.h,v 1.4 2004-05-07 16:29:28 ensonic Exp $
* defines all public methods of the wire class
*/

#ifndef BT_WIRE_METHODS_H
#define BT_WIRE_METHODS_H

#include "wire.h"

/** connect two machines */
static gboolean bt_wire_connect(const BtWire *self, const BtMachine *src, const BtMachine *dst);

#endif // BT_WIRE_METHDOS_H
