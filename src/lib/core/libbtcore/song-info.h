/* $Id: song-info.h,v 1.6 2004-05-14 16:59:22 ensonic Exp $
 * class for a song metadata
 */

#ifndef BT_SONG_INFO_H
#define BT_SONG_INFO_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_SONG_INFO_TYPE:
 *
 * #GType for BtSongInfo instances
 */
#define BT_SONG_INFO_TYPE		         (bt_song_info_get_type ())
#define BT_SONG_INFO(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SONG_INFO_TYPE, BtSongInfo))
#define BT_SONG_INFO_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SONG_INFO_TYPE, BtSongInfoClass))
#define BT_IS_SONG_INFO(obj)	       (G_TYPE_CHECK_TYPE ((obj), BT_SONG_INFO_TYPE))
#define BT_IS_SONG_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SONG_INFO_TYPE))
#define BT_SONG_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SONG_INFO_TYPE, BtSongInfoClass))

/* type macros */

typedef struct _BtSongInfo BtSongInfo;
typedef struct _BtSongInfoClass BtSongInfoClass;
typedef struct _BtSongInfoPrivate BtSongInfoPrivate;

/**
 * BtSongInfo:
 *
 * holds song metadata
 */
struct _BtSongInfo {
  GObject parent;
  
  /* private */
  BtSongInfoPrivate *private;
};
/* structure of the SongInfo class */
struct _BtSongInfoClass {
  GObjectClass parent;
};

/* used by SONG_INFO_TYPE */
GType bt_song_info_get_type(void);

#endif // BT_SONG_INFO_H

