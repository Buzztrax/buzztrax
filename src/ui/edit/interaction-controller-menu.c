/* $Id$
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btinteractioncontrollermenu
 * @short_description: class for the interaction controller assignment popup menu
 *
 * Build a menu with available interaction controllers of a type.
 */

#define BT_EDIT
#define BT_INTERACTION_CONTROLLER_MENU_C

#include "bt-edit.h"

//-- property ids

enum {
  INTERACTION_CONTROLLER_MENU_TYPE=1,
  INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL,
  INTERACTION_CONTROLLER_MENU_ITEM_UNBIND,
  INTERACTION_CONTROLLER_MENU_ITEM_UNBIND_ALL
};


struct _BtInteractionControllerMenuPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  BtInteractionControllerMenuType type;

  /* the selected control */
  BtIcControl *selected_control;

  /* actions */
  GtkWidget *item_unbind,*item_unbind_all;
};

static GQuark widget_parent_quark=0;

//-- the class

G_DEFINE_TYPE (BtInteractionControllerMenu, bt_interaction_controller_menu, GTK_TYPE_MENU);

//-- enums

GType bt_interaction_controller_menu_type_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_INTERACTION_CONTROLLER_RANGE_MENU,   "BT_INTERACTION_CONTROLLER_RANGE_MENU",   "range controllers" },
      { BT_INTERACTION_CONTROLLER_TRIGGER_MENU, "BT_INTERACTION_CONTROLLER_TRIGGER_MENU", "trigger controllers" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtInteractionControllerMenuType", values);
  }
  return type;
}

//-- event handler

static void on_control_bind_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtInteractionControllerMenu *self=BT_INTERACTION_CONTROLLER_MENU(g_object_get_qdata(G_OBJECT(menuitem),widget_parent_quark));
  BtIcControl *control=BTIC_CONTROL(user_data);

#if 0
  GtkWidget *parent;

  parent=gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(menuitem)));
  GST_WARNING("1.) %p =? %p (%s)",self,parent,G_OBJECT_TYPE_NAME(parent));
  parent=gtk_widget_get_toplevel(GTK_WIDGET(menuitem));
  GST_WARNING("2.) %p =? %p (%s)",self,parent,G_OBJECT_TYPE_NAME(parent));
  //parent=gtk_menu_get_attach_widget(GTK_MENU(gtk_widget_get_toplevel(GTK_WIDGET(menuitem))));
  //GST_WARNING("3.) %p =? %p (%s)",self,parent,G_OBJECT_TYPE_NAME(parent));
  parent=gtk_menu_get_attach_widget(GTK_MENU(gtk_widget_get_parent(GTK_WIDGET(menuitem))));
  GST_WARNING("3.) %p =? %p (%s)",self,parent,G_OBJECT_TYPE_NAME(parent));
  parent=gtk_widget_get_parent(gtk_menu_get_attach_widget(GTK_MENU(gtk_widget_get_parent(GTK_WIDGET(menuitem)))));
  GST_WARNING("4.) %p =? %p (%s)",self,parent,G_OBJECT_TYPE_NAME(parent));
  parent=gtk_widget_get_parent(gtk_menu_get_attach_widget(GTK_MENU(
    gtk_widget_get_parent(gtk_menu_get_attach_widget(GTK_MENU(
    gtk_widget_get_parent(GTK_WIDGET(menuitem))))))));
  GST_WARNING("5.) %p =? %p (%s)",self,parent,G_OBJECT_TYPE_NAME(parent));
#endif

  g_object_set(self,"selected-control",control,NULL);
}

static void on_control_learn_activated(GtkMenuItem *menuitem, gpointer user_data) {
  //BtInteractionControllerMenu *self=BT_INTERACTION_CONTROLLER_MENU(g_object_get_qdata(G_OBJECT(menuitem),widget_parent_quark));
  BtIcDevice *device=BTIC_DEVICE(user_data);
  BtInteractionControllerLearnDialog *dialog;

  dialog=bt_interaction_controller_learn_dialog_new(device);
  gtk_widget_show_all(GTK_WIDGET(dialog));

  GST_INFO( "learn function activated on device");
}

//-- helper methods

static GtkWidget *bt_interaction_controller_menu_init_control_menu(const BtInteractionControllerMenu *self,BtIcDevice *device) {
  BtIcControl *control;
  GtkWidget *menu_item;
  GList *node,*list;
  gchar *str;
  GtkWidget *submenu=NULL;

  // add learn function entry for device which implement the BtIcLearn interface
  if(BTIC_IS_LEARN(device)) {
    submenu=gtk_menu_new();
    menu_item=gtk_image_menu_item_new_with_label(_("Learn..."));
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    g_object_set_qdata(G_OBJECT(menu_item),widget_parent_quark,(gpointer)self);
    gtk_widget_show(menu_item);
    // connect handler
    g_signal_connect(menu_item,"activate",G_CALLBACK(on_control_learn_activated),device);
  }

  // get list of controls per device
  g_object_get(device,"controls",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    control=BTIC_CONTROL(node->data);

    // filter by self->priv->type
    switch(self->priv->type) {
      case BT_INTERACTION_CONTROLLER_RANGE_MENU:
        if(!BTIC_IS_ABS_RANGE_CONTROL(control))
          continue;
        break;
      case BT_INTERACTION_CONTROLLER_TRIGGER_MENU:
        if(!BTIC_IS_TRIGGER_CONTROL(control))
          continue;
        break;
    }

    g_object_get(control,"name",&str,NULL);

    if(!submenu) {
      submenu=gtk_menu_new();
    }
    /* @todo: add icon: trigger=button, range=knob/slider (from glade?) */
    menu_item=gtk_image_menu_item_new_with_label(str);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    g_object_set_qdata(G_OBJECT(menu_item),widget_parent_quark,(gpointer)self);
    gtk_widget_show(menu_item);
    g_free(str);

    // connect handler
    g_signal_connect(menu_item,"activate",G_CALLBACK(on_control_bind_activated),(gpointer)control);
  }
  g_list_free(list);
  
  return(submenu);
}

static void bt_interaction_controller_menu_init_device_menu(const BtInteractionControllerMenu *self,GtkWidget *submenu) {
  BtIcRegistry *ic_registry;
  BtIcDevice *device;
  GtkWidget *menu_item,*parentmenu;
  GList *node,*list;
  gchar *str;

  // get list of interaction devices
  g_object_get(self->priv->app,"ic-registry",&ic_registry,NULL);
  g_object_get(ic_registry,"devices",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    device=BTIC_DEVICE(node->data);

    // only create items for non-empty submenus
    if((parentmenu=bt_interaction_controller_menu_init_control_menu(self,device))) {
      g_object_get(device,"name",&str,NULL);
  
      menu_item=gtk_image_menu_item_new_with_label(str);
      gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
      gtk_widget_show(menu_item);
      g_free(str);

      gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),parentmenu);
    }
  }
  g_list_free(list);
  g_object_unref(ic_registry);
}

static void bt_interaction_controller_menu_init_ui(const BtInteractionControllerMenu *self) {
  GtkWidget *menu_item,*submenu,*image;

  gtk_widget_set_name(GTK_WIDGET(self),"interaction controller menu");

  menu_item=gtk_image_menu_item_new_with_label(_("Bind controller"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);
  // add another submenu
  submenu=gtk_menu_new();
  gtk_widget_set_name(submenu,"interaction controller submenu");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),submenu);

  bt_interaction_controller_menu_init_device_menu(self,submenu);

  self->priv->item_unbind=menu_item=gtk_image_menu_item_new_with_label(_("Unbind controller"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);

  self->priv->item_unbind_all=menu_item=gtk_image_menu_item_new_with_label(_("Unbind all controllers"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);
}

//-- constructor methods

/**
 * bt_interaction_controller_menu_new:
 * @type: for which kind of controllers make a menu
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtInteractionControllerMenu *bt_interaction_controller_menu_new(BtInteractionControllerMenuType type) {
  BtInteractionControllerMenu *self;

  self=BT_INTERACTION_CONTROLLER_MENU(g_object_new(BT_TYPE_INTERACTION_CONTROLLER_MENU,"type",type,NULL));
  bt_interaction_controller_menu_init_ui(self);
  return(self);
}

//-- methods


//-- class internals

static void bt_interaction_controller_menu_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case INTERACTION_CONTROLLER_MENU_TYPE: {
      g_value_set_enum(value, self->priv->type);
    } break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL: {
      g_value_set_object(value, self->priv->selected_control);
    } break;
    case INTERACTION_CONTROLLER_MENU_ITEM_UNBIND: {
      g_value_set_object(value, self->priv->item_unbind);
    } break;
    case INTERACTION_CONTROLLER_MENU_ITEM_UNBIND_ALL: {
      g_value_set_object(value, self->priv->item_unbind_all);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_interaction_controller_menu_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case INTERACTION_CONTROLLER_MENU_TYPE: {
      self->priv->type=g_value_get_enum(value);
    } break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL: {
      g_object_try_unref(self->priv->selected_control);
      self->priv->selected_control = BTIC_CONTROL(g_value_get_object(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_interaction_controller_menu_dispose(GObject *object) {
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_unref(self->priv->selected_control);
  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(bt_interaction_controller_menu_parent_class)->dispose(object);
}

static void bt_interaction_controller_menu_init(BtInteractionControllerMenu *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_INTERACTION_CONTROLLER_MENU, BtInteractionControllerMenuPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();
}

static void bt_interaction_controller_menu_class_init(BtInteractionControllerMenuClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  widget_parent_quark=g_quark_from_static_string("BtInteractionControllerMenu::widget-parent");

  g_type_class_add_private(klass,sizeof(BtInteractionControllerMenuPrivate));

  gobject_class->set_property = bt_interaction_controller_menu_set_property;
  gobject_class->get_property = bt_interaction_controller_menu_get_property;
  gobject_class->dispose      = bt_interaction_controller_menu_dispose;

  g_object_class_install_property(gobject_class,INTERACTION_CONTROLLER_MENU_TYPE,
                                  g_param_spec_enum("type",
                                     "menu type construct prop",
                                     "control types to list in the menu",
                                     BT_TYPE_INTERACTION_CONTROLLER_MENU_TYPE,  /* enum type */
                                     BT_INTERACTION_CONTROLLER_RANGE_MENU, /* default value */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL,
                                  g_param_spec_object("selected-control",
                                     "selected control prop",
                                     "control after menu selection",
                                     BTIC_TYPE_CONTROL, /* object type */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,INTERACTION_CONTROLLER_MENU_ITEM_UNBIND,
                                  g_param_spec_object("item-unbind",
                                     "item unbind prop",
                                     "menu item for unbind command",
                                     GTK_TYPE_WIDGET, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,INTERACTION_CONTROLLER_MENU_ITEM_UNBIND_ALL,
                                  g_param_spec_object("item-unbind-all",
                                     "item unbind-all prop",
                                     "menu item for unbind-all command",
                                     GTK_TYPE_WIDGET, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

