/* $Id: settings-dialog.c,v 1.1 2004-10-11 16:19:15 ensonic Exp $
 * class for the editor settings dialog
 */

#define BT_EDIT
#define BT_SETTINGS_DIALOG_C

#include "bt-edit.h"

enum {
  SETTINGS_DIALOG_APP=1,
};


struct _BtSettingsDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  GtkTreeView *settings_list;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

//-- helper methods

static gboolean bt_settings_dialog_init_ui(const BtSettingsDialog *self) {
  GtkWidget *box,*scrolled_window;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  
  //gtk_widget_set_size_request(GTK_WIDGET(self),800,600);
  gtk_window_set_title(GTK_WINDOW(self), _("buzztard settings"));
  
  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);
  
  // add widgets to the dialog content area
  box=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  // add a list on the right and a notebook without tabs on the left
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->settings_list=GTK_TREE_VIEW(gtk_tree_view_new());
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_set_headers_visible(self->priv->settings_list,FALSE);
  gtk_tree_view_insert_column_with_attributes(self->priv->settings_list,-1,NULL,renderer,"text",0,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->settings_list));
  gtk_container_add(GTK_CONTAINER(box),GTK_WIDGET(scrolled_window));

  store=gtk_list_store_new(1,G_TYPE_STRING);
  //-- append entries fior settings pages
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,_("Audio Devices"),-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,_("Colors"),-1);
  gtk_tree_view_set_model(self->priv->settings_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview

  // @todo add notebook
  gtk_container_add(GTK_CONTAINER(box),gtk_label_new("no settings here yet"));
  
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self)->vbox),box);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_settings_dialog_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtSettingsDialog *bt_settings_dialog_new(const BtEditApplication *app) {
  BtSettingsDialog *self;

  if(!(self=BT_SETTINGS_DIALOG(g_object_new(BT_TYPE_SETTINGS_DIALOG,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_settings_dialog_init_ui(self)) {
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
static void bt_settings_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSettingsDialog *self = BT_SETTINGS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_settings_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSettingsDialog *self = BT_SETTINGS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_settings_dialog_dispose(GObject *object) {
  BtSettingsDialog *self = BT_SETTINGS_DIALOG(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_settings_dialog_finalize(GObject *object) {
  BtSettingsDialog *self = BT_SETTINGS_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_settings_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtSettingsDialog *self = BT_SETTINGS_DIALOG(instance);
  self->priv = g_new0(BtSettingsDialogPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_settings_dialog_class_init(BtSettingsDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

  parent_class=g_type_class_ref(GTK_TYPE_DIALOG);
  
  gobject_class->set_property = bt_settings_dialog_set_property;
  gobject_class->get_property = bt_settings_dialog_get_property;
  gobject_class->dispose      = bt_settings_dialog_dispose;
  gobject_class->finalize     = bt_settings_dialog_finalize;

  g_object_class_install_property(gobject_class,SETTINGS_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_settings_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSettingsDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSettingsDialog),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_settings_dialog_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_DIALOG,"BtSettingsDialog",&info,0);
  }
  return type;
}

