/* $Id: song-io-methods.h,v 1.2 2004-05-07 11:46:57 ensonic Exp $
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
gboolean bt_song_io_load(const gpointer self, const BtSong *song, const gchar *filename);

#endif // BT_SONG_IO_METHDOS_H
