/* $Id: pattern.c,v 1.1 2004-07-07 12:49:55 ensonic Exp $
 * class for an event pattern of a #BtMachine instance
 */
 
#define BT_CORE
#define BT_PATTERN_C

#include <libbtcore/core.h>

enum {
  PATTERN_SONG=1,
  PATTERN_LENGTH,
  PATTERN_TRACKS
};

struct _BtPatternPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the pattern belongs to */
	BtSong *song;

  /* the number of tick entries */
  glong      length;

  /* the number of tracks */
  glong tracks;

  /* the machine the pattern belongs to */
  BtMachine *machine;
	GValue  ***data;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_pattern_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPattern *self = BT_PATTERN(object);
  return_if_disposed();
  switch (property_id) {
    case PATTERN_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    case PATTERN_LENGTH: {
      g_value_set_long(value, self->private->length);
      // @todo reallocate self->private->timelines
    } break;
    case PATTERN_TRACKS: {
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
static void bt_pattern_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPattern *self = BT_PATTERN(object);
  return_if_disposed();
  switch (property_id) {
    case PATTERN_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the song for pattern: %p",self->private->song);
    } break;
    case PATTERN_LENGTH: {
      self->private->length = g_value_get_long(value);
    } break;
    case PATTERN_TRACKS: {
      self->private->tracks = g_value_get_long(value);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_pattern_dispose(GObject *object) {
  BtPattern *self = BT_PATTERN(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_pattern_finalize(GObject *object) {
  BtPattern *self = BT_PATTERN(object);
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_pattern_init(GTypeInstance *instance, gpointer g_class) {
  BtPattern *self = BT_PATTERN(instance);
  self->private = g_new0(BtPatternPrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->length=0;
  self->private->tracks=0;
}

static void bt_pattern_class_init(BtPatternClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_pattern_set_property;
  gobject_class->get_property = bt_pattern_get_property;
  gobject_class->dispose      = bt_pattern_dispose;
  gobject_class->finalize     = bt_pattern_finalize;

  g_object_class_install_property(gobject_class,PATTERN_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the pattern belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,PATTERN_LENGTH,
																	g_param_spec_long("length",
                                     "length prop",
                                     "length of the pattern in ticks",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,PATTERN_TRACKS,
																	g_param_spec_long("tracks",
                                     "tracks prop",
                                     "number of tracks in the pattern",
                                     1,
                                     G_MAXLONG,
                                     1,
                                     G_PARAM_READWRITE));

}

GType bt_pattern_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtPatternClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_pattern_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtPattern),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_pattern_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtPattern",&info,0);
  }
  return type;
}

