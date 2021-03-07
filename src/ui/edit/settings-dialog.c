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
 * SECTION:btsettingsdialog
 * @short_description: class for the editor settings dialog
 * @see_also: #BtSettings
 *
 * Provides UI to access the #BtSettings.
 */

#define BT_EDIT
#define BT_SETTINGS_DIALOG_C

#include "bt-edit.h"

enum
{
  PROP_PAGE = 1,
  PROP_PAGE_WIDGET
};

enum
{
  COL_LABEL = 0,
  COL_ID,
  COL_ICON_NAME
};

struct _BtSettingsDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* current page */
  BtSettingsPage page;

  /* the list of settings pages */
  GtkTreeView *settings_list;

  /* the pages */
  GtkNotebook *settings_pages;

  /* the audiodevices settings */
  BtSettingsPageAudiodevices *audiodevices_page;
  BtSettingsPageDirectories *directories_page;
  BtSettingsPageInteractionController *interaction_controller_page;
  BtSettingsPagePlaybackController *playback_controller_page;
  BtSettingsPageShortcuts *shortcuts_page;
  BtSettingsPageUI *ui_page;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSettingsDialog, bt_settings_dialog, GTK_TYPE_DIALOG, 
    G_ADD_PRIVATE(BtSettingsDialog));


//-- enums

GType
bt_settings_page_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_SETTINGS_PAGE_AUDIO_DEVICES, "BT_SETTINGS_PAGE_AUDIO_DEVICES",
          "audio devices"},
      {BT_SETTINGS_PAGE_INTERACTION_CONTROLLER,
            "BT_SETTINGS_PAGE_INTERACTION_CONTROLLER",
          "interaction controller"},
      {BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER,
          "BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER", "playback controller"},
      {BT_SETTINGS_PAGE_SHORTCUTS, "BT_SETTINGS_PAGE_SHORTCUTS", "shortcuts"},
      {BT_SETTINGS_PAGE_DIRECTORIES, "BT_SETTINGS_PAGE_DIRECTORIES",
          "directories"},
      {BT_SETTINGS_PAGE_UI, "BT_SETTINGS_PAGE_UI", "ui"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtSettingsPage", values);
  }
  return type;
}

//-- event handler

static void
on_settings_list_cursor_changed (GtkTreeView * treeview, gpointer user_data)
{
  BtSettingsDialog *self = BT_SETTINGS_DIALOG (user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  return_if_disposed ();

  GST_INFO ("settings list cursor changed");
  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->settings_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gulong id;

    gtk_tree_model_get (model, &iter, COL_ID, &id, -1);
    GST_INFO ("selected entry id %lu", id);
    gtk_notebook_set_current_page (self->priv->settings_pages, id);
    self->priv->page = id;
    g_object_notify ((gpointer) self, "page");
  }
}

/* we adjust the scrollable-window size to contain the whole area */
static void
on_settings_list_realize (GtkWidget * widget, gpointer user_data)
{
  GtkWidget *parent = GTK_WIDGET (user_data);
  GtkRequisition requisition;
  gint height, max_height;

  gtk_widget_get_preferred_size (widget, NULL, &requisition);
  bt_gtk_workarea_size (NULL, &max_height);

  GST_LOG ("#### list size req %d x %d (max-height %d)", requisition.width,
      requisition.height, max_height);
  // constrain the height by screen height
  height = MIN (requisition.height,  max_height);
  // TODO(ensonic): is the '2' some border or padding
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (parent),
      height + 4);
}

//-- helper methods

static void
select_page (BtSettingsDialog * self, BtSettingsPage new_page)
{
  GtkTreeSelection *selection;
  GtkTreePath *path;

  if (self->priv->page == new_page)
    return;

  self->priv->page = new_page;
  // switch page
  selection = gtk_tree_view_get_selection (self->priv->settings_list);
  if ((path = gtk_tree_path_new_from_indices (new_page, -1))) {
    gtk_tree_selection_select_path (selection, path);
    gtk_tree_view_set_cursor (self->priv->settings_list, path, NULL, FALSE);
    gtk_tree_path_free (path);
    on_settings_list_cursor_changed (self->priv->settings_list, self);
  }
}

static void
bt_settings_dialog_init_ui (const BtSettingsDialog * self)
{
  GtkWidget *box, *scrolled_window, *pages;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;

  gtk_widget_set_name (GTK_WIDGET (self), "buzztrax settings");

  //gtk_widget_set_size_request(GTK_WIDGET(self),800,600);
  gtk_window_set_title (GTK_WINDOW (self), _("buzztrax settings"));

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons (GTK_DIALOG (self),
      _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  // add widgets to the dialog content area
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);

  // add a list on the right and a notebook without tabs on the left
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_ETCHED_IN);
  self->priv->settings_list = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_tree_view_set_headers_visible (self->priv->settings_list, FALSE);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_attributes (self->priv->settings_list, -1,
      NULL, renderer, "icon-name", COL_ICON_NAME, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_tree_view_insert_column_with_attributes (self->priv->settings_list, -1,
      NULL, renderer, "text", COL_LABEL, NULL);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (self->
          priv->settings_list), GTK_SELECTION_BROWSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
      GTK_WIDGET (self->priv->settings_list));
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (scrolled_window), FALSE, FALSE,
      0);

  g_signal_connect (self->priv->settings_list, "realize",
      G_CALLBACK (on_settings_list_realize), (gpointer) scrolled_window);
  g_signal_connect (self->priv->settings_list, "cursor-changed",
      G_CALLBACK (on_settings_list_cursor_changed), (gpointer) self);

  store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_LONG, G_TYPE_STRING);
  //-- append entries for settings pages
  gtk_list_store_append (store, &tree_iter);
  gtk_list_store_set (store, &tree_iter,
      COL_LABEL, _("Audio Devices"),
      COL_ID, BT_SETTINGS_PAGE_AUDIO_DEVICES, COL_ICON_NAME, "audio-card", -1);
  gtk_list_store_append (store, &tree_iter);
  gtk_list_store_set (store, &tree_iter,
      COL_LABEL, _("Directories"),
      COL_ID, BT_SETTINGS_PAGE_DIRECTORIES, COL_ICON_NAME, "folder", -1);
  gtk_list_store_append (store, &tree_iter);
  gtk_list_store_set (store, &tree_iter,
      COL_LABEL, _("Interaction Controller"),
      COL_ID, BT_SETTINGS_PAGE_INTERACTION_CONTROLLER,
      COL_ICON_NAME, "input-gaming", -1);
  gtk_list_store_append (store, &tree_iter);
  gtk_list_store_set (store, &tree_iter,
      COL_LABEL, _("Playback Controller"),
      COL_ID, BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER,
      COL_ICON_NAME, "media-playback-start", -1);
  gtk_list_store_append (store, &tree_iter);
  gtk_list_store_set (store, &tree_iter,
      COL_LABEL, _("Shortcuts"),
      COL_ID, BT_SETTINGS_PAGE_SHORTCUTS, COL_ICON_NAME, "input-keyboard", -1);
  gtk_list_store_append (store, &tree_iter);
  gtk_list_store_set (store, &tree_iter,
      COL_LABEL, _("User interface"),
      COL_ID, BT_SETTINGS_PAGE_UI, COL_ICON_NAME, "preferences-desktop-theme",
      -1);
  gtk_tree_view_set_model (self->priv->settings_list, GTK_TREE_MODEL (store));
  g_object_unref (store);       // drop with treeview

  // add notebook
  pages = gtk_notebook_new ();
  self->priv->settings_pages = GTK_NOTEBOOK (pages);
  gtk_widget_set_name (pages, "settings pages");
  gtk_notebook_set_show_tabs (self->priv->settings_pages, FALSE);
  gtk_notebook_set_show_border (self->priv->settings_pages, FALSE);
  gtk_container_add (GTK_CONTAINER (box), pages);

  // add audio device page
  self->priv->audiodevices_page = bt_settings_page_audiodevices_new (pages);
  gtk_container_add (GTK_CONTAINER (self->priv->settings_pages),
      GTK_WIDGET (self->priv->audiodevices_page));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (self->priv->settings_pages),
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (self->priv->settings_pages),
          BT_SETTINGS_PAGE_AUDIO_DEVICES), gtk_label_new (_("Audio Devices")));

  // add directories page
  self->priv->directories_page = bt_settings_page_directories_new (pages);
  gtk_container_add (GTK_CONTAINER (self->priv->settings_pages),
      GTK_WIDGET (self->priv->directories_page));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (self->priv->settings_pages),
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (self->priv->settings_pages),
          BT_SETTINGS_PAGE_DIRECTORIES), gtk_label_new (_("Directories")));

  // add interaction controller page
  self->priv->interaction_controller_page =
      bt_settings_page_interaction_controller_new (pages);
  gtk_container_add (GTK_CONTAINER (self->priv->settings_pages),
      GTK_WIDGET (self->priv->interaction_controller_page));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (self->priv->settings_pages),
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (self->priv->settings_pages),
          BT_SETTINGS_PAGE_INTERACTION_CONTROLLER),
      gtk_label_new (_("Interaction Controller")));

  // add playback controller page
  self->priv->playback_controller_page =
      bt_settings_page_playback_controller_new (pages);
  gtk_container_add (GTK_CONTAINER (self->priv->settings_pages),
      GTK_WIDGET (self->priv->playback_controller_page));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (self->priv->settings_pages),
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (self->priv->settings_pages),
          BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER),
      gtk_label_new (_("Playback Controller")));

  // add shortcuts pags
  self->priv->shortcuts_page = bt_settings_page_shortcuts_new (pages);
  gtk_container_add (GTK_CONTAINER (self->priv->settings_pages),
      GTK_WIDGET (self->priv->shortcuts_page));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (self->priv->settings_pages),
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (self->priv->settings_pages),
          BT_SETTINGS_PAGE_SHORTCUTS), gtk_label_new (_("Shortcuts")));

  // add ui page
  self->priv->ui_page = bt_settings_page_ui_new (pages);
  gtk_container_add (GTK_CONTAINER (self->priv->settings_pages),
      GTK_WIDGET (self->priv->ui_page));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (self->priv->settings_pages),
      gtk_notebook_get_nth_page (GTK_NOTEBOOK (self->priv->settings_pages),
          BT_SETTINGS_PAGE_UI), gtk_label_new (_("User Interface")));

  /* TODO(ensonic): more settings
   * - misc
   *   - initial song bpm (from, to)
   *   - cpu monitor (view menu?)
   */

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      box, TRUE, TRUE, 0);
}

//-- constructor methods

/**
 * bt_settings_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsDialog *
bt_settings_dialog_new (void)
{
  BtSettingsDialog *self =
      BT_SETTINGS_DIALOG (g_object_new (BT_TYPE_SETTINGS_DIALOG, NULL));
  bt_settings_dialog_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_dialog_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtSettingsDialog *self = BT_SETTINGS_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case PROP_PAGE:
      g_value_set_enum (value, self->priv->page);
      break;
    case PROP_PAGE_WIDGET:
      switch (self->priv->page) {
        case BT_SETTINGS_PAGE_AUDIO_DEVICES:
          g_value_set_object (value, self->priv->audiodevices_page);
          break;
        case BT_SETTINGS_PAGE_DIRECTORIES:
          g_value_set_object (value, self->priv->directories_page);
          break;
        case BT_SETTINGS_PAGE_INTERACTION_CONTROLLER:
          g_value_set_object (value, self->priv->interaction_controller_page);
          break;
        case BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER:
          g_value_set_object (value, self->priv->playback_controller_page);
          break;
        case BT_SETTINGS_PAGE_SHORTCUTS:
          g_value_set_object (value, self->priv->shortcuts_page);
          break;
        case BT_SETTINGS_PAGE_UI:
          g_value_set_object (value, self->priv->ui_page);
          break;
        default:
          g_assert_not_reached ();
          break;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_settings_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtSettingsDialog *self = BT_SETTINGS_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case PROP_PAGE:
      select_page (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_settings_dialog_dispose (GObject * object)
{
  BtSettingsDialog *self = BT_SETTINGS_DIALOG (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_dialog_parent_class)->dispose (object);
}

static void
bt_settings_dialog_init (BtSettingsDialog * self)
{
  self->priv = bt_settings_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_settings_dialog_class_init (BtSettingsDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_settings_dialog_set_property;
  gobject_class->get_property = bt_settings_dialog_get_property;
  gobject_class->dispose = bt_settings_dialog_dispose;

  g_object_class_install_property (gobject_class, PROP_PAGE,
      g_param_spec_enum ("page", "page prop", "Current settings page",
          BT_TYPE_SETTINGS_PAGE, BT_SETTINGS_PAGE_AUDIO_DEVICES,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAGE_WIDGET,
      g_param_spec_object ("page-widget", "page-widget prop",
          "Current settings page widget",
          GTK_TYPE_WIDGET, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
