// $Id: sink-machine.c,v 1.55 2005-11-27 22:44:46 ensonic Exp $
/**
 * SECTION:btsinkmachine
 * @short_description: class for signal processing machines with inputs only
 *
 * Sinks are machines that do playback or recording of the song.
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

//-- helper methods

static gchar *bt_sink_machine_determine_plugin_name(const BtSettings *settings) {
  gchar *audiosink_name,*system_audiosink_name;
  gchar *plugin_name=NULL;
  
  g_object_get(G_OBJECT(settings),"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);
  if(BT_IS_STRING(audiosink_name)) {
    GST_INFO("get audiosink from config");
    plugin_name=audiosink_name;
    audiosink_name=NULL;
  }
  else if(BT_IS_STRING(system_audiosink_name)) {
    GST_INFO("get audiosink from system config");
    plugin_name=system_audiosink_name;
    system_audiosink_name=NULL;
  }
  if(plugin_name) {
    gchar *sink_name,*eon;
    // this can be a whole pipeline like "audioconvert ! osssink sync=false"
    // seek for the last '!'
    if(!(sink_name=g_strrstr(plugin_name,"!"))) {
      sink_name=plugin_name;
    }
    else {
      // skip '!' and spaces
      sink_name++;
      while(*sink_name==' ') sink_name++;
    }
    // if there is a space following put '\0' in there
    if((eon=strstr(sink_name," "))) {
      *eon='\0';
    }
    plugin_name=sink_name;
  }
  if (!BT_IS_STRING(plugin_name)) {
    GST_INFO("get audiosink from gst registry by rank");
    // iterate over gstreamer-audiosink list and choose element with highest rank
    GList *node,*audiosink_names=bt_gst_registry_get_element_names_by_class("Sink/Audio");
    GstElementFactory *factory;
    guint max_rank=0,cur_rank;
    
    for(node=audiosink_names;node;node=g_list_next(node)) {
      factory=gst_element_factory_find(node->data);
      cur_rank=gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
      //GST_INFO("  trying audio sink: \"%s\" with rank: %d",node->data,cur_rank);
      if((cur_rank>max_rank) || (!plugin_name)) {
        plugin_name=g_strdup(node->data);
        max_rank=cur_rank;
      }
    }
    g_list_free(audiosink_names);
  }
  GST_INFO("using audio sink : \"%s\"",plugin_name);

  g_free(system_audiosink_name);
  g_free(audiosink_name);
  return(plugin_name);
}

//-- event handler

static void on_audio_sink_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  //BtSinkMachine *self=BT_SINK_MACHINE(user_data);
  gchar *plugin_name;

  g_assert(user_data);
  GST_INFO("audio-sink has changed");
  plugin_name=bt_sink_machine_determine_plugin_name(settings);
  GST_INFO("  -> '%s'",plugin_name);
  
  /* @todo exchange the machine
  //// version 1
    g_object_set(self,"plugin-name",plugin_name,NULL);
    plugin-name is construct only :(
    if sink has input-gain or input-level, we do not need to relink all wires.
  
  //// version 2
    g_object_get(self,"id",&id,NULL)
    wires=bt_setup_get_wires_by_machine_type(setup,self,"dst");
    sink=bt_sink_machine_new(song,id);
    for(node=wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      g_object-set(wire,"dst",sink,NULL);
      bt_wire_reconnect(wire);
    }
  */
  g_free(plugin_name);
}

static void on_system_audio_sink_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  //BtSinkMachine *self=BT_SINK_MACHINE(user_data);
  gchar *plugin_name;

  g_assert(user_data);
  GST_INFO("audio-sink has changed");
  plugin_name=bt_sink_machine_determine_plugin_name(settings);
  GST_INFO("  -> '%s'",plugin_name);
  
  // @todo exchange the machine (only if the system-audiosink is in use)
  // g_object_set(self,"plugin-name",plugin_name,NULL);
  // plugin-name is construct only :(
  g_free(plugin_name);
}

//-- constructor methods

/**
 * bt_sink_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 *
 * Create a new instance.
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 * The plugin used for this machine is taken from the #BtSettings.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkMachine *bt_sink_machine_new(const BtSong *song, const gchar *id) {
  BtSinkMachine *self=NULL;
  BtApplication *app;
  BtSettings *settings;
  gchar *plugin_name;

  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);

  // get plugin_name from song->app->settings
  g_object_get(G_OBJECT(song),"app",&app,NULL);
  g_object_get(app,"settings",&settings,NULL);

  plugin_name=bt_sink_machine_determine_plugin_name(settings);
  if(!BT_IS_STRING(plugin_name)) {
    GST_ERROR("no audiosink configured/registered");
    goto Error;
  }
  // @todo: try using sink-bin
  if(!(self=BT_SINK_MACHINE(g_object_new(BT_TYPE_SINK_MACHINE,"song",song,"id",id,"plugin-name","bt-sink-bin",NULL)))) {
  //if(!(self=BT_SINK_MACHINE(g_object_new(BT_TYPE_SINK_MACHINE,"song",song,"id",id,"plugin-name",plugin_name,NULL)))) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }
  //g_object_get(G_OBJECT (self), "machine", &elem, NULL);
  //g_object_set(G_OBJECT (elem), "sync", FALSE, NULL);
  //gst_object_unref(GST_OBJECT(elem));

  // @todo where do we put that, should the sink watch itself, or should the song do it?
  // add notifies to audio-sink and system-audio-sink
  g_signal_connect(G_OBJECT(settings), "notify::audio-sink", G_CALLBACK(on_audio_sink_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::system-audio-sink", G_CALLBACK(on_system_audio_sink_changed), (gpointer)self);
  
  g_object_try_unref(settings);
  g_object_try_unref(app);
  return(self);
Error:
  g_object_try_unref(self);
  g_object_try_unref(settings);
  g_object_try_unref(app);
  return(NULL);
}

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
  //BtMachineClass *base_class = BT_MACHINE_CLASS(klass);

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
