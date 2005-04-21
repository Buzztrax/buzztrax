/* $Id: song-io-native.h,v 1.14 2005-04-21 19:47:53 ensonic Exp $
 * class for native song input and output
 */

#ifndef BT_SONG_IO_NATIVE_H
#define BT_SONG_IO_NATIVE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_IO_NATIVE		        (bt_song_io_native_get_type ())
#define BT_SONG_IO_NATIVE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO_NATIVE, BtSongIONative))
#define BT_SONG_IO_NATIVE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO_NATIVE, BtSongIONativeClass))
#define BT_IS_SONG_IO_NATIVE(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO_NATIVE))
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
 * @xpath_get_meta: compiled xpath expression to get the song meta data
 * @xpath_get_setup: compiled xpath expression to get the virtual hardware setup
 * @xpath_get_patterns: compiled xpath expression to get the pattern definitions
 * @xpath_get_sequence: compiled xpath expression to get the timeline sequence
 * @xpath_get_sequence_labels: compiled xpath expression to get the labels of the timeline sequence
 * @xpath_get_sequence_tracks: compiled xpath expression to get the tracks of the timeline sequence
 * @xpath_get_sequence_length: compiled xpath expression to get the length of the timeline sequence
 * @xpath_get_wavetable: compiled xpath expression to get the wavetable definitions
 *
 * class for song input and output in native zip/xml format
 */
struct _BtSongIONativeClass {
  BtSongIOClass parent_class;

	/* class data */
	xmlXPathCompExprPtr xpath_get_meta;
	xmlXPathCompExprPtr xpath_get_setup;
	xmlXPathCompExprPtr xpath_get_patterns;
	xmlXPathCompExprPtr xpath_get_sequence;
  xmlXPathCompExprPtr xpath_get_sequence_labels;
  xmlXPathCompExprPtr xpath_get_sequence_tracks;
  xmlXPathCompExprPtr xpath_get_sequence_length;
	xmlXPathCompExprPtr xpath_get_wavetable;
};

/* used by SONG_IO_NATIVE_TYPE */
GType bt_song_io_native_get_type(void);

#endif // BT_SONG_IO_NATIVE_H
