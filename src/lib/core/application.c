/* $Id: application.c,v 1.23 2005-01-28 18:04:20 ensonic Exp $
 * base class for a buzztard based application
 */
 
#define BT_CORE
#define BT_APPLICATION_C

#include <libbtcore/core.h>

enum {
  APPLICATION_BIN=1,
  APPLICATION_SETTINGS
};

struct _BtApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the main gstreamer container element */
	GstElement *bin;
  /* a reference to the buzztard settings object */
  BtSettings *settings;
};

static GObjectClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_application_new:
 * @self: instance to finish construction of
 *
 * This is the common part of application construction. It needs to be called from
 * within the sub-classes constructor methods.
 *
 * Returns: %TRUE for succes, %FALSE otherwise
 */
gboolean bt_application_new(BtApplication *self) {
  gboolean res=FALSE;

  g_assert(BT_IS_APPLICATION(self));
  
#ifdef USE_GCONF
  self->priv->settings=BT_SETTINGS(bt_gconf_settings_new());
#else
  self->priv->settings=BT_SETTINGS(bt_plainfile_settings_new());
#endif
  { // DEBUG
    gchar *audiosink_name,*system_audiosink_name;
    g_object_get(self->priv->settings,"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);
    if(system_audiosink_name) {
      GST_INFO("default audiosink is \"%s\"",system_audiosink_name);
      g_free(system_audiosink_name);
    }
    if(audiosink_name) {
      GST_INFO("buzztard audiosink is \"%s\"",audiosink_name);
      g_free(audiosink_name);
    }
  } // DEBUG
  if(!self->priv->settings) goto Error;
  res=TRUE;
Error:
  return(res);
}

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
      g_value_set_object(value, self->priv->bin);
    } break;
    case APPLICATION_SETTINGS: {
      g_value_set_object(value, self->priv->settings);
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
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);
  GST_INFO("bin->ref_ct=%d",G_OBJECT(self->priv->bin)->ref_count);
  GST_INFO("bin->numchildren=%d",GST_BIN(self->priv->bin)->numchildren);
	g_object_try_unref(self->priv->bin);
  g_object_try_unref(self->priv->settings);

	GST_DEBUG("  chaining up");
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
	GST_DEBUG("  done");
}

static void bt_application_finalize(GObject *object) {
  BtApplication *self = BT_APPLICATION(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

	GST_DEBUG("  chaining up");
  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
	GST_DEBUG("  done");
}

static void bt_application_init(GTypeInstance *instance, gpointer g_class) {
  BtApplication *self = BT_APPLICATION(instance);
  self->priv = g_new0(BtApplicationPrivate,1);
  self->priv->dispose_has_run = FALSE;
	self->priv->bin = gst_thread_new("thread");
  g_assert(GST_IS_BIN(self->priv->bin));
}

static void bt_application_class_init(BtApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

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

  g_object_class_install_property(gobject_class,APPLICATION_SETTINGS,
																	g_param_spec_object("settings",
                                     "settings ro prop",
                                     "applications configuration settings",
                                     BT_TYPE_SETTINGS, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_application_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      (guint16)(sizeof(BtApplicationClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtApplication)),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_application_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtApplication",&info,0);
  }
  return type;
}
