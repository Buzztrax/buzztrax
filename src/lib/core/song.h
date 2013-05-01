/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef BT_SONG_H
#define BT_SONG_H

#include <glib.h>
#include <glib-object.h>

#include "application.h"

#define BT_TYPE_SONG            (bt_song_get_type ())
#define BT_SONG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG, BtSong))
#define BT_SONG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG, BtSongClass))
#define BT_IS_SONG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG))
#define BT_IS_SONG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG))
#define BT_SONG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG, BtSongClass))

/* type macros */

typedef struct _BtSong BtSong;
typedef struct _BtSongClass BtSongClass;
typedef struct _BtSongPrivate BtSongPrivate;

/**
 * BtSong:
 *
 * Song project object
 * (contains #BtSongInfo, #BtSetup and #BtSequence)
 */
struct _BtSong {
  const GObject parent;
  
  /*< private >*/
  BtSongPrivate *priv;
};

/**
 * BtSongClass:
 * @parent: parent class type
 *
 * Base class for songs
 */
struct _BtSongClass {
  const GObjectClass parent;
 
  /*< private >*/
};

GType bt_song_get_type(void) G_GNUC_CONST;

BtSong *bt_song_new(const BtApplication * const app);

gboolean bt_song_play(const BtSong * const self);
gboolean bt_song_stop(const BtSong * const self);
gboolean bt_song_pause(const BtSong * const self);
gboolean bt_song_continue(const BtSong * const self);

gboolean bt_song_update_playback_position(const BtSong * const self);

// for debugging
void bt_song_write_to_highlevel_dot_file(const BtSong * const self);
void bt_song_write_to_lowlevel_dot_file(const BtSong * const self);

#endif // BT_SONG_H
