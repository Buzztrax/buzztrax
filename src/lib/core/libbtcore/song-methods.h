/* $Id: song-methods.h,v 1.19 2005-08-29 22:21:03 ensonic Exp $
 * defines all public methods of the song class
 */

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "application.h"
#include "song-info.h"
#include "setup.h"
#include "sequence.h"

extern BtSong *bt_song_new(const BtApplication *app);

extern void bt_song_set_unsaved(const BtSong *self,gboolean unsaved);

extern gboolean bt_song_play(const BtSong *self);
extern gboolean bt_song_stop(const BtSong *self);
extern gboolean bt_song_pause(const BtSong *self);
extern gboolean bt_song_continue(const BtSong *self);

extern gboolean bt_song_update_playback_position(const BtSong *self);

extern void bt_song_write_to_xml_file(const BtSong *self);

#endif // BT_SONG_METHDOS_H
