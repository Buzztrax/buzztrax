/* $Id: song-methods.h,v 1.8 2004-05-11 16:16:38 ensonic Exp $
 * defines all public methods of the song class
 */

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "song-info.h"
#include "setup.h"
#include "sequence.h"

void bt_song_start_play(const BtSong *self);

BtSongInfo *bt_song_get_song_info(const BtSong *self);

BtSetup *bt_song_get_setup(const BtSong *self);

BtSequence *bt_song_get_sequence(const BtSong *self);

GstElement *bt_song_get_bin(const BtSong *self);

#endif // BT_SONG_METHDOS_H 
