/* $Id: settings-dialog.c,v 1.5 2004-10-15 15:39:33 ensonic Exp $
 * class for the editor settings dialog
 */

#define BT_EDIT
#define BT_SETTINGS_DIALOG_C

#include "bt-edit.h"

enum {
  SETTINGS_DIALOG_APP=1,
};

enum {
  SETTINGS_PAGE_AUDIO_DEVICES=0,
  SETTINGS_PAGE_COLORS,
  SETTINGS_PAGE_SHORTCUTS
};

enum {
  COL_LABEL=0,
  COL_ID
};

struct _BtSettingsDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the list of settings pages */
  GtkTreeView *settings_list;
  
  /* the pages */
  GtkNotebook *settings_pages;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

void on_settings_list_cursor_changed(GtkTreeView *treeview,gpointer user_data) {
  BtSettingsDialog *self=BT_SETTINGS_DIALOG(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_assert(user_data);
  
  GST_INFO("settings list cursor changed");
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->settings_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gulong id;

    gtk_tree_model_get(model,&iter,COL_ID,&id,-1);
    GST_INFO("selected entry id %d",id);
    gtk_notebook_set_current_page(self->priv->settings_pages,id);
  }
}

//-- helper methods

static gboolean bt_settings_dialog_init_ui(const BtSettingsDialog *self) {
  BtSettings *settings;
  GtkWidget *box,*scrolled_window,*page;
  GtkWidget *label,*spacer,*table,*widget,*menu,*menu_item;  
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  gchar *audio_sink_name,*str;
  GList *audio_sink_names,*node;
  
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(settings,"audiosink",&audio_sink_name,NULL);
  audio_sink_names=bt_gst_registry_get_element_names_by_class("Sink/Audio");

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
  gtk_tree_view_insert_column_with_attributes(self->priv->settings_list,-1,NULL,renderer,"text",COL_LABEL,NULL);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->settings_list),GTK_SELECTION_BROWSE);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->settings_list));
  gtk_container_add(GTK_CONTAINER(box),GTK_WIDGET(scrolled_window));
  
  g_signal_connect(G_OBJECT(self->priv->settings_list),"cursor-changed",G_CALLBACK(on_settings_list_cursor_changed),(gpointer)self);

  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_LONG);
  //-- append entries fior settings pages
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,COL_LABEL,_("Audio Devices"),COL_ID,SETTINGS_PAGE_AUDIO_DEVICES,-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,COL_LABEL,_("Colors"),COL_ID,SETTINGS_PAGE_COLORS,-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,COL_LABEL,_("Shortcuts"),COL_ID,SETTINGS_PAGE_SHORTCUTS,-1);
  gtk_tree_view_set_model(self->priv->settings_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview

  // add notebook
  self->priv->settings_pages=GTK_NOTEBOOK(gtk_notebook_new());
  gtk_widget_set_name(GTK_WIDGET(self->priv->settings_pages),_("settings pages"));
  gtk_notebook_set_show_tabs(self->priv->settings_pages,FALSE);
  gtk_notebook_set_show_border(self->priv->settings_pages,FALSE);
  gtk_container_add(GTK_CONTAINER(box),GTK_WIDGET(self->priv->settings_pages));

  // @todo move pages into separate classes
  
  // add notebook page #1
  spacer=gtk_label_new("    ");
  table=gtk_table_new(3,4,FALSE);
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Audio Device"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 3, 0, 1,  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 2,1);
  gtk_table_attach(GTK_TABLE(table),spacer, 0, 1, 1, 4, GTK_SHRINK,GTK_SHRINK, 2,1);

  label=gtk_label_new(_("Sink"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 1, 2, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);
  widget=gtk_option_menu_new();
  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label(g_strdup_printf(_("system default (%s)"),audio_sink_name));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
  gtk_widget_show(menu_item);
  // add audio sinks gstreamer provides
  node=audio_sink_names;
  while(node) {
    menu_item=gtk_menu_item_new_with_label(g_strdup(node->data));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    gtk_widget_show(menu_item);
    node=g_list_next(node);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(widget),menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(widget),0);
  gtk_table_attach(GTK_TABLE(table),widget, 2, 3, 1, 2, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  
  gtk_container_add(GTK_CONTAINER(self->priv->settings_pages),table);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self->priv->settings_pages),
    gtk_notebook_get_nth_page(GTK_NOTEBOOK(self->priv->settings_pages),0),
    gtk_label_new(_("Audio Device")));

  // add notebook page #2
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(page),gtk_label_new("no settings on page 2 yet"));
  gtk_container_add(GTK_CONTAINER(self->priv->settings_pages),page);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self->priv->settings_pages),
    gtk_notebook_get_nth_page(GTK_NOTEBOOK(self->priv->settings_pages),1),
    gtk_label_new("page 2"));

  // add notebook page #3
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(page),gtk_label_new("no settings on page 3 yet"));
  gtk_container_add(GTK_CONTAINER(self->priv->settings_pages),page);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self->priv->settings_pages),
    gtk_notebook_get_nth_page(GTK_NOTEBOOK(self->priv->settings_pages),2),
    gtk_label_new("page 3"));

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self)->vbox),box);
  
  g_list_free(audio_sink_names);
  g_free(audio_sink_name);
  g_object_try_unref(settings);
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

