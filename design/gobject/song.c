
#include <stdio.h>

#include "song.h"

static void bt_song_init(GTypeInstance *instance, gpointer g_class);

GType bt_song_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongClass),
      NULL, // base_init
      NULL, // base_finalize
      NULL, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSong),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_init, // instance_init
    };
  type = g_type_register_static(G_TYPE_OBJECT,
                                "BtSongType",
                                &info, 0);
  }
  return type;
}


static void bt_song_init(GTypeInstance *instance, gpointer g_class) {
  BtSong *self = (BtSong*)instance;
  self->private = g_new0(BtSongPrivate,1);
  
  puts(__FUNCTION__);
}

