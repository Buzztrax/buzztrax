/* Buzztard
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
 * In any case it is always sufficient to talk to this class instance. Single
 * settings are accessed via normat g_object_get() and g_object_set() calls. If
 * the backends supports it changes in the settings will be notified to the
 * application by the GObject::notify signal.
 */
/* TODO(ensonic): how can we decouple application specific settings for core settings?
 * We'd need to register schemas and create the GObject properties as needed.
 */
#define BT_CORE
#define BT_SETTINGS_C

#include "core_private.h"
#include "settings-private.h"
#include <gst/audio/multichannel.h>

static BtSettingsFactory bt_settings_factory=NULL;
static BtSettings *singleton=NULL;

//-- the class

G_DEFINE_ABSTRACT_TYPE (BtSettings, bt_settings, G_TYPE_OBJECT);

//-- helper

static gchar *parse_and_check_audio_sink(gchar *plugin_name) {
  gchar *sink_name,*eon;
  
  if(!plugin_name)
    return(NULL);

  GST_DEBUG("plugin name is: '%s'", plugin_name);

  // this can be a whole pipeline like "audioconvert ! osssink sync=false"
  // seek for the last '!'
  if(!(sink_name=strrchr(plugin_name,'!'))) {
    sink_name=plugin_name;
  }
  else {
    // skip '!' and spaces
    sink_name++;
    while(*sink_name==' ') sink_name++;
  }
  // if there is a space following put '\0' in there
  if((eon=strstr(sink_name," "))) {
    *eon='\0';
  }
  if ((sink_name!=plugin_name) || eon) {
    // no g_free() to partial memory later
    gchar * const temp=plugin_name;
    plugin_name=g_strdup(sink_name);
    g_free(temp);
  }
  if (BT_IS_STRING(plugin_name)) {
    GstPluginFeature *f;
    gboolean invalid=FALSE;

    if ((f=gst_registry_lookup_feature(gst_registry_get_default(), plugin_name))) {
      if(GST_IS_ELEMENT_FACTORY(f)) {
        gboolean can_int_caps,can_float_caps;

        can_int_caps=bt_gst_element_factory_can_sink_media_type((GstElementFactory *)f,"audio/x-raw-int");
        can_float_caps=bt_gst_element_factory_can_sink_media_type((GstElementFactory *)f,"audio/x-raw-float");
        if(!(can_int_caps || can_float_caps)) {
          GST_INFO("audiosink '%s' has no compatible caps", plugin_name);
          invalid=TRUE;
        }  
      } else {
        GST_INFO("audiosink '%s' not an element factory", plugin_name);
        invalid=TRUE;
      }
      gst_object_unref(f);
    }
    else {
      GST_INFO("audiosink '%s' not in registry", plugin_name);
      invalid=TRUE;
    }
    if(invalid) {
      g_free(plugin_name);
      plugin_name=NULL;
    }
  }
  return(plugin_name);
}

//-- constructor methods

/**
 * bt_settings_make:
 *
 * Create a new instance. The type of the settings depends on the subsystem
 * found during configuration run.
 *
 * Settings are implemented as a singleton. Thus the first invocation will
 * create the object and further calls will just give back a reference.
 *
 * Returns: the instance or %NULL in case of an error
 */
BtSettings *bt_settings_make(void) {

  if(G_UNLIKELY(!singleton)) {
    GST_INFO("create a new settings object");
    if(G_LIKELY(!bt_settings_factory)) {
#ifdef USE_GCONF
      singleton=(BtSettings *)bt_gconf_settings_new();
#else
      singleton=(BtSettings *)bt_plainfile_settings_new();
#endif
      GST_INFO("settings created %p",singleton);
    }
    else {
      singleton=bt_settings_factory();
      GST_INFO("created new settings object from factory %p",singleton);
    }
    g_object_add_weak_pointer((GObject *)singleton,(gpointer*)(gpointer)&singleton);
  }
  else {
    GST_INFO("return cached settings object %p (ref_ct=%d)",singleton,G_OBJECT_REF_COUNT(singleton));
    singleton=g_object_ref(singleton);
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
    GST_WARNING("can't change factory while having %d instances in use",G_OBJECT_REF_COUNT(singleton));
  }
}

/**
 * bt_settings_determine_audiosink_name:
 * @self: the settings
 *
 * Check the settings for the configured audio sink. Pick a fallback if none has
 * been chosen. Verify that the sink works.
 *
 * Returns: the elemnt name, free when done.
 */
gchar *bt_settings_determine_audiosink_name(const BtSettings * const self) {
  gchar *audiosink_name,*system_audiosink_name;
  gchar *plugin_name=NULL;

  g_object_get((GObject *)self,"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);

  if(BT_IS_STRING(audiosink_name)) {
    GST_INFO("get audiosink from config");
    plugin_name=parse_and_check_audio_sink(audiosink_name);
    audiosink_name=NULL;
  }
  if(!plugin_name && BT_IS_STRING(system_audiosink_name)) {
    GST_INFO("get audiosink from system config");
    plugin_name=parse_and_check_audio_sink(system_audiosink_name);
    system_audiosink_name=NULL;
  }
  if(!plugin_name) {
    // TODO(ensonic): try autoaudiosink (if it exists)
    // iterate over gstreamer-audiosink list and choose element with highest rank
    const GList *node;
    GList * const audiosink_factories=bt_gst_registry_get_element_factories_matching_all_categories("Sink/Audio");
    guint max_rank=0,cur_rank;
    gboolean can_int_caps,can_float_caps;

    GST_INFO("get audiosink from gst registry by rank");
    /* @bug: https://bugzilla.gnome.org/show_bug.cgi?id=601775 */
    GST_TYPE_AUDIO_CHANNEL_POSITION;

    for(node=audiosink_factories;node;node=g_list_next(node)) {
      GstElementFactory * const factory=node->data;
      const gchar *feature_name = gst_plugin_feature_get_name((GstPluginFeature *)factory);

      GST_INFO("  probing audio sink: \"%s\"",feature_name);

      // can the sink accept raw audio?
      can_int_caps=bt_gst_element_factory_can_sink_media_type(factory,"audio/x-raw-int");
      can_float_caps=bt_gst_element_factory_can_sink_media_type(factory,"audio/x-raw-float");
      if(can_int_caps || can_float_caps) {
        // get element max(rank)
        cur_rank=gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));
        GST_INFO("  trying audio sink: \"%s\" with rank: %d",feature_name,cur_rank);
        if((cur_rank>=max_rank) || (!plugin_name)) {
          g_free(plugin_name);
          plugin_name=g_strdup(feature_name);
          max_rank=cur_rank;
          GST_INFO("  audio sink \"%s\" is current best sink", plugin_name);
        }
      }
      else {
        GST_INFO("  skipping audio sink: \"%s\" because of incompatible caps",feature_name);
      }
    }
    gst_plugin_feature_list_free(audiosink_factories);
   }
  GST_INFO("using audio sink : \"%s\"",plugin_name);

  g_free(system_audiosink_name);
  g_free(audiosink_name);

  return(plugin_name);
}

//-- wrapper

//-- g_object overrides

static void bt_settings_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const GObjectClass * const gobject_class = G_OBJECT_GET_CLASS(object);

  // call implementation
  gobject_class->get_property(object,property_id,value,pspec);
}

static void bt_settings_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  GObjectClass * const gobject_class = G_OBJECT_GET_CLASS(object);

  // call implementation
  gobject_class->set_property(object,property_id,value,pspec);
}

//-- class internals

static void bt_settings_init(BtSettings * self) {
}

static void bt_settings_class_init(BtSettingsClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = bt_settings_set_property;
  gobject_class->get_property = bt_settings_get_property;

  // ui
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
                                     "list of tip-numbers that were shown already",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_PRESENTED_TIPS,
                                  g_param_spec_string("presented-tips",
                                     "presented-tips prop",
                                     "list of missing machines to ignore",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_SHOW_TIPS,
                                  g_param_spec_boolean("show-tips",
                                     "show-tips prop",
                                     "show tips on startup",
                                     TRUE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MENU_TOOLBAR_HIDE,
                                  g_param_spec_boolean("toolbar-hide",
                                     "toolbar-hide prop",
                                     "hide main toolbar",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MENU_STATUSBAR_HIDE,
                                  g_param_spec_boolean("statusbar-hide",
                                     "statusbar-hide prop",
                                     "hide bottom statusbar",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MENU_TABS_HIDE,
                                  g_param_spec_boolean("tabs-hide",
                                     "tabs-hide prop",
                                     "hide main page tabs",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
                                  g_param_spec_string("grid-density",
                                     "grid-density prop",
                                     "machine view grid detail level",
                                     "low", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_WINDOW_XPOS,
                                  g_param_spec_int("window-xpos",
                                     "window-xpos prop",
                                     "last application window x-position",
                                     G_MININT, G_MAXINT, 0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_WINDOW_YPOS,
                                  g_param_spec_int("window-ypos",
                                     "window-ypos prop",
                                     "last application window y-position",
                                     G_MININT, G_MAXINT, 0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_WINDOW_WIDTH,
                                  g_param_spec_int("window-width",
                                     "window-width prop",
                                     "last application window width",
                                     -1, G_MAXINT, -1,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_WINDOW_HEIGHT,
                                  g_param_spec_int("window-height",
                                     "window-height prop",
                                     "last application window height",
                                     -1, G_MAXINT, -1,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  // audio settings
  g_object_class_install_property(gobject_class,BT_SETTINGS_AUDIOSINK,
                                  g_param_spec_string("audiosink",
                                     "audiosink prop",
                                     "audio output gstreamer element",
                                     "autoaudiosink", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_SAMPLE_RATE,
                                  g_param_spec_uint("sample-rate",
                                     "sample-rate prop",
                                     "audio output sample-rate",
                                     1,
                                     96000,
                                     GST_AUDIO_DEF_RATE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_CHANNELS,
                                  g_param_spec_uint("channels",
                                     "channels prop",
                                     "number of audio output channels",
                                     1,
                                     2,
                                     2, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_LATENCY,
                                  g_param_spec_uint("latency",
                                     "latency prop",
                                     "target audio latency in ms",
                                     1,
                                     200,
                                     30, /* default value */
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

  g_object_class_install_property(gobject_class,BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_MASTER,
                                  g_param_spec_boolean("jack-transport-master",
                                     "jack-transport-master",
                                     "sync other jack clients to buzztard playback state",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_SLAVE,
                                  g_param_spec_boolean("jack-transport-slave",
                                     "jack-transport-slave",
                                     "sync buzztard to the playback state other jack clients",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));


  // directory settings
  g_object_class_install_property(gobject_class,BT_SETTINGS_FOLDER_SONG,
                                  g_param_spec_string("song-folder",
                                     "song-folder prop",
                                     "default directory for songs",
                                     g_get_home_dir(), /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_FOLDER_RECORD,
                                  g_param_spec_string("record-folder",
                                     "record-folder prop",
                                     "default directory for recordings",
                                     g_get_home_dir(), /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,BT_SETTINGS_FOLDER_SAMPLE,
                                  g_param_spec_string("sample-folder",
                                     "sample-folder prop",
                                     "default directory for sample-waveforms",
                                     g_get_home_dir(), /* default value */
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

