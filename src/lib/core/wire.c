/** $Id: wire.c,v 1.6 2004-05-06 18:26:58 ensonic Exp $
 * class for a machine to machine connection
 */
 
#define BT_CORE
#define BT_WIRE_C

#include <libbtcore/core.h>

enum {
  WIRE_SONG=1
};

struct _BtWirePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the wire belongs to */
	BtSong *song;
};

//-- methods

//static gboolean bt_wire_connect(const BtWire *self, const BtMachine *src, const BtMachine *dst) {
//}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wire_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWire *self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_wire_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWire *self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_INFO("set the song for wire: %p",self->private->song);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_wire_dispose(GObject *object) {
  BtWire *self = BT_WIRE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_wire_finalize(GObject *object) {
  BtWire *self = BT_WIRE(object);
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_wire_init(GTypeInstance *instance, gpointer g_class) {
  BtWire *self = BT_WIRE(instance);
  self->private = g_new0(BtWirePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_wire_class_init(BtWireClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_wire_set_property;
  gobject_class->get_property = bt_wire_get_property;
  gobject_class->dispose      = bt_wire_dispose;
  gobject_class->finalize     = bt_wire_finalize;

  g_param_spec = g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wire belongs to",
                                     BT_SONG_TYPE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 WIRE_SONG,
                                 g_param_spec);
}

GType bt_wire_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtWireClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtWire),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wire_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtWireType",&info,0);
  }
  return type;
}

