/* $Id$
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

extern BtSongIO *bt_song_io_from_file(const gchar * const file_name);
extern BtSongIO *bt_song_io_from_data(gpointer *data, guint len, const gchar *media_type);

extern const GList *bt_song_io_get_module_info_list(void);

extern gboolean bt_song_io_load(BtSongIO const *self, const BtSong * const song);
extern gboolean bt_song_io_save(BtSongIO const *self, const BtSong * const song);

#endif // BT_SONG_IO_METHDOS_H
