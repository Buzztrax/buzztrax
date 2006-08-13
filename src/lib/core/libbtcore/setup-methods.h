/* $Id: setup-methods.h,v 1.20 2006-08-13 12:45:07 ensonic Exp $
 * defines all public methods of the setup class
 */

#ifndef BT_SETUP_METHODS_H
#define BT_SETUP_METHODS_H

#include "machine.h"
#include "setup.h"
#include "wire.h"

extern BtSetup *bt_setup_new(const BtSong *song);

extern gboolean bt_setup_add_machine(const BtSetup *self, const BtMachine *machine);
extern gboolean bt_setup_add_wire(const BtSetup *self, const BtWire *wire);

extern void bt_setup_remove_machine(const BtSetup *self, const BtMachine *machine);
extern void bt_setup_remove_wire(const BtSetup *self, const BtWire *wire);

extern BtMachine *bt_setup_get_machine_by_id(const BtSetup *self, const gchar *id);
extern BtMachine *bt_setup_get_machine_by_index(const BtSetup *self, gulong index);
extern BtMachine *bt_setup_get_machine_by_type(const BtSetup *self, GType type);

extern GList *bt_setup_get_machines_by_type(const BtSetup *self, GType type);


extern BtWire *bt_setup_get_wire_by_src_machine(const BtSetup *self,const BtMachine *src);
extern BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup *self,const BtMachine *dst);
extern BtWire *bt_setup_get_wire_by_machines(const BtSetup *self,const BtMachine *src,const BtMachine *dst);

extern GList *bt_setup_get_wires_by_src_machine(const BtSetup *self,const BtMachine *src);
extern GList *bt_setup_get_wires_by_dst_machine(const BtSetup *self,const BtMachine *dst);

extern gchar *bt_setup_get_unique_machine_id(const BtSetup *self,gchar *base_name);

extern void bt_setup_remember_missing_machine(const BtSetup *self,const gchar *str);

#endif // BT_SETUP_METHDOS_H
