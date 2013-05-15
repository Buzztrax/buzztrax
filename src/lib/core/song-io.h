/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_SONG_IO_H
#define BT_SONG_IO_H

#include <glib.h>
#include <glib-object.h>

#include "song.h"

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
  const GObject parent;
  
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
typedef gboolean (*bt_song_io_virtual_load)(gconstpointer self, const BtSong * const song);
/**
 * bt_song_io_virtual_save:
 * @self: song-io instance
 * @song: song object to save
 *
 * Subclasses will override this methods with the saver function.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*bt_song_io_virtual_save)(gconstpointer const self, const BtSong * const song);

/**
 * BtSongIOClass:
 * @parent: parent class type
 * @load: virtual method for loading a song
 * @save: virtual method for saving a song
 *
 * Base class for song input and output plugins
 */
struct _BtSongIOClass {
  const GObjectClass parent;

  /* class methods */
  bt_song_io_virtual_load load;
  bt_song_io_virtual_save save;
};

/**
 * BtSongIOInit:
 *
 * Function to init the plugin.
 *
 * Returns: %TRUE if the plugin was initialized fine
 */
typedef gboolean (*BtSongIOInit)(void);

/* TODO(ensonic): additional fields:
 * - wheter it support load/save? (we have vmethods
 * - some flags?
 *
 * Ugly:
 * - still duplicated info with usr/share/mime database
 * - files are detected twice (e.g. nautilus, buzztrax)
 *   - pass the media-type as a comamndline option and map it right away
 */

/**
 * BtSongIOFormatInfo:
 * @type: the io module #GType
 * @name: format name
 * @mime_type: mime type
 * @extension: file extension
 *
 * Metadata structure for #BtSongIO plugins describing one format.
 */
typedef struct {
  GType type;
  const gchar *name;
  const gchar *mime_type;
  const gchar *extension;
} BtSongIOFormatInfo;

/**
 * BT_SONG_IO_MODULE_INFO_MAX_FORMATS:
 *
 * Maximum number of #BtSongIOFormatInfo per plugin (10).
 */
#define BT_SONG_IO_MODULE_INFO_MAX_FORMATS 10

/**
 * BtSongIOModuleInfo:
 * @init: pointer to init function, can be %NULL.
 * @formats: %NULL terminated array of formats supported by this plugin
 *
 * Metadata structure for #BtSongIO plugins.
 */
typedef struct {
  BtSongIOInit init;
  BtSongIOFormatInfo formats[BT_SONG_IO_MODULE_INFO_MAX_FORMATS];
} BtSongIOModuleInfo;

GType bt_song_io_get_type(void) G_GNUC_CONST;

BtSongIO *bt_song_io_from_file(const gchar * const file_name);
BtSongIO *bt_song_io_from_data(gpointer *data, guint len, const gchar *media_type);

const GList *bt_song_io_get_module_info_list(void);

gboolean bt_song_io_load(BtSongIO const *self, const BtSong * const song);
gboolean bt_song_io_save(BtSongIO const *self, const BtSong * const song);

#endif // BT_SONG_IO_H
