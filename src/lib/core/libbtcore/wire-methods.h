/* $Id: wire-methods.h,v 1.7 2004-05-12 09:35:14 ensonic Exp $
 * defines all public methods of the wire class
 */

#ifndef BT_WIRE_METHODS_H
#define BT_WIRE_METHODS_H

#include "wire.h"

gboolean bt_wire_connect(const BtWire *self, const BtMachine *src, const BtMachine *dst);

#endif // BT_WIRE_METHDOS_H
