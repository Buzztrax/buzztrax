/* $Id: song-methods.h,v 1.6 2004-05-06 18:26:58 ensonic Exp $
* defines all public methods of the song class
*/

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "song-info.h"

/** start to play the entire song 
 * at the moment this method does nothing!
 */
void bt_song_start_play(const BtSong *self);

/** get the info conatiner obejct for the song */
BtSongInfo *bt_song_get_song_info(const BtSong *self);

#endif // BT_SONG_METHDOS_H 
