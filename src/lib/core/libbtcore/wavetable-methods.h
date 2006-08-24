/* $Id: wavetable-methods.h,v 1.5 2006-08-24 20:00:53 ensonic Exp $
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

#ifndef BT_WAVETABLE_METHODS_H
#define BT_WAVETABLE_METHODS_H

#include "wavetable.h"
#include "wave.h"

extern BtWavetable *bt_wavetable_new(const BtSong *song);

extern gboolean bt_wavetable_add_wave(const BtWavetable *self, const BtWave *wave);
extern BtWave *bt_wavetable_get_wave_by_index(const BtWavetable *self, gulong index);

extern void bt_wavetable_remember_missing_wave(const BtWavetable *self,const gchar *str);

#endif // BT_WAVETABLE_METHDOS_H
