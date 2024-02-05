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
 * SECTION:btmainmenu
 * @short_description: class for the editor main menu
 *
 * Provides the main application menu.
 */

/* TODO(ensonic): main-menu tasks
 *  - enable/disable edit menu entries based on selection and focus
 */

#define BT_EDIT
#define BT_MAIN_MENU_C

#include "bt-edit.h"
#include "main-menu.h"

struct _BtMainMenu
{
  GtkWidget parent;
  
  BtEditApplication *app;
  BtChangeLog *change_log;
  gchar *debug_graph_format;
  guint debug_graph_details;
  GtkWidget *popover_menu;
};  

G_DEFINE_TYPE (BtMainMenu, bt_main_menu, GTK_TYPE_WIDGET);

//-- event handler
#if 0 /// GTK4 many of these actions could likely be moved to EditApplication.
static void
on_menu_new_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu new event occurred");
  bt_main_window_new_song (self->main_window);
}

static void
on_menu_open_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu open event occurred");
  bt_main_window_open_song (self->main_window);
}

static void
on_menu_open_recent_activate (GtkRecentChooser * chooser, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  GtkRecentInfo *info = NULL;
  gchar *file_name = NULL;
  GError *err = NULL;
  const gchar *uri;

  if (!(info = gtk_recent_chooser_get_current_item (chooser))) {
    GST_WARNING ("Unable to retrieve the current recent-item, aborting...");
    return;
  }

  if (!bt_main_window_check_unsaved_song (self->main_window,
          _("Load new song?"), _("Load new song?")))
    goto done;

  uri = gtk_recent_info_get_uri (info);
  file_name = g_filename_from_uri (uri, NULL, NULL);

  GST_INFO ("menu open recent event occurred : %s", file_name);

  if (!bt_edit_application_load_song (self->app, file_name, &err)) {
    gchar *msg = g_strdup_printf (_("Can't load song '%s'."), file_name);
    bt_dialog_message (self->main_window, _("Can't load song"), msg,
        err->message);
    g_free (msg);
    g_error_free (err);
  }
  g_free (file_name);
done:
  gtk_recent_info_unref (info);
}

static void
on_menu_save_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu save event occurred");
  bt_main_window_save_song (self->main_window);
}

static void
on_menu_saveas_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu saveas event occurred");
  bt_main_window_save_song_as (self->main_window);
}

static void
on_menu_recover_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu crash-log recoder event occurred");
  if (!bt_main_window_check_unsaved_song (self->main_window,
          _("Recover a song?"), _("Recover a song?")))
    return;

  /* replay expects an empty song */
  bt_edit_application_new_song (self->app);
  bt_edit_application_crash_log_recover (self->app);
}

static void
on_menu_recover_changed (const BtChangeLog * change_log, GParamSpec * arg,
    gpointer user_data)
{
  GtkWidget *menuitem = GTK_WIDGET (user_data);
  GList *crash_logs;

  g_object_get ((GObject *) change_log, "crash-logs", &crash_logs, NULL);
  if (!crash_logs) {
    gtk_widget_set_sensitive (GTK_WIDGET (menuitem), FALSE);
  }
}

static void
on_menu_render_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  GtkWidget *settings;

  GST_INFO ("menu render event occurred");
  settings = GTK_WIDGET (bt_render_dialog_new ());
  bt_edit_application_attach_child_window (self->app,
      GTK_WINDOW (settings));
  gtk_widget_show_all (settings);
  while (gtk_dialog_run (GTK_DIALOG (settings)) == GTK_RESPONSE_NONE);
  GST_INFO ("rendering done");
  gtk_widget_hide (settings);
  gtk_widget_destroy (settings);
}

static void
on_menu_quit_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu quit event occurred");
  if (bt_edit_application_quit (self->app)) {
    gtk_widget_destroy (GTK_WIDGET (self->main_window));
  }
}


static void
on_menu_undo_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_change_log_undo (self->change_log);
}

static void
on_menu_redo_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_change_log_redo (self->change_log);
}

// TODO(ensonic): call bt_main_pages_cut() and make that dispatch
static void
on_menu_cut_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (pages))) {
    case BT_MAIN_PAGES_MACHINES_PAGE:
      GST_INFO ("menu cut event occurred for machine page");
      break;
    case BT_MAIN_PAGES_PATTERNS_PAGE:{
      BtMainPagePatterns *page;
      GST_INFO ("menu cut event occurred for pattern page");
      g_object_get (pages, "patterns-page", &page, NULL);
      bt_main_page_patterns_cut_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_SEQUENCE_PAGE:{
      BtMainPageSequence *page;
      GST_INFO ("menu cut event occurred for sequence page");
      g_object_get (pages, "sequence-page", &page, NULL);
      bt_main_page_sequence_cut_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_WAVES_PAGE:
      GST_INFO ("menu cut event occurred for waves page");
      break;
    case BT_MAIN_PAGES_INFO_PAGE:
      GST_INFO ("menu cut event occurred for info page");
      break;
  }
  g_object_unref (pages);
}

static void
on_menu_copy_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (pages))) {
    case BT_MAIN_PAGES_MACHINES_PAGE:
      GST_INFO ("menu copy event occurred for machine page");
      break;
    case BT_MAIN_PAGES_PATTERNS_PAGE:{
      BtMainPagePatterns *page;
      GST_INFO ("menu copy event occurred for pattern page");
      g_object_get (pages, "patterns-page", &page, NULL);
      bt_main_page_patterns_copy_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_SEQUENCE_PAGE:{
      BtMainPageSequence *page;
      GST_INFO ("menu copy event occurred for sequence page");
      g_object_get (pages, "sequence-page", &page, NULL);
      bt_main_page_sequence_copy_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_WAVES_PAGE:
      GST_INFO ("menu copy event occurred for waves page");
      break;
    case BT_MAIN_PAGES_INFO_PAGE:
      GST_INFO ("menu copy event occurred for info page");
      break;
  }
  g_object_unref (pages);
}

static void
on_menu_paste_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (pages))) {
    case BT_MAIN_PAGES_MACHINES_PAGE:
      GST_INFO ("menu paste event occurred for machine page");
      break;
    case BT_MAIN_PAGES_PATTERNS_PAGE:{
      BtMainPagePatterns *page;
      GST_INFO ("menu paste event occurred for pattern page");
      g_object_get (pages, "patterns-page", &page, NULL);
      bt_main_page_patterns_paste_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_SEQUENCE_PAGE:{
      BtMainPageSequence *page;
      GST_INFO ("menu paste event occurred for sequence page");
      g_object_get (pages, "sequence-page", &page, NULL);
      bt_main_page_sequence_paste_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_WAVES_PAGE:
      GST_INFO ("menu paste event occurred for waves page");
      break;
    case BT_MAIN_PAGES_INFO_PAGE:
      GST_INFO ("menu paste event occurred for info page");
      break;
  }
  g_object_unref (pages);
}

static void
on_menu_delete_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (pages))) {
    case BT_MAIN_PAGES_MACHINES_PAGE:
      GST_INFO ("menu delete event occurred for machine page");
      break;
    case BT_MAIN_PAGES_PATTERNS_PAGE:{
      BtMainPagePatterns *page;
      GST_INFO ("menu delete event occurred for pattern page");
      g_object_get (pages, "patterns-page", &page, NULL);
      bt_main_page_patterns_delete_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_SEQUENCE_PAGE:{
      BtMainPageSequence *page;
      GST_INFO ("menu delete event occurred for sequence page");
      g_object_get (pages, "sequence-page", &page, NULL);
      bt_main_page_sequence_delete_selection (page);
      g_object_unref (page);
      break;
    }
    case BT_MAIN_PAGES_WAVES_PAGE:
      GST_INFO ("menu delete event occurred for waves page");
      break;
    case BT_MAIN_PAGES_INFO_PAGE:
      GST_INFO ("menu delete event occurred for info page");
      break;
  }
  g_object_unref (pages);
}

static void
on_menu_settings_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  GtkWidget *dialog;

  GST_INFO ("menu settings event occurred");
  dialog = GTK_WIDGET (bt_settings_dialog_new ());
  bt_edit_application_attach_child_window (self->app,
      GTK_WINDOW (dialog));
  gtk_widget_show_all (dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
on_menu_view_toolbar_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean shown =
      gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));

  GST_INFO ("menu 'view toolbar' event occurred");
  bt_child_proxy_set (self->app, "settings::toolbar-hide", !shown, NULL);
  bt_child_proxy_set (self->main_window, "toolbar::visible", shown, NULL);
}

static void
on_menu_view_statusbar_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean shown =
      gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));

  GST_INFO ("menu 'view statusbar' event occurred");
  bt_child_proxy_set (self->app, "settings::statusbar-hide", !shown,
      NULL);
  bt_child_proxy_set (self->main_window, "statusbar::visible", shown,
      NULL);
}

static void
on_menu_view_tabs_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean shown =
      gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));

  GST_INFO ("menu 'view tabs' event occurred");
  bt_child_proxy_set (self->app, "settings::tabs-hide", !shown, NULL);
  bt_child_proxy_set (self->main_window, "pages::show-tabs", shown, NULL);
}

static void
on_menu_fullscreen_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean fullscreen;

  /* IDEA(ensonic): reflow things a bit for full-screen:
   * - hide menu bar and have a menu-button on toolbar
   *   - we are a menu-bar, for this we would need to be a menu
   * - have a right justified label on toolbar to show window title
   *   - this both causes problems if toolbar is hidden!
   *   - so we have to check for it
   */
  fullscreen = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
  if (fullscreen) {
    gtk_window_fullscreen (GTK_WINDOW (self->main_window));
  } else {
    gtk_window_unfullscreen (GTK_WINDOW (self->main_window));
  }
}

static void
on_menu_goto_machine_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_MACHINES_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_pattern_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_PATTERNS_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_sequence_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_SEQUENCE_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_waves_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_WAVES_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_info_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages), BT_MAIN_PAGES_INFO_PAGE);
  g_object_unref (pages);
}

static void
on_menu_play_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtSong *song;

  // get song from app and start playback
  g_object_get (self->app, "song", &song, NULL);
  if (!bt_song_play (song)) {
    GST_WARNING ("failed to play");
  }
  // release the reference
  g_object_unref (song);
}

static void
on_menu_play_from_cursor_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtSong *song;
  glong pos;

  // get song from app and seek to cursor
  g_object_get (self->app, "song", &song, NULL);
  bt_child_proxy_get (self->main_window,
      "pages::sequence-page::cursor-row", &pos, NULL);
  g_object_set (song, "play-pos", pos, NULL);
  // play
  if (!bt_song_play (song)) {
    GST_WARNING ("failed to play");
  }
  // release the reference
  g_object_unref (song);
}

static void
on_menu_stop_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtSong *song;

  // get song from app and stop playback
  g_object_get (self->app, "song", &song, NULL);
  if (!bt_song_stop (song)) {
    GST_WARNING ("failed to stop");
  }
  // release the reference
  g_object_unref (song);
}

static void
on_menu_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  //BtMainMenu *self=BT_MAIN_MENU(user_data);
  GST_INFO ("menu help event occurred");

  // use "help:buzztrax-edit?topic" for context specific help
  gtk_show_uri_simple (GTK_WIDGET (menuitem), "help:buzztrax-edit");
}

static void
on_menu_join_irc (GtkMenuItem * menuitem, gpointer user_data)
{
  //BtMainMenu *self=BT_MAIN_MENU(user_data);
  gchar *uri;

  GST_INFO ("menu join-irc event occurred");

  //gtk_show_uri_simple(GTK_WIDGET(menuitem),"irc://irc.freenode.net/#buzztrax");
  uri =
      g_strdup_printf ("http://webchat.freenode.net?nick=%s&channels=#buzztrax",
      g_get_user_name ());
  gtk_show_uri_simple (GTK_WIDGET (menuitem), uri);
  g_free (uri);
}

static void
on_menu_report_problem (GtkMenuItem * menuitem, gpointer user_data)
{
  //BtMainMenu *self=BT_MAIN_MENU(user_data);
  GST_INFO ("menu submit_bug event occurred");

  gtk_show_uri_simple (GTK_WIDGET (menuitem), "http://buzztrax.org/bugs/?");
}

static void
on_menu_help_show_tip (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_edit_application_show_tip (self->app);
}

static void
on_menu_about_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_edit_application_show_about (self->app);
}

#ifdef USE_DEBUG
static void
on_menu_debug_show_media_types_toggled (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE;
  else
    self->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE;
}

static void
on_menu_debug_show_caps_details_types_toggled (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
  else
    self->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
}

static void
on_menu_debug_show_non_default_params_toggled (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS;
  else
    self->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS;
}

static void
on_menu_debug_show_states_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_STATES;
  else
    self->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_STATES;
}

static void
on_menu_debug_use_svg_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->debug_graph_format = "svg";
}

static void
on_menu_debug_use_png_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->debug_graph_format = "png";
}

static void
on_menu_debug_dump_pipeline_graph_and_show (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  const gchar *path;

  if ((path = g_getenv ("GST_DEBUG_DUMP_DOT_DIR"))) {
    GstBin *bin;
    gchar *cmd;
    GError *error = NULL;

    bt_child_proxy_get (self->app, "song::bin", &bin, NULL);

    GST_INFO_OBJECT (bin, "dump dot graph as %s with 0x%x details",
        self->debug_graph_format, self->debug_graph_details);

    GST_DEBUG_BIN_TO_DOT_FILE (bin, self->debug_graph_details,
        PACKAGE_NAME);

    // release the reference
    gst_object_unref (bin);

    // convert file
    cmd =
        g_strdup_printf ("dot -T%s -o%s" G_DIR_SEPARATOR_S "" PACKAGE_NAME
        ".%s %s" G_DIR_SEPARATOR_S "" PACKAGE_NAME ".dot",
        self->debug_graph_format, path, self->debug_graph_format,
        path);
    if (!g_spawn_command_line_sync (cmd, NULL, NULL, NULL, &error)) {
      GST_WARNING ("Failed to convert dot-graph: %s\n", error->message);
      g_error_free (error);
    } else {
      gchar *png_uri;

      png_uri =
          g_strdup_printf ("file://%s" G_DIR_SEPARATOR_S "" PACKAGE_NAME ".%s",
          path, self->debug_graph_format);
      gtk_show_uri_simple (GTK_WIDGET (menuitem), png_uri);
      g_free (png_uri);
    }
    g_free (cmd);
  } else {
    // the envvar is only checked at gst_init()
    GST_WARNING
        ("You need to start the app with GST_DEBUG_DUMP_DOT_DIR env-var set.");
  }
}

static void
on_menu_debug_update_registry (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_edit_application_ui_lock (self->app);

  /* we cannot reload plugins
   * I've tried to patch gst/gstregistry.c:gst_registry_scan_path_level
   * if (plugin->registered && !_priv_plugin_deps_files_changed (plugin)) ...
   * but reloading a plugin does not work, as we would end up re-registering
   * existing GTypes types.
   */
  if (!gst_update_registry ()) {
    GST_WARNING ("failed to update registry");
  } else {
    // TODO(ensonic): update machine menu
  }

  bt_edit_application_ui_unlock (self->app);
}
#endif // USE_DEBUG
#endif /// GTK4


GtkWidget *
bt_main_menu_new (BtEditApplication * app)
{
  BtMainMenu* self = g_object_new (BT_TYPE_MAIN_MENU, NULL);

  self->app = app;
  g_object_ref (app);

  gtk_widget_set_layout_manager (GTK_WIDGET (self),
      gtk_box_layout_new (GTK_ORIENTATION_HORIZONTAL));
  
  GtkBuilder* builder = gtk_builder_new_from_resource (
      "/org/buzztrax/ui/main-menu.ui");
  
  self->popover_menu = GTK_WIDGET (
      gtk_builder_get_object (builder, "popover_menu"));

  GMenuModel* menu = G_MENU_MODEL (gtk_builder_get_object (builder, "model"));
  gtk_popover_menu_bar_set_menu_model (
      GTK_POPOVER_MENU_BAR (self->popover_menu), menu);

  // Any manual uses of set_parent need to be undone in dispose.
  gtk_widget_set_parent (self->popover_menu, GTK_WIDGET (self));
  
#if 0 /// GTK4 finish implementation of recent files
  GMenuItem* recent_item = G_MENU_ITEM (
      gtk_builder_get_object (builder, "recent"));
  
  gtk_container_add (GTK_CONTAINER (menu), subitem);

  GtkRecentManager* recent = gtk_recent_manager_get_default ();
  
  item =
      gtk_recent_chooser_menu_new_for_manager (gtk_recent_manager_get_default
      ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (subitem), item);
  g_signal_connect (item, "item-activated",
      G_CALLBACK (on_menu_open_recent_activate), (gpointer) self);
  
  //gtk_recent_filter_add_application (filter, "buzztrax-edit");
  // set filters
  plugins = bt_song_io_get_module_info_list ();
  for (node = plugins; node; node = g_list_next (node)) {
    info = (BtSongIOModuleInfo *) node->data;
    ix = 0;
    while (info->formats[ix].name) {
      gtk_recent_filter_add_mime_type (filter, info->formats[ix].mime_type);
      ix++;
    }
  }
  gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (item), filter);
  gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (item), filter);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());
#endif

#if 0 /// GTK4 move to app actions, Actions have an "enabled" state.
  g_object_bind_property (self->change_log, "can-redo", subitem,
      "sensitive", G_BINDING_SYNC_CREATE);
  
  // register event handlers
  g_signal_connect (self->app, "notify::unsaved",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);

  g_signal_connect (self->change_log, "notify::can-undo",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);
#endif

  /// TBD
  GSimpleActionGroup* ag = g_simple_action_group_new ();
  const GActionEntry entries[] = {
//    { "app.file.new", on_context_menu_disconnect_activate, "s" },
//    { "app.file.open", on_context_menu_analysis_activate, "s" }
  };
  g_action_map_add_action_entries (G_ACTION_MAP (ag), entries, G_N_ELEMENTS (entries), NULL);

  g_object_unref (builder);
  
  return GTK_WIDGET (self);
}

static void
bt_main_menu_init (BtMainMenu * self)
{
  self->change_log = bt_change_log_new ();
  
#ifdef USE_DEBUG
  self->debug_graph_details =
      GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS | GST_DEBUG_GRAPH_SHOW_STATES;
  self->debug_graph_format = "svg";
#endif
}

void bt_main_menu_dispose (GObject *object)
{
  BtMainMenu* self = BT_MAIN_MENU (object);
  
  g_clear_object (&self->app);
  g_clear_object (&self->change_log);
  g_clear_pointer (&self->popover_menu, gtk_widget_unparent);
}

static void
bt_main_menu_class_init (BtMainMenuClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->dispose = bt_main_menu_dispose;
}
