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

#ifndef BT_WAVE_METHODS_H
#define BT_WAVE_METHODS_H

#include "wave.h"
#include "wavelevel.h"

extern BtWave *bt_wave_new(const BtSong * const song, const gchar * const name, const gchar * const uri, const gulong index, const gdouble volume, const  BtWaveLoopMode loop_mode);

extern gboolean bt_wave_add_wavelevel(const BtWave * const self, const BtWavelevel * const wavelevel);
extern BtWavelevel *bt_wave_get_level_by_index(const BtWave * const self,const gulong index);

#endif // BT_WAVE_METHDOS_H
