/* $Id: sequence.c,v 1.10 2004-07-07 15:39:03 ensonic Exp $
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

  /* the timelines that form the sequence */
  BtTimeLine **timelines;
};

//-- helper methods

/**
 * bt_sequence_free_timelines:
 * Freeing old timelines when we create/load a new song
 */
static void bt_sequence_free_timelines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_free_timelines");
  if(self->private->timelines) {
    if(self->private->length) {
      glong i;
      for(i=0;i<self->private->length;i++) {
        g_object_unref(G_OBJECT(self->private->timelines[i]));
      }
      self->private->length=0;
    }
    g_free(self->private->timelines);
    self->private->timelines=NULL;
  }
}

/**
 * bt_sequence_init_timelines:
 * Prepare new timelines when we create/load a new song
 */
static void bt_sequence_init_timelines(const BtSequence *self) {
  GST_DEBUG("bt_sequence_init_timelines");
  if(self->private->length) {
    self->private->timelines=g_new0(BtTimeLine*,self->private->length);
    if(self->private->timelines) {
      glong i;
      for(i=0;i<self->private->length;i++) {
        self->private->timelines[i]=g_object_new(BT_TYPE_TIMELINE,"song",self->private->song,NULL);
      }
    }
  }
}

/**
 * bt_sequence_init_timelinetracks:
 * Initialize the timelinetracks when we create/load a new song
 */
static void bt_sequence_init_timelinetracks(const BtSequence *self) {
  GST_DEBUG("bt_sequence_init_timelinetracks");
  if(self->private->timelines) {
    if(self->private->length && self->private->tracks) {
      glong i;
      GValue lval={0,};

      g_value_init(&lval,G_TYPE_LONG);
      g_value_set_long(&lval, self->private->tracks);
      for(i=0;i<self->private->length;i++) {
        g_object_set_property(G_OBJECT(self->private->timelines[i]),"tracks", &lval);
      }
    }
  }
}

//-- methods

/**
 * bt_sequence_get_timeline:
 * @self the #Sequence that holds the #TimeLine objects
 * @time the requested index
 *
 * fetches the required timeline.
 *
 * Returns: the object pointer or NULL in case of an error
 */
BtTimeLine *bt_sequence_get_timeline(const BtSequence *self,const glong time) {
  BtTimeLine *tl=NULL;
  
  if(time<self->private->length) {
    GST_DEBUG("  fetching timeline object for index %d",time);
    tl=self->private->timelines[time];
  }
  else {
    GST_ERROR(" index out of bounds %d should be < %d",time,self->private->length);
  }
  return(tl);
}

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
    } break;
    case SEQUENCE_TRACKS: {
      g_value_set_long(value, self->private->tracks);
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
      bt_sequence_free_timelines(self);
      self->private->length = g_value_get_long(value);
      GST_DEBUG("set the length for sequence: %d",self->private->length);
      bt_sequence_init_timelines(self);
    } break;
    case SEQUENCE_TRACKS: {
      self->private->tracks = g_value_get_long(value);
      GST_DEBUG("set the tracks for sequence: %d",self->private->tracks);
      bt_sequence_init_timelinetracks(self);
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
  bt_sequence_free_timelines(self);
  g_free(self->private);
}

static void bt_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtSequence *self = BT_SEQUENCE(instance);
  self->private = g_new0(BtSequencePrivate,1);
  self->private->dispose_has_run = FALSE;
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
                                     "number of tracks in the sequence",
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

