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
  BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_ACTIVE,
  BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_SPEC,
  BT_SETTINGS_FOLDER_SONG,
  BT_SETTINGS_FOLDER_RECORD,
  BT_SETTINGS_FOLDER_SAMPLE,
  BT_SETTINGS_UI_DARK_THEME,
  BT_SETTINGS_UI_COMPACT_THEME,
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
  GSettings *org_buzztrax_ui;
  GSettings *org_gnome_desktop_interface;
  GSettings *org_freedesktop_gstreamer_defaults;
};

G_DEFINE_TYPE_WITH_CODE (BtSettings, bt_settings, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtSettings));

//-- helper

static gchar *
parse_and_check_audio_sink (gchar * plugin_name)
{
  gchar *sink_name, *eon;

  g_assert (plugin_name);
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
    gboolean valid = FALSE;

    if ((f = gst_registry_lookup_feature (gst_registry_get (), plugin_name))) {
      if (GST_IS_ELEMENT_FACTORY (f)) {
        valid = bt_gst_try_element ((GstElementFactory *) f, "audio/x-raw");
      } else {
        GST_INFO_OBJECT (f, "not an element factory");
      }
      gst_object_unref (f);
    } else {
      GST_INFO ("audiosink '%s' not in registry", plugin_name);
    }
    if (!valid) {
      g_free (plugin_name);
      plugin_name = NULL;
    }
  }
  GST_INFO ("parsed and checked element name: '%s'", plugin_name);
  return plugin_name;
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
g_settings_is_schema_installed (const gchar * schema_id)
{
  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  GSettingsSchema *schema;

  if ((schema = g_settings_schema_source_lookup (source, schema_id, TRUE))) {
    g_settings_schema_unref (schema);
    return TRUE;
  } else {
    return FALSE;
  }
}

static gint
compare_strings (gchar ** a, gchar ** b, gpointer user_data)
{
  return g_strcmp0 (*a, *b);
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
 * Returns: (transfer full): the instance or %NULL in case of an error
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
    // TODO: for local installs we can avoid the extra env-var by using
    // g_settings_new_with_backend_and_path("org.buzztrax", backend,
    //     DATADIR "/glib-2.0/schemas/")
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
    p->org_buzztrax_ui = g_settings_get_child (p->org_buzztrax, "ui");

    // add bindings
    g_signal_connect_object (p->org_gnome_desktop_interface, "changed",
        G_CALLBACK (on_settings_changed), (gpointer) singleton, 0);
    if (p->org_freedesktop_gstreamer_defaults) {
      g_signal_connect_object (p->org_freedesktop_gstreamer_defaults, "changed",
          G_CALLBACK (on_settings_changed), (gpointer) singleton, 0);
    }

    g_object_add_weak_pointer ((GObject *) singleton,
        (gpointer *) (gpointer) & singleton);
  } else {
    GST_INFO ("return cached settings object %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (singleton));
    singleton = g_object_ref (singleton);
  }
  return BT_SETTINGS (singleton);
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
    GST_INFO ("get audiosink from config: %s", audiosink_name);
    element_name = parse_and_check_audio_sink (audiosink_name);
    audiosink_name = NULL;
    if (element_name && _device_name) {
      g_object_get ((GObject *) self, "audiosink-device", _device_name, NULL);
    }
  }
  if (!element_name && BT_IS_STRING (system_audiosink_name)) {
    GST_INFO ("get audiosink from system config: %s", system_audiosink_name);
    element_name = parse_and_check_audio_sink (system_audiosink_name);
    system_audiosink_name = NULL;
  }
  if (!element_name) {
    // iterate over gstreamer-audiosink list and choose element with highest
    // rank, there is not much point in using autoaudiosink, as this will do
    // the same ...
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

      cur_rank = gst_plugin_feature_get_rank (GST_PLUGIN_FEATURE (factory));
      if (cur_rank > GST_RANK_NONE) {
        GST_INFO ("  trying audio sink: \"%s\" with rank: %d", feature_name,
            cur_rank);
        if (bt_gst_try_element (factory, "audio/x-raw")) {
          if ((cur_rank >= max_rank) || (!element_name)) {
            g_free (element_name);
            element_name = g_strdup (feature_name);
            max_rank = cur_rank;
            GST_INFO ("  audio sink \"%s\" is current best sink", element_name);
          }
        } else {
          GST_INFO ("  skipping audio sink: \"%s\"", feature_name);
        }
      } else {
        GST_INFO ("  skipping audio sink: \"%s\" due to rank=0", feature_name);
      }
    }
    gst_plugin_feature_list_free (audiosink_factories);
  }
  if (!element_name) {
    // support running unit tests on virtual machines
    if (!g_strcmp0 (g_getenv ("GSETTINGS_BACKEND"), "memory")) {
      GST_INFO ("no real audiosink found, falling back to fakesink");
      element_name = g_strdup ("fakesink");
    }
  }
  GST_INFO ("using audio sink : \"%s\"", element_name);

  g_free (system_audiosink_name);
  g_free (audiosink_name);

  *_element_name = element_name;

  return (element_name != NULL);
}

/**
 * bt_settings_parse_ic_playback_spec:
 * @spec: the spec string from the settings
 *
 * Parses the string.
 *
 * Returns: (element-type utf8 utf8) (transfer full): a hashtable with strings
 * as keys and values
 */
GHashTable *
bt_settings_parse_ic_playback_spec (const gchar * spec)
{
  GHashTable *ht;

  ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  if (BT_IS_STRING (spec)) {
    gchar **part, **parts;
    gchar *key, *value;

    parts = g_strsplit (spec, "\n", 0);
    for (part = parts; BT_IS_STRING (*part); part++) {
      key = *part;
      value = strchr (key, '\t');
      if (value) {
        *value = '\0';
        value++;
        g_strchomp (value);
        g_hash_table_insert (ht, g_strdup (key), g_strdup (value));
      } else {
        GST_WARNING ("missing '\t' in entry '%s'", key);
      }
    }
    g_strfreev (parts);
  }
  return ht;
}

/**
 * bt_settings_format_ic_playback_spec:
 * @ht: the ht settings
 *
 * Format the settings as a string.
 *
 * Returns: a string for storage.
 */
gchar *
bt_settings_format_ic_playback_spec (GHashTable * ht)
{
  GString *spec;
  gpointer *keys;
  guint num_keys, i;

  if (!ht || !g_hash_table_size (ht)) {
    GST_WARNING ("ht is %s", (ht ? "empty" : "NULL"));
    return NULL;
  }

  spec = g_string_new ("");
  // TODO(ensonic): since glib 2.40
  keys = g_hash_table_get_keys_as_array (ht, &num_keys);
  g_qsort_with_data (keys, num_keys, sizeof (gchar *),
      (GCompareDataFunc) compare_strings, NULL);

  for (i = 0; i < num_keys; i++) {
    g_string_append_printf (spec, "%s\t%s\n", (gchar *) keys[i],
        (gchar *) g_hash_table_lookup (ht, keys[i]));
  }
  g_free (keys);
  return g_string_free (spec, FALSE);
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
    case BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_ACTIVE:
      read_boolean (self->priv->org_buzztrax_playback_controller,
          "ic-playback-active", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_SPEC:
      read_string_def (self->priv->org_buzztrax_playback_controller,
          "ic-playback-spec", value, (GParamSpecString *) pspec);
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
    case BT_SETTINGS_UI_DARK_THEME:
      read_boolean (self->priv->org_buzztrax_ui, "dark-theme", value);
      break;
    case BT_SETTINGS_UI_COMPACT_THEME:
      read_boolean (self->priv->org_buzztrax_ui, "compact-theme", value);
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
    case BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_ACTIVE:
      write_boolean (self->priv->org_buzztrax_playback_controller,
          "ic-playback-active", value);
      break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_SPEC:
      write_string (self->priv->org_buzztrax_playback_controller,
          "ic-playback-spec", value);
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
    case BT_SETTINGS_UI_DARK_THEME:
      write_boolean (self->priv->org_buzztrax_ui, "dark-theme", value);
      break;
    case BT_SETTINGS_UI_COMPACT_THEME:
      write_boolean (self->priv->org_buzztrax_ui, "compact-theme", value);
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
  BtSettingsPrivate *p = singleton->priv;

  return_if_disposed ();
  p->dispose_has_run = TRUE;

  g_object_unref (p->org_buzztrax);
  g_object_unref (p->org_buzztrax_window);
  g_object_unref (p->org_buzztrax_audio);
  g_object_unref (p->org_buzztrax_playback_controller);
  g_object_unref (p->org_buzztrax_directories);
  g_object_unref (p->org_buzztrax_ui);

  g_object_unref (p->org_gnome_desktop_interface);
  g_object_try_unref (p->org_freedesktop_gstreamer_defaults);

  G_OBJECT_CLASS (bt_settings_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

//-- class internals

static void
bt_settings_init (BtSettings * self)
{
  GST_DEBUG ("!!!! self=%p", self);
  self->priv = bt_settings_get_instance_private(self);
}

static void
bt_settings_class_init (BtSettingsClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

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
          "audio output gstreamer element", NULL,
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

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_ACTIVE,
      g_param_spec_boolean ("ic-playback-active", "ic-playback-active",
          "activate interaction controller library based playback controller",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_PLAYBACK_CONTROLLER_IC_PLAYBACK_SPEC,
      g_param_spec_string ("ic-playback-spec", "ic-playback-spec",
          "list of device and control names", NULL,
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

  // ui settings
  g_object_class_install_property (gobject_class,
      BT_SETTINGS_UI_DARK_THEME,
      g_param_spec_boolean ("dark-theme", "dark-theme",
          "use dark theme variant", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_UI_COMPACT_THEME,
      g_param_spec_boolean ("compact-theme", "compact-theme",
          "use dense theme variant for small screens", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  // system settings
  g_object_class_install_property (gobject_class, BT_SETTINGS_SYSTEM_AUDIOSINK,
      g_param_spec_string ("system-audiosink", "system-audiosink prop",
          "system audio output gstreamer element", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      BT_SETTINGS_SYSTEM_TOOLBAR_STYLE, g_param_spec_string ("toolbar-style",
          "toolbar-style prop", "system tolbar style", "both",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
