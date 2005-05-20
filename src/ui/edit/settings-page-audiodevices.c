/* $Id: settings-page-audiodevices.c,v 1.12 2005-05-20 13:54:34 ensonic Exp $
 * class for the editor settings audiodevices page
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
  BtEditApplication *app;
  
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
  g_object_try_unref(settings);
}

//-- helper methods

static gboolean bt_settings_page_audiodevices_init_ui(const BtSettingsPageAudiodevices *self) {
  BtSettings *settings;
  GtkWidget *box,*scrolled_window,*page;
  GtkWidget *label,*spacer,*widget;  
  gchar *audiosink_name,*system_audiosink_name,*str;
  GList *node;
  gulong audiosink_index=0,ct;
  gboolean use_system_audiosink=TRUE;
  
  self->priv->audiosink_names=bt_gst_registry_get_element_names_by_class("Sink/Audio");

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(settings,"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);
  if(is_string(audiosink_name)) use_system_audiosink=FALSE;

  // add notebook page #1
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
	
	str=g_strdup_printf(_("system default (%s)"),system_audiosink_name);
	gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),str);
	g_free(str);
  // add audio sinks gstreamer provides
	for(node=self->priv->audiosink_names,ct=1;node;node=g_list_next(node),ct++) {
    if(!use_system_audiosink) {
      // compare with audiosink_name and set audiosink_index if equal
      if(!strcmp(audiosink_name,node->data)) audiosink_index=ct;
    }
		gtk_combo_box_append_text(GTK_COMBO_BOX(self->priv->audiosink_menu),node->data);
  }
  gtk_combo_box_set_active(self->priv->audiosink_menu,audiosink_index);
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(self->priv->audiosink_menu), 2, 3, 1, 2, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  g_signal_connect(G_OBJECT(self->priv->audiosink_menu), "changed", G_CALLBACK(on_audiosink_menu_changed), (gpointer)self);
	// @todo add pages/subdialogs for each audiosink with its settings
  
  g_free(audiosink_name);
  g_free(system_audiosink_name);
  g_object_try_unref(settings);
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

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_settings_page_audiodevices_finalize(GObject *object) {
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(object);

  GST_DEBUG("!!!! self=%p",self);
  g_list_free(self->priv->audiosink_names);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_settings_page_audiodevices_init(GTypeInstance *instance, gpointer g_class) {
  BtSettingsPageAudiodevices *self = BT_SETTINGS_PAGE_AUDIODEVICES(instance);
  self->priv = g_new0(BtSettingsPageAudiodevicesPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_settings_page_audiodevices_class_init(BtSettingsPageAudiodevicesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_TABLE);
  
  gobject_class->set_property = bt_settings_page_audiodevices_set_property;
  gobject_class->get_property = bt_settings_page_audiodevices_get_property;
  gobject_class->dispose      = bt_settings_page_audiodevices_dispose;
  gobject_class->finalize     = bt_settings_page_audiodevices_finalize;

  g_object_class_install_property(gobject_class,SETTINGS_PAGE_AUDIODEVICES_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_settings_page_audiodevices_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSettingsPageAudiodevicesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_page_audiodevices_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSettingsPageAudiodevices),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_settings_page_audiodevices_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_TABLE,"BtSettingsPageAudiodevices",&info,0);
  }
  return type;
}
