/** $Id: setup.c,v 1.2 2004-05-06 18:26:58 ensonic Exp $
 * class for machine and wire setup
 */
 
#define BT_CORE
#define BT_SETUP_C

#include <libbtcore/core.h>

enum {
  SETUP_SONG=1
};

struct _BtSetupPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the setup belongs to */
	BtSong *song;
	
	GList *machines;	// each entry points to BtMachine
	GList *wires;			// each entry points to BtWire
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_setup_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSetup *self = BT_SETUP(object);
  return_if_disposed();
  switch (property_id) {
    case SETUP_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_setup_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSetup *self = BT_SETUP(object);
  return_if_disposed();
  switch (property_id) {
    case SETUP_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_INFO("set the song for setup: %p",self->private->song);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_setup_dispose(GObject *object) {
  BtSetup *self = BT_SETUP(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_setup_finalize(GObject *object) {
  BtSetup *self = BT_SETUP(object);
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_setup_init(GTypeInstance *instance, gpointer g_class) {
  BtSetup *self = BT_SETUP(instance);
  self->private = g_new0(BtSetupPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_setup_class_init(BtSetupClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_setup_set_property;
  gobject_class->get_property = bt_setup_get_property;
  gobject_class->dispose      = bt_setup_dispose;
  gobject_class->finalize     = bt_setup_finalize;

  g_param_spec = g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the setup belongs to",
                                     BT_SONG_TYPE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 SETUP_SONG,
                                 g_param_spec);
}

GType bt_setup_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSetupClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_setup_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSetup),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_setup_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSetupType",&info,0);
  }
  return type;
}

