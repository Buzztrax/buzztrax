/* $Id: settings-page-audiodevices.c,v 1.36 2007-11-02 15:29:54 ensonic Exp $
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
 * List available GStreamer audio devices and allows to selection to use.
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_AUDIODEVICES_C

#include "bt-edit.h"

enum {
  SETTINGS_PAGE_AUDIODEVICES_APP=1,
};

struct _BtSettingsPageAudiodevicesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  GtkComboBox *audiosink_menu;
  GList *audiosink_names;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_audiosink_menu_changed(GtkComboBox *combo_box, gpointer user_data) {
  BtSettingsPageAudiodevices *self=BT_SETTINGS_PAGE_AUDIODEVICES(user_data);
  BtSettings *settings;
  gulong index;

  g_assert(user_data);

  index=gtk_combo_box_get_active(self->priv->audiosink_menu);
  GST_INFO("audiosink change : index=%d",index);

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  if(index) {
    g_object_set(settings,"audiosink",g_list_nth_data(self->priv->audiosink_names,index-1),NULL);
  }
  else {
    g_object_set(settings,"audiosink","",NULL);
  }
  g_object_unref(settings);
}

//-- helper methods

static gboolean bt_settings_page_audiodevices_init_ui(const BtSettingsPageAudiodevices *self) {
  BtSettings *settings;
  GtkWidget *label,*spacer;
  gchar *audiosink_name,*system_audiosink_name,*str;
  GList *node,*audiosink_names;
  gulong audiosink_index=0,ct;
  gboolean use_system_audiosink=TRUE;
  gboolean can_int_caps,can_float_caps;
  //GstCaps *int_caps=gst_caps_from_string(GST_AUDIO_INT_PAD_TEMPLATE_CAPS);
  //GstCaps *float_caps=gst_caps_from_string(GST_AUDIO_FLOAT_PAD_TEMPLATE_CAPS);

  gtk_widget_set_name(GTK_WIDGET(self),_("audio device settings"));

  // get settings
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(settings,"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);
  if(BT_IS_STRING(audiosink_name)) use_system_audiosink=FALSE;

  // add setting widgets
  spacer=gtk_label_new("    ");
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Audio Device"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 0, 3, 0, 1,  GTK_FILL|GTK_EXPAND, GTK_SHRINK, 2,1);
  gtk_table_attach(GTK_TABLE(self),spacer, 0, 1, 1, 4, GTK_SHRINK,GTK_SHRINK, 2,1);

  label=gtk_label_new(_("Sink"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);
  self->priv->audiosink_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());

  str=g_strdup_printf(_("system default (%s)"),(system_audiosink_name?system_audiosink_name:"-"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),str);
  g_free(str);

  audiosink_names=bt_gst_registry_get_element_names_matching_all_categories("Sink/Audio");
  // this will also contain stuff like: "Sink/Analyzer/Audio"
  // solution would be to give all real AudioSinks a "Device" category

  // @todo: sort list alphabetically ?
  
  /* @todo: use GST_IS_BIN(gst_element_factory_get_element_type(factory)) to skip bins
   * add autoaudiosink as the first and make it default
   * but then again bins seem to be filtered out already, by below
   */
   
  /* we need to get rid of a few more entries still:
   * ladspa (some are analyzers) and sfsink (is a formatter)
   */

  // add audio sinks gstreamer provides
  for(node=audiosink_names,ct=1;node;node=g_list_next(node)) {
    GstElementFactory * const factory=gst_element_factory_find(node->data);

    // can the sink accept raw audio?
    can_int_caps=bt_gst_element_factory_can_sink_media_type(factory,"audio/x-raw-int");
    can_float_caps=bt_gst_element_factory_can_sink_media_type(factory,"audio/x-raw-float");
    //can_int_caps=gst_element_factory_can_sink_caps(factory,int_caps);
    //can_float_caps=gst_element_factory_can_sink_caps(factory,float_caps);
    if(can_int_caps || can_float_caps) {
      // @ todo: try to open the element and skip those that we can't open

      // compare with audiosink_name and set audiosink_index if equal
      if(!use_system_audiosink) {
        if(!strcmp(audiosink_name,node->data)) audiosink_index=ct;
      }
      gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),node->data);
      self->priv->audiosink_names=g_list_append(self->priv->audiosink_names,node->data);
      GST_INFO("  adding audio sink: \"%s\"",node->data);
      ct++;
    }
    else {
      GST_INFO("  skipping audio sink: \"%s\" because of incompatible caps (%d,%d)",node->data,can_int_caps,can_float_caps);
    }
  }
  GST_INFO("current sink (is_system? %d): %d",use_system_audiosink,audiosink_index);
  gtk_combo_box_set_active(self->priv->audiosink_menu,audiosink_index);
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(self->priv->audiosink_menu), 2, 3, 1, 2, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  g_signal_connect(G_OBJECT(self->priv->audiosink_menu), "changed", G_CALLBACK(on_audiosink_menu_changed), (gpointer)self);

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
  return(TRUE);
}

//-- constructor methods

/**
 * bt_settings_page_audiodevices_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSettingsPageAudiodevices *bt_settings_page_audiodevices_new(const BtEditApplication *app) {
  BtSettingsPageAudiodevices *self;

  if(!(self=BT_SETTINGS_PAGE_AUDIODEVICES(g_object_new(BT_TYPE_SETTINGS_PAGE_AUDIODEVICES,
    "app",app,
    "n-rows",3,
    "n-columns",3,
    "homogeneous",FALSE,
    NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_settings_page_audiodevices_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_settings_page_audiodevices_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_PAGE_AUDIODEVICES_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_settings_page_audiodevices_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_PAGE_AUDIODEVICES_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for settings_page_audiodevices: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_settings_page_audiodevices_dispose(GObject *object) {
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);

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
}

static void bt_settings_page_audiodevices_class_init(BtSettingsPageAudiodevicesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSettingsPageAudiodevicesPrivate));

  gobject_class->set_property = bt_settings_page_audiodevices_set_property;
  gobject_class->get_property = bt_settings_page_audiodevices_get_property;
  gobject_class->dispose      = bt_settings_page_audiodevices_dispose;
  gobject_class->finalize     = bt_settings_page_audiodevices_finalize;

  g_object_class_install_property(gobject_class,SETTINGS_PAGE_AUDIODEVICES_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

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
