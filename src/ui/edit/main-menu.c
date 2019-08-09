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

struct _BtMainMenuPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the main window */
  BtMainWindow *main_window;

  /* MenuItems */
  GtkWidget *save_item;

  /* editor change log */
  BtChangeLog *change_log;

#ifdef USE_DEBUG
  /* debug menu */
  GstDebugGraphDetails debug_graph_details;
  gchar *debug_graph_format;
#endif
};

//-- the class

G_DEFINE_TYPE (BtMainMenu, bt_main_menu, GTK_TYPE_MENU_BAR);

//-- event handler

static void
on_menu_new_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu new event occurred");
  bt_main_window_new_song (self->priv->main_window);
}

static void
on_menu_open_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu open event occurred");
  bt_main_window_open_song (self->priv->main_window);
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

  if (!bt_main_window_check_unsaved_song (self->priv->main_window,
          _("Load new song?"), _("Load new song?")))
    goto done;

  uri = gtk_recent_info_get_uri (info);
  file_name = g_filename_from_uri (uri, NULL, NULL);

  GST_INFO ("menu open recent event occurred : %s", file_name);

  if (!bt_edit_application_load_song (self->priv->app, file_name, &err)) {
    gchar *msg = g_strdup_printf (_("Can't load song '%s'."), file_name);
    bt_dialog_message (self->priv->main_window, _("Can't load song"), msg,
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
  bt_main_window_save_song (self->priv->main_window);
}

static void
on_menu_saveas_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu saveas event occurred");
  bt_main_window_save_song_as (self->priv->main_window);
}

static void
on_menu_recover_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  GST_INFO ("menu crash-log recoder event occurred");
  if (!bt_main_window_check_unsaved_song (self->priv->main_window,
          _("Recover a song?"), _("Recover a song?")))
    return;

  /* replay expects an empty song */
  bt_edit_application_new_song (self->priv->app);
  bt_edit_application_crash_log_recover (self->priv->app);
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
  bt_edit_application_attach_child_window (self->priv->app,
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
  if (bt_edit_application_quit (self->priv->app)) {
    gtk_widget_destroy (GTK_WIDGET (self->priv->main_window));
  }
}


static void
on_menu_undo_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_change_log_undo (self->priv->change_log);
}

static void
on_menu_redo_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_change_log_redo (self->priv->change_log);
}

// TODO(ensonic): call bt_main_pages_cut() and make that dispatch
static void
on_menu_cut_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
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

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
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

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
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

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
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
  bt_edit_application_attach_child_window (self->priv->app,
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
  bt_child_proxy_set (self->priv->app, "settings::toolbar-hide", !shown, NULL);
  bt_child_proxy_set (self->priv->main_window, "toolbar::visible", shown, NULL);
}

static void
on_menu_view_statusbar_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean shown =
      gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));

  GST_INFO ("menu 'view statusbar' event occurred");
  bt_child_proxy_set (self->priv->app, "settings::statusbar-hide", !shown,
      NULL);
  bt_child_proxy_set (self->priv->main_window, "statusbar::visible", shown,
      NULL);
}

static void
on_menu_view_tabs_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean shown =
      gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));

  GST_INFO ("menu 'view tabs' event occurred");
  bt_child_proxy_set (self->priv->app, "settings::tabs-hide", !shown, NULL);
  bt_child_proxy_set (self->priv->main_window, "pages::show-tabs", shown, NULL);
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
    gtk_window_fullscreen (GTK_WINDOW (self->priv->main_window));
  } else {
    gtk_window_unfullscreen (GTK_WINDOW (self->priv->main_window));
  }
}

static void
on_menu_goto_machine_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_MACHINES_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_pattern_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_PATTERNS_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_sequence_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_SEQUENCE_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_waves_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_WAVES_PAGE);
  g_object_unref (pages);
}

static void
on_menu_goto_info_view_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtMainPages *pages;

  g_object_get (self->priv->main_window, "pages", &pages, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages), BT_MAIN_PAGES_INFO_PAGE);
  g_object_unref (pages);
}

static void
on_menu_play_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  BtSong *song;

  // get song from app and start playback
  g_object_get (self->priv->app, "song", &song, NULL);
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
  g_object_get (self->priv->app, "song", &song, NULL);
  bt_child_proxy_get (self->priv->main_window,
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
  g_object_get (self->priv->app, "song", &song, NULL);
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

  bt_edit_application_show_tip (self->priv->app);
}

static void
on_menu_about_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  bt_edit_application_show_about (self->priv->app);
}

#ifdef USE_DEBUG
static void
on_menu_debug_show_media_types_toggled (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->priv->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE;
  else
    self->priv->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE;
}

static void
on_menu_debug_show_caps_details_types_toggled (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->priv->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
  else
    self->priv->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
}

static void
on_menu_debug_show_non_default_params_toggled (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->priv->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS;
  else
    self->priv->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS;
}

static void
on_menu_debug_show_states_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->priv->debug_graph_details |= GST_DEBUG_GRAPH_SHOW_STATES;
  else
    self->priv->debug_graph_details &= ~GST_DEBUG_GRAPH_SHOW_STATES;
}

static void
on_menu_debug_use_svg_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->priv->debug_graph_format = "svg";
}

static void
on_menu_debug_use_png_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    self->priv->debug_graph_format = "png";
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

    bt_child_proxy_get (self->priv->app, "song::bin", &bin, NULL);

    GST_INFO_OBJECT (bin, "dump dot graph as %s with 0x%x details",
        self->priv->debug_graph_format, self->priv->debug_graph_details);

    GST_DEBUG_BIN_TO_DOT_FILE (bin, self->priv->debug_graph_details,
        PACKAGE_NAME);

    // release the reference
    gst_object_unref (bin);

    // convert file
    cmd =
        g_strdup_printf ("dot -T%s -o%s" G_DIR_SEPARATOR_S "" PACKAGE_NAME
        ".%s %s" G_DIR_SEPARATOR_S "" PACKAGE_NAME ".dot",
        self->priv->debug_graph_format, path, self->priv->debug_graph_format,
        path);
    if (!g_spawn_command_line_sync (cmd, NULL, NULL, NULL, &error)) {
      GST_WARNING ("Failed to convert dot-graph: %s\n", error->message);
      g_error_free (error);
    } else {
      gchar *png_uri;

      png_uri =
          g_strdup_printf ("file://%s" G_DIR_SEPARATOR_S "" PACKAGE_NAME ".%s",
          path, self->priv->debug_graph_format);
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

  bt_edit_application_ui_lock (self->priv->app);

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

  bt_edit_application_ui_unlock (self->priv->app);
}
#endif


static void
on_song_unsaved_changed (const GObject * object, GParamSpec * arg,
    gpointer user_data)
{
  BtMainMenu *self = BT_MAIN_MENU (user_data);
  gboolean unsaved = bt_edit_application_is_song_unsaved (self->priv->app);

  gtk_widget_set_sensitive (self->priv->save_item, unsaved);
}

//-- helper methods

static void
bt_main_menu_init_ui (const BtMainMenu * self)
{
  GtkWidget *item, *menu, *subitem;
#ifdef USE_DEBUG
  GtkWidget *submenu;
#endif
  BtSettings *settings;
  gboolean toolbar_hide, statusbar_hide, tabs_hide;
  GtkAccelGroup *accel_group = bt_ui_resources_get_accel_group ();
  GtkSettings *gtk_settings;
  GtkRecentFilter *filter = gtk_recent_filter_new ();
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  guint ix;

  // disable F10 keybinding to activate the menu
  gtk_settings = gtk_settings_get_for_screen (gdk_screen_get_default ());
  g_object_set (gtk_settings, "gtk-menu-bar-accel", NULL, NULL);

  gtk_widget_set_name (GTK_WIDGET (self), "main menu");
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings,
      "toolbar-hide", &toolbar_hide,
      "statusbar-hide", &statusbar_hide, "tabs-hide", &tabs_hide, NULL);
  g_object_unref (settings);
  //gtk_menu_set_accel_path(GTK_MENU(self),"<Buzztrax-Main>/MainMenu");

  //-- file menu
  item = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_container_add (GTK_CONTAINER (self), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_menu_set_accel_path (GTK_MENU (menu), "<Buzztrax-Main>/MainMenu/File");

  subitem = gtk_menu_item_new_with_mnemonic (_("_New"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/File/New", GDK_KEY_n, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_new_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Open"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/File/Open", GDK_KEY_o, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_open_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Recently used"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);

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

  subitem = gtk_menu_item_new_with_mnemonic (_("_Save"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/File/Save", GDK_KEY_s, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_save_activate),
      (gpointer) self);
  self->priv->save_item = subitem;

  subitem = gtk_menu_item_new_with_mnemonic (_("Save _As"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/File/SaveAs", GDK_KEY_s,
      GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_saveas_activate),
      (gpointer) self);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_label (_("Recover unsaved songs"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_recover_activate),
      (gpointer) self);
  on_menu_recover_changed (self->priv->change_log, NULL, subitem);
  g_signal_connect (self->priv->change_log, "notify::crash-logs",
      G_CALLBACK (on_menu_recover_changed), (gpointer) subitem);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_mnemonic (_("_Record"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_render_activate),
      (gpointer) self);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_mnemonic (_("_Quit"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/File/Quit", GDK_KEY_q, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_quit_activate),
      (gpointer) self);

  // edit menu
  item = gtk_menu_item_new_with_mnemonic (_("_Edit"));
  gtk_container_add (GTK_CONTAINER (self), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_menu_set_accel_path (GTK_MENU (menu), "<Buzztrax-Main>/MainMenu/Edit");

  subitem = gtk_menu_item_new_with_mnemonic (_("_Undo"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Edit/Undo", GDK_KEY_z, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_undo_activate),
      (gpointer) self);
  g_object_bind_property (self->priv->change_log, "can-undo", subitem,
      "sensitive", G_BINDING_SYNC_CREATE);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Redo"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Edit/Redo", GDK_KEY_z,
      GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_redo_activate),
      (gpointer) self);
  g_object_bind_property (self->priv->change_log, "can-redo", subitem,
      "sensitive", G_BINDING_SYNC_CREATE);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_mnemonic (_("Cu_t"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Edit/Cut", GDK_KEY_x, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_cut_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Copy"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Edit/Copy", GDK_KEY_c, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_copy_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Paste"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Edit/Paste", GDK_KEY_v, GDK_CONTROL_MASK);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_paste_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Delete"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_delete_activate),
      (gpointer) self);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_mnemonic (_("_Preferences"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_settings_activate),
      (gpointer) self);

  // view menu
  item = gtk_menu_item_new_with_mnemonic (_("_View"));
  gtk_container_add (GTK_CONTAINER (self), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_menu_set_accel_path (GTK_MENU (menu), "<Buzztrax-Main>/MainMenu/View");

  subitem = gtk_check_menu_item_new_with_mnemonic (_("Toolbar"));
  // from here we can't hide the toolbar as it is not yet created and shown
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), !toolbar_hide);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "toggled",
      G_CALLBACK (on_menu_view_toolbar_toggled), (gpointer) self);

  subitem = gtk_check_menu_item_new_with_mnemonic (_("Statusbar"));
  // from here we can't hide the statusbar as it is not yet created and shown
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem),
      !statusbar_hide);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "toggled",
      G_CALLBACK (on_menu_view_statusbar_toggled), (gpointer) self);

  subitem = gtk_check_menu_item_new_with_mnemonic (_("Tabs"));
  // from here we can't hide the tabs as they are not yet created and shown
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), !tabs_hide);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "toggled", G_CALLBACK (on_menu_view_tabs_toggled),
      (gpointer) self);

  /* TODO(ensonic): 'Machine properties' show/hide toggle */
  /* TODO(ensonic): 'Analyzer windows' show/hide toggle */

  subitem = gtk_check_menu_item_new_with_mnemonic (_("Fullscreen"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/View/FullScreen", GDK_KEY_F11, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "toggled", G_CALLBACK (on_menu_fullscreen_toggled),
      (gpointer) self);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_label (_("Go to machine view"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/View/MachineView", GDK_KEY_F3, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_goto_machine_view_activate), (gpointer) self);

  subitem = gtk_menu_item_new_with_label (_("Go to pattern view"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/View/PatternView", GDK_KEY_F2, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_goto_pattern_view_activate), (gpointer) self);

  subitem = gtk_menu_item_new_with_label (_("Go to sequence view"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/View/SequenceView", GDK_KEY_F4, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_goto_sequence_view_activate), (gpointer) self);

  subitem = gtk_menu_item_new_with_label (_("Go to wave table view"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/View/WaveteableView", GDK_KEY_F9, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_goto_waves_view_activate), (gpointer) self);

  subitem = gtk_menu_item_new_with_label (_("Go to song information"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/View/InfoView", GDK_KEY_F10, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_goto_info_view_activate), (gpointer) self);

  /* TODO(ensonic): zoom menu items
   *  machine view:  zoom-in/zoom-out/zoom-fit
   *  sequence vide: zoom-in/zoom-out
   *

   gtk_container_add(GTK_CONTAINER(menu),gtk_separator_menu_item_new());

   subitem=gtk_menu_item_new_with_mnemonic(_)"Best _Fit"));
   gtk_container_add(GTK_CONTAINER(menu),subitem);
   g_signal_connect(subitem,"activate",G_CALLBACK(on_menu_zoom_fit_activate),(gpointer)self);

   subitem=gtk_menu_item_new_with_mnemonic(_("Zoom _In"));
   gtk_container_add(GTK_CONTAINER(menu),subitem);
   g_signal_connect(subitem,"activate",G_CALLBACK(on_menu_zoom_in_activate),(gpointer)self);

   subitem=gtk_menu_item_new_with_mnemonic(_("Zoom _Out"));
   gtk_container_add(GTK_CONTAINER(menu),subitem);
   g_signal_connect(subitem,"activate",G_CALLBACK(on_menu_zoom_out_activate),(gpointer)self);
   */

  // playback menu
  item = gtk_menu_item_new_with_mnemonic (_("_Playback"));
  gtk_container_add (GTK_CONTAINER (self), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_menu_set_accel_path (GTK_MENU (menu),
      "<Buzztrax-Main>/MainMenu/Playback");

  subitem = gtk_menu_item_new_with_mnemonic (_("_Play"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Playback/Play", GDK_KEY_F5, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_play_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_label (_("Play from cursor"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Playback/PlayFromCursor", GDK_KEY_F6, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_play_from_cursor_activate), (gpointer) self);

  subitem = gtk_menu_item_new_with_mnemonic (_("_Stop"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Playback/Stop", GDK_KEY_F8, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_stop_activate),
      (gpointer) self);

  /* TODO(ensonic): toggle loop item
   *   gtk_check_menu_item_new_with_mnemonic
   */

  /* TODO(ensonic): tools menu
   * 'normalize song'
   * - dummy render with master->input-pre-gain, adjust master volume
   * 'cleanup'
   * - remove unsused patterns, unconnected machines with no empty/tracks
   */

  // help menu
  item = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_container_add (GTK_CONTAINER (self), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_menu_set_accel_path (GTK_MENU (menu), "<Buzztrax-Main>/MainMenu/Help");

  subitem = gtk_menu_item_new_with_label (_("Help"));
  gtk_menu_item_add_accel (GTK_MENU_ITEM (subitem),
      "<Buzztrax-Main>/MainMenu/Help/Help", GDK_KEY_F1, 0);
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_help_activate),
      (gpointer) self);

  subitem = gtk_menu_item_new_with_label (_("Tip of the day"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_help_show_tip),
      (gpointer) self);

  /* join web-chat */
  subitem = gtk_menu_item_new_with_label (_("Join IRC discussion"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_join_irc),
      (gpointer) self);

  /* TODO(ensonic): 'translate application' -> link to translator project
   * liblaunchpad-integration1:/usr/share/icons/hicolor/16x16/apps/lpi-translate.png
   */

  /* 'report a problem' -> link to bug tracker
   * liblaunchpad-integration1:/usr/share/icons/hicolor/16x16/apps/lpi-bug.png
   * /usr/share/app-install/icons/bug-buddy.png
   */
  subitem = gtk_menu_item_new_with_label (_("Report a problem"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_report_problem),
      (gpointer) self);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_mnemonic (_("_About"));
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate", G_CALLBACK (on_menu_about_activate),
      (gpointer) self);

#ifdef USE_DEBUG
  // debug menu
  item = gtk_menu_item_new_with_label ("Debug");
  gtk_container_add (GTK_CONTAINER (self), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

  /* TODO(ensonic): add toggle for keep image (no by default) */
  item = gtk_menu_item_new_with_mnemonic ("Graph details");
  gtk_container_add (GTK_CONTAINER (menu), item);
  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

  subitem = gtk_check_menu_item_new_with_mnemonic ("show media types");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), FALSE);
  gtk_container_add (GTK_CONTAINER (submenu), subitem);
  g_signal_connect (subitem, "toggled",
      G_CALLBACK (on_menu_debug_show_media_types_toggled), (gpointer) self);

  subitem = gtk_check_menu_item_new_with_mnemonic ("show caps details");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), TRUE);
  gtk_container_add (GTK_CONTAINER (submenu), subitem);
  g_signal_connect (subitem, "toggled",
      G_CALLBACK (on_menu_debug_show_caps_details_types_toggled),
      (gpointer) self);

  subitem = gtk_check_menu_item_new_with_mnemonic ("show non default params");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), FALSE);
  gtk_container_add (GTK_CONTAINER (submenu), subitem);
  g_signal_connect (subitem, "toggled",
      G_CALLBACK (on_menu_debug_show_non_default_params_toggled),
      (gpointer) self);

  subitem = gtk_check_menu_item_new_with_mnemonic ("show states");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), TRUE);
  gtk_container_add (GTK_CONTAINER (submenu), subitem);
  g_signal_connect (subitem, "toggled",
      G_CALLBACK (on_menu_debug_show_states_toggled), (gpointer) self);

  item = gtk_menu_item_new_with_mnemonic ("Graph format");
  gtk_container_add (GTK_CONTAINER (menu), item);
  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

  {
    GSList *group = NULL;

    subitem = gtk_radio_menu_item_new_with_mnemonic (group, "use svg");
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (subitem));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), TRUE);
    gtk_container_add (GTK_CONTAINER (submenu), subitem);
    g_signal_connect (subitem, "toggled",
        G_CALLBACK (on_menu_debug_use_svg_toggled), (gpointer) self);

    subitem = gtk_radio_menu_item_new_with_mnemonic (group, "use png");
    //group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(subitem));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (subitem), FALSE);
    gtk_container_add (GTK_CONTAINER (submenu), subitem);
    g_signal_connect (subitem, "toggled",
        G_CALLBACK (on_menu_debug_use_png_toggled), (gpointer) self);
  }

  subitem = gtk_menu_item_new_with_mnemonic ("Dump pipeline graph and show");
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_debug_dump_pipeline_graph_and_show), (gpointer) self);

  gtk_container_add (GTK_CONTAINER (menu), gtk_separator_menu_item_new ());

  subitem = gtk_menu_item_new_with_mnemonic ("Update plugin registry");
  gtk_container_add (GTK_CONTAINER (menu), subitem);
  g_signal_connect (subitem, "activate",
      G_CALLBACK (on_menu_debug_update_registry), (gpointer) self);
#endif

  // register event handlers
  g_signal_connect (self->priv->app, "notify::unsaved",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);

  g_signal_connect (self->priv->change_log, "notify::can-undo",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);
}

//-- constructor methods

/**
 * bt_main_menu_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainMenu *
bt_main_menu_new (void)
{
  BtMainMenu *self;

  self = BT_MAIN_MENU (g_object_new (BT_TYPE_MAIN_MENU, NULL));
  bt_main_menu_init_ui (self);
  return self;
}

//-- methods

//-- class internals

static void
bt_main_menu_map (GtkWidget * widget)
{
  BtMainMenu *self = BT_MAIN_MENU (widget);
  GtkWidget *toplevel;

  GTK_WIDGET_CLASS (bt_main_menu_parent_class)->map (widget);

  /* bah, it is still NULL here becasue of the construction sequence */
  //g_object_get(self->priv->app,"main-window",&self->priv->main_window,NULL);
  //GST_DEBUG("main-window = %p",self->priv->main_window);

  toplevel = gtk_widget_get_toplevel (widget);
  if (gtk_widget_is_toplevel (toplevel)) {
    self->priv->main_window = BT_MAIN_WINDOW (toplevel);
    GST_DEBUG ("top-level-window = %p", toplevel);
  }
}

static void
bt_main_menu_dispose (GObject * object)
{
  BtMainMenu *self = BT_MAIN_MENU (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  self->priv->main_window = NULL;

  g_object_unref (self->priv->change_log);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_main_menu_parent_class)->dispose (object);
}

static void
bt_main_menu_init (BtMainMenu * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MAIN_MENU, BtMainMenuPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();

  self->priv->change_log = bt_change_log_new ();

#ifdef USE_DEBUG
  self->priv->debug_graph_details =
      GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS | GST_DEBUG_GRAPH_SHOW_STATES;
  self->priv->debug_graph_format = "svg";
#endif
}

static void
bt_main_menu_class_init (BtMainMenuClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtMainMenuPrivate));

  gobject_class->dispose = bt_main_menu_dispose;

  widget_class->map = bt_main_menu_map;
}
