// $Id: machine-menu.c,v 1.1 2005-09-03 13:40:30 ensonic Exp $
/**
 * SECTION:btmachinemenu
 * @short_description: class for the machine selection sub-menu
 */ 

#define BT_EDIT
#define BT_MACHINE_MENU_C

#include "bt-edit.h"

enum {
  MACHINE_MENU_APP=1,
};


struct _BtMachineMenuPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
  
  /* MenuItems */
  //GtkWidget *save_item;  
};

static GtkMenuClass *parent_class=NULL;

//-- event handler

static void on_source_machine_add_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMachineMenu *self=BT_MACHINE_MENU(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  gchar *name,*id;

  g_assert(user_data);
  name=(gchar *)gtk_widget_get_name(GTK_WIDGET(menuitem));
  GST_DEBUG("adding source machine \"%s\"",name);
  
  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"setup",&setup,NULL);
  
  id=bt_setup_get_unique_machine_id(setup,name);
  // @todo check voices
  if((machine=BT_MACHINE(bt_source_machine_new(song,id,name,1)))) {
    g_object_unref(machine);
  }
  g_free(id);
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

static void on_processor_machine_add_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMachineMenu *self=BT_MACHINE_MENU(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  gchar *name,*id;
  
  g_assert(user_data);
  name=(gchar *)gtk_widget_get_name(GTK_WIDGET(menuitem));
  GST_DEBUG("adding processor machine \"%s\"",name);
  
  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"setup",&setup,NULL);
  
  id=bt_setup_get_unique_machine_id(setup,name);
  // @todo check voices
  if((machine=BT_MACHINE(bt_processor_machine_new(song,id,name,1)))) {
    g_object_unref(machine);
  }
  g_free(id);
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_machine_menu_init_ui(const BtMachineMenu *self) {
  GtkWidget *menu_item,*submenu,*image;
  GList *node,*element_names;
  GstElementFactory *factory;
  
  gtk_widget_set_name(GTK_WIDGET(self),_("add menu"));

  menu_item=gtk_image_menu_item_new_with_label(_("Generators")); // red machine icon
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=bt_ui_ressources_get_image_by_machine_type(BT_TYPE_SOURCE_MACHINE);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);
  // add another submenu
  submenu=gtk_menu_new();
  gtk_widget_set_name(submenu,_("generators menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),submenu);

  // scan registered sources
  element_names=bt_gst_registry_get_element_names_by_class("Source/Audio");
  for(node=element_names;node;node=g_list_next(node)) {
    GST_INFO("found source element : '%s'",node->data);
    factory=gst_element_factory_find(node->data);

    menu_item=gtk_menu_item_new_with_label(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
    gtk_widget_set_name(menu_item,node->data);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_source_machine_add_activated),(gpointer)self);
  }
  g_list_free(element_names);  

  menu_item=gtk_image_menu_item_new_with_label(_("Effects")); // green machine icon
  gtk_menu_shell_append(GTK_MENU_SHELL(self),menu_item);
  image=bt_ui_ressources_get_image_by_machine_type(BT_TYPE_PROCESSOR_MACHINE);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);
  // add another submenu
  submenu=gtk_menu_new();
  gtk_widget_set_name(submenu,_("effects menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),submenu);

  // scan registered processors
  element_names=bt_gst_registry_get_element_names_by_class("Filter/Effect/Audio");
  for(node=element_names;node;node=g_list_next(node)) {
    GST_INFO("found processor element : '%s'",node->data);
    factory=gst_element_factory_find(node->data);

    menu_item=gtk_menu_item_new_with_label(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
    gtk_widget_set_name(menu_item,node->data);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    gtk_widget_show(menu_item);
    g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_processor_machine_add_activated),(gpointer)self);
  }
  g_list_free(element_names);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_menu_new:
 * @app: the application the menu belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMachineMenu *bt_machine_menu_new(const BtEditApplication *app) {
  BtMachineMenu *self;

  if(!(self=BT_MACHINE_MENU(g_object_new(BT_TYPE_MACHINE_MENU,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_machine_menu_init_ui(self)) {
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
static void bt_machine_menu_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachineMenu *self = BT_MACHINE_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_MENU_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_machine_menu_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachineMenu *self = BT_MACHINE_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_MENU_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for machine_menu: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_menu_dispose(GObject *object) {
  BtMachineMenu *self = BT_MACHINE_MENU(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);  
  g_object_try_weak_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_menu_finalize(GObject *object) {
  BtMachineMenu *self = BT_MACHINE_MENU(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_menu_init(GTypeInstance *instance, gpointer g_class) {
  BtMachineMenu *self = BT_MACHINE_MENU(instance);
  self->priv = g_new0(BtMachineMenuPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_machine_menu_class_init(BtMachineMenuClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_MENU);
  
  gobject_class->set_property = bt_machine_menu_set_property;
  gobject_class->get_property = bt_machine_menu_get_property;
  gobject_class->dispose      = bt_machine_menu_dispose;
  gobject_class->finalize     = bt_machine_menu_finalize;

  g_object_class_install_property(gobject_class,MACHINE_MENU_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_machine_menu_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMachineMenuClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_menu_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMachineMenu),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_machine_menu_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_MENU,"BtMachineMenu",&info,0);
  }
  return type;
}
