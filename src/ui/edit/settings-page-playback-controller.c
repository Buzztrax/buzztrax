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
 *   - we also need a enum-based control-type, or a set of trigger controls
 *    (start, continue, stop), trigger based control we could actually learn
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

typedef enum
{
  CONTROLLER_PAGE_UPNP = 0,
  CONTROLLER_PAGE_JACK
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
};

//-- the class

G_DEFINE_TYPE (BtSettingsPagePlaybackController,
    bt_settings_page_playback_controller, GTK_TYPE_TABLE);


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
  gboolean active;              // gtk_cell_renderer_toggle_get_active(cell_renderer); <- this does not seem to work
  gboolean active_setting;
  guint id;

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

//-- helper methods

static void
bt_settings_page_playback_controller_init_ui (const
    BtSettingsPagePlaybackController * self)
{
  GtkWidget *label, *spacer, *table, *scrolled_window;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkAdjustment *spin_adjustment;
  gboolean coherence_upnp_active;
  guint coherence_upnp_port;
  gboolean jack_transport_master, jack_transport_slave;
  gchar *str, *element_name;

  gtk_widget_set_name (GTK_WIDGET (self), "playback controller settings");

  // get settings
  g_object_get (self->priv->settings,
      "coherence-upnp-active", &coherence_upnp_active,
      "coherence-upnp-port", &coherence_upnp_port,
      "jack-transport-master", &jack_transport_master,
      "jack-transport-slave", &jack_transport_slave, NULL);

  // add setting widgets
  spacer = gtk_label_new ("    ");
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Playback Controller"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 0, 3, 0, 1, GTK_FILL | GTK_EXPAND,
      GTK_SHRINK, 2, 1);
  gtk_table_attach (GTK_TABLE (self), spacer, 0, 1, 1, 4, GTK_SHRINK,
      GTK_SHRINK, 2, 1);

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
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (scrolled_window), 1, 3, 1, 2,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

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

  gtk_tree_view_set_model (self->priv->controller_list, GTK_TREE_MODEL (store));
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
  gtk_tree_selection_select_iter (selection, &iter);
  g_object_unref (store);       // drop with treeview

  // add a notebook widget with hidden tabs
  self->priv->controller_pages = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_notebook_set_show_tabs (self->priv->controller_pages, FALSE);
  gtk_notebook_set_show_border (self->priv->controller_pages, FALSE);
  gtk_table_attach (GTK_TABLE (self), GTK_WIDGET (self->priv->controller_pages),
      1, 3, 2, 3, GTK_FILL, GTK_FILL, 2, 1);


  // coherence upnp tab
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (self->priv->controller_pages), table);

  // local network port number for socket communication
  label = gtk_label_new (_("Port number"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_SHRINK,
      2, 1);

  spin_adjustment =
      GTK_ADJUSTMENT (gtk_adjustment_new ((gdouble) coherence_upnp_port, 1024.0,
          65536.0, 1.0, 5.0, 0.0));
  self->priv->port_entry = gtk_spin_button_new (spin_adjustment, 1.0, 0);
  g_signal_connect (self->priv->port_entry, "value-changed",
      G_CALLBACK (on_port_changed), (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), self->priv->port_entry, 1, 2, 0, 1,
      GTK_FILL | GTK_EXPAND, GTK_SHRINK, 2, 1);

  // add coherence URL
  label =
      gtk_label_new
      ("Requires Coherence UPnP framework which can be found at: https://coherence.beebits.net.");
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 1, 2, GTK_FILL, GTK_SHRINK,
      2, 1);


  // jack transport tab
  bt_settings_determine_audiosink_name (self->priv->settings, &element_name,
      NULL);
  str = g_strdup_printf
      ("Jack transport requires that the jackaudiosink is active."
      "Your current audiosink is '%s'.",
      (element_name ? element_name : "none"));
  label = gtk_label_new (str);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_container_add (GTK_CONTAINER (self->priv->controller_pages), label);
  g_free (element_name);
  g_free (str);

}

//-- constructor methods

/**
 * bt_settings_page_playback_controller_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPagePlaybackController *
bt_settings_page_playback_controller_new (void)
{
  BtSettingsPagePlaybackController *self;

  self =
      BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER (g_object_new
      (BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER, "n-rows", 4, "n-columns", 3,
          "homogeneous", FALSE, NULL));
  bt_settings_page_playback_controller_init_ui (self);
  gtk_widget_show_all (GTK_WIDGET (self));
  return (self);
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

  g_object_unref (self->priv->settings);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_playback_controller_parent_class)->dispose
      (object);
}

static void
bt_settings_page_playback_controller_init (BtSettingsPagePlaybackController *
    self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self,
      BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER,
      BtSettingsPagePlaybackControllerPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
  g_object_get (self->priv->app, "settings", &self->priv->settings, NULL);
}

static void
    bt_settings_page_playback_controller_class_init
    (BtSettingsPagePlaybackControllerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass,
      sizeof (BtSettingsPagePlaybackControllerPrivate));

  gobject_class->dispose = bt_settings_page_playback_controller_dispose;
}
