/** $Id: song-info.h,v 1.4 2004-05-06 18:26:58 ensonic Exp $
 * class for a machine to machine connection
 */

#ifndef BT_SONG_INFO_H
#define BT_SONG_INFO_H

#include <glib.h>
#include <glib-object.h>

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

struct _BtSongInfo {
  GObject parent;
  
  /* private */
  BtSongInfoPrivate *private;
};
/* structure of the SongInfo class */
struct _BtSongInfoClass {
  GObjectClass parent;
	
  /* class methods */
};

/* used by SongInfo_TYPE */
GType bt_song_info_get_type(void);

#endif // BT_SONG_INFO_H

