/* $Id: song-methods.h,v 1.7 2004-05-07 11:46:57 ensonic Exp $
* defines all public methods of the song class
*/

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "song-info.h"
#include "setup.h"
#include "sequence.h"

/** start to play the entire song 
 * at the moment this method does nothing!
 */
void bt_song_start_play(const BtSong *self);

/** get the song-info for the song */
BtSongInfo *bt_song_get_song_info(const BtSong *self);

/** get the setup for the song */
BtSetup *bt_song_get_setup(const BtSong *self);

/** get the sequence for the song */
BtSequence *bt_song_get_sequence(const BtSong *self);

#endif // BT_SONG_METHDOS_H 
