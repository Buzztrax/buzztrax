/** $Id: machine.c,v 1.5 2004-05-07 15:16:04 ensonic Exp $
 * cbase class for a machine
 */
 
#define BT_CORE
#define BT_MACHINE_C

#include <libbtcore/core.h>

enum {
  MACHINE_SONG=1,
	MACHINE_ID,
	MACHINE_PLUGIN_NAME
};

struct _BtMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the machine belongs to */
	BtSong *song;
	/* the id, we can use to lookup the machine */
	gchar *id;
	/* the gst-plugin the machine is using */
	gchar *plugin_name;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachine *self = BT_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    case MACHINE_ID: {
      g_value_set_string(value, self->private->id);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_value_set_string(value, self->private->plugin_name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachine *self = BT_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_INFO("set the song for machine: %p",self->private->song);
    } break;
    case MACHINE_ID: {
      g_free(self->private->id);
      self->private->id = g_value_dup_string(value);
      GST_INFO("set the id for machine: %s",self->private->id);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_free(self->private->plugin_name);
      self->private->plugin_name = g_value_dup_string(value);
      GST_INFO("set the plugin_name for machine: %s",self->private->plugin_name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_machine_dispose(GObject *object) {
  BtMachine *self = BT_MACHINE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_machine_finalize(GObject *object) {
  BtMachine *self = BT_MACHINE(object);
	g_free(self->private->plugin_name);
	g_free(self->private->id);
	g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtMachine *self = BT_MACHINE(instance);
  self->private = g_new0(BtMachinePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_machine_class_init(BtMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_machine_set_property;
  gobject_class->get_property = bt_machine_get_property;
  gobject_class->dispose      = bt_machine_dispose;
  gobject_class->finalize     = bt_machine_finalize;

  g_param_spec = g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the machine belongs to",
                                     BT_SONG_TYPE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class,MACHINE_SONG,g_param_spec);

  g_param_spec = g_param_spec_string("id",
                                     "id contruct prop",
                                     "Set machine identifier",
                                     "unamed machine", /* default value */
                                     G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class,MACHINE_ID,g_param_spec);

  g_param_spec = g_param_spec_string("plugin_name",
                                     "plugin_name contruct prop",
                                     "Set the name of the gst plugin for the machine",
                                     "unamed machine", /* default value */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class,MACHINE_PLUGIN_NAME,g_param_spec);
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

