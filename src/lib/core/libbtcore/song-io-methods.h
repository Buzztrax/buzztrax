/* $Id: song-io-methods.h,v 1.5 2004-07-12 16:38:49 ensonic Exp $
 * defines all public methods of the song-io base class
 */

#ifndef BT_SONG_IO_METHODS_H
#define BT_SONG_IO_METHODS_H

#include "song.h"
#include "song-io.h"

extern GType bt_song_io_detect(const gchar *filename);

extern gboolean bt_song_io_load(const gpointer self, const BtSong *song, const gchar *filename);

#endif // BT_SONG_IO_METHDOS_H
