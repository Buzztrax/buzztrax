/* $Id$
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
 * SECTION:btsettingspageaudiodevices
 * @short_description: audio device configuration settings page
 *
 * List available GStreamer audio devices. User can pick and configure one.
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_AUDIODEVICES_C

#include "bt-edit.h"

struct _BtSettingsPageAudiodevicesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  GtkComboBox *audiosink_menu;
  GList *audiosink_names;
  
  GtkComboBox *samplerate_menu;
  GtkComboBox *channels_menu;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_audiosink_menu_changed(GtkComboBox *combo_box, gpointer user_data) {
  BtSettingsPageAudiodevices *self=BT_SETTINGS_PAGE_AUDIODEVICES(user_data);
  BtSettings *settings;
  gulong index;

  index=gtk_combo_box_get_active(self->priv->audiosink_menu);
  GST_INFO("audiosink changed : index=%lu",index);

  g_object_get(self->priv->app,"settings",&settings,NULL);
  if(index) {
    g_object_set(settings,"audiosink",g_list_nth_data(self->priv->audiosink_names,index-1),NULL);
  }
  else {
    g_object_set(settings,"audiosink","",NULL);
  }
  g_object_unref(settings);
}

static void on_samplerate_menu_changed(GtkComboBox *combo_box, gpointer user_data) {
  BtSettingsPageAudiodevices *self=BT_SETTINGS_PAGE_AUDIODEVICES(user_data);
  BtSettings *settings;
  gulong index,rate;

  index=gtk_combo_box_get_active(self->priv->samplerate_menu);
  rate=atoi(gtk_combo_box_get_active_text(self->priv->samplerate_menu));
  GST_INFO("sample-rate changed : index=%lu, rate=%lu",index,rate);

  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_set(settings,"sample-rate",rate,NULL);
  g_object_unref(settings);
}

static void on_channels_menu_changed(GtkComboBox *combo_box, gpointer user_data) {
  BtSettingsPageAudiodevices *self=BT_SETTINGS_PAGE_AUDIODEVICES(user_data);
  BtSettings *settings;
  gulong index;

  index=gtk_combo_box_get_active(self->priv->channels_menu);
  GST_INFO("channels changed : index=%lu",index);

  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_set(settings,"channels",index+1,NULL);
  g_object_unref(settings);
}

//-- helper methods

static void bt_settings_page_audiodevices_init_ui(const BtSettingsPageAudiodevices *self) {
  BtSettings *settings;
  GtkWidget *label,*spacer;
  gchar *audiosink_name,*system_audiosink_name,*name,*str;
  GList *node,*audiosink_names;
  gboolean use_system_audiosink=TRUE;
  gboolean can_int_caps,can_float_caps;
  //GstCaps *int_caps=gst_caps_from_string(GST_AUDIO_INT_PAD_TEMPLATE_CAPS);
  //GstCaps *float_caps=gst_caps_from_string(GST_AUDIO_FLOAT_PAD_TEMPLATE_CAPS);
  guint sample_rate,channels;
  gulong audiosink_index=0,sampling_rate_index,ct;

  gtk_widget_set_name(GTK_WIDGET(self),"audio device settings");

  // get settings
  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_get(settings,
    "audiosink",&audiosink_name,
    "system-audiosink",&system_audiosink_name,
    "sample-rate",&sample_rate,
    "channels",&channels,
    NULL);
  if(BT_IS_STRING(audiosink_name)) use_system_audiosink=FALSE;

  // add setting widgets
  spacer=gtk_label_new("    ");
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Audio Device"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 0, 3, 0, 1,  GTK_FILL|GTK_EXPAND, GTK_SHRINK, 2,1);
  gtk_table_attach(GTK_TABLE(self),spacer, 0, 1, 1, 6, GTK_SHRINK,GTK_SHRINK, 2,1);

  label=gtk_label_new(_("Sink"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);
  self->priv->audiosink_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());

  /* @idea: we could use a real combo and use markup in the cells, to have the
   * description in small font below the sink name */
  if(system_audiosink_name) {
    GstElementFactory * const factory=gst_element_factory_find(system_audiosink_name);
    str=g_strdup_printf(_("system default: %s (%s)"),
      system_audiosink_name,
      (factory?gst_element_factory_get_description(factory):"-"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),str);
    g_free(str);
  }
  else {
    gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),_("system default: -"));
  }

  audiosink_names=bt_gst_registry_get_element_names_matching_all_categories("Sink/Audio");
  // @todo: sort list alphabetically ?
  
  /* @todo: use GST_IS_BIN(gst_element_factory_get_element_type(factory)) to skip bins
   * add autoaudiosink as the first and make it default
   *   but then again bins seem to be filtered out already, by below
   * should we filter by baseclass, GST_IS_BASE_AUDIO_SINK(...) - need instances
   */

  // add audio sinks gstreamer provides
  for(node=audiosink_names,ct=1;node;node=g_list_next(node)) {
    name=node->data;

    // filter some known analyzer sinks
    if(strncasecmp("ladspa-",name,7)) {
      GstElementFactory * const factory=gst_element_factory_find(name);

      // can the sink accept raw audio?
      can_int_caps=bt_gst_element_factory_can_sink_media_type(factory,"audio/x-raw-int");
      can_float_caps=bt_gst_element_factory_can_sink_media_type(factory,"audio/x-raw-float");
      //can_int_caps=gst_element_factory_can_sink_caps(factory,int_caps);
      //can_float_caps=gst_element_factory_can_sink_caps(factory,float_caps);
      if(can_int_caps || can_float_caps) {
        // @todo: try to open the element and skip those that we can't open
  
        // compare with audiosink_name and set audiosink_index if equal
        if(!use_system_audiosink) {
          if(!strcmp(audiosink_name,name)) audiosink_index=ct;
        }
        str=g_strdup_printf("%s (%s)",name,gst_element_factory_get_description(factory));
        gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),str);
        g_free(str);
        // add this to instance list
        self->priv->audiosink_names=g_list_append(self->priv->audiosink_names,name);
        GST_INFO("  adding audio sink: \"%s\"",name);
        ct++;
      }
      else {
        GST_INFO("  skipping audio sink: \"%s\" because of incompatible caps (%d,%d)",name,can_int_caps,can_float_caps);
      }
    }
    else {
      GST_INFO("  skipping audio sink: \"%s\" - its a ladspa element but not asm audio output",name);
    }
  }
  GST_INFO("current sink (is_system? %d): %lu",use_system_audiosink,audiosink_index);
  gtk_combo_box_set_active(self->priv->audiosink_menu,audiosink_index);
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(self->priv->audiosink_menu), 2, 3, 1, 2, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  g_signal_connect(self->priv->audiosink_menu, "changed", G_CALLBACK(on_audiosink_menu_changed), (gpointer)self);

  label=gtk_label_new(_("Sampling rate"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);
  
  self->priv->samplerate_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"8000");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"11025");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"16000");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"22050");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"32000");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"44100");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"48000");
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->samplerate_menu),"96000");
  switch(sample_rate) {
    case 8000:  sampling_rate_index=0;break;
    case 11025: sampling_rate_index=1;break;
    case 16000: sampling_rate_index=2;break;
    case 22050: sampling_rate_index=3;break;
    case 32000: sampling_rate_index=4;break;
    case 44100: sampling_rate_index=5;break;
    case 48000: sampling_rate_index=6;break;
    case 96000: sampling_rate_index=7;break;
    default: sampling_rate_index=5; // 44100
  }
  gtk_combo_box_set_active(self->priv->samplerate_menu,sampling_rate_index);
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(self->priv->samplerate_menu), 2, 3, 2, 3, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  g_signal_connect(self->priv->samplerate_menu, "changed", G_CALLBACK(on_samplerate_menu_changed), (gpointer)self);

  label=gtk_label_new(_("Channels"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 3, 4, GTK_SHRINK,GTK_SHRINK, 2,1);
  
  self->priv->channels_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->channels_menu),_("mono"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->channels_menu),_("stereo"));
  gtk_combo_box_set_active(self->priv->channels_menu,(channels-1));
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(self->priv->channels_menu), 2, 3, 3, 4, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  g_signal_connect(self->priv->channels_menu, "changed", G_CALLBACK(on_channels_menu_changed), (gpointer)self);

  /* @todo: add audiosink parameters
   * e.g. device-name
   * GstBaseSink: preroll-queue-len (buffers), max-lateness (ns)
   * GstBaseAudioSink: buffer-time (ms), latency-time (ms)
   * GstAudioSink: (no-properties)
   *
   * If we change settings here, we need to store them per sink, which is not so
   * easy with the current settings system.
   * We also need to restore them, when selecting the sink, allowing to reset
   * them to defaults.
   * Finaly sink-bin needs to apply those.
   *
   * buffer-time: is the total size of the ring-buffer
   * latency-time is the allowed latency
   */

  g_free(audiosink_name);
  g_free(system_audiosink_name);
  g_list_free(audiosink_names);
  //gst_caps_unref(int_caps);
  //gst_caps_unref(float_caps);
  g_object_unref(settings);
}

//-- constructor methods

/**
 * bt_settings_page_audiodevices_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageAudiodevices *bt_settings_page_audiodevices_new() {
  BtSettingsPageAudiodevices *self;

  self=BT_SETTINGS_PAGE_AUDIODEVICES(g_object_new(BT_TYPE_SETTINGS_PAGE_AUDIODEVICES,
    "n-rows",5,
    "n-columns",3,
    "homogeneous",FALSE,
    NULL));
  bt_settings_page_audiodevices_init_ui(self);
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_settings_page_audiodevices_dispose(GObject *object) {
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_settings_page_audiodevices_finalize(GObject *object) {
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(object);

  GST_DEBUG("!!!! self=%p",self);
  g_list_free(self->priv->audiosink_names);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_settings_page_audiodevices_init(GTypeInstance *instance, gpointer g_class) {
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SETTINGS_PAGE_AUDIODEVICES, BtSettingsPageAudiodevicesPrivate);
  self->priv->app = bt_edit_application_new();
}

static void bt_settings_page_audiodevices_class_init(BtSettingsPageAudiodevicesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSettingsPageAudiodevicesPrivate));

  gobject_class->dispose      = bt_settings_page_audiodevices_dispose;
  gobject_class->finalize     = bt_settings_page_audiodevices_finalize;
}

GType bt_settings_page_audiodevices_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSettingsPageAudiodevicesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_page_audiodevices_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSettingsPageAudiodevices),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_settings_page_audiodevices_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_TABLE,"BtSettingsPageAudiodevices",&info,0);
  }
  return type;
}
