/* $Id: processor-machine.c,v 1.7 2004-07-30 15:15:51 ensonic Exp $
 * class for a processor machine
 */

#define BT_CORE
#define BT_PROCESSOR_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/machine.h>

struct _BtProcessorMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

//-- constructor methods

// @todo ideally this would be a protected method, but how to do this in 'C' ?
extern gboolean bt_machine_init_gst_element(BtMachine *self);

/**
 * bt_processor_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtProcessorMachine *bt_processor_machine_new(const BtSong *song, const gchar *id, const gchar *plugin_name, glong voices) {
  BtProcessorMachine *self;
  self=BT_PROCESSOR_MACHINE(g_object_new(BT_TYPE_PROCESSOR_MACHINE,"song",song,"id",id,"plugin_name",plugin_name,"voices",voices,NULL));
  
  bt_machine_init_gst_element(self);
  return(self);
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
      g_assert(FALSE);
      break;
    }
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
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_processor_machine_dispose(GObject *object) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_processor_machine_finalize(GObject *object) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);
  g_free(self->private);
}

static void bt_processor_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(instance);
  self->private = g_new0(BtProcessorMachinePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_processor_machine_class_init(BtProcessorMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	BtMachineClass *base_class = BT_MACHINE_CLASS(klass);
  
  gobject_class->set_property = bt_processor_machine_set_property;
  gobject_class->get_property = bt_processor_machine_get_property;
  gobject_class->dispose      = bt_processor_machine_dispose;
  gobject_class->finalize     = bt_processor_machine_finalize;
}

GType bt_processor_machine_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtProcessorMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_processor_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtProcessorMachine),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_processor_machine_init, // instance_init
    };
		type = g_type_register_static(BT_TYPE_MACHINE,"BtProcessorMachine",&info,0);
  }
  return type;
}

