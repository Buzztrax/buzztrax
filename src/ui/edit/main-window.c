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
 * SECTION:btmainwindow
 * @short_description: root buzztrax editor window
 *
 * The main window class is a container for the #BtMainMenu, the #BtMainToolbar,
 * the #BtMainStatusbar and the #BtMainPages tabbed notebook.
 */

#define BT_EDIT
#define BT_MAIN_WINDOW_C

#include "bt-edit.h"

enum
{
  MAIN_WINDOW_TOOLBAR = 1,
  MAIN_WINDOW_STATUSBAR,
  MAIN_WINDOW_PAGES,
  MAIN_WINDOW_DIALOG
};


struct _BtMainWindowPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the menu of the window */
  BtMainMenu *menu;
  /* the toolbar of the window */
  BtMainToolbar *toolbar;
  /* the content pages of the window */
  BtMainPages *pages;
  /* the statusbar of the window */
  BtMainStatusbar *statusbar;
  /* active dialog */
  GtkDialog *dialog;

  /* file-chooser stuff */
  GtkFileChooser *file_chooser;
  GList *filters;
  gchar *last_folder;
};

enum
{ TARGET_URI_LIST };
static GtkTargetEntry drop_types[] = {
  {"text/uri-list", 0, TARGET_URI_LIST}
};

static gint n_drop_types = sizeof (drop_types) / sizeof (GtkTargetEntry);

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMainWindow, bt_main_window, GTK_TYPE_WINDOW, 
    G_ADD_PRIVATE(BtMainWindow));


//-- helper methods

/* update the filename to have an extension, matching the selected format */
static gchar *
update_filename_ext (const BtMainWindow * self, gchar * file_name,
    gint format_ix)
{
  GtkFileFilter *this_filter, *that_filter;
  gchar *new_file_name = NULL;
  gchar *ext;
  const GList *plugins, *pnode, *fnode;
  BtSongIOModuleInfo *info;
  guint ix;

  plugins = bt_song_io_get_module_info_list ();
  //this_filter=gtk_file_chooser_get_filter(self->priv->file_chooser);
  this_filter = g_list_nth_data (self->priv->filters, format_ix);

  GST_WARNING ("old song name = '%s', filter %p", file_name, this_filter);

  ext = strrchr (file_name, '.');

  if (ext) {
    ext++;
    // cut off known extensions
    for (pnode = plugins, fnode = self->priv->filters; pnode;
        pnode = g_list_next (pnode)) {
      info = (BtSongIOModuleInfo *) pnode->data;
      ix = 0;
      while (info->formats[ix].name) {
        that_filter = fnode->data;

        if ((this_filter != that_filter)
            && !strcmp (ext, info->formats[ix].extension)) {
          file_name[strlen (file_name) - (strlen (info->formats[ix].extension) +
                  1)] = '\0';
          GST_INFO ("cut fn to: %s", file_name);
          pnode = NULL;
          break;
        }
        fnode = g_list_next (fnode);
        ix++;
      }
    }
  }
  // append new extension
  for (pnode = plugins, fnode = self->priv->filters; pnode;
      pnode = g_list_next (pnode)) {
    info = (BtSongIOModuleInfo *) pnode->data;
    ix = 0;
    while (info->formats[ix].name) {
      that_filter = fnode->data;

      if ((this_filter == that_filter) && (!ext
              || strcmp (ext, info->formats[ix].extension))) {
        new_file_name =
            g_strdup_printf ("%s.%s", file_name, info->formats[ix].extension);
        pnode = NULL;
        break;
      }
      fnode = g_list_next (fnode);
      ix++;
    }
  }

  if (new_file_name && strcmp (file_name, new_file_name)) {
    GST_WARNING ("new song name = '%s'", new_file_name);
    return new_file_name;
  }
  g_free (new_file_name);
  return NULL;
}

static gboolean
load_song (const BtMainWindow * self, gchar * file_name)
{
  GError *err = NULL;

  if (!bt_edit_application_load_song (self->priv->app, file_name, &err)) {
    gchar *msg = g_strdup_printf (_("Can't load song '%s'."), file_name);
    bt_dialog_message (self, _("Can't load song"), msg, err->message);
    g_free (msg);
    g_error_free (err);
    return FALSE;
  } else {
    // store recent file
    GtkRecentManager *manager = gtk_recent_manager_get_default ();
    gchar *uri = g_filename_to_uri (file_name, NULL, NULL);

    if (!gtk_recent_manager_add_item (manager, uri)) {
      GST_WARNING ("Can't store recent file");
    }
    g_free (uri);
  }
  return TRUE;
}

//-- event handler

static void
on_format_chooser_changed (GtkComboBox * menu, gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  gint format_ix;
  gchar *file_name, *new_file_name = NULL;

  format_ix = gtk_combo_box_get_active (menu);
  file_name = gtk_file_chooser_get_filename (self->priv->file_chooser);
  GST_WARNING ("Changing %s to extension for %d's filter", file_name,
      format_ix);
  if (!file_name)
    return;

  if ((new_file_name = update_filename_ext (self, file_name, format_ix))) {
    gchar *name;

    name = strrchr (new_file_name, G_DIR_SEPARATOR);
    //gtk_file_chooser_set_filename(self->priv->file_chooser,new_file_name);
    gtk_file_chooser_set_current_name (self->priv->file_chooser,
        name ? &name[1] : new_file_name);
  }
  g_free (file_name);
  g_free (new_file_name);
}

static void
on_window_mapped (GtkWidget * widget, gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  BtSettings *settings;
  gboolean toolbar_hide, statusbar_hide, tabs_hide;

  GST_INFO ("main_window mapped");

  // eventualy hide some ui elements
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings,
      "toolbar-hide", &toolbar_hide,
      "statusbar-hide", &statusbar_hide, "tabs-hide", &tabs_hide, NULL);
  g_object_unref (settings);

  if (toolbar_hide) {
    gtk_widget_hide (GTK_WIDGET (self->priv->toolbar));
  }
  if (statusbar_hide) {
    gtk_widget_hide (GTK_WIDGET (self->priv->statusbar));
  }
  if (tabs_hide) {
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (self->priv->pages), FALSE);
  }
}

static gboolean
on_window_delete_event (GtkWidget * widget, GdkEvent * event,
    gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);

  GST_INFO ("delete event occurred");
  // returning TRUE means, we don't want the window to be destroyed
  return !bt_edit_application_quit (self->priv->app);
}

static void
on_window_destroy (GtkWidget * widget, gpointer user_data)
{
  GST_INFO ("destroy occurred");
  if (gtk_main_level ()) {
    GST_INFO ("  leaving main-loop");
    gtk_main_quit ();
  }
}

static void
on_song_unsaved_changed (const GObject * object, GParamSpec * arg,
    gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  gboolean unsaved = bt_edit_application_is_song_unsaved (self->priv->app);
  BtSong *song;
  gchar *title, *name;

  // compose title
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song)
    return;

  bt_child_proxy_get (song, "song-info::name", &name, NULL);
  // we don't use PACKAGE_NAME = 'buzztrax' for the window title
  title =
      g_strdup_printf ("%s (%s) - Buzztrax", name,
      (unsaved ? _("unsaved") : _("saved")));
  g_free (name);
  gtk_window_set_title (GTK_WINDOW (self), title);
  g_free (title);
  //-- release the references
  g_object_unref (song);
}

static void
on_window_dnd_drop (GtkWidget * widget, GdkDragContext * dc, gint x, gint y,
    GtkSelectionData * selection_data, guint info, guint t, gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  glong i = 0;
  gchar *data = (gchar *) gtk_selection_data_get_data (selection_data);
  gchar *ptr = data;

  GST_INFO ("something has been dropped on our app: window=%p data='%s'",
      user_data, data);
  // find first \0 or \n or \r
  while ((*ptr) && (*ptr != '\n') && (*ptr != '\r')) {
    ptr++;
    i++;
  }
  if (i) {
    gchar *file_name = g_strndup (data, i);
    gtk_drag_finish (dc, load_song (self, file_name), FALSE, t);
    g_free (file_name);
  }
}

/* just for testing
static gboolean on_window_event(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
  if(event->type==GDK_BUTTON_PRESS) {
    GdkEventButton *e=(GdkEventButton*)event;

    GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",e->type,e->window,e->send_event,e->time);
    GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",e->x,e->y,e->axes,e->state);
    GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",e->button,e->device,e->x_root,e->y_root);
  }
  return(FALSE);
}
*/

static gchar *
bt_main_window_make_unsaved_changes_message (const BtSong * song)
{
  BtSongInfo *song_info;
  gchar *msg, *file_name, *since;
  const gchar *dts;
  gint td, tds, tdm, tdh;

  g_object_get ((GObject *) song, "song-info", &song_info, NULL);
  g_object_get (song_info, "file-name", &file_name, NULL);

  td = bt_song_info_get_seconds_since_last_saved (song_info);
  GST_LOG ("time passed since saved/created: td=%d", td);
  // pretty print how much time passed since saved/created and now
  tdh = td / (60 * 60);
  td -= tdh * (60 * 60);
  tdm = td / 60;
  td -= tdm * 60;
  tds = td;
  // unfortunately ngettext does not support multiple plural words in one sentence
  if (tdh != 0) {
    since =
        g_strdup_printf (_("%d %s and %d %s"), tdh,
        (tdh == 1) ? _("hour") : _("hours"), tdm,
        (tdm == 1) ? _("minute") : _("minutes"));
  } else if (tdm != 0) {
    since =
        g_strdup_printf (_("%d %s and %d %s"), tdm,
        (tdm == 1) ? _("minute") : _("minutes"), tds,
        (tds == 1) ? _("second") : _("seconds"));
  } else {
    since =
        g_strdup_printf (_("%d %s"), tds,
        (tds == 1) ? _("second") : _("seconds"));
  }

  // arguments are the time passed as e.g. in " 5 seconds" and the last saved, created time
  dts = bt_song_info_get_change_dts_in_local_tz (song_info);
  if (file_name)
    msg =
        g_strdup_printf (_
        ("All unsaved changes since %s will be lost. This song was last saved on: %s"),
        since, dts);
  else
    msg =
        g_strdup_printf (_
        ("All unsaved changes since %s will be lost. This song was created on: %s"),
        since, dts);

  g_free (file_name);
  g_free (since);
  g_object_unref (song_info);

  return msg;
}

//-- helper methods

static void
bt_main_window_init_ui (const BtMainWindow * self)
{
  GtkWidget *box;
  BtChangeLog *change_log;

  gtk_widget_set_name (GTK_WIDGET (self), "main window");
  gtk_window_set_role (GTK_WINDOW (self), "buzztrax-edit::main");
#if !GTK_CHECK_VERSION (3,14,0)
  gtk_window_set_has_resize_grip (GTK_WINDOW (self), FALSE);
#endif

  // create and set window icon
  gtk_window_set_icon_name (GTK_WINDOW (self), "buzztrax");

  gtk_window_add_accel_group (GTK_WINDOW (self),
      bt_ui_resources_get_accel_group ());

  // create main layout container
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), box);

  GST_INFO ("before creating content, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->app));

  // add the menu-bar
  self->priv->menu = bt_main_menu_new ();
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (self->priv->menu), FALSE,
      FALSE, 0);
  // add the tool-bar
  self->priv->toolbar = bt_main_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (self->priv->toolbar), FALSE,
      FALSE, 0);
  // add the window content pages
  self->priv->pages = bt_main_pages_new ();
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (self->priv->pages), TRUE, TRUE,
      0);
  // add the status bar
  self->priv->statusbar = bt_main_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (self->priv->statusbar), FALSE,
      FALSE, 0);

  gtk_drag_dest_set (GTK_WIDGET (self),
      (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
          GTK_DEST_DEFAULT_DROP), drop_types, n_drop_types, GDK_ACTION_COPY);
  g_signal_connect ((gpointer) self, "drag-data-received",
      G_CALLBACK (on_window_dnd_drop), (gpointer) self);

  GST_INFO ("content created, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->app));

  g_signal_connect ((gpointer) self, "delete-event",
      G_CALLBACK (on_window_delete_event), (gpointer) self);
  g_signal_connect ((gpointer) self, "destroy", G_CALLBACK (on_window_destroy),
      (gpointer) self);
  /* just for testing
     g_signal_connect((gpointer)self,"event",G_CALLBACK(on_window_event),(gpointer)self);
   */
  g_signal_connect (self->priv->app, "notify::unsaved",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);

  change_log = bt_change_log_new ();
  g_signal_connect (change_log, "notify::can-undo",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);
  g_object_unref (change_log);

  GST_INFO ("signal connected, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->app));
}

//-- constructor methods

/**
 * bt_main_window_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainWindow *
bt_main_window_new (void)
{
  BtMainWindow *self;
  BtSettings *settings;
  gint x, y, w, h;

  GST_INFO ("creating a new window");

  self =
      BT_MAIN_WINDOW (g_object_new (BT_TYPE_MAIN_WINDOW, "type",
          GTK_WINDOW_TOPLEVEL, NULL));
  GST_INFO ("new main_window created");
  bt_main_window_init_ui (self);
  GST_INFO ("new main_window layouted");

  // restore last position
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings,
      "window-xpos", &x, "window-ypos", &y, "window-width", &w, "window-height",
      &h, NULL);
  g_object_unref (settings);

  // use position from settings
  if (w >= 0 && h >= 0) {
    /* ensure that we can see the window - would also need to check against
     * bt_gtk_workarea_size(). Also as it seems the position is ignored
     if(x<w) x=0;
     if(y<h) y=0;
     */
    gtk_window_move (GTK_WINDOW (self), x, y);
    gtk_window_set_default_size (GTK_WINDOW (self), w, h);
  } else {
    gtk_window_resize (GTK_WINDOW (self), 800, 600);
  }

  g_signal_connect ((gpointer) self, "map",
      G_CALLBACK (on_window_mapped), (gpointer) self);

  GST_INFO ("new main_window created");
  return self;
}

//-- methods

/**
 * bt_main_window_check_unsaved_song:
 * @self: the main window instance
 * @title: the title of the message
 * @headline: the bold headline of the message
 *
 * Checks if the current song is modified and asks for confirmation to continue
 * (and loose the changes). It only considers the undo/redo stack and not minor
 * changes such as switching tabs or selecting something.
 *
 * Returns: %TRUE if the user has confirmed to continue
 */
gboolean
bt_main_window_check_unsaved_song (const BtMainWindow * self,
    const gchar * title, const gchar * headline)
{
  gboolean res = TRUE;
  BtSong *song;

  g_object_get (self->priv->app, "song", &song, NULL);
  if (song) {
    BtChangeLog *change_log;
    gboolean unsaved;

    change_log = bt_change_log_new ();
    g_object_get (change_log, "can-undo", &unsaved, NULL);
    g_object_unref (change_log);
    if (unsaved) {
      gchar *msg;

      msg = bt_main_window_make_unsaved_changes_message (song);
      res = bt_dialog_question (self, title, headline, msg);
      g_free (msg);
    }
    g_object_unref (song);
  }

  return res;
}


/**
 * bt_main_window_check_quit:
 * @self: the main window instance
 *
 * Displays a dialog box, that asks the user to confirm exiting the application.
 *
 * Returns: %TRUE if the user has confirmed to exit
 */
gboolean
bt_main_window_check_quit (const BtMainWindow * self)
{
  return bt_main_window_check_unsaved_song (self, _("Really quit?"),
      _("Really quit?"));
}

/**
 * bt_main_window_new_song:
 * @self: the main window instance
 *
 * Prepares a new song. Triggers cleaning up the old song and refreshes the ui.
 */
void
bt_main_window_new_song (const BtMainWindow * self)
{
  if (!bt_main_window_check_unsaved_song (self, _("New song?"), _("New song?")))
    return;

  if (!bt_edit_application_new_song (self->priv->app)) {
    // TODO(ensonic): show error message (from where? and which error?)
  }
}

/**
 * bt_main_window_open_song:
 * @self: the main window instance
 *
 * Opens a dialog box, where the user can choose a song to load.
 * If the dialog is not canceld, the old song will be freed, the new song will
 * be loaded and the ui will be refreshed upon success.
 */
void
bt_main_window_open_song (const BtMainWindow * self)
{
  gint result;
  gchar *folder_name, *file_name = NULL;
  GtkFileFilter *filter, *filter_all;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  guint ix;

  if (!bt_main_window_check_unsaved_song (self, _("Load new song?"),
          _("Load new song?")))
    return;

  self->priv->dialog =
      GTK_DIALOG (gtk_file_chooser_dialog_new (_("Open a song"),
          GTK_WINDOW (self), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"),
          GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL));
  bt_edit_application_attach_child_window (self->priv->app,
      GTK_WINDOW (self->priv->dialog));
  // store for format-changed signal handler
  self->priv->file_chooser = GTK_FILE_CHOOSER (self->priv->dialog);

  // set filters
  filter_all = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_all, "all supported files");
  plugins = bt_song_io_get_module_info_list ();
  for (node = plugins; node; node = g_list_next (node)) {
    info = (BtSongIOModuleInfo *) node->data;
    ix = 0;
    while (info->formats[ix].name) {
      filter = gtk_file_filter_new ();
      gtk_file_filter_set_name (filter, info->formats[ix].name);
      gtk_file_filter_add_mime_type (filter, info->formats[ix].mime_type);
      gtk_file_filter_add_mime_type (filter_all, info->formats[ix].mime_type);
      gtk_file_chooser_add_filter (self->priv->file_chooser, filter);
      ix++;
    }
  }
  gtk_file_chooser_add_filter (self->priv->file_chooser, filter_all);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "all files");
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (self->priv->file_chooser, filter);
  // set default filter
  gtk_file_chooser_set_filter (self->priv->file_chooser, filter_all);

  // set a default songs folder
  //gtk_file_chooser_set_current_folder(self->priv->file_chooser,DATADIR""G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S"songs"G_DIR_SEPARATOR_S);
  bt_child_proxy_get (self->priv->app, "settings::song-folder", &folder_name,
      NULL);
  // reuse last folder (store in self, if we loaded something
  gtk_file_chooser_set_current_folder (self->priv->file_chooser,
      self->priv->last_folder ? self->priv->last_folder : folder_name);
  gtk_file_chooser_add_shortcut_folder (self->priv->file_chooser, folder_name,
      NULL);
  // TODO(ensonic): if folder != default it would be nice to show the default as well, unfortunately we can't name the shortcuts
  // - maybe we should only install real demo songs
  g_free (folder_name);

  gtk_widget_show_all (GTK_WIDGET (self->priv->dialog));
  g_object_notify ((gpointer) self, "dialog");

  result = gtk_dialog_run (self->priv->dialog);
  switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      file_name = gtk_file_chooser_get_filename (self->priv->file_chooser);
      // remember last folder
      g_free (self->priv->last_folder);
      self->priv->last_folder =
          gtk_file_chooser_get_current_folder (self->priv->file_chooser);
      break;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      break;
    default:
      GST_WARNING ("unhandled response code = %d", result);
  }
  gtk_widget_destroy (GTK_WIDGET (self->priv->dialog));
  self->priv->dialog = NULL;
  g_object_notify ((gpointer) self, "dialog");

  // load after destoying the dialog, otherwise it stays open all time
  if (file_name) {
    load_song (self, file_name);
    g_free (file_name);
  }
}

/**
 * bt_main_window_save_song:
 * @self: the main window instance
 *
 * Save the song to disk.
 * If it is a new song it will ask for a file_name and location.
 */
void
bt_main_window_save_song (const BtMainWindow * self)
{
  gchar *file_name = NULL;

  // get songs file-name
  bt_child_proxy_get (self->priv->app, "song::song-info::file-name", &file_name,
      NULL);

  // check the file_name of the song
  if (file_name) {
    GError *err = NULL;
    if (!bt_edit_application_save_song (self->priv->app, file_name, &err)) {
      gchar *msg = g_strdup_printf (_("Can't save song '%s'."), file_name);
      bt_dialog_message (self, _("Can't save song"), msg, err->message);
      g_free (msg);
      g_error_free (err);
    }
  } else {
    // it is a new song
    bt_main_window_save_song_as (self);
  }
  g_free (file_name);
}

/**
 * bt_main_window_save_song_as:
 * @self: the main window instance
 *
 * Opens a dialog box, where the user can choose a file_name and location to save
 * the song under.
 */
void
bt_main_window_save_song_as (const BtMainWindow * self)
{
  BtSongInfo *song_info;
  gchar *name, *folder_name, *file_name = NULL;
  gchar *old_file_name = NULL;
  gchar *ext;
  gint result;
  GtkWidget *format_chooser, *box;
  GtkFileFilter *filter, *filter_all;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  BtSongIOClass *songio_class;
  guint ix;
  //gchar *glob;

  self->priv->dialog =
      GTK_DIALOG (gtk_file_chooser_dialog_new (_("Save a song"),
          GTK_WINDOW (self), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"),
          GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL));
  bt_edit_application_attach_child_window (self->priv->app,
      GTK_WINDOW (self->priv->dialog));
  // store for format-changed signal handler
  self->priv->file_chooser = GTK_FILE_CHOOSER (self->priv->dialog);

  // set filters and build format selector
  format_chooser = gtk_combo_box_text_new ();
  filter_all = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter_all, "all supported files");
  plugins = bt_song_io_get_module_info_list ();
  for (node = plugins; node; node = g_list_next (node)) {
    info = (BtSongIOModuleInfo *) node->data;

    ix = 0;
    while (info->formats[ix].name) {
      songio_class =
          (BtSongIOClass *) g_type_class_ref (info->formats[ix].type);
      if (!songio_class->save) {
        GST_DEBUG ("songio module %s supports no saving",
            info->formats[ix].name);
        ix++;
        continue;
      }
      filter = gtk_file_filter_new ();
      gtk_file_filter_set_name (filter, info->formats[ix].name);
      gtk_file_filter_add_mime_type (filter, info->formats[ix].mime_type);
      //glob=g_strconcat("*.",info->formats[ix].extension,NULL);
      //gtk_file_filter_add_pattern(filter,g_strconcat(glob,NULL));
      //g_free(glob);
      gtk_file_filter_add_mime_type (filter_all, info->formats[ix].mime_type);
      gtk_file_chooser_add_filter (self->priv->file_chooser, filter);
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (format_chooser),
          info->formats[ix].name);
      GST_DEBUG ("add filter %p for %s/%s/%s", filter, info->formats[ix].name,
          info->formats[ix].mime_type, info->formats[ix].extension);
      ix++;
      self->priv->filters = g_list_append (self->priv->filters, filter);
    }
  }
  gtk_file_chooser_add_filter (self->priv->file_chooser, filter_all);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "all files");
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (self->priv->file_chooser, filter);
  // set default filter - not here, only in load
  //gtk_file_chooser_set_filter(self->priv->file_chooser,filter_all);

  // get songs file-name
  bt_child_proxy_get (self->priv->app,
      "song::song-info", &song_info, "settings::song-folder", &folder_name,
      NULL);
  g_object_get (song_info, "name", &name, "file-name", &file_name, NULL);
  g_object_unref (song_info);
  gtk_file_chooser_add_shortcut_folder (self->priv->file_chooser, folder_name,
      NULL);
  if (!file_name) {
    gchar *new_file_name;

    // add extension
    info = (BtSongIOModuleInfo *) plugins->data;
    new_file_name = g_strdup_printf ("%s.%s", name, info->formats[0].extension);
    GST_DEBUG ("use defaults %s/%s", folder_name, new_file_name);
    /* the user just created a new document */
    gtk_file_chooser_set_current_folder (self->priv->file_chooser, folder_name);
    gtk_file_chooser_set_current_name (self->priv->file_chooser, new_file_name);
    gtk_combo_box_set_active (GTK_COMBO_BOX (format_chooser), 0);
    g_free (new_file_name);
  } else {
    gboolean found = FALSE;
    GtkFileFilterInfo ffi = {
      GTK_FILE_FILTER_FILENAME | GTK_FILE_FILTER_DISPLAY_NAME,
      file_name,
      NULL,                     // uri
      file_name,
      NULL                      // mime-type
    };
    /* the user edited an existing document */
    gtk_file_chooser_set_filename (self->priv->file_chooser, file_name);
    GST_DEBUG ("use existing %s", file_name);
    /* select the format of this file */
    for (node = self->priv->filters, ix = 0; node;
        node = g_list_next (node), ix++) {
      filter = node->data;
      if (gtk_file_filter_filter (filter, &ffi)) {
        /* @bug: it matches, but this does not update the filter
         * https://bugzilla.gnome.org/show_bug.cgi?id=590941
         * fixed in gtk-2.17.X
         */
        GST_DEBUG ("use last path %s, format is '%s', filter %p", file_name,
            gtk_file_filter_get_name (filter), filter);
        gtk_file_chooser_set_filter (self->priv->file_chooser, filter);
        gtk_combo_box_set_active (GTK_COMBO_BOX (format_chooser), ix);
        found = TRUE;
        break;
      }
    }
    if (!found) {
      ext = strrchr (file_name, '.');
      if (ext && ext[1]) {
        const GList *pnode, *fnode = self->priv->filters;
        /* gtk_file_filter_filter() seems to be buggy :/
         * try matching the extension */
        ext++;
        GST_DEBUG ("file_filter matching failed, match extension '%s'", ext);
        for (pnode = plugins; (pnode && !found); pnode = g_list_next (pnode)) {
          info = (BtSongIOModuleInfo *) pnode->data;

          ix = 0;
          while (info->formats[ix].name && !found) {
            // we ref the class above
            songio_class =
                (BtSongIOClass *) g_type_class_peek_static (info->
                formats[ix].type);
            if (!songio_class->save) {
              GST_DEBUG ("songio module %s supports no saving",
                  info->formats[ix].name);
              ix++;
              continue;
            }
            if (!strcmp (ext, info->formats[ix].extension)) {
              filter = fnode->data;
              /* @bug: it matches, but this does not update the filter
               * https://bugzilla.gnome.org/show_bug.cgi?id=590941
               * fixed in gtk-2.17.X
               */
              GST_DEBUG ("format is '%s', filter %p",
                  gtk_file_filter_get_name (filter), filter);
              gtk_file_chooser_set_filter (self->priv->file_chooser, filter);
              gtk_combo_box_set_active (GTK_COMBO_BOX (format_chooser), ix);
              found = TRUE;
            } else {
              ix++;
              fnode = g_list_next (fnode);
            }
          }
        }
      }
    }
  }
  old_file_name = file_name;
  file_name = NULL;
  g_free (folder_name);
  g_free (name);

  // add format selection to dialog
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_box_pack_start (GTK_BOX (box), gtk_label_new (_("Format")), FALSE, FALSE,
      0);
  gtk_box_pack_start (GTK_BOX (box), format_chooser, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (self->
              priv->dialog)), box, FALSE, FALSE, 0);
  g_signal_connect (format_chooser, "changed",
      G_CALLBACK (on_format_chooser_changed), (gpointer) self);

  gtk_widget_show_all (GTK_WIDGET (self->priv->dialog));
  g_object_notify ((gpointer) self, "dialog");

  result = gtk_dialog_run (self->priv->dialog);
  switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:{
      file_name = gtk_file_chooser_get_filename (self->priv->file_chooser);
      // remember last folder
      g_free (self->priv->last_folder);
      self->priv->last_folder =
          gtk_file_chooser_get_current_folder (self->priv->file_chooser);
      break;
    }
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      break;
    default:
      GST_WARNING ("unhandled response code = %d", result);
  }
  gtk_widget_destroy (GTK_WIDGET (self->priv->dialog));
  self->priv->dialog = NULL;
  g_object_notify ((gpointer) self, "dialog");
  g_list_free (self->priv->filters);
  self->priv->filters = NULL;

  // save after destoying the dialog, otherwise it stays open all time
  if (file_name) {
    gboolean cont = TRUE;

    GST_WARNING ("song name = '%s'", file_name);

    if (g_file_test (file_name, G_FILE_TEST_EXISTS)) {
      GST_INFO ("file already exists");
      // it already exists, ask the user what to do (do not save, choose new name, overwrite song)
      cont = bt_dialog_question (self,
          _("File already exists"),
          _("File already exists"),
          _
          ("Choose 'Okay' to overwrite or 'Cancel' to abort saving the song."));
    } else {
      const gchar *reason = (const gchar *) g_strerror (errno);
      gchar *msg;

      GST_INFO ("file '%s' can not be opened : %d : %s", file_name, errno,
          reason);
      switch (errno) {
        case EACCES:           // Permission denied.
          cont = FALSE;
          msg =
              g_strdup_printf (_
              ("An error occurred while writing the file '%s': %s"), file_name,
              reason);
          bt_dialog_message (self, _("Can't save song"), _("Can't save song"),
              msg);
          g_free (msg);
          break;
        default:
          // ENOENT A component of the path file_name does not exist, or the path is an empty string.
          // -> just save
          break;
      }
    }
    if (cont) {
      GError *err = NULL;
      if (!bt_edit_application_save_song (self->priv->app, file_name, &err)) {
        gchar *msg = g_strdup_printf (_("Can't save song '%s'."), file_name);
        bt_dialog_message (self, _("Can't save song"), msg, err->message);
        g_free (msg);
        g_error_free (err);
      } else {
        // store recent file
        GtkRecentManager *manager = gtk_recent_manager_get_default ();
        gchar *uri;

        if (old_file_name) {
          uri = g_filename_to_uri (old_file_name, NULL, NULL);
          if (!gtk_recent_manager_remove_item (manager, uri, NULL)) {
            GST_WARNING ("Can't store recent file");
          }
          g_free (uri);
        }
        uri = g_filename_to_uri (file_name, NULL, NULL);
        if (!gtk_recent_manager_add_item (manager, uri)) {
          GST_WARNING ("Can't store recent file");
        }
        g_free (uri);
      }
    }
    g_free (file_name);
  }
  g_free (old_file_name);
}

// TODO(ensonic): use GtkMessageDialog for the next two

/**
 * bt_dialog_message:
 * @self: the applications main window
 * @title: the title of the message
 * @headline: the bold headline of the message
 * @message: the message itself
 *
 * Displays a modal message dialog, that needs to be confirmed with "Okay".
 */
void
bt_dialog_message (const BtMainWindow * self, const gchar * title,
    const gchar * headline, const gchar * message)
{
  GtkWidget *label, *icon, *box;
  gchar *str;

  g_return_if_fail (BT_IS_MAIN_WINDOW (self));
  g_return_if_fail (BT_IS_STRING (title));
  g_return_if_fail (BT_IS_STRING (headline));
  g_return_if_fail (BT_IS_STRING (message));

  self->priv->dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (title,
          GTK_WINDOW (self),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          _("_OK"), GTK_RESPONSE_ACCEPT, NULL));

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);

  // TODO(ensonic): when to use GTK_STOCK_DIALOG_WARNING ?
  icon =
      gtk_image_new_from_icon_name ("dialog-information", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);

  // @idea if headline is NULL use title ?
  str = g_strdup_printf ("<big><b>%s</b></big>\n\n%s", headline, message);
  label = g_object_new (GTK_TYPE_LABEL,
      "use-markup", TRUE, "selectable", TRUE, "wrap", TRUE, "label", str, NULL);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG
              (self->priv->dialog))), box, TRUE, TRUE, 0);
  gtk_widget_show_all (GTK_WIDGET (self->priv->dialog));
  g_object_notify ((gpointer) self, "dialog");

  gtk_dialog_run (self->priv->dialog);
  gtk_widget_destroy (GTK_WIDGET (self->priv->dialog));
  self->priv->dialog = NULL;
  g_object_notify ((gpointer) self, "dialog");
}

/**
 * bt_dialog_question:
 * @self: the applications main window
 * @title: the title of the message
 * @headline: the bold headline of the message
 * @message: the message itself
 *
 * Displays a modal question dialog, that needs to be confirmed with "Okay" or aborted with "Cancel".
 * Returns: %TRUE for Okay, %FALSE otherwise
 */
gboolean
bt_dialog_question (const BtMainWindow * self, const gchar * title,
    const gchar * headline, const gchar * message)
{
  gboolean result = FALSE;
  gint answer;
  GtkWidget *label, *icon, *box;
  gchar *str;

  g_return_val_if_fail (BT_IS_MAIN_WINDOW (self), FALSE);
  g_return_val_if_fail (BT_IS_STRING (title), FALSE);
  g_return_val_if_fail (BT_IS_STRING (headline), FALSE);
  g_return_val_if_fail (BT_IS_STRING (message), FALSE);

  self->priv->dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (title,
          GTK_WINDOW (self),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
          _("_OK"), GTK_RESPONSE_ACCEPT,
          _("_Cancel"), GTK_RESPONSE_REJECT, NULL));

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);

  icon = gtk_image_new_from_icon_name ("dialog-question", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);

  // @idea if headline is NULL use title ?
  str = g_strdup_printf ("<big><b>%s</b></big>\n\n%s", headline, message);
  label = g_object_new (GTK_TYPE_LABEL,
      "use-markup", TRUE, "selectable", TRUE, "wrap", TRUE, "label", str, NULL);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG
              (self->priv->dialog))), box, TRUE, TRUE, 0);
  gtk_widget_show_all (GTK_WIDGET (self->priv->dialog));
  g_object_notify ((gpointer) self, "dialog");

  answer = gtk_dialog_run (self->priv->dialog);
  switch (answer) {
    case GTK_RESPONSE_ACCEPT:
      result = TRUE;
      break;
    case GTK_RESPONSE_REJECT:
      result = FALSE;
      break;
    default:
      GST_WARNING ("unhandled response code = %d", answer);
  }
  gtk_widget_destroy (GTK_WIDGET (self->priv->dialog));
  self->priv->dialog = NULL;
  g_object_notify ((gpointer) self, "dialog");

  GST_INFO ("bt_dialog_question(\"%s\") = %d", title, result);
  return result;
}

//-- wrapper

//-- class internals

static void
bt_main_window_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtMainWindow *self = BT_MAIN_WINDOW (object);
  return_if_disposed ();
  switch (property_id) {
    case MAIN_WINDOW_TOOLBAR:
      g_value_set_object (value, self->priv->toolbar);
      break;
    case MAIN_WINDOW_STATUSBAR:
      g_value_set_object (value, self->priv->statusbar);
      break;
    case MAIN_WINDOW_PAGES:
      g_value_set_object (value, self->priv->pages);
      break;
    case MAIN_WINDOW_DIALOG:
      g_value_set_object (value, self->priv->dialog);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_window_dispose (GObject * object)
{
  BtMainWindow *self = BT_MAIN_WINDOW (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  gtk_window_remove_accel_group (GTK_WINDOW (self),
      bt_ui_resources_get_accel_group ());

  g_object_unref (self->priv->app);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_window_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_main_window_finalize (GObject * object)
{
  BtMainWindow *self = BT_MAIN_WINDOW (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->last_folder);

  G_OBJECT_CLASS (bt_main_window_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static void
bt_main_window_init (BtMainWindow * self)
{
  self->priv = bt_main_window_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_main_window_class_init (BtMainWindowClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bt_main_window_get_property;
  gobject_class->dispose = bt_main_window_dispose;
  gobject_class->finalize = bt_main_window_finalize;

  g_object_class_install_property (gobject_class, MAIN_WINDOW_TOOLBAR,
      g_param_spec_object ("toolbar", "toolbar prop", "Get the toolbar",
          BT_TYPE_MAIN_TOOLBAR, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_WINDOW_STATUSBAR,
      g_param_spec_object ("statusbar", "statusbar prop", "Get the status bar",
          BT_TYPE_MAIN_STATUSBAR, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_WINDOW_PAGES,
      g_param_spec_object ("pages", "pages prop", "Get the pages widget",
          BT_TYPE_MAIN_PAGES, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_WINDOW_DIALOG,
      g_param_spec_object ("dialog", "dialog prop", "Get the active dialog",
          GTK_TYPE_DIALOG, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
