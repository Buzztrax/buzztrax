/** $Id: wire.c,v 1.1 2004-05-04 13:47:25 ensonic Exp $
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



/* returns a property for the given property_id for this wire */
static void wire_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWire *self = (BtWire *)object;
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
static void wire_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWire *self = (BtWire *)object;
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
			g_object_unref(G_OBJECT(self->private->song));
      self->private->song = g_object_ref(G_OBJECT(value));
      //g_print("set the song for wire: %p\n",self->private->song);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void wire_dispose(GObject *object) {
  BtWire *self = (BtWire *)object;
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void wire_finalize(GObject *object) {
  BtWire *self = (BtWire *)object;
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_wire_init(GTypeInstance *instance, gpointer g_class) {
  BtWire *self = (BtWire*)instance;
  self->private = g_new0(BtWirePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_wire_class_init(BtWireClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = wire_set_property;
  gobject_class->get_property = wire_get_property;
  gobject_class->dispose = wire_dispose;
  gobject_class->finalize = wire_finalize;

  g_param_spec = g_param_spec_string("song",
                                     "song contruct prop",
                                     "Set song object, the wire belongs to",
                                     NULL, /* default value */
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
  type = g_type_register_static(G_TYPE_OBJECT,
                                "BtWireType",
                                &info, 0);
  }
  return type;
}

