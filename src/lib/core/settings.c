/* $Id: settings.c,v 1.3 2004-09-26 01:50:08 ensonic Exp $
 * base class for buzztard settings handling
 */

#define BT_CORE
#define BT_SETTINGS_C

#include <libbtcore/core.h>

enum {
  SETTINGS_XXX=1
};

struct _BtSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_settings_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSettings *self = BT_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_settings_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSettings *self = BT_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_settings_dispose(GObject *object) {
  BtSettings *self = BT_SETTINGS(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
}

static void bt_settings_finalize(GObject *object) {
  BtSettings *self = BT_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->private);
}

static void bt_settings_init(GTypeInstance *instance, gpointer g_class) {
  BtSettings *self = BT_SETTINGS(instance);
  self->private = g_new0(BtSettingsPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_settings_class_init(BtSettingsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = bt_settings_set_property;
  gobject_class->get_property = bt_settings_get_property;
  gobject_class->dispose      = bt_settings_dispose;
  gobject_class->finalize     = bt_settings_finalize;
}

GType bt_settings_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSettings),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_settings_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSettings",&info,0);
  }
  return type;
}

