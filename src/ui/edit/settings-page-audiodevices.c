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
 * SECTION:btsettingspageaudiodevices
 * @short_description: audio device configuration settings page
 *
 * List available GStreamer audio devices. User can pick and configure one.
 */
/* TODO(ensonic): audio device selection
 * - we should probably parse the system-audiosink settings more to eventualy
 *   get the device from the setting
 */
/* TODO(ensonic): show the latency
 * - gst_base_sink_get_latency()
 *   - when is that available? - 
 *   - we might not be able to swap the sink immediately
 * - we have the latency caused by the fragment-size and depth of graph
 *   - thus it is different for each graph level
 *   - the min/max latency depends on the songs bpm,tpb and graph-depth
 *     - we could show that on the info-page instead
 * - the latency from changing a parameter until we hear something is what
 *   the user is interested in 
 * => we want to check how much ahead of the pipeline clock the data is processed
 *    - compare timestamps we set on buffers in chain functions with the clock
 * - we need a target latency setting
 * - in machine.c:bt_machine_init_interfaces() we calculate subticks so that we
 *   get close to that for the given bpm and tpb
 */
#define BT_EDIT
#define BT_SETTINGS_PAGE_AUDIODEVICES_C

#include "bt-edit.h"

#include <gst/interfaces/propertyprobe.h>

struct _BtSettingsPageAudiodevicesPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  GtkComboBox *audiosink_menu;
  GList *audiosink_names;
  GtkComboBox *device_menu;

  GtkComboBox *samplerate_menu;
  GtkComboBox *channels_menu;
  GtkSpinButton *latency_entry;
};

//-- the class

G_DEFINE_TYPE (BtSettingsPageAudiodevices, bt_settings_page_audiodevices,
    GTK_TYPE_TABLE);

//-- helper

static void
update_device_menu (const BtSettingsPageAudiodevices * self,
    gchar * element_name)
{
  gboolean has_devices = FALSE;
  GtkComboBox *combo_box = GTK_COMBO_BOX_TEXT (self->priv->device_menu);
  gint i, ct =
      gtk_tree_model_iter_n_children (gtk_combo_box_get_model (combo_box),
      NULL);
  gint index = -1;

  for (i = 0; i < ct; i++) {
    gtk_combo_box_text_remove (combo_box, 0);
  }

  if (element_name) {
    GstElement *sink = gst_element_factory_make (element_name, NULL);
    GstStateChangeReturn ret = gst_element_set_state (sink, GST_STATE_READY);

    if (ret != GST_STATE_CHANGE_FAILURE) {
      if (GST_IS_PROPERTY_PROBE (sink)) {
        GstPropertyProbe *probe = GST_PROPERTY_PROBE (sink);
        const GParamSpec *devspec;

        // TODO(ensonic): use "Auto" if value from setting == NULL or does not
        // match
        GST_INFO ("sink \"%s\" support property probe", element_name);
        if ((devspec = gst_property_probe_get_property (probe, "device"))) {
          GValueArray *array;

          if ((array =
                  gst_property_probe_probe_and_get_values (probe, devspec))) {
            BtSettings *settings;
            gchar *cur_device_name;

            g_object_get (self->priv->app, "settings", &settings, NULL);
            g_object_get (settings, "audiosink-device", &cur_device_name, NULL);
            if (array->n_values) {
              has_devices = TRUE;
              gtk_combo_box_text_append_text (combo_box, _("Auto"));
              if (*cur_device_name == '\0') {
                index = 0;
              }
            }

            GST_INFO ("there are %d available devices, current='%s'",
                array->n_values, cur_device_name);
            for (i = 0; i < array->n_values; i++) {
              GValue *device;
              const gchar *device_name;

              /* set this device */
              device = g_value_array_get_nth (array, i);
              device_name = g_value_get_string (device);
              GST_DEBUG ("device[%2d] \"%s\"", i, device_name);
              gtk_combo_box_text_append_text (combo_box, device_name);
              if (cur_device_name && !strcmp (device_name, cur_device_name)) {
                index = i + 1;
              }
            }
            g_free (cur_device_name);
            g_object_unref (settings);
            g_value_array_free (array);
          }
        }
      }
    }
    gst_element_set_state (sink, GST_STATE_NULL);
    gst_object_unref (sink);
  }
  gtk_combo_box_set_active (combo_box, index);
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->device_menu), has_devices);
}

//-- event handler

static void
on_audiosink_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (user_data);
  BtSettings *settings;
  gint index = gtk_combo_box_get_active (self->priv->audiosink_menu);
  gchar *element_name = NULL;

  GST_INFO ("audiosink changed : index=%d", index);

  g_object_get (self->priv->app, "settings", &settings, NULL);
  if (index) {
    element_name = g_list_nth_data (self->priv->audiosink_names, index - 1);
    g_object_set (settings, "audiosink", element_name, "audiosink-device", "",
        NULL);
  } else {
    g_object_set (settings, "audiosink", "", "audiosink-device", "", NULL);
  }
  update_device_menu (self, element_name);
  g_object_unref (settings);
}

static void
on_device_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (user_data);
  BtSettings *settings;
  gint index = gtk_combo_box_get_active (combo_box);

  GST_INFO ("audiodevice changed : index=%d", index);

  g_object_get (self->priv->app, "settings", &settings, NULL);
  if (index > 0) {
    gchar *device_name =
        gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo_box));

    g_object_set (settings, "audiosink-device", device_name, NULL);
    g_free (device_name);
  } else {
    g_object_set (settings, "audiosink-device", "", NULL);
  }
  g_object_unref (settings);
}

static void
on_samplerate_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (user_data);
  BtSettings *settings;
  gulong index, rate;

  index = gtk_combo_box_get_active (self->priv->samplerate_menu);
  rate =
      atoi (gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (self->priv->
              samplerate_menu)));
  GST_INFO ("sample-rate changed : index=%lu, rate=%lu", index, rate);

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_set (settings, "sample-rate", rate, NULL);
  g_object_unref (settings);
}

static void
on_channels_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (user_data);
  BtSettings *settings;
  gulong index;

  index = gtk_combo_box_get_active (self->priv->channels_menu);
  GST_INFO ("channels changed : index=%lu", index);

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_set (settings, "channels", index + 1, NULL);
  g_object_unref (settings);
}

static void
on_latency_entry_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (user_data);
  BtSettings *settings;
  guint latency;

  latency = gtk_spin_button_get_value_as_int (spinbutton);
  GST_INFO ("latency changed : latency=%u", latency);

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_set (settings, "latency", latency, NULL);
  g_object_unref (settings);
}

//-- helper methods

static void
bt_settings_page_audiodevices_init_ui (const BtSettingsPageAudiodevices * self)
{
  BtSettings *settings;
  BtSettingsClass *settings_class;
  GtkWidget *label, *spacer;
  GtkAdjustment *spin_adjustment;
  GParamSpecUInt *pspec;
  gchar *audiosink_name, *system_audiosink_name, *str;
  GList *node, *audiosink_factories;
  gboolean use_system_audiosink = TRUE;
  gboolean can_int_caps, can_float_caps;
  guint sample_rate, channels, latency;
  gulong audiosink_index = 0, sampling_rate_index, ct;

  gtk_widget_set_name (GTK_WIDGET (self), "audio device settings");

  // get settings
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings,
      "audiosink", &audiosink_name,
      "system-audiosink", &system_audiosink_name,
      "sample-rate", &sample_rate,
      "channels", &channels, "latency", &latency, NULL);
  if (BT_IS_STRING (audiosink_name))
    use_system_audiosink = FALSE;
  settings_class = BT_SETTINGS_GET_CLASS (settings);

  // add setting widgets
  spacer = gtk_label_new ("    ");
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Audio Device"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 0, 3, 0, 1, GTK_FILL | GTK_EXPAND,
      GTK_SHRINK, 2, 1);
  gtk_table_attach (GTK_TABLE (self), spacer, 0, 1, 1, 6, GTK_SHRINK,
      GTK_SHRINK, 2, 1);

  label = gtk_label_new (_("Sink"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK,
      2, 1);
  self->priv->audiosink_menu = GTK_COMBO_BOX (gtk_combo_box_text_new ());

  /* IDEA(ensonic): we could use a real combo and use markup in the cells, to
   * have the description in small font below the sink name */
  if (system_audiosink_name) {
    GstElementFactory *const factory =
        gst_element_factory_find (system_audiosink_name);
    str =
        g_strdup_printf (_("system default: %s (%s)"), system_audiosink_name,
        (factory ? gst_element_factory_get_description (factory) : "-"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
            audiosink_menu), str);
    g_free (str);
  } else {
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
            audiosink_menu), _("system default: -"));
  }

  audiosink_factories =
      bt_gst_registry_get_element_factories_matching_all_categories
      ("Sink/Audio");
  // TODO(ensonic): sort list alphabetically/ by rank ?

  /* TODO(ensonic): use GST_IS_BIN(gst_element_factory_get_element_type(factory)) to skip bins
   * add autoaudiosink as the first and make it default
   *   but then again bins seem to be filtered out already, by below
   *
   * we could also filter by baseclass, GST_IS_BASE_AUDIO_SINK(...) now that we have instances
   *
   * we should factor out the code below to a function and add a rescan button
   * -> useful if e.g. someone started jack in the meantime
   */

  // add audio sinks gstreamer provides
  for (node = audiosink_factories, ct = 1; node; node = g_list_next (node)) {
    GstElementFactory *factory = node->data;
    const gchar *name =
        gst_plugin_feature_get_name ((GstPluginFeature *) factory);
    GstPluginFeature *loaded_feature;
    GType type;

    // filter some known analyzer sinks
    if (!strncasecmp ("ladspa-", name, 7)) {
      GST_INFO ("  skipping audio sink: \"%s\" - its a ladspa element", name);
      continue;
    }
    // filter non auto pluggable elements
    if (gst_plugin_feature_get_rank (GST_PLUGIN_FEATURE (factory)) ==
        GST_RANK_NONE) {
      GST_INFO ("  skipping audio sink: \"%s\" - has RANK_NONE", name);
      continue;
    }
    // filter some known plugins
    if (!strcmp (GST_PLUGIN_FEATURE (factory)->plugin_name, "lv2")) {
      GST_INFO ("  skipping audio sink: \"%s\" - its a lv2 element", name);
      continue;
    }
    // get element type for filtering, this slows things down :/
    if (!(loaded_feature =
            gst_plugin_feature_load (GST_PLUGIN_FEATURE (factory)))) {
      GST_INFO ("  skipping unloadable element : '%s'", name);
      continue;
    }
    // we're no longer interested in the potentially-unloaded feature
    factory = (GstElementFactory *) loaded_feature;
    type = gst_element_factory_get_element_type (factory);
    // check class hierarchy
    if (!g_type_is_a (type, GST_TYPE_BASE_AUDIO_SINK)) {
      GST_INFO
          ("  skipping audio sink: \"%s\" - not inherited from baseaudiosink",
          name);
      continue;
    }
    // can the sink accept raw audio?
    can_int_caps =
        bt_gst_element_factory_can_sink_media_type (factory, "audio/x-raw-int");
    can_float_caps =
        bt_gst_element_factory_can_sink_media_type (factory,
        "audio/x-raw-float");
    if (can_int_caps || can_float_caps) {
      GstElement *sink;
      gboolean works = FALSE;

      // try to open the element and skip those that we can't open
      if ((sink = gst_element_factory_make (name, NULL))) {
        GstStateChangeReturn ret =
            gst_element_set_state (sink, GST_STATE_READY);
        if (ret != GST_STATE_CHANGE_FAILURE) {
          works = TRUE;
        } else {
          GST_INFO
              ("  skipping audio sink: \"%s\" as it could not go to READY",
              name);
        }
        gst_element_set_state (sink, GST_STATE_NULL);
        gst_object_unref (sink);
      } else {
        GST_INFO ("  skipping audio sink: \"%s\" as it could be instantiated",
            name);
      }

      if (works) {
        // compare with audiosink_name and set audiosink_index if equal
        if (!use_system_audiosink) {
          if (!strcmp (audiosink_name, name))
            audiosink_index = ct;
        }
        str =
            g_strdup_printf ("%s (%s)", name,
            gst_element_factory_get_description (factory));
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
                audiosink_menu), str);
        g_free (str);
        // add this to instance list
        self->priv->audiosink_names =
            g_list_append (self->priv->audiosink_names, (gpointer) name);
        GST_INFO ("  adding audio sink: \"%s\"", name);
        ct++;
      }
    } else {
      GST_INFO
          ("  skipping audio sink: \"%s\" because of incompatible caps (%d,%d)",
          name, can_int_caps, can_float_caps);
    }
  }
  GST_INFO ("current sink (is_system? %d): %lu", use_system_audiosink,
      audiosink_index);
  gtk_combo_box_set_active (self->priv->audiosink_menu, audiosink_index);
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (self->priv->audiosink_menu),
      2, 3, 1, 2, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  g_signal_connect (self->priv->audiosink_menu, "changed",
      G_CALLBACK (on_audiosink_menu_changed), (gpointer) self);

  label = gtk_label_new (_("Audio device"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK,
      2, 1);

  self->priv->device_menu = GTK_COMBO_BOX (gtk_combo_box_text_new ());
  update_device_menu (self, g_list_nth_data (self->priv->audiosink_names,
          audiosink_index - 1));
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (self->priv->device_menu),
      2, 3, 2, 3, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  g_signal_connect (self->priv->device_menu, "changed",
      G_CALLBACK (on_device_menu_changed), (gpointer) self);

  label = gtk_label_new (_("Sampling rate"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 3, 4, GTK_FILL, GTK_SHRINK,
      2, 1);

  self->priv->samplerate_menu = GTK_COMBO_BOX (gtk_combo_box_text_new ());
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "8000");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "11025");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "16000");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "22050");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "32000");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "44100");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "48000");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          samplerate_menu), "96000");
  switch (sample_rate) {
    case 8000:
      sampling_rate_index = 0;
      break;
    case 11025:
      sampling_rate_index = 1;
      break;
    case 16000:
      sampling_rate_index = 2;
      break;
    case 22050:
      sampling_rate_index = 3;
      break;
    case 32000:
      sampling_rate_index = 4;
      break;
    case 44100:
      sampling_rate_index = 5;
      break;
    case 48000:
      sampling_rate_index = 6;
      break;
    case 96000:
      sampling_rate_index = 7;
      break;
    default:
      sampling_rate_index = 5;  // 44100
  }
  gtk_combo_box_set_active (self->priv->samplerate_menu, sampling_rate_index);
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (self->priv->samplerate_menu),
      2, 3, 3, 4, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  g_signal_connect (self->priv->samplerate_menu, "changed",
      G_CALLBACK (on_samplerate_menu_changed), (gpointer) self);

  label = gtk_label_new (_("Channels"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 4, 5, GTK_FILL, GTK_SHRINK,
      2, 1);

  self->priv->channels_menu = GTK_COMBO_BOX (gtk_combo_box_text_new ());
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          channels_menu), _("mono"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (self->priv->
          channels_menu), _("stereo"));
  gtk_combo_box_set_active (self->priv->channels_menu, (channels - 1));
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (self->priv->channels_menu), 2,
      3, 4, 5, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  g_signal_connect (self->priv->channels_menu, "changed",
      G_CALLBACK (on_channels_menu_changed), (gpointer) self);

  label = gtk_label_new (_("Latency"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 5, 6, GTK_FILL, GTK_SHRINK,
      2, 1);

  pspec = (GParamSpecUInt *) g_object_class_find_property ((GObjectClass *)
      settings_class, "latency");
  spin_adjustment =
      GTK_ADJUSTMENT (gtk_adjustment_new (latency, pspec->minimum,
          pspec->maximum, 5.0, 10.0, 0.0));
  self->priv->latency_entry =
      GTK_SPIN_BUTTON (gtk_spin_button_new (spin_adjustment, 1.0, 0));
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (self->priv->latency_entry), 2,
      3, 5, 6, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);
  g_signal_connect (self->priv->latency_entry, "value-changed",
      G_CALLBACK (on_latency_entry_changed), (gpointer) self);

  /* TODO(ensonic): add audiosink parameters
   * GstBaseSink: preroll-queue-len (buffers), max-lateness (ns)
   * GstBaseAudioSink: buffer-time (ms), latency-time (ms)
   * GstAudioSink: (no-properties)
   *
   * buffer-time: is the total size of the ring-buffer
   * latency-time: is the allowed latency
   *
   * Adding more settings needs changes in sink-bin/audiosession to apply them.
   */

  g_free (audiosink_name);
  g_free (system_audiosink_name);
  gst_plugin_feature_list_free (audiosink_factories);
  g_object_unref (settings);
}

//-- constructor methods

/**
 * bt_settings_page_audiodevices_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageAudiodevices *
bt_settings_page_audiodevices_new (void)
{
  BtSettingsPageAudiodevices *self;

  self =
      BT_SETTINGS_PAGE_AUDIODEVICES (g_object_new
      (BT_TYPE_SETTINGS_PAGE_AUDIODEVICES, "n-rows", 7, "n-columns", 3,
          "homogeneous", FALSE, NULL));
  bt_settings_page_audiodevices_init_ui (self);
  gtk_widget_show_all (GTK_WIDGET (self));
  return (self);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_page_audiodevices_dispose (GObject * object)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_audiodevices_parent_class)->dispose (object);
}

static void
bt_settings_page_audiodevices_finalize (GObject * object)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_list_free (self->priv->audiosink_names);

  G_OBJECT_CLASS (bt_settings_page_audiodevices_parent_class)->finalize
      (object);
}

static void
bt_settings_page_audiodevices_init (BtSettingsPageAudiodevices * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SETTINGS_PAGE_AUDIODEVICES,
      BtSettingsPageAudiodevicesPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_settings_page_audiodevices_class_init (BtSettingsPageAudiodevicesClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSettingsPageAudiodevicesPrivate));

  gobject_class->dispose = bt_settings_page_audiodevices_dispose;
  gobject_class->finalize = bt_settings_page_audiodevices_finalize;
}
