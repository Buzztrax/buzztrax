/* $Id: application.c,v 1.7 2004-09-21 14:01:19 ensonic Exp $
 * base class for a buzztard based application
 */
 
#define BT_CORE
#define BT_APPLICATION_C

#include <libbtcore/core.h>

enum {
  APPLICATION_BIN=1
};

struct _BtApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the main gstreamer container element */
	GstElement *bin;
  /* a reference to the buzztard settings object */
  BtSettings *settings;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_application_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtApplication *self = BT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    case APPLICATION_BIN: {
      g_value_set_object(value, self->private->bin);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_application_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtApplication *self = BT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_application_dispose(GObject *object) {
  BtApplication *self = BT_APPLICATION(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
	g_object_try_unref(self->private->bin);
}

static void bt_application_finalize(GObject *object) {
  BtApplication *self = BT_APPLICATION(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->private);
}

static void bt_application_init(GTypeInstance *instance, gpointer g_class) {
  BtApplication *self = BT_APPLICATION(instance);
  self->private = g_new0(BtApplicationPrivate,1);
  self->private->dispose_has_run = FALSE;
	self->private->bin = gst_thread_new("thread");
  g_assert(self->private->bin!=NULL);
}

static void bt_application_class_init(BtApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = bt_application_set_property;
  gobject_class->get_property = bt_application_get_property;
  gobject_class->dispose      = bt_application_dispose;
  gobject_class->finalize     = bt_application_finalize;

  g_object_class_install_property(gobject_class,APPLICATION_BIN,
																	g_param_spec_object("bin",
                                     "bin ro prop",
                                     "applications top-level GstElement container",
                                     GST_TYPE_BIN, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_application_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtApplicationClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtApplication),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_application_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtApplication",&info,0);
  }
  return type;
}

