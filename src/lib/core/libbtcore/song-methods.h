/* $Id: song-methods.h,v 1.12 2004-07-30 15:15:51 ensonic Exp $
 * defines all public methods of the song class
 */

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "song-info.h"
#include "setup.h"
#include "sequence.h"

extern BtSong *bt_song_new(const GstBin *bin);
extern BtSong *bt_song_new_with_name(const GstBin *bin, const gchar *name);

extern gboolean bt_song_play(const BtSong *self);
extern gboolean bt_song_stop(const BtSong *self);
extern gboolean bt_song_pause(const BtSong *self);
extern gboolean bt_song_continue(const BtSong *self);

extern BtSongInfo *bt_song_get_song_info(const BtSong *self);

extern BtSetup *bt_song_get_setup(const BtSong *self);

extern BtSequence *bt_song_get_sequence(const BtSong *self);

#endif // BT_SONG_METHDOS_H 
