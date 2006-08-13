/* $Id: wave-methods.h,v 1.5 2006-08-13 12:45:07 ensonic Exp $
 * defines all public methods of the wave class
 */

#ifndef BT_WAVE_METHODS_H
#define BT_WAVE_METHODS_H

#include "wave.h"
#include "wavelevel.h"

extern BtWave *bt_wave_new(const BtSong *song,const gchar *name,const gchar *url,gulong index);

extern gboolean bt_wave_add_wavelevel(const BtWave *self, const BtWavelevel *wavelevel);

extern gboolean bt_wave_load_from_url(const BtWave *self);

#endif // BT_WAVE_METHDOS_H
