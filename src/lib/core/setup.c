/* $Id: setup.c,v 1.14 2004-07-13 16:52:11 ensonic Exp $
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
	
	// gfloat zoom;
	GList *machines;	// each entry points to BtMachine
	GList *wires;			// each entry points to BtWire
};

//-- methods

/**
 * bt_setup_add_machine:
 * @self: the setup to add the machine to
 * @machine: the new machine instance
 *
 * let the setup know that the suplied machine is now part of the song.
 */
void bt_setup_add_machine(const BtSetup *self, const BtMachine *machine) {
	self->private->machines=g_list_append(self->private->machines,g_object_ref(G_OBJECT(machine)));
}

/**
 * bt_setup_add_wire:
 * @self: the setup to add the wire to
 * @wire: the new wire instance
 *
 * let the setup know that the suplied wire is now part of the song.
 */
void bt_setup_add_wire(const BtSetup *self, const BtWire *wire) {
	self->private->wires=g_list_append(self->private->wires,g_object_ref(G_OBJECT(wire)));
}

/**
 * bt_setup_get_machine_by_id:
 * @self: the setup to search for the machine
 * @id: the identifier of the machine
 *
 * search the setup for a machine by the supplied id.
 * The machine must have been added previously to this setup with #bt_setup_add_machine().
 *
 * Returns: BtMachine instance or NULL if not found
 */
BtMachine *bt_setup_get_machine_by_id(const BtSetup *self, const gchar *id) {
	BtMachine *machine;
	GList* node=g_list_first(self->private->machines);
	
	while(node) {
		machine=BT_MACHINE(node->data);
		if(!strcmp(bt_g_object_get_string_property(G_OBJECT(machine),"id"),id)) return(machine);
		node=g_list_next(node);
	}
	return(NULL);
}
/* should we better use
 * GList* g_list_find_custom(GList *list, gconstpointer data, GCompareFunc func);
 * what we basically need is
 GObject *g_list_find_object_by_property(GList *list, const gchar *property_name, const GValue *value) {
   GValue val={0,};
	 g_value_init(&val,G_VALUE_TYPE(value));
	 //foreach(item in list)
	   g_object_get_property(G_OBJECT(item),property_name, &val);
	   //now how to compare the contents of value and &val ?
		 //use g_strdup_value_contents() with strcmp ?
		 //use gst_value_compare()
 */
 
/**
 * bt_setup_get_wire_by_src_machine:
 * @self: the setup to search for the wire
 * @src: the machine that is at the src end of the wire
 *
 * searches for the first wire in setup that uses the given machine as a source
 *
 * Returns: the BtWire or NULL 
 */
BtWire *bt_setup_get_wire_by_src_machine(const BtSetup *self,const BtMachine *src) {
	BtWire *wire;
	GList *node=g_list_first(self->private->wires);
	
	while(node) {
		wire=BT_WIRE(node->data);
		if(BT_MACHINE(bt_g_object_get_object_property(G_OBJECT(wire),"src"))==src) return(wire);
		node=g_list_next(node);
	}
	return(NULL);
}

/**
 * bt_setup_get_wire_by_dst_machine:
 * @self: the setup to search for the wire
 * @dst: the machine that is at the dst end of the wire
 *
 * searches for the first wire in setup that uses the given machine as a target
 *
 * Returns: the BtWire or NULL 
 */
BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup *self,const BtMachine *dst) {
	BtWire *wire;
	GList *node=g_list_first(self->private->wires);
	
	while(node) {
		wire=BT_WIRE(node->data);
		if(BT_MACHINE(bt_g_object_get_object_property(G_OBJECT(wire),"dst"))==dst) return(wire);
		node=g_list_next(node);
	}
	return(NULL);
}

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
      //GST_DEBUG("set the song for setup: %p",self->private->song);
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
	GList* node;

	// free list of wires
	if(self->private->wires) {
		node=g_list_first(self->private->wires);
		while(node) {
			g_object_unref(G_OBJECT(node->data));
			node=g_list_next(node);
		}
		g_list_free(self->private->wires);
		self->private->wires=NULL;
	}
	// free list of machines
	if(self->private->machines) {
		node=g_list_first(self->private->machines);
		while(node) {
			g_object_unref(G_OBJECT(node->data));
			node=g_list_next(node);
		}
		g_list_free(self->private->machines);
		self->private->wires=NULL;
	}
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
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
  g_object_class_install_property(gobject_class,SETUP_SONG,g_param_spec);
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
		type = g_type_register_static(G_TYPE_OBJECT,"BtSetup",&info,0);
  }
  return type;
}

