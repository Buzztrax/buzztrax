/* $Id: song-io-methods.h,v 1.1 2004-05-06 18:26:58 ensonic Exp $
* defines all public methods of the song-io base class
*/

#ifndef BT_SONG_IO_METHODS_H
#define BT_SONG_IO_METHODS_H

#include "song-io.h"

/** factory method that returns the GType of the class that is able to handle
 * the supplied file
 */
GType bt_song_io_detect(const gchar *filename);

/** load the song from a file
 */
gboolean bt_song_io_load(const BtSongIO *self, const BtSong *song, const gchar *filename);

#endif // BT_SONG_IO_METHDOS_H
