/* $Id: settings.c,v 1.8 2004-10-21 15:23:04 ensonic Exp $
 * base class for buzztard settings handling
 */

#define BT_CORE
#define BT_SETTINGS_C

#include <libbtcore/core.h>
#include <libbtcore/settings-private.h>

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
  GObjectClass *gobject_class = G_OBJECT_GET_CLASS(object);
  return_if_disposed();

  // call implementation
  gobject_class->get_property(object,property_id,value,pspec);
}

/* sets the given properties for this object */
static void bt_settings_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSettings *self = BT_SETTINGS(object);
  GObjectClass *gobject_class = G_OBJECT_GET_CLASS(object);
  return_if_disposed();

  // call implementation
  gobject_class->set_property(object,property_id,value,pspec);
}

static void bt_settings_dispose(GObject *object) {
  BtSettings *self = BT_SETTINGS(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
}

static void bt_settings_finalize(GObject *object) {
  BtSettings *self = BT_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);
}

static void bt_settings_init(GTypeInstance *instance, gpointer g_class) {
  BtSettings *self = BT_SETTINGS(instance);
  self->priv = g_new0(BtSettingsPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_settings_class_init(BtSettingsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = bt_settings_set_property;
  gobject_class->get_property = bt_settings_get_property;
  gobject_class->dispose      = bt_settings_dispose;
  gobject_class->finalize     = bt_settings_finalize;

	//klass->get           = bt_settings_real_get;
	//klass->set           = bt_settings_real_set;

  g_object_class_install_property(gobject_class,BT_SETTINGS_AUDIOSINK,
                                  g_param_spec_string("audiosink",
                                     "audiosink prop",
                                     "audio output device",
                                     "esdsink", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,BT_SETTINGS_SYSTEM_AUDIOSINK,
                                  g_param_spec_string("system-audiosink",
                                     "system-audiosink prop",
                                     "system audio output device",
                                     "esdsink", /* default value */
                                     G_PARAM_READABLE));

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

