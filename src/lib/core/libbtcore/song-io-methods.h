/* $Id: song-io-methods.h,v 1.7 2004-08-12 13:53:30 waffel Exp $
 * defines all public methods of the song-io base class
 */

#ifndef BT_SONG_IO_METHODS_H
#define BT_SONG_IO_METHODS_H

#include "song.h"
#include "song-io.h"

extern BtSongIO *bt_song_io_new(const gchar *file_name);

extern gboolean bt_song_io_load(const gpointer self, const BtSong *song);

#endif // BT_SONG_IO_METHDOS_H
