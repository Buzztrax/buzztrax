/* $Id: sink-machine.c,v 1.4 2004-07-02 13:44:50 ensonic Exp $
 * class for a sink machine
 */
 
#define BT_CORE
#define BT_SINK_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/sink-machine.h>

struct _BtSinkMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

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
    default: { // call super method
			BtSinkMachineClass *klass=BT_SINK_MACHINE_GET_CLASS(object);
			BtMachineClass *base_klass=BT_MACHINE_CLASS(klass);
			GObjectClass *base_gobject_class = G_OBJECT_CLASS(base_klass);
			
			base_gobject_class->get_property(object,property_id,value,pspec);
      break;
    }
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
    default: { // call super method
			BtSinkMachineClass *klass=BT_SINK_MACHINE_GET_CLASS(object);
			BtMachineClass *base_klass=BT_MACHINE_CLASS(klass);
			GObjectClass *base_gobject_class = G_OBJECT_CLASS(base_klass);
			
			base_gobject_class->set_property(object,property_id,value,pspec);
      break;
    }
  }
}

static void bt_sink_machine_dispose(GObject *object) {
  BtSinkMachine *self = BT_SINK_MACHINE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_sink_machine_finalize(GObject *object) {
  BtSinkMachine *self = BT_SINK_MACHINE(object);
  g_free(self->private);
}

static void bt_sink_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtSinkMachine *self = BT_SINK_MACHINE(instance);
  self->private = g_new0(BtSinkMachinePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_sink_machine_class_init(BtSinkMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	BtMachineClass *base_class = BT_MACHINE_CLASS(klass);
  
  gobject_class->set_property = bt_sink_machine_set_property;
  gobject_class->get_property = bt_sink_machine_get_property;
  gobject_class->dispose      = bt_sink_machine_dispose;
  gobject_class->finalize     = bt_sink_machine_finalize;
}

GType bt_sink_machine_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSinkMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sink_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSinkMachine),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_sink_machine_init, // instance_init
    };
		type = g_type_register_static(BT_TYPE_MACHINE,"BtSinkMachine",&info,0);
  }
  return type;
}

