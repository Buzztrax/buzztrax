/* $Id: settings.c,v 1.38 2007-07-19 13:23:06 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btsettings
 * @short_description: base class for buzztard settings handling
 *
 * Under the gnome platform GConf is a locical choice for settings managment.
 * Unfortunately there currently is no port of GConf for other platforms.
 * This class wraps the settings management. Depending on what settings managment
 * capabillities the <code>configure</code> script find on the system one of the
 * subclasses (#BtGConfSettings,#BtPlainfileSettings) will be used.
 *
 * In any case it is always sufficient to talk to this class instance.
 * Single settings are accessed via normat g_object_get() and g_object_set() calls.
 */
/* @todo: how can we decouple application specific settings?
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
static BtSettingsFactory bt_settings_factory=NULL;
static gpointer singleton=NULL;

//-- constructor methods

/**
 * bt_settings_new:
 *
 * Create a new instance. The type of the settings depends on the subsystem
 * found during configuration run.
 *
 * Settings are implemented as a singleton. Thus the first invocation will
 * create the object and further calls will just give back a reference.
 *
 * Returns: the instance or %NULL in case of an error
 */
BtSettings *bt_settings_new(void) {

  if(G_UNLIKELY(!singleton)) {
    GST_INFO("create a new settings object for thread %p",g_thread_self());
    if(G_LIKELY(!bt_settings_factory)) {
#ifdef USE_GCONF
      singleton=(gpointer)bt_gconf_settings_new();
#else
      singleton=(gpointer)bt_plainfile_settings_new();
#endif
	  GST_INFO("settings created %p",singleton);
    }
    else {
      singleton=bt_settings_factory();
      GST_INFO("created new settings object from factory %p",singleton);
    }
    g_object_add_weak_pointer(G_OBJECT(singleton),&singleton);
  }
  else {
    GST_INFO("return cached settings object (refct=%d) for thread %p",G_OBJECT(singleton)->ref_count,g_thread_self());
    singleton=g_object_ref(G_OBJECT(singleton));
  }
  return(BT_SETTINGS(singleton));
}

//-- methods

/**
 * bt_settings_set_factory:
 * @factory: factory method
 *
 * Set a factory method that creates a new settings instance. This is currently
 * only used by the unit tests to exercise the applications under various
 * conditions. Normal applications should NOT use it.
 */
void bt_settings_set_factory(BtSettingsFactory factory) {
  if(!singleton) {
    bt_settings_factory=factory;
  }
  else {
    GST_INFO("can't change factory while having %d living instances",G_OBJECT(singleton)->ref_count);
  }
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_settings_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtSettings * const self = BT_SETTINGS(object);
  const GObjectClass * const gobject_class = G_OBJECT_GET_CLASS(object);
  return_if_disposed();

  // call implementation
  gobject_class->get_property(object,property_id,value,pspec);
}

/* sets the given properties for this object */
static void bt_settings_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSettings * const self = BT_SETTINGS(object);
  GObjectClass * const gobject_class = G_OBJECT_GET_CLASS(object);
  return_if_disposed();

  // call implementation
  gobject_class->set_property(object,property_id,value,pspec);
}

static void bt_settings_dispose(GObject * const object) {
  const BtSettings * const self = BT_SETTINGS(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_settings_finalize(GObject * const object) {
  const BtSettings * const self = BT_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_settings_init(GTypeInstance * const instance, gpointer g_class) {
  BtSettings * const self = BT_SETTINGS(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SETTINGS, BtSettingsPrivate);
}

static void bt_settings_class_init(BtSettingsClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSettingsPrivate));

  gobject_class->set_property = bt_settings_set_property;
  gobject_class->get_property = bt_settings_get_property;
  gobject_class->dispose      = bt_settings_dispose;
  gobject_class->finalize     = bt_settings_finalize;

  // application settings

  g_object_class_install_property(gobject_class,BT_SETTINGS_AUDIOSINK,
                                  g_param_spec_string("audiosink",
                                     "audiosink prop",
                                     "audio output gstreamer element",
                                     "autoaudiosink", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MENU_TOOLBAR_HIDE,
                                  g_param_spec_boolean("toolbar-hide",
                                     "toolbar-hide",
                                     "hide main toolbar",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MENU_TABS_HIDE,
                                  g_param_spec_boolean("tabs-hide",
                                     "tabs-hide",
                                     "hide main page tabs",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
                                  g_param_spec_string("grid-density",
                                     "grid-density prop",
                                     "machine view grid detail level",
                                     "low", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_NEWS_SEEN,
                                  g_param_spec_uint("news-seen",
                                     "news-seen prop",
                                     "version number for that the user has seen the news",
                                     0,
                                     G_MAXUINT,
                                     0, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MISSING_MACHINES,
                                  g_param_spec_string("missing-machines",
                                     "missing-machines prop",
                                     "list of missing machines to ignore",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  // playback controller
  g_object_class_install_property(gobject_class,BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE,
                                  g_param_spec_boolean("coherence-upnp-active",
                                     "coherence-upnp-active",
                                     "activate Coherence UPnP based playback controller",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT,
                                  g_param_spec_uint("coherence-upnp-port",
                                     "coherence-upnp-port",
                                     "the port number for the communication with the coherence backend",
                                     0,
                                     G_MAXUINT,
                                     7654, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
  // system settings

  g_object_class_install_property(gobject_class,BT_SETTINGS_SYSTEM_AUDIOSINK,
                                  g_param_spec_string("system-audiosink",
                                     "system-audiosink prop",
                                     "system audio output gstreamer element",
                                     "autoaudiosink", /* default value */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_SYSTEM_TOOLBAR_STYLE,
                                  g_param_spec_string("toolbar-style",
                                     "toolbar-style prop",
                                     "system tolbar style",
                                     "both", /* default value */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_settings_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSettings),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_settings_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSettings",&info,G_TYPE_FLAG_ABSTRACT);
  }
  return type;
}
