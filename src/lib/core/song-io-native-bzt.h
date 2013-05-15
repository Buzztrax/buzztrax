/* Buzztrax
 * Copyright (C) 2008 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_SONG_IO_NATIVE_BZT_H
#define BT_SONG_IO_NATIVE_BZT_H

#include <glib.h>
#include <glib-object.h>

#include "song-io-native.h"

#define BT_TYPE_SONG_IO_NATIVE_BZT            (bt_song_io_native_bzt_get_type ())
#define BT_SONG_IO_NATIVE_BZT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO_NATIVE_BZT, BtSongIONativeBZT))
#define BT_SONG_IO_NATIVE_BZT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO_NATIVE_BZT, BtSongIONativeBZTClass))
#define BT_IS_SONG_IO_NATIVE_BZT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO_NATIVE_BZT))
#define BT_IS_SONG_IO_NATIVE_BZT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_IO_NATIVE_BZT))
#define BT_SONG_IO_NATIVE_BZT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_IO_NATIVE_BZT, BtSongIONativeBZTClass))

/* type macros */

typedef struct _BtSongIONativeBZT BtSongIONativeBZT;
typedef struct _BtSongIONativeBZTClass BtSongIONativeBZTClass;
typedef struct _BtSongIONativeBZTPrivate BtSongIONativeBZTPrivate;

/**
 * BtSongIONativeBZT:
 *
 * object for song input and output in native zip/xml format
 */
struct _BtSongIONativeBZT {
  const BtSongIONative parent;
  
  /*< private >*/
  BtSongIONativeBZTPrivate *priv;
};
/**
 * BtSongIONativeBZTClass:
 * @parent: parent class type
 *
 * Class for song input and output in native zip/xml format
 */
struct _BtSongIONativeBZTClass {
  const BtSongIONativeClass parent;

  /*< private >*/
};

GType bt_song_io_native_bzt_get_type(void) G_GNUC_CONST;

gboolean bt_song_io_native_bzt_copy_to_fd(const BtSongIONativeBZT * const self, const gchar *file_name, gint fd);
gboolean bt_song_io_native_bzt_copy_from_uri(const BtSongIONativeBZT * const self, const gchar *file_name, const gchar *uri);

#endif // BT_SONG_IO_NATIVE_BZT_H
