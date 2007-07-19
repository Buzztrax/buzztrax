
#include <stdio.h>

#include "song.h"

#define return_if_disposed(a) if(self->private->dispose_has_run) return a

enum {
  SONG_NAME=1
};

struct _BtSongPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the name for the song */
  gchar *name;
  gpointer data;
};


static void bt_song_real_start_play(BtSong *self) {
  /* emitting signal if we start play */
  g_signal_emit(self, 
                BT_SONG_GET_CLASS(self)->play_signal_id,
                0,
                NULL);
}

/* play method from song */
void bt_song_start_play(BtSong *self) {
  BT_SONG_GET_CLASS(self)->start_play(self);
}

static void song_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSong *self = (BtSong *)object;
  return_if_disposed();
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

static void song_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSong *self = (BtSong *)object;
  return_if_disposed();
  switch (property_id) {
    case SONG_NAME: {
      g_free(self->private->name);
      self->private->name = g_value_dup_string(value);
      //g_print("set the name for song: %s\n",self->private->name);
    } break;
  }
}

static void song_dispose(GObject *object) {
  BtSong *self = (BtSong *)object;
  if (self->private->dispose_has_run) {
    return;
  }
  self->private->dispose_has_run = TRUE;
}

static void song_finalize(GObject *object) {
  BtSong *self = (BtSong *)object;
  g_free(self->private);
}

static void bt_song_init(GTypeInstance *instance, gpointer g_class) {
  BtSong *self = (BtSong*)instance;
  self->private = g_new0(BtSongPrivate,1);
  self->private->dispose_has_run = FALSE;
  //puts(__FUNCTION__);
}

static void bt_song_class_init(BtSongClass *klass) {
  
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *bt_song_param_spec;
  
  gobject_class->set_property = song_set_property;
  gobject_class->get_property = song_get_property;
  gobject_class->dispose = song_dispose;
  gobject_class->finalize = song_finalize;
  
  klass->start_play = bt_song_real_start_play;
  
  /* adding simple signal */
  klass->play_signal_id = g_signal_new("play",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        (guint)G_STRUCT_OFFSET(BtSongClass,play_event),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0 // n_params
                                        );
  
  bt_song_param_spec = g_param_spec_string("name",
                                           "name contruct prop",
                                           "Set songs name",
                                           "unnamed song", /* default value */
                                           G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 SONG_NAME,
                                 bt_song_param_spec);
}

GType bt_song_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof (BtSongClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSong),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_init, // instance_init
      NULL // value_table
    };
  type = g_type_register_static(G_TYPE_OBJECT,
                                "BtSongType",
                                &info, 0);
  }
  return type;
}
