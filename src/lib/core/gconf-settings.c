/* $Id: gconf-settings.c,v 1.9 2004-11-30 20:15:49 waffel Exp $
 * gconf based implementation sub class for buzztard settings handling
 */

#define BT_CORE
#define BT_GCONF_SETTINGS_C

#include <libbtcore/core.h>
#include <libbtcore/settings-private.h>

struct _BtGConfSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* that is the handle we get config data from */
  GConfClient *client;
};

static BtSettingsClass *parent_class=NULL;

#define BT_GCONF_PATH_GSTREAMER "/system/gstreamer/default/"
#define BT_GCONF_PATH_GNOME "/desktop/gnome/interface/"
#define BT_GCONF_PATH_BUZZTARD "/apps/"PACKAGE"/"


//-- constructor methods

/**
 * bt_gconf_settings_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or NULL in case of an error
 */
BtGConfSettings *bt_gconf_settings_new(void) {
  BtGConfSettings *self;
  self=BT_GCONF_SETTINGS(g_object_new(BT_TYPE_GCONF_SETTINGS,NULL));
  
  //bt_settings_new(BT_SETTINGS(self));
  return(self);  
}

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
    case BT_SETTINGS_AUDIOSINK: {
      gchar *prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"audiosink",NULL);
      GST_DEBUG("application reads audiosink gconf_settings : %s",prop);
      g_value_set_string(value, prop);
      g_free(prop);
    } break;
    case BT_SETTINGS_SYSTEM_AUDIOSINK: {
      gchar *prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_GSTREAMER"audiosink",NULL);
      GST_DEBUG("application reads system audiosink gconf_settings : %s",prop);
      g_value_set_string(value, prop);
      g_free(prop);
    } break;
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
	gboolean gconf_ret=FALSE;
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK: {
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes audiosink gconf_settings : %s",prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"audiosink",prop,NULL);
			g_assert(gconf_ret == FALSE);
      g_free(prop);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_gconf_settings_dispose(GObject *object) {
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  // unregister directories to watch
  /* see bt_gconf_settings_init
  gconf_client_remove_dir(self->priv->client,BT_GCONF_PATH_GSTREAMER,NULL);
  gconf_client_remove_dir(self->priv->client,BT_GCONF_PATH_GNOME,NULL);
  gconf_client_remove_dir(self->priv->client,BT_GCONF_PATH_BUZZTARD,NULL);
  */
  g_object_unref(self->priv->client);

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_gconf_settings_finalize(GObject *object) {
  BtGConfSettings *self = BT_GCONF_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);
  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_gconf_settings_init(GTypeInstance *instance, gpointer g_class) {
  BtGConfSettings *self = BT_GCONF_SETTINGS(instance);
  self->priv = g_new0(BtGConfSettingsPrivate,1);
  self->priv->dispose_has_run = FALSE;
  self->priv->client=gconf_client_get_default();
  gconf_client_set_error_handling(self->priv->client,GCONF_CLIENT_HANDLE_UNRETURNED);
  // register the config cache
  /* this atm causes 
  * (process:25225): GConf-CRITICAL **: file gconf-client.c: line 546 (gconf_client_add_dir): assertion `gconf_valid_key (dirname, NULL)' failed
  *
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_GSTREAMER,GCONF_CLIENT_PRELOAD_ONELEVEL,NULL);
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_GNOME,GCONF_CLIENT_PRELOAD_ONELEVEL,NULL);
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_BUZZTARD,GCONF_CLIENT_PRELOAD_RECURSIVE,NULL);
  */
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
      (guint)(sizeof(BtGConfSettingsClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_gconf_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint)(sizeof(BtGConfSettings)),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_gconf_settings_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(BT_TYPE_SETTINGS,"BtGConfSettings",&info,0);
  }
  return type;
}
