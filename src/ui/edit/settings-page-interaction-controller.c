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
 * SECTION:btsettingspageinteractioncontroller
 * @short_description: interaction controller configuration settings page
 *
 * Lists available interaction controller devices and allows to select
 * controllers that should be used.
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_C

#include "bt-edit.h"

enum {
  DEVICE_MENU_LABEL=0
};

enum {
  CONTROLLER_LIST_LABEL=0
};

struct _BtSettingsPageInteractionControllerPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  GtkComboBox *device_menu;
  GtkTreeView *controller_list;
};

//-- the class

G_DEFINE_TYPE (BtSettingsPageInteractionController, bt_settings_page_interaction_controller, GTK_TYPE_TABLE);


//-- event handler

static void on_device_menu_changed(GtkComboBox *combo_box, gpointer user_data) {
  BtSettingsPageInteractionController *self=BT_SETTINGS_PAGE_INTERACTION_CONTROLLER(user_data);
  GObject *device=NULL;
  BtIcControl *control;
  BtObjectListModel *store;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GList *node,*list;

  GST_INFO("interaction controller device changed");
  model=gtk_combo_box_get_model(self->priv->device_menu);
  if(gtk_combo_box_get_active_iter(self->priv->device_menu,&iter)) {
    device=bt_object_list_model_get_object(BT_OBJECT_LIST_MODEL(model),&iter);
  }

  // update list of controllers
  store=bt_object_list_model_new(1,BTIC_TYPE_CONTROL,"name");
  if(device) {
    g_object_get(device,"controls",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      control=BTIC_CONTROL(node->data);
      bt_object_list_model_append(store,(GObject *)control);
    }
    g_list_free(list);
  }
  GST_INFO("control list refreshed");
  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->controller_list),(device!=NULL));
  gtk_tree_view_set_model(self->priv->controller_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}

static void on_ic_registry_devices_changed(BtIcRegistry *ic_registry,GParamSpec *arg,gpointer user_data) {
  BtSettingsPageInteractionController *self=BT_SETTINGS_PAGE_INTERACTION_CONTROLLER(user_data);
  BtIcDevice *device=NULL;
  GList *node,*list;
  BtObjectListModel *store;

  GST_INFO("refreshing device menu");

  store=bt_object_list_model_new(1,BTIC_TYPE_DEVICE,"name");
  g_object_get(ic_registry,"devices",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    device=BTIC_DEVICE(node->data);
    bt_object_list_model_append(store,(GObject *)device);
  }
  g_list_free(list);

  GST_INFO("device menu refreshed");
  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->device_menu),(device!=NULL));
  gtk_combo_box_set_model(self->priv->device_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->device_menu,((device!=NULL)?0:-1));
  g_object_unref(store); // drop with comboxbox
}

//-- helper methods

static void bt_settings_page_interaction_controller_init_ui(const BtSettingsPageInteractionController *self) {
  GtkWidget *label,*spacer, *scrolled_window;
  GtkCellRenderer *renderer;
  BtIcRegistry *ic_registry;
  gchar *str;

  gtk_widget_set_name(GTK_WIDGET(self),"interaction controller settings");

  // add setting widgets
  spacer=gtk_label_new("    ");
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Interaction Controller"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 0, 3, 0, 1,  GTK_FILL|GTK_EXPAND, GTK_SHRINK, 2,1);
  gtk_table_attach(GTK_TABLE(self),spacer, 0, 1, 1, 4, GTK_SHRINK,GTK_SHRINK, 2,1);

  label=gtk_label_new(_("Device"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 1, 2, GTK_FILL,GTK_SHRINK, 2,1);
  self->priv->device_menu=GTK_COMBO_BOX(gtk_combo_box_new());
  /* @todo: add icon: midi, joystick (from hal?)
   * /usr/share/icons/gnome/24x24/stock/media/stock_midi.png
   * /usr/share/gtk-doc/html/libgimpwidgets/stock-controller-midi-16.png
   * /usr/share/icons/Tango/22x22/devices/joystick.png
   */
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->device_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->device_menu),renderer,"text",DEVICE_MENU_LABEL,NULL);

  // get list of devices from libbtic and listen to changes
  g_object_get(self->priv->app,"ic-registry",&ic_registry,NULL);
  g_signal_connect(ic_registry,"notify::devices",G_CALLBACK(on_ic_registry_devices_changed),(gpointer)self);
  on_ic_registry_devices_changed(ic_registry,NULL,(gpointer)self);
  g_object_unref(ic_registry);

  gtk_combo_box_set_active(self->priv->device_menu,0);
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(self->priv->device_menu), 2, 3, 1, 2, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  g_signal_connect(self->priv->device_menu, "changed", G_CALLBACK(on_device_menu_changed), (gpointer)self);

  // add list of controllers (updated when selecting a device)
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->controller_list=GTK_TREE_VIEW(gtk_tree_view_new());
  /* @todo: add icon: trigger=button, range=knob/slider (from glade?)
   * /usr/share/glade3/pixmaps/hicolor/22x22/actions/
   * /usr/share/icons/gnome/22x22/apps/volume-knob.png
   */
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_tree_view_insert_column_with_attributes(self->priv->controller_list,-1,_("Controller"),renderer,
    "text",CONTROLLER_LIST_LABEL,
    NULL);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->controller_list),GTK_SELECTION_BROWSE);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->controller_list));
  gtk_table_attach(GTK_TABLE(self),GTK_WIDGET(scrolled_window), 1, 3, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  /* @todo: add "Learn" and "Tune" buttons
   * Learn: train new controllers (BTIC_IS_LEARN(device))
   * Tune: learn real range (useful if real range is less than the one defined by the api
   */

  on_device_menu_changed(self->priv->device_menu,(gpointer)self);
}

//-- constructor methods

/**
 * bt_settings_page_interaction_controller_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageInteractionController *bt_settings_page_interaction_controller_new(void) {
  BtSettingsPageInteractionController *self;

  self=BT_SETTINGS_PAGE_INTERACTION_CONTROLLER(g_object_new(BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER,
    "n-rows",3,
    "n-columns",3,
    "homogeneous",FALSE,
    NULL));
  bt_settings_page_interaction_controller_init_ui(self);
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_settings_page_interaction_controller_dispose(GObject *object) {
  BtSettingsPageInteractionController *self = BT_SETTINGS_PAGE_INTERACTION_CONTROLLER(object);
  BtIcRegistry *ic_registry;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_get(self->priv->app,"ic-registry",&ic_registry,NULL);
  g_signal_handlers_disconnect_matched(ic_registry,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,G_CALLBACK(on_ic_registry_devices_changed),(gpointer)self);
  g_object_unref(ic_registry);

  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(bt_settings_page_interaction_controller_parent_class)->dispose(object);
}

static void bt_settings_page_interaction_controller_init(BtSettingsPageInteractionController *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER, BtSettingsPageInteractionControllerPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();
}

static void bt_settings_page_interaction_controller_class_init(BtSettingsPageInteractionControllerClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtSettingsPageInteractionControllerPrivate));

  gobject_class->dispose      = bt_settings_page_interaction_controller_dispose;
}

