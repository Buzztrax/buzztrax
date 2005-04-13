/* $Id: wave-methods.h,v 1.3 2005-04-13 18:11:55 ensonic Exp $
 * defines all public methods of the wave class
 */

#ifndef BT_WAVE_METHODS_H
#define BT_WAVE_METHODS_H

#include "wave.h"
#include "wavelevel.h"

extern BtWave *bt_wave_new(const BtSong *song,const gchar *name,const gchar *file_name);

extern gboolean bt_wave_add_wavelevel(const BtWave *self, const BtWavelevel *wavelevel);

#endif // BT_WAVE_METHDOS_H
