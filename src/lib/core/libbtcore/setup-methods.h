/* $Id: setup-methods.h,v 1.10 2004-10-06 17:26:30 ensonic Exp $
 * defines all public methods of the setup class
 */

#ifndef BT_SETUP_METHODS_H
#define BT_SETUP_METHODS_H

#include "machine.h"
#include "setup.h"
#include "wire.h"

extern BtSetup *bt_setup_new(const BtSong *song);

extern void bt_setup_add_machine(const BtSetup *self, const BtMachine *machine);
extern void bt_setup_add_wire(const BtSetup *self, const BtWire *wire);

extern BtMachine *bt_setup_get_machine_by_id(const BtSetup *self, const gchar *id);
extern BtMachine *bt_setup_get_machine_by_index(const BtSetup *self, glong index);

extern BtWire *bt_setup_get_wire_by_src_machine(const BtSetup *self,const BtMachine *src);
extern BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup *self,const BtMachine *dst);

extern gpointer bt_setup_machine_iterator_new(const BtSetup *self);
extern gpointer bt_setup_machine_iterator_next(gpointer iter);
extern BtMachine *bt_setup_machine_iterator_get_machine(gpointer iter);

extern gpointer bt_setup_wire_iterator_new(const BtSetup *self);
extern gpointer bt_setup_wire_iterator_next(gpointer iter);
extern BtWire *bt_setup_wire_iterator_get_wire(gpointer iter);

#endif // BT_SETUP_METHDOS_H
