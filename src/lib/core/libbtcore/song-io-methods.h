/* $Id: song-io-methods.h,v 1.3 2004-05-10 20:46:36 ensonic Exp $
* defines all public methods of the song-io base class
*/

#ifndef BT_SONG_IO_METHODS_H
#define BT_SONG_IO_METHODS_H

#include "song-io.h"

GType bt_song_io_detect(const gchar *filename);

gboolean bt_song_io_load(const gpointer self, const BtSong *song, const gchar *filename);

#endif // BT_SONG_IO_METHDOS_H
