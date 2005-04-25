/* $Id: wavetable-methods.h,v 1.3 2005-04-25 15:27:08 ensonic Exp $
 * defines all public methods of the wavetable class
 */

#ifndef BT_WAVETABLE_METHODS_H
#define BT_WAVETABLE_METHODS_H

#include "wavetable.h"
#include "wave.h"

extern BtWavetable *bt_wavetable_new(const BtSong *song);

extern gboolean bt_wavetable_add_wave(const BtWavetable *self, const BtWave *wave);
extern BtWave *bt_wavetable_get_wave_by_index(const BtWavetable *self, gulong index);

#endif // BT_WAVETABLE_METHDOS_H
