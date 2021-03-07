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
 * SECTION:btsettingspageplaybackcontroller
 * @short_description: playback controller configuration settings page
 *
 * Lists available playback controllers and allows to configure them. Each
 * controller type can support a master and a slave mode. Master mode means that
 * changes in the playback state in buzztrax, will relect on the playback state
 * of clients to that protocol. Slave mode will cause buzztrax to sync to the
 * external playback state.
 */
/* TODO(ensonic): add a list of playback controllers:
 *   - upnp coherence/gupnp (port)
 *   - alsa midi MC         (list of devices to enable)
 *   - jack midi MC
 *   - jack transport
 *   - multimedia keys
 *     - slave = key-presses
 *     - master = emit the key-presses
 *   - MPRIS (DBus Media player iface)
 *
 * - for alsa/raw midi MC, we need to have the IO-loops running on the devices
 *   - for that it would be good to know which devices actually support it, so
 *     that we can only start it for devices that have it, when they are plugged
 *     - list of devices + learn button on the settings page? or just a list and
 *       leave it up to the user to tick only suitable devices
 *     - each one can be enabled or disabled (M/S flags from parent apply)
 *     - draw back is that we can only change settings for plugged devices
 *       - we could also skip that setting initially and just start all devices
 *         if this playback controller is active
 *   - see BtPlaybackControllerMidi
 *   - most midi devices implement buttons as range-controls with pressed=127
 *     and released=0
 *   - we'll only support slave, as bt-ic is input only anyway
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_C

#include "bt-edit.h"

enum
{
  CONTROLLER_LIST_ID = 0,
  CONTROLLER_LIST_LABEL,
  CONTROLLER_LIST_CAN_MASTER,
  CONTROLLER_LIST_MASTER,
  CONTROLLER_LIST_CAN_SLAVE,
  CONTROLLER_LIST_SLAVE
};

enum
{
  DEVICE_MENU_LABEL = 0
};

enum
{
  COMMAND_MENU_LABEL = 0
};

typedef enum
{
  CONTROLLER_PAGE_UPNP = 0,
  CONTROLLER_PAGE_JACK,
  CONTROLLER_PAGE_IC
} BtPlaybackControllerPage;

struct _BtSettingsPagePlaybackControllerPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  BtSettings *settings;

  BtPlaybackControllerPage page;
  GtkTreeView *controller_list;
  GtkNotebook *controller_pages;

  /* coherence upnp */
  GtkWidget *port_entry;

  /* ic playback */
  GtkComboBox *device_menu;
  GHashTable *ic_playback_cfg;
  GHashTable *ic_playback_widgets;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSettingsPagePlaybackController,
    bt_settings_page_playback_controller, GTK_TYPE_GRID, 
    G_ADD_PRIVATE(BtSettingsPagePlaybackController));


//-- helper

static void
update_ic_playback_settings (BtSettingsPagePlaybackController * self)
{
  gchar *spec;

  spec = bt_settings_format_ic_playback_spec (self->priv->ic_playback_cfg);
  GST_DEBUG ("formatted spec: '%s'", spec);
  g_object_set (self->priv->settings, "ic-playback-spec", spec, NULL);
  g_free (spec);
}

//-- event handler

static void
on_controller_list_cursor_changed (GtkTreeView * treeview, gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (self->priv->controller_list);
  if (!selection)
    return;

  GST_INFO ("settings list cursor changed");
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    guint id;

    gtk_tree_model_get (model, &iter, CONTROLLER_LIST_ID, &id, -1);
    GST_INFO ("selected entry id %u", id);
    gtk_notebook_set_current_page (self->priv->controller_pages, id);
    self->priv->page = id;
  }
}

static void
on_activate_master_toggled (GtkCellRendererToggle * cell_renderer, gchar * path,
    gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);
  GtkTreeIter iter;
  GtkTreeModel *store;
  gboolean active;              // gtk_cell_renderer_toggle_get_active(cell_renderer); <- this does not seem to work
  gboolean active_setting;
  guint id;

  store = gtk_tree_view_get_model (self->priv->controller_list);
  gtk_tree_model_get_iter_from_string (store, &iter, path);
  gtk_tree_model_get (store, &iter, CONTROLLER_LIST_MASTER, &active,
      CONTROLLER_LIST_ID, &id, -1);
  active ^= 1;
  gtk_list_store_set (GTK_LIST_STORE (store), &iter, CONTROLLER_LIST_MASTER,
      active, -1);
  self->priv->page = id;

  switch (self->priv->page) {
    case CONTROLLER_PAGE_UPNP:
    case CONTROLLER_PAGE_IC:
      break;
    case CONTROLLER_PAGE_JACK:
      g_object_get (self->priv->settings, "jack-transport-master",
          &active_setting, NULL);
      GST_INFO ("jack transport master changes from %d -> %d", active_setting,
          active);
      if (active != active_setting) {
        g_object_set (self->priv->settings, "jack-transport-master", active,
            NULL);
      }
      break;
  }
}

static void
on_activate_slave_toggled (GtkCellRendererToggle * cell_renderer, gchar * path,
    gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);
  GtkTreeIter iter;
  GtkTreeModel *store;
  gboolean active;
  gboolean active_setting;
  guint id;

  // gtk_cell_renderer_toggle_get_active(cell_renderer); <- this does not seem to work
  store = gtk_tree_view_get_model (self->priv->controller_list);
  gtk_tree_model_get_iter_from_string (store, &iter, path);
  gtk_tree_model_get (store, &iter, CONTROLLER_LIST_SLAVE, &active,
      CONTROLLER_LIST_ID, &id, -1);
  active ^= 1;
  gtk_list_store_set (GTK_LIST_STORE (store), &iter, CONTROLLER_LIST_SLAVE,
      active, -1);
  self->priv->page = id;

  switch (self->priv->page) {
    case CONTROLLER_PAGE_UPNP:
      g_object_get (self->priv->settings, "coherence-upnp-active",
          &active_setting, NULL);
      GST_INFO ("upnp active changes from %d -> %d", active_setting, active);
      if (active != active_setting) {
        g_object_set (self->priv->settings, "coherence-upnp-active", active,
            NULL);
      }
      gtk_widget_set_sensitive (self->priv->port_entry, active);
      break;
    case CONTROLLER_PAGE_JACK:
      g_object_get (self->priv->settings, "jack-transport-slave",
          &active_setting, NULL);
      GST_INFO ("jack transport slave changes from %d -> %d", active_setting,
          active);
      if (active != active_setting) {
        g_object_set (self->priv->settings, "jack-transport-slave", active,
            NULL);
      }
      break;
    case CONTROLLER_PAGE_IC:
      g_object_get (self->priv->settings, "ic-playback-active",
          &active_setting, NULL);
      GST_INFO ("ic-playback active changes from %d -> %d", active_setting,
          active);
      if (active != active_setting) {
        g_object_set (self->priv->settings, "ic-playback-active", active, NULL);
      }
      // FIXME: switch sensitivity of UI
      break;
  }
}

static void
on_port_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);

  g_object_set (self->priv->settings, "coherence-upnp-port",
      gtk_spin_button_get_value_as_int (spinbutton), NULL);
}

static void
on_ic_registry_devices_changed (BtIcRegistry * ic_registry, GParamSpec * arg,
    gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);
  GList *node, *list;
  BtObjectListModel *store;
  BtIcDevice *device;
  gint active = -1, ix;
  gchar *dev_name, *cfg_name;

  GST_INFO ("refreshing device menu");

  cfg_name = g_hash_table_lookup (self->priv->ic_playback_cfg, "!device");

  store = bt_object_list_model_new (1, BTIC_TYPE_DEVICE, "name");
  g_object_get (ic_registry, "devices", &list, NULL);
  for (node = list, ix = 0; node; node = g_list_next (node), ix++) {
    device = BTIC_DEVICE (node->data);
    bt_object_list_model_append (store, (GObject *) device);
    if (cfg_name && active == -1) {
      g_object_get (device, "name", &dev_name, NULL);
      if (!g_strcmp0 (dev_name, cfg_name)) {
        active = ix;
      }
      g_free (dev_name);
    }
  }
  GST_WARNING ("active = %d, device from config = '%s'", active, cfg_name);

  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->device_menu),
      (list != NULL));
  gtk_combo_box_set_model (self->priv->device_menu, GTK_TREE_MODEL (store));
  gtk_combo_box_set_active (self->priv->device_menu, active);
  // TODO(ensonic): should we show a label with what was selected, if it is
  // currently not plugged?
  g_list_free (list);
  g_object_unref (store);       // drop with comboxbox
  GST_WARNING ("device menu refreshed");
}

static void
on_device_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);
  GObject *device = NULL;
  BtObjectListModel *store;
  GtkTreeModel *model;
  GtkTreeIter t_iter;
  GHashTableIter h_iter;
  GList *node, *list;
  gpointer key, value;
  gchar *dev_name, *ctrl_name, *cfg_name;
  gint active, ix;

  model = gtk_combo_box_get_model (combo_box);
  if (gtk_combo_box_get_active_iter (combo_box, &t_iter)) {
    device =
        bt_object_list_model_get_object (BT_OBJECT_LIST_MODEL (model), &t_iter);
  }
  if (!device)
    return;

  g_object_get (device, "name", &dev_name, "controls", &list, NULL);
  g_hash_table_insert (self->priv->ic_playback_cfg, g_strdup ("!device"),
      dev_name);
  GST_WARNING ("active device = '%s'", dev_name);

  // build control menu
  store = bt_object_list_model_new (1, BTIC_TYPE_CONTROL, "name");
  for (node = list; node; node = g_list_next (node)) {
    bt_object_list_model_append (store, (GObject *) node->data);
  }

  // update command combos
  g_hash_table_iter_init (&h_iter, self->priv->ic_playback_widgets);
  while (g_hash_table_iter_next (&h_iter, &key, &value)) {
    gtk_widget_set_sensitive (GTK_WIDGET (value), (list != NULL));
    gtk_combo_box_set_model (GTK_COMBO_BOX (value),
        GTK_TREE_MODEL (g_object_ref (store)));

    active = -1;
    cfg_name = g_hash_table_lookup (self->priv->ic_playback_cfg, key);
    if (cfg_name) {
      // select entry from settings
      for (node = list, ix = 0; (node && active == -1);
          node = g_list_next (node), ix++) {
        g_object_get (node->data, "name", &ctrl_name, NULL);
        if (!g_strcmp0 (ctrl_name, cfg_name)) {
          active = ix;
        }
        g_free (ctrl_name);
      }
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (value), active);
    if (active == -1) {
      g_hash_table_remove (self->priv->ic_playback_cfg, key);
    }
  }
  g_object_unref (store);       // drop with treeview
  g_list_free (list);

  update_ic_playback_settings (self);
}

static void
on_command_menu_changed (GtkComboBox * combo_box, gpointer user_data)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (user_data);
  GObject *control = NULL;
  GtkTreeModel *model;
  GtkTreeIter t_iter;
  const gchar *key = gtk_widget_get_name (GTK_WIDGET (combo_box));
  gchar *ctrl_name;

  model = gtk_combo_box_get_model (combo_box);
  if (gtk_combo_box_get_active_iter (combo_box, &t_iter)) {
    control =
        bt_object_list_model_get_object (BT_OBJECT_LIST_MODEL (model), &t_iter);
  }
  if (!control) {
    g_hash_table_remove (self->priv->ic_playback_cfg, key);
    goto done;
  }

  g_object_get (control, "name", &ctrl_name, NULL);
  g_hash_table_insert (self->priv->ic_playback_cfg, g_strdup (key), ctrl_name);

  GST_DEBUG ("command ['%s'] = '%s'", key, ctrl_name);

done:
  update_ic_playback_settings (self);
}

//-- helper methods

static void
add_ic_playback_command_widgets (const BtSettingsPagePlaybackController * self,
    GtkGrid * table, const gchar * label_str, const gchar * key, gint line)
{
  GtkWidget *label, *widget;
  GtkCellRenderer *renderer;

  label = gtk_label_new (label_str);
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (table, label, 0, line, 1, 1);

  widget = gtk_combo_box_new ();
  g_object_set (widget, "name", key, "hexpand", TRUE, "margin-left",
      LABEL_PADDING, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget),
      renderer, "text", COMMAND_MENU_LABEL, NULL);
  g_signal_connect (widget, "changed", G_CALLBACK (on_command_menu_changed),
      (gpointer) self);

  gtk_grid_attach (table, widget, 1, line, 1, 1);
  g_hash_table_insert (self->priv->ic_playback_widgets, (gpointer) key,
      (gpointer) widget);
}

static void
bt_settings_page_playback_controller_init_ui (const
    BtSettingsPagePlaybackController * self, GtkWidget * pages)
{
  GtkWidget *label, *scrolled_window, *widget;
  GtkGrid *table;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkAdjustment *spin_adjustment;
  gboolean coherence_upnp_active, ic_playback_active;
  guint coherence_upnp_port;
  gboolean jack_transport_master, jack_transport_slave;
  gchar *ic_playback_spec;
  gchar *str, *element_name;
  BtIcRegistry *ic_registry;

  gtk_widget_set_name (GTK_WIDGET (self), "playback controller settings");

  // get settings
  g_object_get (self->priv->settings,
      "coherence-upnp-active", &coherence_upnp_active,
      "coherence-upnp-port", &coherence_upnp_port,
      "jack-transport-master", &jack_transport_master,
      "jack-transport-slave", &jack_transport_slave,
      "ic-playback-active", &ic_playback_active,
      "ic-playback-spec", &ic_playback_spec, NULL);

  self->priv->ic_playback_cfg =
      bt_settings_parse_ic_playback_spec (ic_playback_spec);
  g_free (ic_playback_spec);

  // add setting widgets
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Playback Controller"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 0, 0, 3, 1);
  gtk_grid_attach (GTK_GRID (self), gtk_label_new ("    "), 0, 1, 1, 3);

  // add a list of playback controllers
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_ETCHED_IN);
  self->priv->controller_list = GTK_TREE_VIEW (gtk_tree_view_new ());
  g_signal_connect (self->priv->controller_list, "cursor-changed",
      G_CALLBACK (on_controller_list_cursor_changed), (gpointer) self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_tree_view_insert_column_with_attributes (self->priv->controller_list, -1,
      _("Controller"), renderer, "text", CONTROLLER_LIST_LABEL, NULL);
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_insert_column_with_attributes (self->priv->controller_list, -1,
      _("Master"), renderer, "sensitive", CONTROLLER_LIST_CAN_MASTER, "active",
      CONTROLLER_LIST_MASTER, NULL);
  g_signal_connect (renderer, "toggled",
      G_CALLBACK (on_activate_master_toggled), (gpointer) self);
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_insert_column_with_attributes (self->priv->controller_list, -1,
      _("Slave"), renderer, "sensitive", CONTROLLER_LIST_CAN_SLAVE, "active",
      CONTROLLER_LIST_SLAVE, NULL);
  g_signal_connect (renderer, "toggled", G_CALLBACK (on_activate_slave_toggled),
      (gpointer) self);

  selection = gtk_tree_view_get_selection (self->priv->controller_list);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (self->priv->controller_list));
  g_object_set (GTK_WIDGET (scrolled_window), "hexpand", TRUE, "vexpand", TRUE,
      "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (self), GTK_WIDGET (scrolled_window), 1, 1, 2, 1);

  // add data model
  store =
      gtk_list_store_new (6, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_BOOLEAN,
      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, CONTROLLER_LIST_ID, CONTROLLER_PAGE_UPNP,
      // don't translate, this is a product
      CONTROLLER_LIST_LABEL, "Coherence UPnP",
      CONTROLLER_LIST_CAN_MASTER, FALSE,
      CONTROLLER_LIST_MASTER, FALSE,
      CONTROLLER_LIST_CAN_SLAVE, TRUE,
      CONTROLLER_LIST_SLAVE, coherence_upnp_active, -1);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, CONTROLLER_LIST_ID, CONTROLLER_PAGE_JACK,
      // don't translate, this is a product
      CONTROLLER_LIST_LABEL, "Jack Transport",
      CONTROLLER_LIST_CAN_MASTER, TRUE,
      CONTROLLER_LIST_MASTER, jack_transport_master,
      CONTROLLER_LIST_CAN_SLAVE, TRUE,
      CONTROLLER_LIST_SLAVE, jack_transport_slave, -1);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, CONTROLLER_LIST_ID, CONTROLLER_PAGE_IC,
      // don't translate, this is a product
      CONTROLLER_LIST_LABEL, "Interaction Controller",
      CONTROLLER_LIST_CAN_MASTER, FALSE,
      CONTROLLER_LIST_MASTER, FALSE,
      CONTROLLER_LIST_CAN_SLAVE, TRUE,
      CONTROLLER_LIST_SLAVE, ic_playback_active, -1);

  gtk_tree_view_set_model (self->priv->controller_list, GTK_TREE_MODEL (store));
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
  gtk_tree_selection_select_iter (selection, &iter);
  g_object_unref (store);       // drop with treeview

  // add a notebook widget with hidden tabs
  self->priv->controller_pages = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_notebook_set_show_tabs (self->priv->controller_pages, FALSE);
  gtk_notebook_set_show_border (self->priv->controller_pages, FALSE);
  g_object_set (GTK_WIDGET (self->priv->controller_pages), "hexpand", TRUE,
      "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (self), GTK_WIDGET (self->priv->controller_pages),
      1, 2, 2, 1);


  // coherence upnp tab
  widget = gtk_grid_new ();
  table = GTK_GRID (widget);
  gtk_container_add (GTK_CONTAINER (self->priv->controller_pages), widget);

  // local network port number for socket communication
  label = gtk_label_new (_("Port number"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (table, label, 0, 0, 1, 1);

  spin_adjustment = gtk_adjustment_new ((gdouble) coherence_upnp_port, 1024.0,
      65536.0, 1.0, 5.0, 0.0);
  self->priv->port_entry = gtk_spin_button_new (spin_adjustment, 1.0, 0);
  g_signal_connect (self->priv->port_entry, "value-changed",
      G_CALLBACK (on_port_changed), (gpointer) self);
  g_object_set (self->priv->port_entry, "hexpand", TRUE, "margin-left",
      LABEL_PADDING, NULL);
  gtk_grid_attach (table, self->priv->port_entry, 1, 0, 1, 1);

  // add coherence URL
  label =
      gtk_label_new
      (_
      ("Requires Coherence UPnP framework which can be found at: https://coherence.beebits.net."));
  g_object_set (label, "hexpand", TRUE, "wrap", TRUE, "selectable", TRUE, NULL);
  gtk_grid_attach (table, label, 0, 2, 2, 1);


  // jack transport tab
  bt_settings_determine_audiosink_name (self->priv->settings, &element_name,
      NULL);
  str = g_strdup_printf
      (_("Jack transport requires that the jackaudiosink is active. "
          "Your current audiosink is '%s'."),
      (element_name ? element_name : _("none")));
  label = gtk_label_new (str);
  g_object_set (label, "hexpand", TRUE, "wrap", TRUE, "selectable", TRUE, NULL);
  gtk_container_add (GTK_CONTAINER (self->priv->controller_pages), label);
  g_free (element_name);
  g_free (str);


  // ic tab
  widget = gtk_grid_new ();
  table = GTK_GRID (widget);
  gtk_container_add (GTK_CONTAINER (self->priv->controller_pages), widget);

  label = gtk_label_new (_("Device"));
  g_object_set (label, "xalign", 1.0, "margin-bottom", LABEL_PADDING, NULL);
  gtk_grid_attach (table, label, 0, 0, 1, 1);

  widget = gtk_combo_box_new ();
  self->priv->device_menu = GTK_COMBO_BOX (widget);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING,
      "margin-bottom", LABEL_PADDING, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget),
      renderer, "text", DEVICE_MENU_LABEL, NULL);

  // get list of devices from libbtic and listen to changes
  g_object_get (self->priv->app, "ic-registry", &ic_registry, NULL);
  g_signal_connect_object (ic_registry, "notify::devices",
      G_CALLBACK (on_ic_registry_devices_changed), (gpointer) self, 0);
  on_ic_registry_devices_changed (ic_registry, NULL, (gpointer) self);
  g_object_unref (ic_registry);
  gtk_grid_attach (table, widget, 1, 0, 1, 1);
  g_signal_connect (widget, "changed", G_CALLBACK (on_device_menu_changed),
      (gpointer) self);

  self->priv->ic_playback_widgets = g_hash_table_new (NULL, NULL);
  add_ic_playback_command_widgets (self, table, _("Play"), "play", 1);
  add_ic_playback_command_widgets (self, table, _("Stop"), "stop", 2);
  add_ic_playback_command_widgets (self, table, _("Rewind"), "rewind", 3);
  add_ic_playback_command_widgets (self, table, _("Forward"), "forward", 4);
  // TODO(ensonic): other commands (loop on/off, record)

  on_device_menu_changed (self->priv->device_menu, (gpointer) self);
}

//-- constructor methods

/**
 * bt_settings_page_playback_controller_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPagePlaybackController *
bt_settings_page_playback_controller_new (GtkWidget * pages)
{
  BtSettingsPagePlaybackController *self;

  self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (g_object_new
      (BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER, NULL));
  bt_settings_page_playback_controller_init_ui (self, pages);
  gtk_widget_show_all (GTK_WIDGET (self));
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_page_playback_controller_dispose (GObject * object)
{
  BtSettingsPagePlaybackController *self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_hash_table_destroy (self->priv->ic_playback_cfg);
  g_hash_table_destroy (self->priv->ic_playback_widgets);

  g_object_unref (self->priv->settings);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_playback_controller_parent_class)->dispose
      (object);
}

static void
bt_settings_page_playback_controller_init (BtSettingsPagePlaybackController *
    self)
{
  self->priv = bt_settings_page_playback_controller_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
  g_object_get (self->priv->app, "settings", &self->priv->settings, NULL);
}

static void
    bt_settings_page_playback_controller_class_init
    (BtSettingsPagePlaybackControllerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_settings_page_playback_controller_dispose;
}
