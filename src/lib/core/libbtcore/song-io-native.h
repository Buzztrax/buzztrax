/** $Id: song-io-native.h,v 1.1 2004-05-06 18:26:58 ensonic Exp $
 * class for native song input and output
 */

#ifndef BT_SONG_IO_NATIVE_H
#define BT_SONG_IO_NATIVE_H

#include <glib.h>
#include <glib-object.h>

#define BT_SONG_IO_NATIVE_TYPE		        (bt_song_io_native_get_type ())
#define BT_SONG_IO_NATIVE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SONG_IO_NATIVE_TYPE, BtSongIONative))
#define BT_SONG_IO_NATIVE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SONG_IO_NATIVE_TYPE, BtSongIONativeClass))
#define BT_IS_SONG_IO_NATIVE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SONG_IO_NATIVE_TYPE))
#define BT_IS_SONG_IO_NATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SONG_IO_NATIVE_TYPE))
#define BT_SONG_IO_NATIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SONG_IO_NATIVE_TYPE, BtSongIONativeClass))

/* type macros */

typedef struct _BtSongIONative BtSongIONative;
typedef struct _BtSongIONativeClass BtSongIONativeClass;
typedef struct _BtSongIONativePrivate BtSongIONativePrivate;

struct _BtSongIONative {
  BtSongIO parent;
  
  /* private */
  BtSongIONativePrivate *private;
};
/* structure of the song_io class */
struct _BtSongIONativeClass {
  BtSongIOClass parent;
  
};

/* used by SONG_IO_NATIVE_TYPE */
GType bt_song_io_native_get_type(void);


#endif // BT_SONG_IO_NATIVE_H

