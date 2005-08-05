/* $Id: song.h,v 1.21 2005-08-05 09:36:17 ensonic Exp $
 * class for a basic buzztard song
 */
 
#ifndef BT_SONG_H
#define BT_SONG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG            (bt_song_get_type ())
#define BT_SONG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG, BtSong))
#define BT_SONG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG, BtSongClass))
#define BT_IS_SONG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG))
#define BT_IS_SONG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG))
#define BT_SONG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG, BtSongClass))

/* type macros */

typedef struct _BtSong BtSong;
typedef struct _BtSongClass BtSongClass;
typedef struct _BtSongPrivate BtSongPrivate;

/**
 * BtSong:
 *
 * song project object
 * (contains #BtSongInfo, #BtSetup and #BtSequence)
 */
struct _BtSong {
  GObject parent;
  
  /*< private >*/
  BtSongPrivate *priv;
};
/**
 * BtSongClass:
 * @play_event: default play signal handler
 * @stop_event: default stop signal handler
 *
 * class of a song project object
 */
struct _BtSongClass {
  GObjectClass parent_class;
  
  void (*play_event)(const BtSong *song, gpointer user_data);
  void (*stop_event)(const BtSong *song, gpointer user_data);
};

/* used by SONG_TYPE */
GType bt_song_get_type(void);

#endif // BT_SONG_H

