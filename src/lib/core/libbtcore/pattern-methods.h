/* $Id: pattern-methods.h,v 1.18 2006-09-03 13:21:44 ensonic Exp $
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

#ifndef BT_PATTERN_METHODS_H
#define BT_PATTERN_METHODS_H

#include "pattern.h"

extern BtPattern *bt_pattern_new(const BtSong * const song, const gchar * const id, const gchar * const name, const gulong length, const BtMachine * const machine);
extern BtPattern *bt_pattern_new_with_event(const BtSong * const song, const BtMachine * const machine, const BtPatternCmd cmd);

extern BtPattern *bt_pattern_copy(const BtPattern * const self);

extern gulong bt_pattern_get_global_param_index(const BtPattern * const self, const gchar * const name, GError **error);
extern gulong bt_pattern_get_voice_param_index(const BtPattern * const self, const gchar * const name, GError **error);

extern GValue *bt_pattern_get_global_event_data(const BtPattern * const self, const gulong tick, const gulong param);
extern GValue *bt_pattern_get_voice_event_data(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param);

extern gboolean bt_pattern_set_global_event(const BtPattern * const self, const gulong tick, const gulong param, const gchar * const value);
extern gboolean bt_pattern_set_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param, const gchar * const value);
extern gchar *bt_pattern_get_global_event(const BtPattern * const self, const gulong tick, const gulong param);
extern gboolean bt_pattern_test_global_event(const BtPattern * const self, const gulong tick, const gulong param);
extern gboolean bt_pattern_test_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param);

extern gchar *bt_pattern_get_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param);

extern BtPatternCmd bt_pattern_get_cmd(const BtPattern * const self, const gulong tick);

extern gboolean bt_pattern_tick_has_data(const BtPattern * const self, const gulong tick);

extern void bt_pattern_insert_row(const BtPattern * const self, const gulong tick, const gulong param);

#endif // BT_PATTERN_METHDOS_H
