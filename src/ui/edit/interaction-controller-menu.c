/* $Id: interaction-controller-menu.c,v 1.2 2007-03-25 14:18:31 ensonic Exp $
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

enum {
  INTERACTION_CONTROLLER_MENU_APP=1,
  INTERACTION_CONTROLLER_MENU_TYPE
};


struct _BtInteractionControllerMenuPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* MenuItems */
  //GtkWidget *save_item;

  BtInteractionControllerMenuType type;
};

static GtkMenuClass *parent_class=NULL;

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


#if 0
static void on_controller_bind_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtInteractionControllerMenu *self=BT_INTERACTION_CONTROLLER_MENU(user_data);

  /* need controller-device + controller name
   * on_controller_notify() needs target object,property
   */
  g_signal_connect(G_OBJECT(ic_device),"notify::controller",G_CALLBACK(on_controller_notify),(gpointer)self);
}
#endif

//-- helper methods

static void bt_interaction_controller_menu_init_submenu(const BtInteractionControllerMenu *self,GtkWidget *submenu) {
  BtIcRegistry *ic_registry;
  BtIcDevice *device=NULL;
  GtkWidget *menu_item,*parentmenu;
  GList *node,*list;
  gchar *str;

  // get list of interaction devices
  g_object_get(self->priv->app,"ic-registry",&ic_registry,NULL);
  g_object_get(ic_registry,"devices",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    device=BTIC_DEVICE(node->data);
    g_object_get(G_OBJECT(device),"name",&str,NULL);
    
    menu_item=gtk_image_menu_item_new_with_label(str);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    gtk_widget_show(menu_item);
    g_free(str);
    
    parentmenu=gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),parentmenu);
    // @todo: register controllers as submenu
    //g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_controller_bind_activated),(gpointer)self);
    
  }
  g_list_free(list);
}

static gboolean bt_interaction_controller_menu_init_ui(const BtInteractionControllerMenu *self) {
  GtkWidget *menu_item,*submenu,*image;

  gtk_widget_set_name(GTK_WIDGET(self),_("interaction controller menu"));

  // generators
  menu_item=gtk_image_menu_item_new_with_label(_("Bind controller"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);
  // add another submenu
  submenu=gtk_menu_new();
  gtk_widget_set_name(submenu,_("interaction controllers menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),submenu);

  bt_interaction_controller_menu_init_submenu(self,submenu);

  // effects
  menu_item=gtk_image_menu_item_new_with_label(_("Unbind all controllers"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_interaction_controller_menu_new:
 * @app: the application the menu belongs to
 * @type: for which kind of controllers make a menu
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtInteractionControllerMenu *bt_interaction_controller_menu_new(const BtEditApplication *app,BtInteractionControllerMenuType type) {
  BtInteractionControllerMenu *self;

  if(!(self=BT_INTERACTION_CONTROLLER_MENU(g_object_new(BT_TYPE_INTERACTION_CONTROLLER_MENU,"app",app,"type",type,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_interaction_controller_menu_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_interaction_controller_menu_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case INTERACTION_CONTROLLER_MENU_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case INTERACTION_CONTROLLER_MENU_TYPE: {
      g_value_set_enum(value, self->priv->type);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_interaction_controller_menu_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case INTERACTION_CONTROLLER_MENU_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for interaction_controller_menu: %p",self->priv->app);
    } break;
    case INTERACTION_CONTROLLER_MENU_TYPE: {
      self->priv->type=g_value_get_enum(value);
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
  g_object_try_weak_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_interaction_controller_menu_finalize(GObject *object) {
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(object);

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_interaction_controller_menu_init(GTypeInstance *instance, gpointer g_class) {
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_INTERACTION_CONTROLLER_MENU, BtInteractionControllerMenuPrivate);
}

static void bt_interaction_controller_menu_class_init(BtInteractionControllerMenuClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtInteractionControllerMenuPrivate));

  gobject_class->set_property = bt_interaction_controller_menu_set_property;
  gobject_class->get_property = bt_interaction_controller_menu_get_property;
  gobject_class->dispose      = bt_interaction_controller_menu_dispose;
  gobject_class->finalize     = bt_interaction_controller_menu_finalize;

  g_object_class_install_property(gobject_class,INTERACTION_CONTROLLER_MENU_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,INTERACTION_CONTROLLER_MENU_TYPE,
                                  g_param_spec_enum("type",
                                     "type prop",
                                     "controller types to list",
                                     BT_TYPE_INTERACTION_CONTROLLER_MENU_TYPE,  /* enum type */
                                     BT_INTERACTION_CONTROLLER_RANGE_MENU, /* default value */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
}

GType bt_interaction_controller_menu_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtInteractionControllerMenuClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_interaction_controller_menu_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtInteractionControllerMenu),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_interaction_controller_menu_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_MENU,"BtInteractionControllerMenu",&info,0);
  }
  return type;
}
