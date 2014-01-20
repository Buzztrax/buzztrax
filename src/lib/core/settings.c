/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btsettings
 * @short_description: class for buzztrax settings handling
 *
 * Wraps the settings a #GObject. Single settings are accessed via normal
 * g_object_get() and g_object_set() calls. Changes in the settings will be notified
 * to the application by the GObject::notify signal.
 */
/* TODO(ensonic): how can we split application settings and core settings?
 * - we should create empty settings by default
 * - then each components (core, app) would attach the schemas
 *   - one can either attach a full schema, a named key from a schema or maybe a
 *     group of keys matching a prefix from a schema
 * - the settings object would register the gobject properties and map them to the
 *   schema keys
 *   - this could be modelled a bit after GOption, libs provide a conext, apps
 *     gathers a list of context objects and create the settings
 *   - we can make a new GObject for each schema and use the child_proxy_{set,get}
 * - the schema files seem to end up at $prefix/share/glib-2.0/schemas/
 * - we would alway listen for settings changes to turn that into notifies
 */
/* TODO(ensonic): use child groups more often
 * - we can group e.g. the settings for each settings page
 * - this makes it nicer to browse in dconf-editor and leads to shorter names
 * - but we also have to use the code below when editing them
 *   child_settings = g_settings_get_child(settings, "child")
 * - we could probably drop the whole set/get code for our own settings and use
 *   g_settings_bind
 */
#define BT_CORE
#define BT_SETTINGS_C

#include "core_private.h"
#include <gio/gsettingsbackend.h>
#include <gst/audio/audio.h>

static BtSettings *singleton = NULL;

//-- the class

/* don't forget to add entries to tests/bt-test-settings.c */
enum
{
  /* ui */
  BT_SETTINGS_NEWS_SEEN = 1,
  BT_SETTINGS_MISSING_MACHINES,
  BT_SETTINGS_PRESENTED_TIPS,
  BT_SETTINGS_SHOW_TIPS,
  BT_SETTINGS_MENU_TOOLBAR_HIDE,
  BT_SETTINGS_MENU_STATUSBAR_HIDE,
  BT_SETTINGS_MENU_TABS_HIDE,
  BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
  BT_SETTINGS_WINDOW_XPOS,
  BT_SETTINGS_WINDOW_YPOS,
  BT_SETTINGS_WINDOW_WIDTH,
  BT_SETTINGS_WINDOW_HEIGHT,
  /* preferences */
  BT_SETTINGS_AUDIOSINK,
  BT_SETTINGS_AUDIOSINK_DEVICE,
  BT_SETTINGS_SAMPLE_RATE,
  BT_SETTINGS_CHANNELS,
  BT_SETTINGS_LATENCY,
  BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE,
  BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT,
  BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_MASTER,
  BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_SLAVE,
  BT_SETTINGS_FOLDER_SONG,
  BT_SETTINGS_FOLDER_RECORD,
  BT_SETTINGS_FOLDER_SAMPLE,
  /* system settings */
  BT_SETTINGS_SYSTEM_AUDIOSINK,
  BT_SETTINGS_SYSTEM_TOOLBAR_STYLE,
  /* IDEA(ensonic): additional system settings
     BT_SETTINGS_SYSTEM_TOOLBAR_DETACHABLE <gboolean> org.gnome.desktop.interface/toolbar_detachable
     BT_SETTINGS_SYSTEM_TOOLBAR_ICON_SIZE  <gint>     org.gnome.desktop.interface/toolbar_icon_size
     BT_SETTINGS_SYSTEM_MENUBAR_DETACHABLE <gboolean> org.gnome.desktop.interface/menubar_detachable
     BT_SETTINGS_SYSTEM_MENU_HAVE_ICONS    <gboolean> org.gnome.desktop.interface/menus_have_icons
     BT_SETTINGS_SYSTEM_MENU_HAVE_TEAROFF  <gboolean> org.gnome.desktop.interface/menus_have_tearoff
     BT_SETTINGS_SYSTEM_KEYBOARD_LAYOUT    <gchar*>   org.gnome.desktop.peripherals/keyboard/{kbd,xkb}/layouts
   */
};

struct _BtSettingsPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  GSettings *org_buzztrax;
  GSettings *org_buzztrax_window;
  GSettings *org_buzztrax_audio;
  GSettings *org_buzztrax_playback_controller;
  GSettings *org_buzztrax_directories;
  GSettings *org_gnome_desktop_interface;
  GSettings *org_freedesktop_gstreamer_defaults;
};


G_DEFINE_TYPE (BtSettings, bt_settings, G_TYPE_OBJECT);

//-- helper

static gchar *
parse_and_check_audio_sink (gchar * plugin_name)
{
  gchar *sink_name, *eon;

  if (!plugin_name)
    return (NULL);

  GST_DEBUG ("plugin name is: '%s'", plugin_name);

  // this can be a whole pipeline like "audioconvert ! osssink sync=false"
  // seek for the last '!'
  if (!(sink_name = strrchr (plugin_name, '!'))) {
    sink_name = plugin_name;
  } else {
    // skip '!' and spaces
    sink_name++;
    while (*sink_name == ' ')
      sink_name++;
  }
  // if there is a space following put '\0' in there
  if ((eon = strstr (sink_name, " "))) {
    *eon = '\0';
  }
  if ((sink_name != plugin_name) || eon) {
    // no g_free() to partial memory later
    gchar *const temp = plugin_name;
    plugin_name = g_strdup (sink_name);
    g_free (temp);
  }
  if (BT_IS_STRING (plugin_name)) {
    GstPluginFeature *f;
    gboolean invalid = FALSE;

    if ((f = gst_registry_lookup_feature (gst_registry_get (), plugin_name))) {
      if (GST_IS_ELEMENT_FACTORY (f)) {
        if (!bt_gst_element_factory_can_sink_media_type ((GstElementFactory *)
                f, "audio/x-raw")) {
          GST_INFO ("audiosink '%s' has no compatible caps", plugin_name);
          invalid = TRUE;
        }
      } else {
        GST_INFO ("audiosink '%s' not an element factory", plugin_name);
        invalid = TRUE;
      }
      gst_object_unref (f);
    } else {
      GST_INFO ("audiosink '%s' not in registry", plugin_name);
      invalid = TRUE;
    }
    if (invalid) {
      g_free (plugin_name);
      plugin_name = NULL;
    }
  }
  return (plugin_name);
}

static void
read_boolean (GSettings * settings, const gchar * path, GValue * const value)
{
  gboolean prop = g_settings_get_boolean (settings, path);
  GST_DEBUG ("application reads '%s' : '%d'", path, prop);
  g_value_set_boolean (value, prop);
}

#if 0
static void
read_int (GSettings * settings, const gchar * path, GValue * const value)
{
  gint prop = g_settings_get_int (settings, path);
  GST_DEBUG ("application reads '%s' : '%i'", path, prop);
  g_value_set_int (value, prop);
}
#endif

static void
read_int_def (GSettings * settings, const gchar * path, GValue * const value,
    GParamSpecInt * const pspec)
{
  gint prop = g_settings_get_int (settings, path);
  if (prop) {
    GST_DEBUG ("application reads '%s' : '%i'", path, prop);
    g_value_set_int (value, prop);
  } else {
    GST_DEBUG ("application reads [def] '%s' : '%i'", path,
        pspec->default_value);
    g_value_set_int (value, pspec->default_value);
  }
}

static void
read_uint (GSettings * settings, const gchar * path, GValue * const value)
{
  guint prop = g_settings_get_uint (settings, path);
  GST_DEBUG ("application reads '%s' : '%u'", path, prop);
  g_value_set_uint (value, prop);
}

static void
read_uint_def (GSettings * settings, const gchar * path, GValue * const value,
    GParamSpecUInt * const pspec)
{
  guint prop = g_settings_get_uint (settings, path);
  if (prop) {
    GST_DEBUG ("application reads '%s' : '%u'", path, prop);
    g_value_set_uint (value, prop);
  } else {
    GST_DEBUG ("application reads [def] '%s' : '%u'", path,
        pspec->default_value);
    g_value_set_uint (value, pspec->default_value);
  }
}

static void
read_string (GSettings * settings, const gchar * path, GValue * const value)
{
  gchar *const prop = g_settings_get_string (settings, path);
  GST_DEBUG ("application reads '%s' : '%s'", path, prop);
  g_value_take_string (value, prop);
}

static void
read_string_def (GSettings * settings, const gchar * path, GValue * const value,
    GParamSpecString * const pspec)
{
  gchar *const prop = g_settings_get_string (settings, path);
  if (prop) {
    GST_DEBUG ("application reads '%s' : '%s'", path, prop);
    g_value_take_string (value, prop);
    //g_value_set_string(value,prop);
    //g_free(prop);
  } else {
    GST_DEBUG ("application reads [def] '%s' : '%s'", path,
        pspec->default_value);
    g_value_set_static_string (value, pspec->default_value);
  }
}


static void
write_boolean (GSettings * settings, const gchar * path,
    const GValue * const value)
{
  gboolean prop = g_value_get_boolean (value);
#ifndef GST_DISABLE_GST_DEBUG
  gboolean res =
#endif
      g_settings_set_boolean (settings, path, prop);
  GST_DEBUG ("application wrote '%s' : '%d' (%s)", path, prop,
      (res ? "okay" : "fail"));
}

static void
write_int (GSettings * settings, const gchar * path, const GValue * const value)
{
  gint prop = g_value_get_int (value);
#ifndef GST_DISABLE_GST_DEBUG
  gboolean res =
#endif
      g_settings_set_int (settings, path, prop);
  GST_DEBUG ("application wrote '%s' : '%i' (%s)", path, prop,
      (res ? "okay" : "fail"));
}

static void
write_uint (GSettings * settings, const gchar * path,
    const GValue * const value)
{
  guint prop = g_value_get_uint (value);
#ifndef GST_DISABLE_GST_DEBUG
  gboolean res =
#endif
      g_settings_set_uint (settings, path, prop);
  GST_DEBUG ("application wrote '%s' : '%u' (%s)", path, prop,
      (res ? "okay" : "fail"));
}

static void
write_string (GSettings * settings, const gchar * path,
    const GValue * const value)
{
  const gchar *prop = g_value_get_string (value);
#ifndef GST_DISABLE_GST_DEBUG
  gboolean res =
#endif
      g_settings_set_string (settings, path, (prop ? prop : ""));
  GST_DEBUG ("application wrote '%s' : '%s' (%s)", path, prop,
      (res ? "okay" : "fail"));
}

/*
 * g_settings_is_schema_installed:
 * schema: the schema to check
 *
 * g_settings_new() and co. fail with a fatal error if a schema is not found.
 *
 * Returns: TRUE if a schema is installed.
 */
static gboolean
g_settings_is_schema_installed (const gchar * schema)
{
  const gchar *const *schemas = g_settings_list_schemas ();
  gint i = 0;

  while (schemas[i]) {
    if (!strcmp (schemas[i], schema))
      return TRUE;
    i++;
  }
  return FALSE;
}

//-- signals

static void
on_settings_changed (GSettings * settings, gchar * key, gpointer user_data)
{
  const BtSettings *const self = BT_SETTINGS (user_data);

  if (!strcmp (key, "toolbar-style")) {
    g_object_notify ((GObject *) self, "toolbar-style");
  } else if (!strcmp (key, "music-audiosink")) {
    g_object_notify ((GObject *) self, "audiosink");
  }
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
BtSettings *
bt_settings_make (void)
{
  if (G_UNLIKELY (!singleton)) {
    GST_INFO ("create a new settings object");
    singleton = (BtSettings *) g_object_new (BT_TYPE_SETTINGS, NULL);
    BtSettingsPrivate *p = singleton->priv;
    GSettingsBackend *backend = g_settings_backend_get_default ();

    // add schemas
    p->org_buzztrax = g_settings_new_with_backend ("org.buzztrax", backend);
    p->org_gnome_desktop_interface =
        g_settings_new_with_backend ("org.gnome.desktop.interface", backend);
    if (g_settings_is_schema_installed
        ("org.freedesktop.gstreamer.default-elements")) {
      // FIXME: not yet ported
      p->org_freedesktop_gstreamer_defaults =
          g_settings_new_with_backend
          ("org.freedesktop.gstreamer.default-elements", backend);
    } else {
      GST_WARNING
          ("no gsettings schema for 'org.freedesktop.gstreamer.default-elements'");
    }

    // get child settings
    p->org_buzztrax_window = g_settings_get_child (p->org_buzztrax, "window");
    p->org_buzztrax_audio = g_settings_get_child (p->org_buzztrax, "audio");
    p->org_buzztrax_playback_controller =
        g_settings_get_child (p->org_buzztrax, "playback-controller");
    p->org_buzztrax_directories =
        g_settings_get_child (p->org_buzztrax, "directories");

    // add bindings
    g_signal_connect (p->org_gnome_desktop_interface, "changed",
        G_CALLBACK (on_settings_changed), (gpointer) singleton);
    if (p->org_freedesktop_gstreamer_defaults) {
      g_signal_connect (p->org_freedesktop_gstreamer_defaults, "changed",
          G_CALLBACK (on_settings_changed), (gpointer) singleton);
    }

    g_object_add_weak_pointer ((GObject *) singleton,
        (gpointer *) (gpointer) & singleton);
  } else {
    GST_INFO ("return cached settings object %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (singleton));
    singleton = g_object_ref (singleton);
  }
  return (BT_SETTINGS (singleton));
}

//-- methods

/**
 * bt_settings_determine_audiosink_name:
 * @self: the settings
 * @element_name: out variable for the element name
 * @device_name: out variable for the device property, if any
 *
 * Check the settings for the configured audio sink. Pick a fallback if none has
 * been chosen. Verify that the sink works.
 *
 * Free the strings in the output variables, when done.
 *
 * Returns: %TRUE if a audiosink has been found.
 */
gboolean
bt_settings_determine_audiosink_name (const BtSettings * const self,
    gchar ** _element_name, gchar ** _device_name)
{
  gchar *audiosink_name, *system_audiosink_name;
  gchar *element_name = NULL;

  g_return_val_if_fail (_element_name, FALSE);
  if (_device_name) {
    *_device_name = NULL;
  }

  g_object_get ((GObject *) self, "audiosink", &audiosink_name,
      "system-audiosink", &system_audiosink_name, NULL);

  if (BT_IS_STRING (audiosink_name)) {
    GST_INFO ("get audiosink from config");
    element_name = parse_and_check_audio_sink (audiosink_name);
    audiosink_name = NULL;
    if (element_name && _device_name) {
      g_object_get ((GObject *) self, "audiosink-device", _device_name, NULL);
    }
  }
  if (!element_name && BT_IS_STRING (system_audiosink_name)) {
    GST_INFO ("get audiosink from system config");
    element_name = parse_and_check_audio_sink (system_audiosink_name);
    system_audiosink_name = NULL;
  }
  if (!element_name) {
    // TODO(ensonic): try autoaudiosink (if it exists)
    // iterate over gstreamer-audiosink list and choose element with highest rank
    const GList *node;
    GList *const audiosink_factories =
        bt_gst_registry_get_element_factories_matching_all_categories
        ("Sink/Audio");
    guint max_rank = 0, cur_rank;

    GST_INFO ("get audiosink from gst registry by rank");
    /* @bug: https://bugzilla.gnome.org/show_bug.cgi?id=601775 */
    GST_TYPE_AUDIO_CHANNEL_POSITION;

    for (node = audiosink_factories; node; node = g_list_next (node)) {
      GstElementFactory *const factory = node->data;
      const gchar *feature_name =
          gst_plugin_feature_get_name ((GstPluginFeature *) factory);

      GST_INFO ("  probing audio sink: \"%s\"", feature_name);

      // can the sink accept raw audio?
      if (bt_gst_element_factory_can_sink_media_type (factory, "audio/x-raw")) {
        // get element max(rank)
        cur_rank = gst_plugin_feature_get_rank (GST_PLUGIN_FEATURE (factory));
        GST_INFO ("  trying audio sink: \"%s\" with rank: %d", feature_name,
            cur_rank);
        if ((cur_rank >= max_rank) || (!element_name)) {
          g_free (element_name);
          element_name = g_strdup (feature_name);
          max_rank = cur_rank;
          GST_INFO ("  audio sink \"%s\" is current best sink", element_name);
        }
      } else {
        GST_INFO ("  skipping audio sink: \"%s\" because of incompatible caps",
            feature_name);
      }
    }
    gst_plugin_feature_list_free (audiosink_factories);
  }
  GST_INFO ("using audio sink : \"%s\"", element_name);

  g_free (system_audiosink_name);
  g_free (audiosink_name);

  *_element_name = element_name;

  return (element_name != NULL);
}

//-- wrapper

//-- g_object overrides

static void
bt_settings_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtSettings *const self = BT_SETTINGS (object);
  return_if_disposed ();

  switch (property_id) {
      /* ui */
    case BT_SETTINGS_NEWS_SEEN:
      read_uint (self->priv->org_buzztrax, "news-seen", value);
      break;
    case BT_SETTINGS_MISSING_MACHINES:
      read_string (self->priv->org_buzztrax, "missing-machines", value);
      break;
    case BT_SETTINGS_PRESENTED_TIPS:
      read_string (self->priv->org_buzztrax, "presented-tips", value);
      break;
    case BT_SETTINGS_SHOW_TIPS:
      read_boolean (self->priv->org_buzztrax, "show-tips", value);
      break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE:
      read_boolean (self->priv->org_buzztrax, "toolbar-hide", value);
      break;
    case BT_SETTINGS_MENU_STATUSBAR_HIDE:
      read_boolean (self->priv->org_buzztrax, "statusbar-hide", value);
      break;
    case BT_SETTINGS_MENU_TABS_HIDE:
      read_boolean (self->priv->org_buzztrax, "tabs-hide", value);
      break;
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY:
      read_string_def (self->priv->org_buzztrax, "grid-density", value,
          (GParamSpecString *) pspec);
      break;
    case BT_SETTINGS_WINDOW_XPOS:
      read_int_def (self->priv->org_buzztrax_window, "x-pos", value,
          (GParamSpecInt *) pspec);
      break;
    case BT_SETTINGS_WINDOW_YPOS:
      read_int_def (self->priv->org_buzztrax_window, "y-pos", value,
          (GParamSpecInt *) pspec);
      break;
    case BT_SETTINGS_WINDOW_WIDTH:
      read_int_def (self->priv->org_buzztrax_window, "width", value,
          (GParamSpecInt *) pspec);
      break;
    case BT_SETTINGS_WINDOW_HEIGHT:
      read_int_def (self->priv->org_buzztrax_window, "height", value,
          (GParamSpecInt *) pspec);
      break;
      /* audio settings */
    case BT_SETTINGS_AUDIOSINK:
      read_string_def (self->priv->org_buzztrax_audio, "audiosink", value,
          (GParamSpecString *) pspec);
      break;
    case BT_SETTINGS_AUDIOSINK_DEVICE:
      read_string_def (self->priv->org_buzztrax_audio, "audiosink-device",
          value, (GParamSpecString *) pspec);
      break;
    case BT_SETTINGS_SAMPLE_RATE:
      read_uint_def (self->priv->org_buzztrax_audio, "sample-rate", value,
          (GParamSpecUInt *) pspec);
      break;
    case BT_SETTINGS_CHANNELS:
      read_uint_def (self->priv->org_buzztrax_audio, "channels", value,
          (GParamSpecUInt *) pspec);
      break;
    case BT_SETTINGS_LATENCY:
      read_uint_def (self->priv->org_buzztrax_audio, "latency", value,
          (GParamSpecUInt *) pspec);
      break;
      /* playback controller */
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE:
      read_boolean (self->priv->org_buzztrax_playback_controller,
          "coherence-upnp-active", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT:
      read_uint (self->priv->org_buzztrax_playback_controller,
          "coherence-upnp-port", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_MASTER:
      read_boolean (self->priv->org_buzztrax_playback_controller,
          "jack-transport-master", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_SLAVE:
      read_boolean (self->priv->org_buzztrax_playback_controller,
          "jack-transport-slave", value);
      break;
      /* directory settings */
    case BT_SETTINGS_FOLDER_SONG:
      read_string_def (self->priv->org_buzztrax_directories, "song-folder",
          value, (GParamSpecString *) pspec);
      break;
    case BT_SETTINGS_FOLDER_RECORD:
      read_string_def (self->priv->org_buzztrax_directories, "record-folder",
          value, (GParamSpecString *) pspec);
      break;
    case BT_SETTINGS_FOLDER_SAMPLE:
      read_string_def (self->priv->org_buzztrax_directories, "sample-folder",
          value, (GParamSpecString *) pspec);
      break;
      /* system settings */
    case BT_SETTINGS_SYSTEM_AUDIOSINK:
      // org.freedesktop.gstreamer.default-elements : music-audiosink
      if (self->priv->org_freedesktop_gstreamer_defaults) {
        read_string (self->priv->org_freedesktop_gstreamer_defaults,
            "music-audiosink", value);
      } else {
        g_value_set_static_string (value,
            ((GParamSpecString *) pspec)->default_value);
      }
      break;
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE:
      // org.gnome.desktop.interface
      read_string (self->priv->org_gnome_desktop_interface, "toolbar-style",
          value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_settings_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtSettings *const self = BT_SETTINGS (object);
  return_if_disposed ();

  switch (property_id) {
      /* ui */
    case BT_SETTINGS_NEWS_SEEN:
      write_uint (self->priv->org_buzztrax, "news-seen", value);
      break;
    case BT_SETTINGS_MISSING_MACHINES:
      write_string (self->priv->org_buzztrax, "missing-machines", value);
      break;
    case BT_SETTINGS_PRESENTED_TIPS:
      write_string (self->priv->org_buzztrax, "presented-tips", value);
      break;
    case BT_SETTINGS_SHOW_TIPS:
      write_boolean (self->priv->org_buzztrax, "show-tips", value);
      break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE:
      write_boolean (self->priv->org_buzztrax, "toolbar-hide", value);
      break;
    case BT_SETTINGS_MENU_STATUSBAR_HIDE:
      write_boolean (self->priv->org_buzztrax, "statusbar-hide", value);
      break;
    case BT_SETTINGS_MENU_TABS_HIDE:
      write_boolean (self->priv->org_buzztrax, "tabs-hide", value);
      break;
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY:
      write_string (self->priv->org_buzztrax, "grid-density", value);
      break;
    case BT_SETTINGS_WINDOW_XPOS:
      write_int (self->priv->org_buzztrax_window, "x-pos", value);
      break;
    case BT_SETTINGS_WINDOW_YPOS:
      write_int (self->priv->org_buzztrax_window, "y-pos", value);
      break;
    case BT_SETTINGS_WINDOW_WIDTH:
      write_int (self->priv->org_buzztrax_window, "width", value);
      break;
    case BT_SETTINGS_WINDOW_HEIGHT:
      write_int (self->priv->org_buzztrax_window, "height", value);
      break;
      /* audio settings */
    case BT_SETTINGS_AUDIOSINK:
      write_string (self->priv->org_buzztrax_audio, "audiosink", value);
      break;
    case BT_SETTINGS_AUDIOSINK_DEVICE:
      write_string (self->priv->org_buzztrax_audio, "audiosink-device", value);
      break;
    case BT_SETTINGS_SAMPLE_RATE:
      write_uint (self->priv->org_buzztrax_audio, "sample-rate", value);
      break;
    case BT_SETTINGS_CHANNELS:
      write_uint (self->priv->org_buzztrax_audio, "channels", value);
      break;
    case BT_SETTINGS_LATENCY:
      write_uint (self->priv->org_buzztrax_audio, "latency", value);
      break;
      /* playback controller */
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE:
      write_boolean (self->priv->org_buzztrax_playback_controller,
          "coherence-upnp-active", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT:
      write_uint (self->priv->org_buzztrax_playback_controller,
          "coherence-upnp-port", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_MASTER:
      write_boolean (self->priv->org_buzztrax_playback_controller,
          "jack-transport-master", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_SLAVE:
      write_boolean (self->priv->org_buzztrax_playback_controller,
          "jack-transport-slave", value);
      break;
      /* directory settings */
    case BT_SETTINGS_FOLDER_SONG:
      write_string (self->priv->org_buzztrax_directories, "song-folder", value);
      break;
    case BT_SETTINGS_FOLDER_RECORD:
      write_string (self->priv->org_buzztrax_directories, "record-folder",
          value);
      break;
    case BT_SETTINGS_FOLDER_SAMPLE:
      write_string (self->priv->org_buzztrax_directories, "sample-folder",
          value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_settings_dispose (GObject * const object)
{
  const BtSettings *const self = BT_SETTINGS (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  g_object_unref (self->priv->org_buzztrax);
  g_object_unref (self->priv->org_buzztrax_window);
  g_object_unref (self->priv->org_buzztrax_audio);
  g_object_unref (self->priv->org_buzztrax_playback_controller);
  g_object_unref (self->priv->org_buzztrax_directories);
  g_object_unref (self->priv->org_gnome_desktop_interface);
  g_object_try_unref (self->priv->org_freedesktop_gstreamer_defaults);

  G_OBJECT_CLASS (bt_settings_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

//-- class internals

static void
bt_settings_init (BtSettings * self)
{
  GST_DEBUG ("!!!! self=%p", self);
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SETTINGS, BtSettingsPrivate);
}

static void
bt_settings_class_init (BtSettingsClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSettingsPrivate));

  gobject_class->set_property = bt_settings_set_property;
  gobject_class->get_property = bt_settings_get_property;
  gobject_class->dispose = bt_settings_dispose;

  // ui
  g_object_class_install_property (gobject_class, BT_SETTINGS_NEWS_SEEN,
      g_param_spec_uint ("news-seen", "news-seen prop",
          "version number for that the user has seen the news", 0, G_MAXUINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_MISSING_MACHINES,
      g_param_spec_string ("missing-machines", "missing-machines prop",
          "list of tip-numbers that were shown already", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_PRESENTED_TIPS,
      g_param_spec_string ("presented-tips", "presented-tips prop",
          "list of missing machines to ignore", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_SHOW_TIPS,
      g_param_spec_boolean ("show-tips", "show-tips prop",
          "show tips on startup", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_MENU_TOOLBAR_HIDE,
      g_param_spec_boolean ("toolbar-hide", "toolbar-hide prop",
          "hide main toolbar", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_MENU_STATUSBAR_HIDE, g_param_spec_boolean ("statusbar-hide",
          "statusbar-hide prop", "hide bottom statusbar", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_MENU_TABS_HIDE,
      g_param_spec_boolean ("tabs-hide", "tabs-hide prop",
          "hide main page tabs", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
      g_param_spec_string ("grid-density", "grid-density prop",
          "machine view grid detail level", "low",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_WINDOW_XPOS,
      g_param_spec_int ("window-xpos",
          "window-xpos prop",
          "last application window x-position",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_WINDOW_YPOS,
      g_param_spec_int ("window-ypos",
          "window-ypos prop",
          "last application window y-position",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_WINDOW_WIDTH,
      g_param_spec_int ("window-width",
          "window-width prop",
          "last application window width",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_WINDOW_HEIGHT,
      g_param_spec_int ("window-height",
          "window-height prop",
          "last application window height",
          -1, G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  // audio settings
  g_object_class_install_property (gobject_class, BT_SETTINGS_AUDIOSINK,
      g_param_spec_string ("audiosink", "audiosink prop",
          "audio output gstreamer element", "autoaudiosink",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_AUDIOSINK_DEVICE,
      g_param_spec_string ("audiosink-device", "audiosink-device prop",
          "audio output device name", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_SAMPLE_RATE,
      g_param_spec_uint ("sample-rate", "sample-rate prop",
          "audio output sample-rate", 1, 96000, GST_AUDIO_DEF_RATE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_CHANNELS,
      g_param_spec_uint ("channels", "channels prop",
          "number of audio output channels", 1, 2, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_LATENCY,
      g_param_spec_uint ("latency", "latency prop",
          "target audio latency in ms", 1, 200, 30,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  // playback controller
  g_object_class_install_property (gobject_class,
      BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE,
      g_param_spec_boolean ("coherence-upnp-active", "coherence-upnp-active",
          "activate Coherence UPnP based playback controller", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT,
      g_param_spec_uint ("coherence-upnp-port", "coherence-upnp-port",
          "the port number for the communication with the coherence backend", 0,
          G_MAXUINT, 7654, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_MASTER,
      g_param_spec_boolean ("jack-transport-master", "jack-transport-master",
          "sync other jack clients to buzztrax playback state", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_PLAYBACK_CONTROLLER_JACK_TRANSPORT_SLAVE,
      g_param_spec_boolean ("jack-transport-slave", "jack-transport-slave",
          "sync buzztrax to the playback state other jack clients", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  // directory settings
  g_object_class_install_property (gobject_class, BT_SETTINGS_FOLDER_SONG,
      g_param_spec_string ("song-folder", "song-folder prop",
          "default directory for songs", g_get_home_dir (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_FOLDER_RECORD,
      g_param_spec_string ("record-folder", "record-folder prop",
          "default directory for recordings", g_get_home_dir (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, BT_SETTINGS_FOLDER_SAMPLE,
      g_param_spec_string ("sample-folder", "sample-folder prop",
          "default directory for sample-waveforms", g_get_home_dir (),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  // system settings
  g_object_class_install_property (gobject_class, BT_SETTINGS_SYSTEM_AUDIOSINK,
      g_param_spec_string ("system-audiosink", "system-audiosink prop",
          "system audio output gstreamer element", "autoaudiosink",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_SYSTEM_TOOLBAR_STYLE, g_param_spec_string ("toolbar-style",
          "toolbar-style prop", "system tolbar style", "both",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
