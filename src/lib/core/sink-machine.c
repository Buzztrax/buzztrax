/* $Id: sink-machine.c,v 1.34 2005-02-07 21:45:25 ensonic Exp $
 * class for a sink machine
 */
 
#define BT_CORE
#define BT_SINK_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/sink-machine.h>
#include <libbtcore/machine-private.h>

struct _BtSinkMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_sink_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 *
 * Create a new instance.
 * A machine should be added to a songs setup using
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 * The plugin used for this machine is taken from the #BtSettings.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkMachine *bt_sink_machine_new(const BtSong *song, const gchar *id) {
  BtSinkMachine *self;
  BtApplication *app;
  BtSettings *settings;
  gchar *audiosink_name,*system_audiosink_name,*sink_name,*eon;
  gchar *plugin_name=NULL;

  g_assert(BT_IS_SONG(song));
  g_assert(id);

  // get plugin_name from song->app->settings
  g_object_get(G_OBJECT(song),"app",&app,NULL);
  g_object_get(app,"settings",&settings,NULL);
  g_object_get(settings,"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);
  if(is_string(audiosink_name)) plugin_name=audiosink_name;
  else if(is_string(system_audiosink_name)) {
		// this can be a whole pipeline like "audioconvert ! osssink sync=false"
		// seek for the last '!'
		if(!(sink_name=g_strrstr(system_audiosink_name,"!"))) {
			sink_name=system_audiosink_name;
		}
		// if there is a space following put '\0' in there
		if(eon=strstr(sink_name," ")) {
			*eon='\0';
		}
		plugin_name=sink_name;
	}
  else {
    // iterate over gstreamer-audiosink list and choose element with highest rank
    GList *node,*audiosink_names=bt_gst_registry_get_element_names_by_class("Sink/Audio");
		GstElementFactory *factory;
		guint max_rank=0,cur_rank;
		
		node=audiosink_names;
		while(node) {
			factory=gst_element_factory_find(node->data);
			cur_rank=gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
			//GST_INFO("  trying audio sink: \"%s\" with rank: %d",node->data,cur_rank);
			if((cur_rank>max_rank) || (!plugin_name)) {
      	plugin_name=node->data;
				max_rank=cur_rank;
			}
			node=g_list_next(node);
    }
    g_list_free(audiosink_names);
  }
	GST_INFO("using audio sink : \"%s\"",plugin_name);
  if((!plugin_name) || (!strlen(plugin_name))) {
    GST_ERROR("no audiosink configured/register");
    goto Error;
  }
  
  if(!(self=BT_SINK_MACHINE(g_object_new(BT_TYPE_SINK_MACHINE,"song",song,"id",id,"plugin-name",plugin_name,NULL)))) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }  
  g_free(system_audiosink_name);
  g_free(audiosink_name);
  g_object_try_unref(settings);
  g_object_try_unref(app);
  return(self);
Error:
  g_free(system_audiosink_name);
  g_object_try_unref(settings);
  g_object_try_unref(app);
  g_object_try_unref(self);
  return(NULL);
}

/*
 * bt_sink_machine_new_recorder:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @format: specify the file format to record in
 *
 *
BtSinkMachine *bt_sink_machine_new_recorder(const BtSong *song, const gchar *id, const gchar *format) {
 	// get gst mimetype from the extension
  // and then look at all encoders which supply that mimetype
	// check elements in "codec/encoder/audio", "codec/muxer/audio"
	// build caps using this mimetype
	// gst_element_factory_can_src_caps()
}
*/

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_sink_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSinkMachine *self = BT_SINK_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_sink_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSinkMachine *self = BT_SINK_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sink_machine_dispose(GObject *object) {
  BtSinkMachine *self = BT_SINK_MACHINE(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_sink_machine_finalize(GObject *object) {
  BtSinkMachine *self = BT_SINK_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sink_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtSinkMachine *self = BT_SINK_MACHINE(instance);
  self->priv = g_new0(BtSinkMachinePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_sink_machine_class_init(BtSinkMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	BtMachineClass *base_class = BT_MACHINE_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_MACHINE);
  
  gobject_class->set_property = bt_sink_machine_set_property;
  gobject_class->get_property = bt_sink_machine_get_property;
  gobject_class->dispose      = bt_sink_machine_dispose;
  gobject_class->finalize     = bt_sink_machine_finalize;
}

GType bt_sink_machine_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSinkMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sink_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSinkMachine),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_sink_machine_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(BT_TYPE_MACHINE,"BtSinkMachine",&info,0);
  }
  return type;
}
