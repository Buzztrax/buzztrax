/* $Id: machine.c,v 1.13 2004-07-06 15:44:57 ensonic Exp $
 * base class for a machine
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
  
  GList *patterns;	// each entry points to BtPattern
};

//-- helper methods

static gboolean bt_machine_init_gst_element(BtMachine *self) {
	if(self->machine) {
		/* @todo change id of machine in gst */
	}
	else {
		if(self->private->id && self->private->plugin_name) {
			GValue oval={0,};
			
		  self->machine=gst_element_factory_make(self->private->plugin_name,self->private->id);
			if(!self->machine) {
				GST_ERROR("  failed to instantiate machine \"%s\"",self->private->plugin_name);
				return(FALSE);
			}
			// there is no adder or spreader in use by default
			self->dst_elem=self->src_elem=self->machine;
			GST_DEBUG("  instantiated machine \"%s\"",self->private->plugin_name);
			if((self->dparam_manager=gst_dpman_get_manager(self->machine))) {
				GParamSpec **specs;
				GstDParam **dparam;
				guint i;
		
				// setting param mode. Only synchronized is currently supported
				gst_dpman_set_mode(self->dparam_manager, "synchronous");
				GST_DEBUG("  machine \"%s\" supports dparams",self->private->plugin_name);
				
				/** @todo manage dparams */
				specs=gst_dpman_list_dparam_specs(self->dparam_manager);
				// count the specs
				for(i=0;specs[i];i++);
				self->dparams=(GstDParam **)g_new0(gpointer,i);
				self->dparams_count=i;
				// iterate over all dparam
				for(i=0,dparam=self->dparams;specs[i];i++,dparam++) {
					*dparam=gst_dparam_new(G_PARAM_SPEC_VALUE_TYPE(specs[i]));
					gst_dpman_attach_dparam(self->dparam_manager,g_param_spec_get_name(specs[i]),*dparam);
					GST_DEBUG("    added param \"%s\"",g_param_spec_get_name(specs[i]));
				}
			}
			g_value_init(&oval,G_TYPE_OBJECT);
			g_object_get_property(G_OBJECT(self->private->song),"bin",&oval);
			gst_bin_add(GST_BIN(g_value_get_object(&oval)), self->machine);
      g_assert(self->machine!=NULL);
      g_assert(self->src_elem!=NULL);
      g_assert(self->dst_elem!=NULL);
      return(TRUE);
		}
	}
  return(FALSE);
}

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
      //GST_DEBUG("set the song for machine: %p",self->private->song);
    } break;
    case MACHINE_ID: {
      g_free(self->private->id);
      self->private->id = g_value_dup_string(value);
      GST_DEBUG("set the id for machine: %s",self->private->id);
			bt_machine_init_gst_element(self);
    } break;
    case MACHINE_PLUGIN_NAME: {
      g_free(self->private->plugin_name);
      self->private->plugin_name = g_value_dup_string(value);
      GST_DEBUG("set the plugin_name for machine: %s",self->private->plugin_name);
			bt_machine_init_gst_element(self);
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
                                     BT_TYPE_SONG, /* object type */
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
		type = g_type_register_static(G_TYPE_OBJECT,"BtMachine",&info,0);
  }
  return type;
}

