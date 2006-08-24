/* $Id: song-io-methods.h,v 1.9 2006-08-24 20:00:53 ensonic Exp $
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

#ifndef BT_SONG_IO_METHODS_H
#define BT_SONG_IO_METHODS_H

#include "song.h"
#include "song-io.h"

extern BtSongIO *bt_song_io_new(const gchar *file_name);

extern gboolean bt_song_io_load(const gpointer self, const BtSong *song);
extern gboolean bt_song_io_save(const gpointer self, const BtSong *song);

#endif // BT_SONG_IO_METHDOS_H
