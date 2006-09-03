/* $Id: song-methods.h,v 1.23 2006-09-03 13:21:44 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_SONG_METHODS_H
#define BT_SONG_METHODS_H

#include "song.h"
#include "application.h"
#include "song-info.h"
#include "setup.h"
#include "sequence.h"

extern BtSong *bt_song_new(const BtApplication * const app);

extern void bt_song_set_unsaved(const BtSong * const self, const gboolean unsaved);

extern gboolean bt_song_idle_start(const BtSong * const self);
extern gboolean bt_song_idle_stop(const BtSong * const self);

extern gboolean bt_song_play(const BtSong * const self);
extern gboolean bt_song_stop(const BtSong * const self);
extern gboolean bt_song_pause(const BtSong * const self);
extern gboolean bt_song_continue(const BtSong * const self);

extern gboolean bt_song_update_playback_position(const BtSong * const self);

extern void bt_song_write_to_xml_file(const BtSong * const self);
extern void bt_song_write_to_dot_file(const BtSong * const self);

#endif // BT_SONG_METHDOS_H
