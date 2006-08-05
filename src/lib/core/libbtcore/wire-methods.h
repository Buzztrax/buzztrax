/* $Id: wire-methods.h,v 1.12 2006-08-05 16:14:18 ensonic Exp $
 * defines all public methods of the wire class
 */

#ifndef BT_WIRE_METHODS_H
#define BT_WIRE_METHODS_H

#include "wire.h"

extern BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine);

extern gboolean bt_wire_reconnect(BtWire *self);

extern GList *bt_wire_get_element_list(const BtWire *self);
extern void bt_wire_dbg_print_parts(const BtWire *self);

#endif // BT_WIRE_METHDOS_H
