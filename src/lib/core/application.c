// $Id: application.c,v 1.51 2006-02-28 19:03:29 ensonic Exp $
/**
 * SECTION:btapplication
 * @short_description: base class for a buzztard based application
 *
 * Everyone who writes an application using the #btcore library should create a
 * child of the <classname>BtApplication</classname> class.
 *
 * The base class automatically creates a #GstBin element as a thread.
 * This can be retrieved via the bin property of an application instance.
 * When creating #BtSong instances, the #BtApplication instance needs to be passed
 * to the bt_song_new() constructor, so that it can retrieve the #GstBin element.
 * <informalexample>
 *  <programlisting language="c">song=bt_song_new(app)</programlisting>
 * </informalexample>
 *
 * Another module the application base class maintains is a settings instance (see
 * #BtSettings), that manages application preferences. 
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
  
  /* application bus-handlers */
  GList *bus_handlers;
};

static GObjectClass *parent_class=NULL;

typedef struct {
  GstBusFunc handler;
  gpointer user_data;
} BtBusWatchEntry;

//-- helper

//-- handler

/*
 * bus_handler:
 *
 * distribute pipeline-bus message
 * walks the callback list and invoke the callbacks until one returns TRUE
 */
static gboolean bus_handler(GstBus *bus, GstMessage *message, gpointer user_data) {
  BtApplication *self = BT_APPLICATION(user_data);
  BtBusWatchEntry *entry;
  gboolean handled=FALSE;
  GList* node;
  
  g_return_val_if_fail(GST_IS_BUS(bus),TRUE);
  
  //GST_INFO("received bus messgage: '%s'",gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
  
  for(node=self->priv->bus_handlers;(node && !handled);node=g_list_next(node)) {
    entry=(BtBusWatchEntry *)node->data;
    handled=entry->handler(bus,message,entry->user_data);
  }
  if(!handled) {
    GError *err;
    gchar *debug;
    gchar *msg_type=NULL;
    switch(GST_MESSAGE_TYPE(message)) {
      case GST_MESSAGE_WARNING:
        msg_type=_("Warning");
        gst_message_parse_warning (message, &err, &debug);
        break;
      case GST_MESSAGE_ERROR:
        msg_type=_("Error");
        gst_message_parse_error (message, &err, &debug);
        break;
      default:
        //GST_DEBUG("  unhandled bus message : %s",gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
        break;
    }
    if(msg_type) {
      //gst_object_default_error (GST_MESSAGE_SRC (message), err, debug);
      g_print("%s: %s\n%s\n",msg_type,safe_string(err->message),debug);
      g_error_free (err);
      g_free (debug);
    }
  }
  // pop off *all* messages
  return(TRUE);
}
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

  g_return_val_if_fail(BT_IS_APPLICATION(self),FALSE);

  /*
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
  */
  res=TRUE;
//Error:
  return(res);
}

//-- methods

/**
 * bt_application_add_bus_watch:
 * @self: the application instance
 * @handler : function to call when a message is received.
 * @user_data : user data passed to handler
 *
 * The #BtApplication class manages communication with the #GstBus of the loaded
 * #BtSong. To process bus messages register a handler with this method.
 */
void bt_application_add_bus_watch(const BtApplication *self,GstBusFunc handler,gpointer user_data) {
  BtBusWatchEntry *entry;
  GList* node;
  
  g_return_if_fail(BT_IS_APPLICATION(self));
  g_return_if_fail(handler);
  
  for(node=self->priv->bus_handlers;node;node=g_list_next(node)) {
    entry=(BtBusWatchEntry *)node->data;
    if((entry->handler==handler) && (entry->user_data==user_data)) {
      GST_WARNING("trying to register bus_watch again");
      return;
    }
  }
  entry=g_new(BtBusWatchEntry,1);
  entry->handler=handler;
  entry->user_data=user_data;
  self->priv->bus_handlers=g_list_prepend(self->priv->bus_handlers,entry);
}

/**
 * bt_application_remove_bus_watch:
 * @self: the application instance
 * @handler : function to remove from the handler list.
 *
 * Unregister a handler previously registered using bt_application_add_bus_watch().
 */
void bt_application_remove_bus_watch(const BtApplication *self,GstBusFunc handler) {
  BtBusWatchEntry *entry;
  GList* node;
  
  g_return_if_fail(BT_IS_APPLICATION(self));
  g_return_if_fail(handler);
  
  for(node=self->priv->bus_handlers;node;node=g_list_next(node)) {
    entry=(BtBusWatchEntry *)node->data;
    if(entry->handler==handler) {
      self->priv->bus_handlers=g_list_remove(self->priv->bus_handlers,entry);
      break;
    }
  }
}

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
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->bin);
      #endif
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
  GST_INFO("settings->ref_ct=%d",G_OBJECT(self->priv->settings)->ref_count);

  gst_object_unref(self->priv->bin);
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

  // free list of bus-handlers
  if(self->priv->bus_handlers) {
    GList* node;
    for(node=self->priv->bus_handlers;node;node=g_list_next(node)) {
      g_free(node->data);
      node->data=NULL;
    }
    g_list_free(self->priv->bus_handlers);
    self->priv->bus_handlers=NULL;
  }

  GST_DEBUG("  chaining up");
  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
  GST_DEBUG("  done");
}

static void bt_application_init(GTypeInstance *instance, gpointer g_class) {
  BtApplication *self = BT_APPLICATION(instance);
  GstBus *bus;
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_APPLICATION, BtApplicationPrivate);
  self->priv->bin = gst_pipeline_new("song");
  g_assert(GST_IS_ELEMENT(self->priv->bin));
  GST_INFO("bin->ref_ct=%d",G_OBJECT(self->priv->bin)->ref_count);
  
  bus=gst_element_get_bus(self->priv->bin);
  g_assert(GST_IS_BUS(bus));
  gst_bus_add_watch_full(bus,G_PRIORITY_DEFAULT_IDLE,bus_handler,(gpointer)self,NULL);
  gst_object_unref(bus);
  
  // if we enable this we get lots of diagnostics
  //g_signal_connect (self->priv->bin, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);
  
  self->priv->settings=bt_settings_new();
  g_assert(BT_IS_SETTINGS(self->priv->settings));
}

static void bt_application_class_init(BtApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  g_type_class_add_private(klass,sizeof(BtApplicationPrivate));

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
  if (G_UNLIKELY(type == 0)) {
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
