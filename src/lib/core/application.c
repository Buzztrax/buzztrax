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
 * SECTION:btapplication
 * @short_description: base class for a buzztrax based application
 *
 * Every application using the libbtcore library should inherit from this class.
 * Implementations should implement the singleton pattern.
 *
 * The base class automatically creates a #GstBin element as a container for the
 * song. This can be retrieved via the #BtApplication:bin property.
 * When creating #BtSong instances, the #BtApplication instance needs to be
 * passed to the bt_song_new() constructor, so that it can retrieve the #GstBin
 * element.
 * |[
 * BtApplication *app;
 * BtSong *song;
 * ...
 * song=bt_song_new(app);
 * ]|
 *
 * Another module the application base class maintains is a settings instance (see
 * #BtSettings), that manages application preferences.
 */

#define BT_CORE
#define BT_APPLICATION_C

#include "core_private.h"

enum
{
  APPLICATION_BIN = 1,
  APPLICATION_SETTINGS
};

struct _BtApplicationPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the main gstreamer container element */
  GstElement *bin;
  /* a reference to the buzztrax settings object */
  BtSettings *settings;
};

//-- the class

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtApplication, bt_application, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtApplication));

//-- helper

//-- handler

//-- constructor methods

//-- methods

//-- wrapper

//-- g_object overrides

static void
bt_application_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtApplication *const self = BT_APPLICATION (object);
  return_if_disposed ();
  switch (property_id) {
    case APPLICATION_BIN:
      g_value_set_object (value, self->priv->bin);
      break;
    case APPLICATION_SETTINGS:
      g_value_set_object (value, self->priv->settings);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_application_dispose (GObject * const object)
{
  const BtApplication *const self = BT_APPLICATION (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self));
  GST_INFO ("bin: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->bin));
  GST_INFO ("bin->numchildren=%d", GST_BIN (self->priv->bin)->numchildren);
  GST_INFO ("settings: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->settings));

  if (self->priv->bin) {
    GstStateChangeReturn res;
    GstBus *bus;

    if ((res =
            gst_element_set_state (GST_ELEMENT (self->priv->bin),
                GST_STATE_NULL)) == GST_STATE_CHANGE_FAILURE) {
      GST_WARNING ("can't go to null state");
    }
    GST_DEBUG ("->NULL state change returned '%s'",
        gst_element_state_change_return_get_name (res));

    bus = gst_element_get_bus (self->priv->bin);
    gst_bus_disable_sync_message_emission (bus);
    gst_object_unref (bus);
    gst_object_unref (self->priv->bin);
  }
  g_object_try_unref (self->priv->settings);

  G_OBJECT_CLASS (bt_application_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

//-- class internals

static void
bt_application_init (BtApplication * self)
{
  GstBus *bus;

  GST_DEBUG ("!!!! self=%p", self);
  self->priv = bt_application_get_instance_private(self);;
  self->priv->bin = gst_pipeline_new ("song");
  g_assert (GST_IS_ELEMENT (self->priv->bin));
  GST_DEBUG ("bin: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->bin));

  bus = gst_element_get_bus (self->priv->bin);
  gst_bus_enable_sync_message_emission (bus);
  gst_object_unref (bus);

  // tried this when debuging a case where we don't get bus messages
  //gst_pipeline_set_auto_flush_bus(GST_PIPELINE(self->priv->bin),FALSE);

  // if we enable this we get lots of diagnostics
  //g_signal_connect (self->priv->bin, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);

  self->priv->settings = bt_settings_make ();
  g_assert (BT_IS_SETTINGS (self->priv->settings));
  GST_INFO ("app has settings %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->settings));
}

static void
bt_application_class_init (BtApplicationClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  GST_DEBUG ("!!!!");

  gobject_class->get_property = bt_application_get_property;
  gobject_class->dispose = bt_application_dispose;

  /**
   * BtApplication:bin
   *
   * The top-level gstreamer element for the song, e.g. a #GstPipeline or
   * #GstBin.
   */
  g_object_class_install_property (gobject_class, APPLICATION_BIN,
      g_param_spec_object ("bin", "bin ro prop",
          "applications top-level GstElement container",
          GST_TYPE_BIN, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, APPLICATION_SETTINGS,
      g_param_spec_object ("settings", "settings ro prop",
          "applications configuration settings",
          BT_TYPE_SETTINGS, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
