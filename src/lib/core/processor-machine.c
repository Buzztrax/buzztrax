/* $Id: processor-machine.c,v 1.6 2004-07-05 12:22:45 ensonic Exp $
 * class for a processor machine
 */

#define BT_CORE
#define BT_PROCESSOR_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/processor-machine.h>

struct _BtProcessorMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

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

