// $Id: processor-machine.c,v 1.30 2005-09-21 19:46:03 ensonic Exp $
/**
 * SECTION:btprocessormachine
 * @short_description: class for signal processing machines with inputs and 
 * outputs
 *
 */ 

#define BT_CORE
#define BT_PROCESSOR_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/machine.h>
#include <libbtcore/machine-private.h>

struct _BtProcessorMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_processor_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 *
 * Create a new instance.
 * The machine is automaticly added to the setup of the given song. You don't 
 * need to call <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtProcessorMachine *bt_processor_machine_new(const BtSong *song, const gchar *id, const gchar *plugin_name, glong voices) {
  BtProcessorMachine *self;
  
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);
  g_return_val_if_fail(BT_IS_STRING(plugin_name),NULL);
  
  if(!(self=BT_PROCESSOR_MACHINE(g_object_new(BT_TYPE_PROCESSOR_MACHINE,"song",song,"id",id,"plugin-name",plugin_name,"voices",voices,NULL)))) {
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

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_processor_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_processor_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_processor_machine_dispose(GObject *object) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_processor_machine_finalize(GObject *object) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_processor_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(instance);
  self->priv = g_new0(BtProcessorMachinePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_processor_machine_class_init(BtProcessorMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_MACHINE);
  
  gobject_class->set_property = bt_processor_machine_set_property;
  gobject_class->get_property = bt_processor_machine_get_property;
  gobject_class->dispose      = bt_processor_machine_dispose;
  gobject_class->finalize     = bt_processor_machine_finalize;
}

GType bt_processor_machine_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtProcessorMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_processor_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtProcessorMachine),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_processor_machine_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_MACHINE,"BtProcessorMachine",&info,0);
  }
  return type;
}
