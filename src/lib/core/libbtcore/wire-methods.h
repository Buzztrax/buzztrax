/* $Id: wire-methods.h,v 1.9 2004-07-30 15:15:51 ensonic Exp $
 * defines all public methods of the wire class
 */

#ifndef BT_WIRE_METHODS_H
#define BT_WIRE_METHODS_H

#include "wire.h"

extern BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine);

#endif // BT_WIRE_METHDOS_H
