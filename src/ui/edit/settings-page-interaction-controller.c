/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btsettingspageinteractioncontroller
 * @short_description: interaction controller configuration settings page
 *
 * Lists available interaction controller devices and allows to select
 * controllers that should be used.
 */
/* TODO(ensonic): more information
 * - show the type as icon
 * - show the live values for each control
 *   - while all concrete controls have a value-property, they have a different
 *     type (trigger: boolean, abs-range: long)
 *     - we could add a value-str property to BtIcControl and a describe()
 *       vmethod in subclasses
 */
/* TODO(ensonic): configure things:
 * - allow to hide devices
 * - allow to limit the value range (e.g. for the accelerometer)
 */
#define BT_EDIT
#define BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_C

#include "bt-edit.h"

enum
{
  PROP_DEVICE = 1,
  PROP_CONTROL
};

enum
{
  DEVICE_MENU_LABEL = 0
};

enum
{
  CONTROLLER_LIST_USED = 0,
  CONTROLLER_LIST_LABEL
};

struct _BtSettingsPageInteractionControllerPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  GtkComboBox *device_menu;
  GtkTreeView *controller_list;
  GtkLabel *message;
  GtkCellRenderer *id_renderer;

  /* the active device or NULL */
  BtIcDevice *device;
  /* the last selected control */
  BtIcControl *control;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSettingsPageInteractionController,
    bt_settings_page_interaction_controller,
    GTK_TYPE_GRID, 
    G_ADD_PRIVATE(BtSettingsPageInteractionController));

//-- helper

static void start_device (BtSettingsPageInteractionController * self);
static void stop_device (BtSettingsPageInteractionController * self);

static gint
get_device_pos (BtIcRegistry * registry, BtIcDevice * device)
{
  GList *list;

  g_object_get (registry, "devices", &list, NULL);
  return g_list_index (list, device);
}

static gint
get_control_pos (BtIcDevice * device, BtIcControl * control)
{
  GList *list;

  g_object_get (device, "controls", &list, NULL);
  return g_list_index (list, control);
}

//-- event handler

static void
notify_controlchange (BtIcControl * control, GParamSpec * arg,
    gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  gint pos = get_control_pos (self->priv->device, control);
  GtkTreePath *path = gtk_tree_path_new_from_indices (pos, -1);

  // select the control
  gtk_widget_grab_focus (GTK_WIDGET (self->priv->controller_list));
  gtk_tree_view_set_cursor (self->priv->controller_list, path,
      gtk_tree_view_get_column (self->priv->controller_list,
          CONTROLLER_LIST_LABEL), TRUE);
  gtk_tree_path_free (path);
}

static void
notify_device_controlchange (BtIcLearn * learn, GParamSpec * arg,
    gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  gchar *id;
  BtIcControl *control;

  g_object_get (learn, "device-controlchange", &id, NULL);
  control = btic_learn_register_learned_control (learn, id);
  g_free (id);
  if (control) {
    gint pos = get_control_pos (self->priv->device, control);
    GtkTreePath *path = gtk_tree_path_new_from_indices (pos, -1);

    // add the new control to the list
    bt_object_list_model_insert (BT_OBJECT_LIST_MODEL (gtk_tree_view_get_model
            (self->priv->controller_list)), (GObject *) control, pos);
    // select the control
    gtk_widget_grab_focus (GTK_WIDGET (self->priv->controller_list));
    gtk_tree_view_set_cursor (self->priv->controller_list, path,
        gtk_tree_view_get_column (self->priv->controller_list,
            CONTROLLER_LIST_LABEL), TRUE);
    gtk_tree_path_free (path);

    g_signal_connect_object (control, "notify::value",
        G_CALLBACK (notify_controlchange), (gpointer) self, 0);
  }
}

static void
on_device_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  GObject *device = NULL;
  BtObjectListModel *store;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GList *node, *list;

  // release the old one
  if (self->priv->device) {
    stop_device (self);
  }

  GST_INFO ("interaction controller device changed");
  model = gtk_combo_box_get_model (self->priv->device_menu);
  if (gtk_combo_box_get_active_iter (self->priv->device_menu, &iter)) {
    device =
        bt_object_list_model_get_object (BT_OBJECT_LIST_MODEL (model), &iter);
  }
  // update list of controllers
  store = bt_object_list_model_new (2, BTIC_TYPE_CONTROL, "bound", "name");
  if (device) {
    g_object_get (device, "controls", &list, NULL);
    for (node = list; node; node = g_list_next (node)) {
      bt_object_list_model_append (store, (GObject *) node->data);
      g_signal_connect_object (node->data, "notify::value",
          G_CALLBACK (notify_controlchange), (gpointer) self, 0);
    }
    g_list_free (list);

    // activate the new one
    self->priv->device = g_object_ref ((gpointer) device);
    start_device (self);
  }
  GST_INFO ("control list refreshed");
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->controller_list),
      (device != NULL));
  gtk_tree_view_set_model (self->priv->controller_list, GTK_TREE_MODEL (store));
  g_object_unref (store);       // drop with treeview
}

static void
on_ic_registry_devices_changed (BtIcRegistry * ic_registry, GParamSpec * arg,
    gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  GList *node, *list;
  BtObjectListModel *store;
  gint index = -1, i;

  GST_INFO ("refreshing device menu");

  store = bt_object_list_model_new (1, BTIC_TYPE_DEVICE, "name");
  g_object_get (ic_registry, "devices", &list, NULL);
  for (node = list, i = 0; node; node = g_list_next (node), i++) {
    bt_object_list_model_append (store, (GObject *) node->data);
    if (self->priv->device) {
      if (node->data == self->priv->device) {
        index = i;
      }
    } else {
      index = 0;
    }
  }

  GST_INFO ("device menu refreshed: index = %d", index);
  gtk_label_set_text (self->priv->message, (list == NULL) ? NULL :
      _("Plug a USB input device or midi controller"));
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->device_menu),
      (list != NULL));
  gtk_combo_box_set_model (self->priv->device_menu, GTK_TREE_MODEL (store));
  gtk_combo_box_set_active (self->priv->device_menu, index);
  g_list_free (list);
  g_object_unref (store);       // drop with comboxbox
}

static void
on_control_name_edited (GtkCellRendererText * cellrenderertext,
    gchar * path_string, gchar * new_text, gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  GtkTreeModel *store;

  if ((store = gtk_tree_view_get_model (self->priv->controller_list))) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_from_string (store, &iter, path_string)) {
      GObject *control;

      if ((control =
              bt_object_list_model_get_object ((BtObjectListModel *) store,
                  &iter))) {
        GtkTreePath *path;
        gint pos;

        g_object_set (control, "name", new_text, NULL);
        // resort the list
        on_device_menu_changed (self->priv->device_menu, self);
        // select control again
        pos = get_control_pos (self->priv->device, (BtIcControl *) control);
        path = gtk_tree_path_new_from_indices (pos, -1);
        gtk_widget_grab_focus (GTK_WIDGET (self->priv->controller_list));
        gtk_tree_view_set_cursor (self->priv->controller_list, path,
            gtk_tree_view_get_column (self->priv->controller_list,
                CONTROLLER_LIST_LABEL), TRUE);
        gtk_tree_path_free (path);
      }
    }
  }
}

static void
on_control_selected (GtkTreeSelection * tree_sel, gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (tree_sel, NULL, &iter)) {
    GtkTreeModel *store;

    if ((store = gtk_tree_view_get_model (self->priv->controller_list))) {
      GObject *control;

      if ((control =
              bt_object_list_model_get_object ((BtObjectListModel *) store,
                  &iter))) {
        self->priv->control = (BtIcControl *) control;
        g_object_notify ((GObject *) self, "control");
      }
    }
  }
}

static void
on_page_switched (GtkNotebook * notebook, GParamSpec * arg, gpointer user_data)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (user_data);
  guint page_num;
  static gint prev_page_num = -1;

  g_object_get (notebook, "page", &page_num, NULL);

  if (page_num == BT_SETTINGS_PAGE_INTERACTION_CONTROLLER) {
    // only do this if the page really has changed
    if (prev_page_num != BT_SETTINGS_PAGE_INTERACTION_CONTROLLER) {
      GST_DEBUG ("enter page");
      on_device_menu_changed (self->priv->device_menu, (gpointer) self);
    }
  } else {
    // only do this if the page was BT_SETTINGS_PAGE_INTERACTION_CONTROLLER
    if (prev_page_num == BT_SETTINGS_PAGE_INTERACTION_CONTROLLER) {
      GST_DEBUG ("leave page");
      if (self->priv->device) {
        stop_device (self);
      }
    }
  }
  prev_page_num = page_num;
}

//-- helper methods

static void
select_device (BtSettingsPageInteractionController * self, BtIcDevice * device)
{
  BtIcRegistry *ic_registry;
  gint index;

  if (self->priv->device == device) {
    return;
  }
  // update combo selection
  // on_device_menu_changed() will store the device -> self->priv->device
  g_object_get (self->priv->app, "ic-registry", &ic_registry, NULL);
  index = get_device_pos (ic_registry, device);
  gtk_combo_box_set_active (self->priv->device_menu, index);
  g_object_unref (ic_registry);
}

static void
start_device (BtSettingsPageInteractionController * self)
{
  if (BTIC_IS_LEARN (self->priv->device)) {
    g_signal_connect_object (self->priv->device, "notify::device-controlchange",
        G_CALLBACK (notify_device_controlchange), (gpointer) self, 0);
    btic_learn_start (BTIC_LEARN (self->priv->device));
    gtk_label_set_text (self->priv->message,
        _("Use the device's controls to train them."));
    g_object_set (self->priv->id_renderer,
        "mode", GTK_CELL_RENDERER_MODE_EDITABLE, "editable", TRUE, NULL);
  } else {
    btic_device_start (self->priv->device);
    gtk_label_set_text (self->priv->message, NULL);
    g_object_set (self->priv->id_renderer,
        "mode", GTK_CELL_RENDERER_MODE_INERT, "editable", FALSE, NULL);
  }
}

static void
stop_device (BtSettingsPageInteractionController * self)
{
  if (BTIC_IS_LEARN (self->priv->device)) {
    btic_learn_stop (BTIC_LEARN (self->priv->device));
    g_signal_handlers_disconnect_by_func (self->priv->device,
        notify_device_controlchange, self);
  } else {
    btic_device_stop (self->priv->device);
  }

  g_object_unref (self->priv->device);
  self->priv->device = NULL;
}

static void
bt_settings_page_interaction_controller_init_ui (const
    BtSettingsPageInteractionController * self, GtkWidget * pages)
{
  GtkWidget *label, *widget, *scrolled_window;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  GtkTreeSelection *tree_sel;
  BtIcRegistry *ic_registry;
  gchar *str;

  gtk_widget_set_name (GTK_WIDGET (self), "interaction controller settings");

  // create the widget already so that we can set the initial text
  self->priv->message = GTK_LABEL (gtk_label_new (NULL));
  g_object_set (GTK_WIDGET (self->priv->message), "hexpand", TRUE, NULL);

  // add setting widgets
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Interaction Controller"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 0, 0, 3, 1);
  gtk_grid_attach (GTK_GRID (self), gtk_label_new ("    "), 0, 1, 1, 3);

  label = gtk_label_new (_("Device"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 1, 1, 1, 1);

  widget = gtk_combo_box_new ();
  self->priv->device_menu = GTK_COMBO_BOX (widget);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->priv->device_menu),
      renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->priv->device_menu),
      renderer, "text", DEVICE_MENU_LABEL, NULL);

  // get list of devices from libbtic and listen to changes
  g_object_get (self->priv->app, "ic-registry", &ic_registry, NULL);
  g_signal_connect_object (ic_registry, "notify::devices",
      G_CALLBACK (on_ic_registry_devices_changed), (gpointer) self, 0);
  g_object_unref (ic_registry);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (self), widget, 2, 1, 1, 1);
  g_signal_connect (widget, "changed",
      G_CALLBACK (on_device_menu_changed), (gpointer) self);

  // add list of controllers (updated when selecting a device)
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_ETCHED_IN);
  self->priv->controller_list = GTK_TREE_VIEW (gtk_tree_view_new ());
  g_object_set (self->priv->controller_list,
      "enable-search", FALSE, "rules-hint", TRUE, NULL);

  // have this first as the last column gets the remaining space (always :/)
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_insert_column_with_attributes (self->priv->controller_list, -1,
      _("In use"), renderer, "active", CONTROLLER_LIST_USED, NULL);

  self->priv->id_renderer = renderer = gtk_cell_renderer_text_new ();
  //gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_signal_connect (renderer, "edited", G_CALLBACK (on_control_name_edited),
      (gpointer) self);
  if ((tree_col =
          gtk_tree_view_column_new_with_attributes (_("Controller"), renderer,
              "text", CONTROLLER_LIST_LABEL, NULL))
      ) {
    g_object_set (tree_col, "expand", TRUE, NULL);
    gtk_tree_view_insert_column (self->priv->controller_list, tree_col, -1);
  } else
    GST_WARNING ("can't create treeview column");

  tree_sel = gtk_tree_view_get_selection (self->priv->controller_list);
  gtk_tree_selection_set_mode (tree_sel, GTK_SELECTION_BROWSE);
  g_signal_connect (tree_sel, "changed", G_CALLBACK (on_control_selected),
      (gpointer) self);

  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (self->priv->controller_list));
  g_object_set (GTK_WIDGET (scrolled_window), "hexpand", TRUE, "vexpand", TRUE,
      "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (self), GTK_WIDGET (scrolled_window), 1, 2, 2, 1);

  // add a message pane
#if ! defined(USE_GUDEV) && ! defined(USE_ALSA)
  gtk_label_set_text (self->priv->message,
      _("This package has been built without GUdev and Alsa support and thus "
          "supports no interaction controllers."));
#endif
  gtk_grid_attach (GTK_GRID (self), GTK_WIDGET (self->priv->message), 0, 3, 3,
      1);

  // listen to page changes
  g_signal_connect ((gpointer) pages, "notify::page",
      G_CALLBACK (on_page_switched), (gpointer) self);

  // initial refresh
  on_ic_registry_devices_changed (ic_registry, NULL, (gpointer) self);
}

//-- constructor methods

/**
 * bt_settings_page_interaction_controller_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageInteractionController *
bt_settings_page_interaction_controller_new (GtkWidget * pages)
{
  BtSettingsPageInteractionController *self;

  self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (g_object_new
      (BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER, NULL));
  bt_settings_page_interaction_controller_init_ui (self, pages);
  gtk_widget_show_all (GTK_WIDGET (self));
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_page_interaction_controller_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (object);
  return_if_disposed ();
  switch (property_id) {
    case PROP_DEVICE:
      g_value_set_object (value, self->priv->device);
      break;
    case PROP_CONTROL:
      g_value_set_object (value, self->priv->control);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_settings_page_interaction_controller_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (object);
  return_if_disposed ();
  switch (property_id) {
    case PROP_DEVICE:
      select_device (self, BTIC_DEVICE (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_settings_page_interaction_controller_dispose (GObject * object)
{
  BtSettingsPageInteractionController *self =
      BT_SETTINGS_PAGE_INTERACTION_CONTROLLER (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->device) {
    stop_device (self);
  }

  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_interaction_controller_parent_class)->dispose
      (object);
}

static void
    bt_settings_page_interaction_controller_init
    (BtSettingsPageInteractionController * self)
{
  self->priv = bt_settings_page_interaction_controller_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
    bt_settings_page_interaction_controller_class_init
    (BtSettingsPageInteractionControllerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property =
      bt_settings_page_interaction_controller_set_property;
  gobject_class->get_property =
      bt_settings_page_interaction_controller_get_property;
  gobject_class->dispose = bt_settings_page_interaction_controller_dispose;

  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_object ("device", "device prop", "device object",
          BTIC_TYPE_DEVICE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CONTROL,
      g_param_spec_object ("control", "control prop", "selected control",
          BTIC_TYPE_CONTROL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
