/* $Id: song-methods.h,v 1.2 2004-05-04 14:21:45 ensonic Exp $
* defines all public methods of the song-class
*/

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"

/** load the song from a file
 */
void bt_song_load(const BtSong *self, const gchar *filename);

/** start to play the entire song 
 * at the moment this method does nothing!
 */
void bt_song_start_play(const BtSong *self);

#endif //BT_SONG_METHDOS_H 
