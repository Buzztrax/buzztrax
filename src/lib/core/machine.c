/** $Id: machine.c,v 1.1 2004-05-04 15:24:59 ensonic Exp $
 * cbase class for a machine
 */
 
#define BT_CORE
#define BT_MACHINE_C

#include <libbtcore/core.h>

enum {
  MACHINE_SONG=1
};

struct _BtMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the machine belongs to */
	BtSong *song;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this machine */
static void machine_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachine *self = (BtMachine *)object;
  return_if_disposed();
  switch (property_id) {
    case MACHINE_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachine *self = (BtMachine *)object;
  return_if_disposed();
  switch (property_id) {
    case MACHINE_SONG: {
			g_object_unref(G_OBJECT(self->private->song));
      self->private->song = g_object_ref(G_OBJECT(value));
      //g_print("set the song for machine: %p\n",self->private->song);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void machine_dispose(GObject *object) {
  BtMachine *self = (BtMachine *)object;
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void machine_finalize(GObject *object) {
  BtMachine *self = (BtMachine *)object;
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtMachine *self = (BtMachine*)instance;
  self->private = g_new0(BtMachinePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_machine_class_init(BtMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = machine_set_property;
  gobject_class->get_property = machine_get_property;
  gobject_class->dispose      = machine_dispose;
  gobject_class->finalize     = machine_finalize;

  g_param_spec = g_param_spec_string("song",
                                     "song contruct prop",
                                     "Set song object, the machine belongs to",
                                     NULL, /* default value */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 MACHINE_SONG,
                                 g_param_spec);
}

GType bt_machine_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachine),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_machine_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtMachineType",&info,0);
  }
  return type;
}

