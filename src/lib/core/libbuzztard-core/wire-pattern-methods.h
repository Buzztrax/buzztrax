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

#ifndef BT_WIRE_PATTERN_METHODS_H
#define BT_WIRE_PATTERN_METHODS_H

#include "wire-pattern.h"

extern BtWirePattern *bt_wire_pattern_new(const BtSong * const song, const BtWire * const wire, const BtPattern * const pattern);

extern BtWirePattern *bt_wire_pattern_copy(const BtWirePattern * const self, const BtPattern * const pattern);

extern GValue *bt_wire_pattern_get_event_data(const BtWirePattern * const self, const gulong tick, const gulong param);

extern gboolean bt_wire_pattern_set_event(const BtWirePattern * const self, const gulong tick, const gulong param, const gchar * const value);
extern gchar *bt_wire_pattern_get_event(const BtWirePattern * const self, const gulong tick, const gulong param);

extern gboolean bt_wire_pattern_test_event(const BtWirePattern * const self, const gulong tick, const gulong param);
extern gboolean bt_wire_pattern_tick_has_data(const BtWirePattern * const self, const gulong tick);

extern void bt_wire_pattern_insert_row(const BtWirePattern * const self, const gulong tick, const gulong param);
extern void bt_wire_pattern_insert_full_row(const BtWirePattern * const self, const gulong tick);
extern void bt_wire_pattern_delete_row(const BtWirePattern * const self, const gulong tick, const gulong param);
extern void bt_wire_pattern_delete_full_row(const BtWirePattern * const self, const gulong tick);

extern void bt_wire_pattern_delete_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
extern void bt_wire_pattern_delete_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);

extern void bt_wire_pattern_blend_column(const BtWirePattern * const self, const gulong start_tick,const gulong end_tick, const gulong param); 
extern void bt_wire_pattern_blend_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);
extern void bt_wire_pattern_flip_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
extern void bt_wire_pattern_flip_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);
extern void bt_wire_pattern_randomize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
extern void bt_wire_pattern_randomize_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);

extern void bt_wire_pattern_serialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data);
extern void bt_wire_pattern_serialize_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, GString *data);
extern gboolean bt_wire_pattern_deserialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, const gchar *data);

#endif // BT_WIRE_PATTERN_METHDOS_H
