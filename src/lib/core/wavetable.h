/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_WAVETABLE_H
#define BT_WAVETABLE_H

#include <glib.h>
#include <glib-object.h>

/**
 * BtWavetable:
 *
 * Holds a list of sound samples that can be used by machines.
 *
 * Also implements GListModel for ease of use by list views.
 * The container element type is BtWave.
 */
G_DECLARE_FINAL_TYPE(BtWavetable, bt_wavetable, BT, WAVETABLE, GObject);

#define BT_TYPE_WAVETABLE             (bt_wavetable_get_type ())

#include "song.h"
#include "wave.h"

BtWavetable *bt_wavetable_new(const BtSong * const song);

gboolean bt_wavetable_add_wave(BtWavetable * const self, const BtWave * const wave);
gboolean bt_wavetable_remove_wave(BtWavetable * const self, const BtWave * const wave);

BtWave *bt_wavetable_get_wave_by_index(const BtWavetable * const self, const gulong index);

void bt_wavetable_remember_missing_wave(BtWavetable * const self, const gchar * const str);

#endif // BT_WAVETABLE_H
