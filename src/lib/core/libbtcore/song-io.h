/** $Id: song-io.h,v 1.1 2004-05-06 18:26:58 ensonic Exp $
 * base class for song input and output
 */

#ifndef BT_SONG_IO_H
#define BT_SONG_IO_H

#include <glib.h>
#include <glib-object.h>

#define BT_SONG_IO_TYPE		         (bt_song_io_get_type ())
#define BT_SONG_IO(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SONG_IO_TYPE, BtSongIO))
#define BT_SONG_IO_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SONG_IO_TYPE, BtSongIOClass))
#define BT_IS_SONG_IO(obj)	       (G_TYPE_CHECK_TYPE ((obj), BT_SONG_IO_TYPE))
#define BT_IS_SONG_IO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SONG_IO_TYPE))
#define BT_SONG_IO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SONG_IO_TYPE, BtSongIOClass))

/* type macros */

typedef struct _BtSongIO BtSongIO;
typedef struct _BtSongIOClass BtSongIOClass;
typedef struct _BtSongIOPrivate BtSongIOPrivate;

struct _BtSongIO {
  GObject parent;
  
  /* private */
  BtSongIOPrivate *private;
};
/* structure of the song_io class */
struct _BtSongIOClass {
  GObjectClass parent;
  
  /* class methods */
	gboolean (*load)(const BtSongIO *self, const BtSong *song, const gchar *filename);
};

/* used by SONG_IO_TYPE */
GType bt_song_io_get_type(void);


#endif // BT_SONG_IO_H

