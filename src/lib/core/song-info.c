/** $Id: song-info.c,v 1.2 2004-05-05 13:53:15 ensonic Exp $
 * class for a machine to machine connection
 */
 
#define BT_CORE
#define BT_SONG_INFO_C

#include <libbtcore/core.h>

enum {
  SONG_INFO_SONG=1
};

struct _BtSongInfoPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the song-info belongs to */
	BtSong *song;
};

//-- methods

static gboolean bt_song_info_real_load(const BtSongInfo *self, const xmlNodePtr xml_node) {
	g_print("loading the meta-data from the song\n");
	return(TRUE);
}

//-- wrapper

gboolean bt_song_info_load(const BtSongInfo *self, const xmlNodePtr xml_node) {
	return(BT_SONG_INFO_GET_CLASS(self)->load(self,xml_node));
}

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_info_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSongInfo *self = (BtSongInfo *)object;
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_song_info_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSongInfo *self = (BtSongInfo *)object;
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      g_print("set the song for song-info: %p\n",self->private->song);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_song_info_dispose(GObject *object) {
  BtSongInfo *self = (BtSongInfo *)object;
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_song_info_finalize(GObject *object) {
  BtSongInfo *self = (BtSongInfo *)object;
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_song_info_init(GTypeInstance *instance, gpointer g_class) {
  BtSongInfo *self = (BtSongInfo*)instance;
	
	//g_print("song_info_init self=%p\n",self);
  self->private = g_new0(BtSongInfoPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_song_info_class_init(BtSongInfoClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_song_info_set_property;
  gobject_class->get_property = bt_song_info_get_property;
  gobject_class->dispose      = bt_song_info_dispose;
  gobject_class->finalize     = bt_song_info_finalize;
	
  klass->load       = bt_song_info_real_load;
	
  g_param_spec = g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the song-info belongs to",
                                     BT_SONG_TYPE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 SONG_INFO_SONG,
                                 g_param_spec);
}

GType bt_song_info_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongInfoClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_info_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSongInfo),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_info_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSongInfoType",&info,0);
  }
  return type;
}

