/* $Id: song.h,v 1.12 2004-05-13 18:43:30 ensonic Exp $
 * class for a basic buzztard song
 */
 
#ifndef BT_SONG_H
#define BT_SONG_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_SONG_TYPE:
 *
 * #GType for BtSong instances
 */
#define BT_SONG_TYPE		        (bt_song_get_type ())
#define BT_SONG(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SONG_TYPE, BtSong))
#define BT_SONG_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SONG_TYPE, BtSongClass))
#define BT_IS_SONG(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SONG_TYPE))
#define BT_IS_SONG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SONG_TYPE))
#define BT_SONG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SONG_TYPE, BtSongClass))

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
  
  /* private */
  BtSongPrivate *private;
};
/**
 * BtSongClass:
 * @play_signal_id: will be emitted upon starting to play a song
 *
 * class of a song project object
 */
struct _BtSongClass {
  GObjectClass parent;
  
  guint play_signal_id;

  void (*start_play)(const BtSong *self);
};

/* used by SONG_TYPE */
GType bt_song_get_type(void);

#endif // BT_SONG_H

