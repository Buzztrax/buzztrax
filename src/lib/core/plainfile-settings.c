// $Id: plainfile-settings.c,v 1.12 2005-07-26 06:43:45 waffel Exp $
/**
 * SECTION:btplainfilesettings
 * @short_description: plain file based implementation sub class for buzztard 
 * settings handling
 */ 

/*
idea is to use a XML file liek below
<settings>
  <setting name="" value=""/>
  ...
</settings>
at init the whole document is loaded and
get/set methods use xpath expr to get the nodes.
*/

#define BT_CORE
#define BT_PLAINFILE_SETTINGS_C

#include <libbtcore/core.h>
#include <libbtcore/settings-private.h>

struct _BtPlainfileSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* key=value list, keys are defined in BtSettings */
  //GHashTable *settings;
  //xmlDoc *settings;
};

static BtSettingsClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_plainfile_settings_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPlainfileSettings *bt_plainfile_settings_new(void) {
  BtPlainfileSettings *self;
  self=BT_PLAINFILE_SETTINGS(g_object_new(BT_TYPE_PLAINFILE_SETTINGS,NULL));
  
  //bt_settings_new(BT_SETTINGS(self));
  return(self);  
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_plainfile_settings_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPlainfileSettings *self = BT_PLAINFILE_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK:
		case BT_SETTINGS_SYSTEM_AUDIOSINK: {
      g_value_set_string(value, "esdsink");
    } break;
		case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE: {
      g_value_set_string(value, "both");
			} break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_plainfile_settings_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPlainfileSettings *self = BT_PLAINFILE_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK: {
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes audiosink plainfile_settings : %s",prop);
      // @todo set property value
      g_free(prop);
    } break;
     default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_plainfile_settings_dispose(GObject *object) {
  BtPlainfileSettings *self = BT_PLAINFILE_SETTINGS(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_plainfile_settings_finalize(GObject *object) {
  BtPlainfileSettings *self = BT_PLAINFILE_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  //g_hash_table_destroy(self->priv->settings);
  g_free(self->priv);
  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_plainfile_settings_init(GTypeInstance *instance, gpointer g_class) {
  BtPlainfileSettings *self = BT_PLAINFILE_SETTINGS(instance);
  self->priv = g_new0(BtPlainfileSettingsPrivate,1);
  self->priv->dispose_has_run = FALSE;
  //self->priv->settings=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
}

static void bt_plainfile_settings_class_init(BtPlainfileSettingsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_SETTINGS);

  gobject_class->set_property = bt_plainfile_settings_set_property;
  gobject_class->get_property = bt_plainfile_settings_get_property;
  gobject_class->dispose      = bt_plainfile_settings_dispose;
  gobject_class->finalize     = bt_plainfile_settings_finalize;
}

GType bt_plainfile_settings_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtPlainfileSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_plainfile_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtPlainfileSettings),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_plainfile_settings_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(BT_TYPE_SETTINGS,"BtPlainfileSettings",&info,0);
  }
  return type;
}
