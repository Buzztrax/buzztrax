/* $Id: song-io.h,v 1.22 2006-08-24 20:00:53 ensonic Exp $
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

#ifndef BT_SONG_IO_H
#define BT_SONG_IO_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_IO             (bt_song_io_get_type ())
#define BT_SONG_IO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO, BtSongIO))
#define BT_SONG_IO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO, BtSongIOClass))
#define BT_IS_SONG_IO(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO))
#define BT_IS_SONG_IO_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_IO))
#define BT_SONG_IO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_IO, BtSongIOClass))

/* type macros */

typedef struct _BtSongIO BtSongIO;
typedef struct _BtSongIOClass BtSongIOClass;
typedef struct _BtSongIOPrivate BtSongIOPrivate;

/**
 * BtSongIO:
 *
 * base object for song input and output plugins
 */
struct _BtSongIO {
  GObject parent;
  
  /*< private >*/
  BtSongIOPrivate *priv;
};

/**
 * bt_song_io_virtual_load:
 * @self: song-io instance
 * @song: song object to load
 *
 * Subclasses will override this methods with the loader function.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*bt_song_io_virtual_load)(const gpointer self, const BtSong *song);
/**
 * bt_song_io_virtual_save:
 * @self: song-io instance
 * @song: song object to save
 *
 * Subclasses will override this methods with the saver function.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*bt_song_io_virtual_save)(const gpointer self, const BtSong *song);

/**
 * BtSongIOClass:
 * @load: virtual method for loading a song
 * @save: virtual method for saving a song
 *
 * base class for song input and output plugins
 */
struct _BtSongIOClass {
  GObjectClass parent;

  /* class methods */
  bt_song_io_virtual_load load;
  bt_song_io_virtual_save save;
};

/* used by SONG_IO_TYPE */
GType bt_song_io_get_type(void) G_GNUC_CONST;

/**
 * BtSongIODetect:
 * @file_name: the file to run the detection against
 *
 * Type of the file-format detect function.
 * Each #BtSongIO plugin must provide this one.
 *
 * Returns: the #GType of the song-io class on succes or %NULL otherwise
 */
typedef GType (*BtSongIODetect)(const gchar *file_name);

#endif // BT_SONG_IO_H
