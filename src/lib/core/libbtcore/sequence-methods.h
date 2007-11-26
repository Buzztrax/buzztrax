/* $Id: sequence-methods.h,v 1.24 2007-11-26 22:36:43 ensonic Exp $
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

#ifndef BT_SEQUENCE_METHODS_H
#define BT_SEQUENCE_METHODS_H

#include "sequence.h"
#include "pattern.h"
#include "machine.h"

extern BtSequence *bt_sequence_new(const BtSong * const song);

extern BtMachine *bt_sequence_get_machine(const BtSequence * const self,const gulong track);

extern gboolean bt_sequence_add_track(const BtSequence * const self,const BtMachine * const machine);
extern gboolean bt_sequence_remove_track_by_ix(const BtSequence * const self, const gulong track);
extern gboolean bt_sequence_remove_track_by_machine(const BtSequence * const self,const BtMachine * const machine);
extern gboolean bt_sequence_move_track_left(const BtSequence * const self, const gulong track);
extern gboolean bt_sequence_move_track_right(const BtSequence * const self, const gulong track);

extern gchar *bt_sequence_get_label(const BtSequence * const self, const gulong time);
extern void bt_sequence_set_label(const BtSequence * const self, const gulong time, const gchar * const label);
extern BtPattern *bt_sequence_get_pattern(const BtSequence * const self, const gulong time, const gulong track);
extern void bt_sequence_set_pattern(const BtSequence * const self, const gulong time, const gulong track, const BtPattern * const pattern);

extern GstClockTime bt_sequence_get_bar_time(const BtSequence * const self);
extern GstClockTime bt_sequence_get_loop_time(const BtSequence * const self);
extern gulong bt_sequence_limit_play_pos(const BtSequence * const self, const gulong play_pos);

extern gboolean bt_sequence_is_pattern_used(const BtSequence * const self,const BtPattern * const pattern);

extern void bt_sequence_insert_row(const BtSequence * const self, const gulong time, const gulong track);
extern void bt_sequence_insert_full_row(const BtSequence * const self, const gulong time);
extern void bt_sequence_delete_row(const BtSequence * const self, const gulong time, const gulong track);
extern void bt_sequence_delete_full_row(const BtSequence * const self, const gulong time);


#endif // BT_SEQUENCE_METHDOS_H
