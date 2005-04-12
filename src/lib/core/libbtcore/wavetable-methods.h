/* $Id: wavetable-methods.h,v 1.2 2005-04-12 18:56:01 ensonic Exp $
 * defines all public methods of the wavetable class
 */

#ifndef BT_WAVETABLE_METHODS_H
#define BT_WAVETABLE_METHODS_H

#include "wavetable.h"
#include "wave.h"

extern BtWavetable *bt_wavetable_new(const BtSong *song);

extern gboolean bt_wavetable_add_wave(const BtWavetable *self, const BtWave *wave);

#endif // BT_WAVETABLE_METHDOS_H
