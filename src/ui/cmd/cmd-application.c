/* $Id: cmd-application.c,v 1.1 2004-05-12 09:35:14 ensonic Exp $
 * class for a commandline buzztard based application
 */
 
//#define BT_CORE -> BT_CMD ?
#define BT_CMD_APPLICATION_C

#include "bt-cmd.h"

struct _BtCmdApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_cmd_application_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
 			BtCmdApplicationClass *klass=BT_CMD_APPLICATION_GET_CLASS(object);
			BtApplicationClass *base_klass=BT_APPLICATION_CLASS(klass);
			GObjectClass *base_gobject_class = G_OBJECT_CLASS(base_klass);
			
			base_gobject_class->get_property(object,property_id,value,pspec);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_cmd_application_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
			BtCmdApplicationClass *klass=BT_CMD_APPLICATION_GET_CLASS(object);
			BtApplicationClass *base_klass=BT_APPLICATION_CLASS(klass);
			GObjectClass *base_gobject_class = G_OBJECT_CLASS(base_klass);
			
			base_gobject_class->set_property(object,property_id,value,pspec);
      break;
    }
  }
}

static void bt_cmd_application_dispose(GObject *object) {
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_cmd_application_finalize(GObject *object) {
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
  g_free(self->private);
}

static void bt_cmd_application_init(GTypeInstance *instance, gpointer g_class) {
  BtCmdApplication *self = BT_CMD_APPLICATION(instance);
  self->private = g_new0(BtCmdApplicationPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_cmd_application_class_init(BtCmdApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_cmd_application_set_property;
  gobject_class->get_property = bt_cmd_application_get_property;
  gobject_class->dispose      = bt_cmd_application_dispose;
  gobject_class->finalize     = bt_cmd_application_finalize;
}

GType bt_cmd_application_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtCmdApplicationClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_cmd_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtCmdApplication),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_cmd_application_init, // instance_init
    };
		type = g_type_register_static(BT_APPLICATION_TYPE,"BtCmdApplication",&info,0);
  }
  return type;
}

