/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btinteractioncontrollermenu
 * @short_description: class for the interaction controller assignment popup menu
 *
 * Build a menu with available interaction controllers of a type to be used for a
 * specified machine.
 *
 * The menu will show whether a control is bound or not and if it is bound the label
 * will include the currently bound target.
 */

#define BT_EDIT
#define BT_INTERACTION_CONTROLLER_MENU_C

#include "bt-edit.h"

//-- property ids

enum
{
  INTERACTION_CONTROLLER_MENU_TYPE = 1,
  INTERACTION_CONTROLLER_MENU_MACHINE,
  INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL,
  INTERACTION_CONTROLLER_MENU_SELECTED_OBJECT,
  INTERACTION_CONTROLLER_MENU_SELECTED_PARAMETER_GROUP,
  INTERACTION_CONTROLLER_MENU_SELECTED_PROPERTY_NAME
};


struct _BtInteractionControllerMenuPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the properties */
  BtInteractionControllerMenuType type;
  BtMachine *machine;

  /* the context for the current menu invocation */
  BtIcControl *selected_control;
  GstObject *selected_object;
  BtParameterGroup *selected_pg;
  gchar *selected_property_name;

  /* sub menu (needed to update the menu) */
  GtkWidget *device_menu;

  /* actions */
  GtkWidget *item_unbind, *item_unbind_all;

  /* signal handler id */
  gulong ic_changed_handler_id;

  /* context for the menu items */
  GHashTable *menuitem_to_control;
  GHashTable *menuitem_to_device;
  GHashTable *control_to_menuitem;
};

extern GQuark bt_machine_machine;
extern GQuark bt_machine_property_name;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtInteractionControllerMenu, bt_interaction_controller_menu,
    GTK_TYPE_MENU, 
    G_ADD_PRIVATE(BtInteractionControllerMenu));

//-- prototypes

static void bt_interaction_controller_menu_init_device_menu (const
    BtInteractionControllerMenu * self);

//-- enums

GType
bt_interaction_controller_menu_type_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_INTERACTION_CONTROLLER_RANGE_MENU,
          "BT_INTERACTION_CONTROLLER_RANGE_MENU", "range controllers"},
      {BT_INTERACTION_CONTROLLER_TRIGGER_MENU,
          "BT_INTERACTION_CONTROLLER_TRIGGER_MENU", "trigger controllers"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtInteractionControllerMenuType", values);
  }
  return type;
}

//-- helper

static gchar *
build_label_for_control (BtIcControl * const control)
{
  gchar *cname, *desc;
  gboolean is_bound;

  g_object_get (control, "name", &cname, "bound", &is_bound, NULL);

  // build label
  if (is_bound) {
    BtMachine *machine = g_object_get_qdata ((GObject *) control,
        bt_machine_machine);
    gchar *pname = g_object_get_qdata ((GObject *) control,
        bt_machine_property_name);
    gchar *mid;

    g_object_get (machine, "id", &mid, NULL);
    desc = g_strdup_printf ("%s (%s:%s)", cname, mid, pname);
    g_free (mid);
    g_free (cname);
  } else {
    desc = cname;
  }
  return desc;
}

//-- event handler

static void
on_devices_changed (BtIcRegistry * const registry, const GParamSpec * const arg,
    gconstpointer const user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);

  GST_INFO ("devices changed");

  bt_interaction_controller_menu_init_device_menu (self);
}

static void
on_controls_changed (BtIcDevice * const device, const GParamSpec * const arg,
    gconstpointer const user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);

  GST_INFO ("controls changed");

  bt_interaction_controller_menu_init_device_menu (self);
}

static void
on_machine_notify_id (BtMachine * const machine,
    const GParamSpec * const arg, gpointer user_data)
{
  // as we just forward the signal, it is safe to leave some of these connected
  // we take care to not connect them twice
  g_object_notify (user_data, "bound");
}

static void
on_control_notify_bound (BtIcControl * const control,
    const GParamSpec * const arg, gpointer user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);
  GObject *menuitem =
      G_OBJECT (g_hash_table_lookup (self->priv->control_to_menuitem,
          (gpointer) control));
  BtMachine *machine;
  gchar *desc = build_label_for_control (control);

  g_object_set (menuitem, "label", desc, NULL);
  g_free (desc);

  machine = g_object_get_qdata ((GObject *) control, bt_machine_machine);
  if (machine) {
    g_signal_connect_object (machine, "notify::id",
        G_CALLBACK (on_machine_notify_id), (gpointer) control, 0);
  }
}

static void
on_control_bind (GtkMenuItem * menuitem, gpointer user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);
  BtIcControl *control =
      BTIC_CONTROL (g_hash_table_lookup (self->priv->menuitem_to_control,
          (gpointer) menuitem));

  if (bt_machine_is_polyphonic (self->priv->machine) &&
      !GST_IS_CHILD_PROXY (self->priv->selected_object)) {
    bt_machine_bind_poly_parameter_control (self->priv->machine,
        self->priv->selected_property_name, control, self->priv->selected_pg);
  } else {
    bt_machine_bind_parameter_control (self->priv->machine,
        self->priv->selected_object, self->priv->selected_property_name,
        control, self->priv->selected_pg);
  }
}

static void
on_control_learn (GtkMenuItem * menuitem, gpointer user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);
  BtIcDevice *device =
      BTIC_DEVICE (g_hash_table_lookup (self->priv->menuitem_to_device,
          (gpointer) menuitem));
  BtSettingsDialog *dialog;

  if ((dialog = bt_settings_dialog_new ())) {
    GtkWidget *page;

    bt_edit_application_attach_child_window (self->priv->app,
        GTK_WINDOW (dialog));
    g_object_set (dialog, "page", BT_SETTINGS_PAGE_INTERACTION_CONTROLLER,
        NULL);
    g_object_get (dialog, "page-widget", &page, NULL);
    g_object_set (page, "device", device, NULL);
    gtk_widget_show_all (GTK_WIDGET (dialog));
    switch (gtk_dialog_run (GTK_DIALOG (dialog))) {
      case GTK_RESPONSE_ACCEPT:{
        // assign last selected control
        BtIcControl *control;

        g_object_get (page, "control", &control, NULL);
        if (control) {
          GtkMenuItem *menuitem =
              g_hash_table_lookup (self->priv->control_to_menuitem,
              (gpointer) control);
          on_control_bind (menuitem, self);
          g_object_unref (control);
        }
        break;
      }
      default:
        break;
    }
    g_object_unref (page);
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }
  GST_INFO ("learn function activated on device");
}

static void
on_control_unbind (GtkMenuItem * menuitem, gpointer user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);

  bt_machine_unbind_parameter_control (self->priv->machine,
      self->priv->selected_object, self->priv->selected_property_name);
}

static void
on_control_unbind_all (GtkMenuItem * menuitem, gpointer user_data)
{
  BtInteractionControllerMenu *self =
      BT_INTERACTION_CONTROLLER_MENU (user_data);

  bt_machine_unbind_parameter_controls (self->priv->machine);
}

//-- helper methods

static GtkWidget *
bt_interaction_controller_menu_init_control_menu (const
    BtInteractionControllerMenu * self, BtIcDevice * device)
{
  BtIcControl *control;
  GtkWidget *menu_item;
  BtMachine *machine;
  GList *node, *list;
  gchar *str, *desc;
  GtkWidget *submenu = NULL;
  gboolean is_bound;

  // add learn function entry for device which implement the BtIcLearn interface
  if (BTIC_IS_LEARN (device)) {
    submenu = gtk_menu_new ();
    menu_item = gtk_menu_item_new_with_label (_("Learn…"));
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
    gtk_widget_show (menu_item);
    // connect handler
    g_hash_table_insert (self->priv->menuitem_to_device, menu_item, device);
    g_signal_connect (menu_item, "activate",
        G_CALLBACK (on_control_learn), (gpointer) self);
  }
  // get list of controls per device
  g_object_get (device, "controls", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    control = BTIC_CONTROL (node->data);

    // filter by self->priv->type
    switch (self->priv->type) {
      case BT_INTERACTION_CONTROLLER_RANGE_MENU:
        if (!BTIC_IS_ABS_RANGE_CONTROL (control))
          continue;
        break;
      case BT_INTERACTION_CONTROLLER_TRIGGER_MENU:
        if (!BTIC_IS_TRIGGER_CONTROL (control))
          continue;
        break;
    }

    g_object_get (control, "name", &str, "bound", &is_bound, NULL);
    GST_INFO ("  Add control '%s'", str);

    if (!submenu) {
      submenu = gtk_menu_new ();
    }
    desc = build_label_for_control (control);
    menu_item = gtk_menu_item_new_with_label (desc);
    gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
    gtk_widget_show (menu_item);
    g_free (desc);

    // connect handlers
    g_hash_table_insert (self->priv->menuitem_to_control, menu_item, control);
    g_hash_table_insert (self->priv->control_to_menuitem, control, menu_item);
    g_signal_connect (menu_item, "activate", G_CALLBACK (on_control_bind),
        (gpointer) self);
    g_signal_connect_object (control, "notify::bound",
        G_CALLBACK (on_control_notify_bound), (gpointer) self, 0);

    machine = g_object_get_qdata ((GObject *) control, bt_machine_machine);
    if (machine) {
      if (!g_signal_handler_find (machine,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_machine_notify_id, (gpointer) control)) {
        g_signal_connect_object (machine, "notify::id",
            G_CALLBACK (on_machine_notify_id), (gpointer) control, 0);
      }
    }
  }
  g_list_free (list);

  return submenu;
}

static void
bt_interaction_controller_menu_init_device_menu (const
    BtInteractionControllerMenu * self)
{
  BtIcRegistry *ic_registry;
  GList *list;

  // get list of interaction devices
  g_object_get (self->priv->app, "ic-registry", &ic_registry, NULL);
  g_object_get (ic_registry, "devices", &list, NULL);
  if (!self->priv->ic_changed_handler_id) {
    self->priv->ic_changed_handler_id =
        g_signal_connect_object (ic_registry, "notify::devices",
        G_CALLBACK (on_devices_changed), (gpointer) self, 0);
  }
  g_object_unref (ic_registry);
  if (list) {
    GtkWidget *menu_item, *submenu, *parentmenu;
    BtIcDevice *device;
    GList *node;
    gchar *str;

    submenu = gtk_menu_new ();
    gtk_widget_set_name (submenu, "interaction controller submenu");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (self->priv->device_menu),
        submenu);
    gtk_widget_set_sensitive (self->priv->device_menu, TRUE);

    for (node = list; node; node = g_list_next (node)) {
      device = BTIC_DEVICE (node->data);
      if (!g_signal_handler_find (device,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_controls_changed, (gpointer) self)) {
        g_signal_connect_object (device, "notify::controls",
            G_CALLBACK (on_controls_changed), (gpointer) self, 0);
      }
      // only create items for non-empty submenus
      if ((parentmenu =
              bt_interaction_controller_menu_init_control_menu (self,
                  device))) {
        g_object_get (device, "name", &str, NULL);
        GST_INFO ("Build control menu for '%s'", str);

        menu_item = gtk_menu_item_new_with_label (str);
        gtk_menu_shell_append (GTK_MENU_SHELL (submenu), menu_item);
        gtk_widget_show (menu_item);
        g_free (str);

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), parentmenu);
      }
    }
    g_list_free (list);
  } else {
    gtk_widget_set_sensitive (self->priv->device_menu, FALSE);
  }
}

static void
bt_interaction_controller_menu_init_ui (const BtInteractionControllerMenu *
    self)
{
  BtInteractionControllerMenuPrivate *p = self->priv;
  GtkWidget *item;

  gtk_widget_set_name (GTK_WIDGET (self), "interaction controller menu");

  p->device_menu = item = gtk_menu_item_new_with_label (_("Bind controller"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self), item);
  gtk_widget_show (item);

  // add the device submenu
  bt_interaction_controller_menu_init_device_menu (self);

  p->item_unbind = item = gtk_menu_item_new_with_label (_("Unbind controller"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self), item);
  g_signal_connect (item, "activate", G_CALLBACK (on_control_unbind),
      (gpointer) self);
  gtk_widget_show (item);

  p->item_unbind_all = item =
      gtk_menu_item_new_with_label (_("Unbind all controllers"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self), item);
  g_signal_connect (item, "activate", G_CALLBACK (on_control_unbind_all),
      (gpointer) self);
  gtk_widget_show (item);
}

//-- constructor methods

/**
 * bt_interaction_controller_menu_new:
 * @type: for which kind of controllers make a menu
 * @machine: to which machine we want to bind controllers
 *
 * Create a new instance.
 *
 * Returns: the new instance
 */
BtInteractionControllerMenu *
bt_interaction_controller_menu_new (BtInteractionControllerMenuType type,
    BtMachine * machine)
{
  BtInteractionControllerMenu *self;

  self =
      BT_INTERACTION_CONTROLLER_MENU (g_object_new
      (BT_TYPE_INTERACTION_CONTROLLER_MENU, "type", type, "machine", machine,
          NULL));
  bt_interaction_controller_menu_init_ui (self);
  return self;
}

//-- methods

//-- class internals

static void
bt_interaction_controller_menu_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU (object);
  return_if_disposed ();
  switch (property_id) {
    case INTERACTION_CONTROLLER_MENU_TYPE:
      g_value_set_enum (value, self->priv->type);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL:
      g_value_set_object (value, self->priv->selected_control);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_OBJECT:
      g_value_set_object (value, self->priv->selected_object);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_PARAMETER_GROUP:
      g_value_set_object (value, self->priv->selected_pg);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_PROPERTY_NAME:
      g_value_set_string (value, self->priv->selected_property_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_interaction_controller_menu_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU (object);
  return_if_disposed ();
  switch (property_id) {
    case INTERACTION_CONTROLLER_MENU_TYPE:
      self->priv->type = g_value_get_enum (value);
      break;
    case INTERACTION_CONTROLLER_MENU_MACHINE:
      self->priv->machine = g_value_dup_object (value);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL:
      g_object_try_unref (self->priv->selected_control);
      self->priv->selected_control = BTIC_CONTROL (g_value_dup_object (value));
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_OBJECT:
      g_object_try_unref (self->priv->selected_object);
      self->priv->selected_object = g_value_dup_object (value);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_PARAMETER_GROUP:
      g_object_try_unref (self->priv->selected_pg);
      self->priv->selected_pg = g_value_dup_object (value);
      break;
    case INTERACTION_CONTROLLER_MENU_SELECTED_PROPERTY_NAME:
      g_free (self->priv->selected_property_name);
      self->priv->selected_property_name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_interaction_controller_menu_dispose (GObject * object)
{
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->selected_control);
  g_object_try_unref (self->priv->selected_object);
  g_object_try_unref (self->priv->selected_pg);
  g_object_try_unref (self->priv->machine);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_interaction_controller_menu_parent_class)->dispose
      (object);
}

static void
bt_interaction_controller_menu_finalize (GObject * object)
{
  BtInteractionControllerMenu *self = BT_INTERACTION_CONTROLLER_MENU (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->selected_property_name);
  g_hash_table_destroy (self->priv->menuitem_to_control);
  g_hash_table_destroy (self->priv->menuitem_to_device);
  g_hash_table_destroy (self->priv->control_to_menuitem);

  G_OBJECT_CLASS (bt_interaction_controller_menu_parent_class)->finalize
      (object);
}

static void
bt_interaction_controller_menu_init (BtInteractionControllerMenu * self)
{
  self->priv = bt_interaction_controller_menu_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
  self->priv->menuitem_to_control = g_hash_table_new (NULL, NULL);
  self->priv->menuitem_to_device = g_hash_table_new (NULL, NULL);
  self->priv->control_to_menuitem = g_hash_table_new (NULL, NULL);
}

static void
bt_interaction_controller_menu_class_init (BtInteractionControllerMenuClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_interaction_controller_menu_set_property;
  gobject_class->get_property = bt_interaction_controller_menu_get_property;
  gobject_class->dispose = bt_interaction_controller_menu_dispose;
  gobject_class->finalize = bt_interaction_controller_menu_finalize;

  g_object_class_install_property (gobject_class,
      INTERACTION_CONTROLLER_MENU_TYPE, g_param_spec_enum ("type",
          "menu type construct prop", "control types to list in the menu",
          BT_TYPE_INTERACTION_CONTROLLER_MENU_TYPE,
          BT_INTERACTION_CONTROLLER_RANGE_MENU,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      INTERACTION_CONTROLLER_MENU_MACHINE, g_param_spec_object ("machine",
          "machine construct prop", "Set machine object, the menu handles",
          BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      INTERACTION_CONTROLLER_MENU_SELECTED_CONTROL,
      g_param_spec_object ("selected-control", "selected control prop",
          "control after menu selection", BTIC_TYPE_CONTROL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      INTERACTION_CONTROLLER_MENU_SELECTED_OBJECT,
      g_param_spec_object ("selected-object", "selected object prop",
          "object the menu is invoked on", GST_TYPE_OBJECT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      INTERACTION_CONTROLLER_MENU_SELECTED_PARAMETER_GROUP,
      g_param_spec_object ("selected-parameter-group",
          "selected parameter-group prop",
          "object-parameter-group the menu is invoked on",
          BT_TYPE_PARAMETER_GROUP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      INTERACTION_CONTROLLER_MENU_SELECTED_PROPERTY_NAME,
      g_param_spec_string ("selected-property-name",
          "selected property-name prop",
          "object-property-name the menu is invoked on", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
