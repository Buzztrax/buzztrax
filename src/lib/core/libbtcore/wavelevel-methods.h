/* $Id: wavelevel-methods.h,v 1.2 2005-04-13 18:11:55 ensonic Exp $
 * defines all public methods of the wavelevel class
 */

#ifndef BT_WAVELEVEL_METHODS_H
#define BT_WAVELEVEL_METHODS_H

#include "wavelevel.h"

extern BtWavelevel *bt_wavelevel_new(const BtSong *song,const BtWave *wave,guchar root_note,gulong length,glong loop_start,glong loop_end,gulong rate);

#endif // BT_WAVELEVEL_METHDOS_H
