/* $Id: song-io-native.h,v 1.5 2004-05-14 16:59:22 ensonic Exp $
 * class for native song input and output
 */

#ifndef BT_SONG_IO_NATIVE_H
#define BT_SONG_IO_NATIVE_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_SONG_IO_NATIVE_TYPE:
 *
 * #GType for BtSongIONative instances
 */
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

/**
 * BtSongIONative:
 *
 * object for song input and output in native zip/xml format
 */
struct _BtSongIONative {
  BtSongIO parent;
  
  /* private */
  BtSongIONativePrivate *private;
};
/**
 * BtSongIONativeClass:
 * @xpath_get_meta: compiled xpath expression to get the song meta data
 * @xpath_get_setup: compiled xpath expression to get the virtual hardware setup
 * @xpath_get_patterns: compiled xpath expression to get the pattern definitions
 * @xpath_get_sequence: compiled xpath expression to get the timeline sequence
 *
 * class for song input and output in native zip/xml format
 */
struct _BtSongIONativeClass {
  BtSongIOClass parent;

	/* class data */
	xmlXPathCompExprPtr xpath_get_meta;
	xmlXPathCompExprPtr xpath_get_setup;
	xmlXPathCompExprPtr xpath_get_patterns;
	xmlXPathCompExprPtr xpath_get_sequence;
};

/* used by SONG_IO_NATIVE_TYPE */
GType bt_song_io_native_get_type(void);

#endif // BT_SONG_IO_NATIVE_H

