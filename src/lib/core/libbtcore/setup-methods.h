/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_SETUP_METHODS_H
#define BT_SETUP_METHODS_H

#include "machine.h"
#include "setup.h"
#include "wire.h"

extern BtSetup *bt_setup_new(const BtSong * const song);

extern gboolean bt_setup_add_machine(const BtSetup * const self, const BtMachine * const machine);
extern gboolean bt_setup_add_wire(const BtSetup * const self, const BtWire * const wire);

extern void bt_setup_remove_machine(const BtSetup * const self, const BtMachine * const machine);
extern void bt_setup_remove_wire(const BtSetup * const self, const BtWire * const wire);

extern BtMachine *bt_setup_get_machine_by_id(const BtSetup * const self, const gchar * const id);
extern BtMachine *bt_setup_get_machine_by_index(const BtSetup * const self, const gulong index);
extern BtMachine *bt_setup_get_machine_by_type(const BtSetup * const self, const GType type);

extern GList *bt_setup_get_machines_by_type(const BtSetup * const self, const GType type);


extern BtWire *bt_setup_get_wire_by_src_machine(const BtSetup * const self, const BtMachine * const src);
extern BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup * const self, const BtMachine * const dst);
extern BtWire *bt_setup_get_wire_by_machines(const BtSetup * const self, const BtMachine * const src, const BtMachine * const dst);

extern GList *bt_setup_get_wires_by_src_machine(const BtSetup * const self, const BtMachine * const src);
extern GList *bt_setup_get_wires_by_dst_machine(const BtSetup * const self, const BtMachine * const dst);

extern gchar *bt_setup_get_unique_machine_id(const BtSetup * const self, gchar * const base_name);

extern void bt_setup_remember_missing_machine(const BtSetup * const self, const gchar * const str);

#endif // BT_SETUP_METHDOS_H
