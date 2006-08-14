/* $Id: song-io-native.h,v 1.18 2006-08-14 20:49:40 ensonic Exp $
 * class for native song input and output
 */

#ifndef BT_SONG_IO_NATIVE_H
#define BT_SONG_IO_NATIVE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_IO_NATIVE            (bt_song_io_native_get_type ())
#define BT_SONG_IO_NATIVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO_NATIVE, BtSongIONative))
#define BT_SONG_IO_NATIVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO_NATIVE, BtSongIONativeClass))
#define BT_IS_SONG_IO_NATIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO_NATIVE))
#define BT_IS_SONG_IO_NATIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_IO_NATIVE))
#define BT_SONG_IO_NATIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_IO_NATIVE, BtSongIONativeClass))

/* type macros */

typedef struct _BtSongIONative BtSongIONative;
typedef struct _BtSongIONativeClass BtSongIONativeClass;
typedef struct _BtSongIONativePrivate BtSongIONativePrivate;

/**
 * BtSongIONative:
 *
 * object for song input and output in native zip/xml format
 */
struct _BtSongIONative {
  BtSongIO parent;
  
  /*< private >*/
  BtSongIONativePrivate *priv;
};
/**
 * BtSongIONativeClass:
 *
 * class for song input and output in native zip/xml format
 */
struct _BtSongIONativeClass {
  BtSongIOClass parent;

  /*< private >*/
};

/* used by SONG_IO_NATIVE_TYPE */
GType bt_song_io_native_get_type(void) G_GNUC_CONST;

#endif // BT_SONG_IO_NATIVE_H
