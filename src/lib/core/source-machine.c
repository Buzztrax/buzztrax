// $Id: source-machine.c,v 1.38 2006-04-08 22:08:34 ensonic Exp $
/**
 * SECTION:btsourcemachine
 * @short_description: class for signal processing machines with outputs only
 *
 * Sources are machines that generate audio.
 */
 
#define BT_CORE
#define BT_SOURCE_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/source-machine.h>
#include <libbtcore/machine-private.h>

struct _BtSourceMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_source_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 *
 * Create a new instance
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSourceMachine *bt_source_machine_new(const BtSong *song, const gchar *id, const gchar *plugin_name, glong voices) {
  BtSourceMachine *self;
  
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);
  g_return_val_if_fail(BT_IS_STRING(plugin_name),NULL);
  
  if(!(self=BT_SOURCE_MACHINE(g_object_new(BT_TYPE_SOURCE_MACHINE,"song",song,"id",id,"plugin-name",plugin_name,"voices",voices,NULL)))) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- io interface

static xmlNodePtr bt_source_machine_persistence_save(BtPersistence *persistence, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
  BtSourceMachine *self = BT_SOURCE_MACHINE(persistence);
  BtPersistenceInterface *parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlNodePtr node=NULL;
  gchar *plugin_name;
  gulong voices;

  GST_DEBUG("PERSISTENCE::source-machine");

  // save parent class stuff
  if((node=parent_iface->save(persistence,parent_node,selection))) {
    xmlNewProp(node,XML_CHAR_PTR("type"),XML_CHAR_PTR("source"));

    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,"voices",&voices,NULL);
    xmlNewProp(node,XML_CHAR_PTR("plugin-name"),XML_CHAR_PTR(plugin_name));
    xmlNewProp(node,XML_CHAR_PTR("voices"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(voices)));
    g_free(plugin_name);
  }
  return(node);
}

static gboolean bt_source_machine_persistence_load(BtPersistence *persistence, xmlNodePtr node, BtPersistenceLocation *location) {
  BtSourceMachine *self = BT_SOURCE_MACHINE(persistence);
  BtPersistenceInterface *parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlChar *plugin_name,*voices_str;
  gulong voices;

  plugin_name=xmlGetProp(node,XML_CHAR_PTR("plugin-name"));
  voices_str=xmlGetProp(node,XML_CHAR_PTR("voices"));
  voices=voices_str?atol((char *)voices_str):0;
  g_object_set(G_OBJECT(self),"plugin-name",plugin_name,"voices",voices,NULL);
  xmlFree(plugin_name);xmlFree(voices_str);
  
  // load parent class stuff
  return(parent_iface->load(persistence,node,location));
}

static void bt_source_machine_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface *iface = g_iface;
  
  iface->load = bt_source_machine_persistence_load;
  iface->save = bt_source_machine_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_source_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSourceMachine *self = BT_SOURCE_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_source_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSourceMachine *self = BT_SOURCE_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_source_machine_dispose(GObject *object) {
  BtSourceMachine *self = BT_SOURCE_MACHINE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_source_machine_finalize(GObject *object) {
  BtSourceMachine *self = BT_SOURCE_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_source_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtSourceMachine *self = BT_SOURCE_MACHINE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SOURCE_MACHINE, BtSourceMachinePrivate);
}

static void bt_source_machine_class_init(BtSourceMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSourceMachinePrivate));
  
  gobject_class->set_property = bt_source_machine_set_property;
  gobject_class->get_property = bt_source_machine_get_property;
  gobject_class->dispose      = bt_source_machine_dispose;
  gobject_class->finalize     = bt_source_machine_finalize;
}

GType bt_source_machine_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSourceMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_source_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSourceMachine),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_source_machine_init, // instance_init
      NULL // value_table
    };
    static const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_source_machine_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(BT_TYPE_MACHINE,"BtSourceMachine",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
