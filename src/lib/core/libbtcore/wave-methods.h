/* $Id: wave-methods.h,v 1.2 2005-04-12 18:56:01 ensonic Exp $
 * defines all public methods of the wave class
 */

#ifndef BT_WAVE_METHODS_H
#define BT_WAVE_METHODS_H

#include "wave.h"
#include "wavelevel.h"

extern BtWave *bt_wave_new(const BtSong *song);

extern gboolean bt_wave_add_wavelevel(const BtWave *self, const BtWavelevel *wavelevel);

#endif // BT_WAVE_METHDOS_H
