
#include <stdio.h>

#include "song.h"

enum {
  SONG_NAME=1
};

static void bt_song_init(GTypeInstance *instance, gpointer g_class);

static void bt_song_class_init(BtSongClass *klass);

static void song_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec);
                              
static void song_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec);

GType bt_song_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_class_init, // class_init
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
  
  //puts(__FUNCTION__);
}

static void bt_song_class_init(BtSongClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *bt_song_param_spec;
  
  gobject_class->set_property = song_set_property;
  gobject_class->get_property = song_get_property;
  
  bt_song_param_spec = g_param_spec_string("name",
                                           "name contruct prop",
                                           "Set songs name",
                                           "unnamed song", /* default value */
                                           G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 SONG_NAME,
                                 bt_song_param_spec);
}

static void song_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSong *self = (BtSong *)object;
  switch (property_id) {
    case SONG_NAME: {
      g_free(self->private->name);
      self->private->name = g_value_dup_string(value);
      //g_print("set the name for song: %s\n",self->private->name);
    } break;
  }
}

static void song_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSong *self = (BtSong *)object;
  switch (property_id) {
    case SONG_NAME: {
      g_value_set_string(value, self->private->name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}
