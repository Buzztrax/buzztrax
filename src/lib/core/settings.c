/* $Id: settings.c,v 1.14 2005-04-21 16:13:28 ensonic Exp $
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

static GObjectClass *parent_class=NULL;

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

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_settings_finalize(GObject *object) {
  BtSettings *self = BT_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_settings_init(GTypeInstance *instance, gpointer g_class) {
  BtSettings *self = BT_SETTINGS(instance);
  self->priv = g_new0(BtSettingsPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_settings_class_init(BtSettingsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_settings_set_property;
  gobject_class->get_property = bt_settings_get_property;
  gobject_class->dispose      = bt_settings_dispose;
  gobject_class->finalize     = bt_settings_finalize;

	//klass->get           = bt_settings_real_get;
	//klass->set           = bt_settings_real_set;

  g_object_class_install_property(gobject_class,BT_SETTINGS_AUDIOSINK,
                                  g_param_spec_string("audiosink",
                                     "audiosink prop",
                                     "audio output gstreamer element",
                                     "esdsink", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MENU_TOOLBAR_HIDE,
                                  g_param_spec_boolean("toolbar-hide",
                                     "toolbar-hide",
                                     "hide main toolbar",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
                                  g_param_spec_string("grid-density",
                                     "grid-density prop",
                                     "machine view grid detail level",
                                     "low", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,BT_SETTINGS_SYSTEM_AUDIOSINK,
                                  g_param_spec_string("system-audiosink",
                                     "system-audiosink prop",
                                     "system audio output gstreamer element",
                                     "esdsink", /* default value */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,BT_SETTINGS_SYSTEM_TOOLBAR_STYLE,
                                  g_param_spec_string("toolbar-style",
                                     "toolbar-style prop",
                                     "system tolbar style",
                                     "both", /* default value */
                                     G_PARAM_READABLE));
}

GType bt_settings_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSettings),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_settings_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSettings",&info,G_TYPE_FLAG_ABSTRACT);
  }
  return type;
}
