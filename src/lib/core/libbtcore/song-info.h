/* $Id: song-info.h,v 1.10 2004-09-29 16:56:46 ensonic Exp $
 * class for a song metadata
 */

#ifndef BT_SONG_INFO_H
#define BT_SONG_INFO_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_INFO		         (bt_song_info_get_type ())
#define BT_SONG_INFO(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_INFO, BtSongInfo))
#define BT_SONG_INFO_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_INFO, BtSongInfoClass))
#define BT_IS_SONG_INFO(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_INFO))
#define BT_IS_SONG_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_INFO))
#define BT_SONG_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_INFO, BtSongInfoClass))

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
  BtSongInfoPrivate *priv;
};
/* structure of the SongInfo class */
struct _BtSongInfoClass {
  GObjectClass parent_class;
};

/* used by SONG_INFO_TYPE */
GType bt_song_info_get_type(void);

#endif // BT_SONG_INFO_H

