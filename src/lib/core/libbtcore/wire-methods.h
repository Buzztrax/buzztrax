/* $Id: wire-methods.h,v 1.13 2006-08-24 20:00:54 ensonic Exp $
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

#ifndef BT_WIRE_METHODS_H
#define BT_WIRE_METHODS_H

#include "wire.h"
#include "wire-pattern.h"

extern BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine);

extern gboolean bt_wire_reconnect(BtWire *self);
extern BtWirePattern *bt_wire_get_pattern(const BtWire * const self, BtPattern *pattern);

extern GParamSpec *bt_wire_get_param_spec(const BtWire * const self, const gulong index);
extern GType bt_wire_get_param_type(const BtWire * const self, const gulong index);
extern void bt_wire_get_param_details(const BtWire * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val);

extern GList *bt_wire_get_element_list(const BtWire *self);
extern void bt_wire_dbg_print_parts(const BtWire *self);

#endif // BT_WIRE_METHDOS_H
