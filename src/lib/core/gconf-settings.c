/* $Id: gconf-settings.c,v 1.1 2004-09-26 01:50:08 ensonic Exp $
 * gconf based implementation sub class for buzztard settings handling
 */

#define BT_CORE
#define BT_GCONF_SETTINGS_C

#include <libbtcore/core.h>

enum {
  GCONF_SETTINGS_XXX=1
};

struct _BtGConfSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  GConfClient *client;
};

static BtSettingsClass *parent_class=NULL;

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_gconf_settings_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_gconf_settings_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_gconf_settings_dispose(GObject *object) {
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;
  
  g_object_unref(self->private->client);

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_gconf_settings_finalize(GObject *object) {
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->private);
  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_gconf_settings_init(GTypeInstance *instance, gpointer g_class) {
  BtGConfSettings *self = BT_GCONF_SETTINGS(instance);
  self->private = g_new0(BtGConfSettingsPrivate,1);
  self->private->dispose_has_run = FALSE;
  self->private->client=gconf_client_get_default();
}

static void bt_gconf_settings_class_init(BtGConfSettingsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_SETTINGS);

  gobject_class->set_property = bt_gconf_settings_set_property;
  gobject_class->get_property = bt_gconf_settings_get_property;
  gobject_class->dispose      = bt_gconf_settings_dispose;
  gobject_class->finalize     = bt_gconf_settings_finalize;
}

GType bt_gconf_settings_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtGConfSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_gconf_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtGConfSettings),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_gconf_settings_init, // instance_init
    };
		type = g_type_register_static(BT_TYPE_SETTINGS,"BtGConfSettings",&info,0);
  }
  return type;
}

