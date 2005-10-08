/* $Id: song-io.h,v 1.18 2005-10-08 18:12:13 ensonic Exp $
 * base class for song input and output
 */

#ifndef BT_SONG_IO_H
#define BT_SONG_IO_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_IO             (bt_song_io_get_type ())
#define BT_SONG_IO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO, BtSongIO))
#define BT_SONG_IO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO, BtSongIOClass))
#define BT_IS_SONG_IO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO))
#define BT_IS_SONG_IO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_IO))
#define BT_SONG_IO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_IO, BtSongIOClass))

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
  GObjectClass parent_class;

  /* class methods */
  bt_song_io_virtual_load load;
  bt_song_io_virtual_save save;

  void (*status_changed)(const BtSongIO *songio, gpointer user_data);
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
