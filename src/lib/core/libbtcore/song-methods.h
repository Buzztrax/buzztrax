/* $Id: song-methods.h,v 1.10 2004-07-12 16:38:50 ensonic Exp $
 * defines all public methods of the song class
 */

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "song-info.h"
#include "setup.h"
#include "sequence.h"

extern void bt_song_start_play(const BtSong *self);

extern BtSongInfo *bt_song_get_song_info(const BtSong *self);

extern BtSetup *bt_song_get_setup(const BtSong *self);

extern BtSequence *bt_song_get_sequence(const BtSong *self);

#endif // BT_SONG_METHDOS_H 
