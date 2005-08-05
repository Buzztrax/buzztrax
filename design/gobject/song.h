#ifndef SONG_H
#define SONG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG            (bt_song_get_type ())
#define BT_SONG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG, BtSong))
#define BT_SONG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG, BtSongClass))
#define BT_IS_SONG(obj)          (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_SONG))
#define BT_IS_SONG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG))
#define BT_SONG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG, BtSongClass))

/*
* type macros
*/
typedef struct _BtSong BtSong;
typedef struct _BtSongClass BtSongClass;
typedef struct _BtSongPrivate BtSongPrivate;

struct _BtSong {
  GObject parent;
  
  /*< private >*/
  BtSongPrivate *private;
};

struct _BtSongClass {
  GObjectClass parent_class;
  
  /* id for the play signal */
  guint play_signal_id;
  /* class method start_play */
  void (*start_play) (BtSong *self);
};

/* used by SONG_TYPE */
GType bt_song_get_type(void);

/* play method of the song class */
void bt_song_start_play(BtSong *self);

#endif //SONG_H
