/* $Id: sequence.c,v 1.7 2004-07-06 15:44:57 ensonic Exp $
 * class for the pattern sequence
 */
 
#define BT_CORE
#define BT_SEQUENCE_C

#include <libbtcore/core.h>

enum {
  SEQUENCE_SONG=1,
  SEQUENCE_LENGTH,
  SEQUENCE_TRACKS
};

struct _BtSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the sequence belongs to */
	BtSong *song;

  /* the number of timeline entries */
  glong length;
  /* the number of tracks */
  glong tracks;

  //BtTimeLine **timelines;	// each entry points to a BtTimeLine;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_sequence_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSequence *self = BT_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    case SEQUENCE_LENGTH: {
      g_value_set_long(value, self->private->length);
      // @todo reallocate self->private->timelines
    } break;
    case SEQUENCE_TRACKS: {
      g_value_set_long(value, self->private->tracks);
      // @todo reallocate self->private->timelines->private->tracks
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_sequence_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSequence *self = BT_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the song for sequence: %p",self->private->song);
    } break;
    case SEQUENCE_LENGTH: {
      self->private->length = g_value_get_long(value);
    } break;
    case SEQUENCE_TRACKS: {
      self->private->tracks = g_value_get_long(value);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_sequence_dispose(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_sequence_finalize(GObject *object) {
  BtSequence *self = BT_SEQUENCE(object);
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtSequence *self = BT_SEQUENCE(instance);
  self->private = g_new0(BtSequencePrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->length=0;
  self->private->tracks=0;
}

static void bt_sequence_class_init(BtSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_sequence_set_property;
  gobject_class->get_property = bt_sequence_get_property;
  gobject_class->dispose      = bt_sequence_dispose;
  gobject_class->finalize     = bt_sequence_finalize;

  g_object_class_install_property(gobject_class,SEQUENCE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the sequence belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_LENGTH,
																	g_param_spec_long("length",
                                     "length prop",
                                     "length of the sequence in timeline bars",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,SEQUENCE_TRACKS,
																	g_param_spec_long("tracks",
                                     "tracks prop",
                                     "number of tracks of the sequence",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

}

GType bt_sequence_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSequence),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_sequence_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSequence",&info,0);
  }
  return type;
}

