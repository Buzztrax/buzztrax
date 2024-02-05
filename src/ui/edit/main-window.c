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
#include "main-menu.h"

enum
{
  MAIN_WINDOW_STATUSBAR = 1,
  MAIN_WINDOW_PAGES,
  MAIN_WINDOW_DIALOG
};


struct _BtMainWindow
{
  AdwWindow parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the content pages of the window */
  BtMainPages *pages;
  /* the statusbar of the window */
  BtMainStatusbar *statusbar;
  /* active dialog */
  GtkDialog *dialog;

  /* file-chooser stuff */
  GtkFileChooser *file_chooser;
  GFile *last_folder;
};

#if 0 /// GTK4
enum
{ TARGET_URI_LIST };
static GtkTargetEntry drop_types[] = {
  {"text/uri-list", 0, TARGET_URI_LIST}
};

static gint n_drop_types = sizeof (drop_types) / sizeof (GtkTargetEntry);
#endif

//-- the class

G_DEFINE_TYPE (BtMainWindow, bt_main_window, ADW_TYPE_WINDOW);


//-- helper methods

#if 0 /// GTK4 still needed?
/* update the filename to have an extension, matching the selected format */
static gchar *
update_filename_ext (BtMainWindow * self, gchar * file_name,
    gint format_ix)
{
  GtkFileFilter *this_filter, *that_filter;
  gchar *new_file_name = NULL;
  gchar *ext;
  const GList *plugins, *pnode, *fnode;
  BtSongIOModuleInfo *info;
  guint ix;

  plugins = bt_song_io_get_module_info_list ();

  GST_WARNING ("old song name = '%s'", file_name);

  ext = strrchr (file_name, '.');

  if (ext) {
    ext++;
    // cut off known extensions
    for (pnode = plugins, fnode = self->filters; pnode;
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
  for (pnode = plugins, fnode = self->filters; pnode;
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
#endif

static gboolean
load_song (BtMainWindow * self, gchar * file_name)
{
  GError *err = NULL;

  if (!bt_edit_application_load_song (self->app, file_name, &err)) {
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

#if 0 /// GTK4 still needed?
static void
on_format_chooser_changed (GtkComboBox * menu, gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  gint format_ix;
  gchar *file_name, *new_file_name = NULL;

  format_ix = gtk_combo_box_get_active (menu);
  file_name = gtk_file_chooser_get_filename (self->file_chooser);
  GST_WARNING ("Changing %s to extension for %d's filter", file_name,
      format_ix);
  if (!file_name)
    return;

  if ((new_file_name = update_filename_ext (self, file_name, format_ix))) {
    gchar *name;

    name = strrchr (new_file_name, G_DIR_SEPARATOR);
    //gtk_file_chooser_set_filename(self->file_chooser,new_file_name);
    gtk_file_chooser_set_current_name (self->file_chooser,
        name ? &name[1] : new_file_name);
  }
  g_free (file_name);
  g_free (new_file_name);
}
#endif

static void
on_window_mapped (GtkWidget * widget, gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  BtSettings *settings;
  gboolean toolbar_hide, statusbar_hide, tabs_hide;

  GST_INFO ("main_window mapped");

  // eventualy hide some ui elements
  g_object_get (self->app, "settings", &settings, NULL);
  g_object_get (settings,
      "toolbar-hide", &toolbar_hide,
      "statusbar-hide", &statusbar_hide, "tabs-hide", &tabs_hide, NULL);
  g_object_unref (settings);

#if 0 /// GTK4 toolbar variable has gone away, is it still necessary to hide it?
  gtk_widget_set_visible (GTK_WIDGET (self->toolbar), !toolbar_hide);
#endif
  gtk_widget_set_visible (GTK_WIDGET (self->statusbar), !statusbar_hide);
  if (tabs_hide) {
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (self->pages), FALSE);
  }
}

#if 0 /// GTK4
static void
on_window_destroy (GtkWidget * widget, gpointer user_data)
{
  GST_INFO ("destroy occurred");
  if (gtk_main_level ()) {
    GST_INFO ("  leaving main-loop");
    gtk_main_quit ();
  }
}
#endif

static void
bt_main_window_update_title (BtMainWindow *self)
{
  gboolean unsaved = bt_edit_application_is_song_unsaved (self->app);
  BtSong *song;
  gchar *title;

  // compose title
  g_object_get (self->app, "song", &song, NULL);
  if (!song)
    return;

  BtSongInfo* song_info;
  g_object_get ((GObject *) song, "song-info", &song_info, NULL);
  gchar* name = bt_song_info_get_name(song_info);
  
  // we don't use PACKAGE_NAME = 'buzztrax' for the window title
  title =
      g_strdup_printf ("%s (%s) - Buzztrax", name,
      (unsaved ? _("unsaved") : _("saved")));
  g_free (name);
  gtk_window_set_title (GTK_WINDOW (self), title);
  g_free (title);
  //-- release the references
  g_object_unref (song);
  g_object_unref (song_info);
}

static void
on_song_unsaved_changed (const GObject * object, GParamSpec * arg,
    gpointer user_data)
{
  bt_main_window_update_title (BT_MAIN_WINDOW (user_data));
}

static void
on_song_title_changed (const GObject * object, GParamSpec * arg,
    gpointer user_data)
{
  bt_main_window_update_title (BT_MAIN_WINDOW (user_data));
}

static void
on_song_changed (const GObject * object, GParamSpec * arg,
    gpointer user_data)
{
  BtMainWindow *self = BT_MAIN_WINDOW (user_data);
  bt_main_window_update_title (BT_MAIN_WINDOW (user_data));
  
  BtSong *song;

  g_object_get (self->app, "song", &song, NULL);
  if (!song)
    return;

  BtSongInfo* song_info;
  g_object_get (song, "song-info", &song_info, NULL);

  g_signal_connect (song_info, "notify::name",
      G_CALLBACK (on_song_title_changed), (gpointer) self);
  
  g_object_unref (song);
  g_object_unref (song_info);
}

#if 0 /// GTK4
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
#endif

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
bt_main_window_init_ui (BtMainWindow * self)
{
  BtChangeLog *change_log;

  AdwToolbarView *toolbar_view = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  adw_window_set_content (ADW_WINDOW (self), GTK_WIDGET (toolbar_view));

  // add the tool-bar and  menu-bar to the AdwToolbarView's "top bar"
  GtkBox *topbar_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5));
  gtk_box_append (GTK_BOX (topbar_box), GTK_WIDGET (bt_main_menu_new (self->app)));
  gtk_box_append (GTK_BOX (topbar_box), GTK_WIDGET (bt_main_toolbar_new (self->app)));
  adw_toolbar_view_add_top_bar (toolbar_view, GTK_WIDGET (topbar_box));
  
  gtk_widget_set_name (GTK_WIDGET (self), "main window");

  // create and set window icon
  gtk_window_set_icon_name (GTK_WINDOW (self), "buzztrax");

#if 0 /// GTK4  
  gtk_window_add_accel_group (GTK_WINDOW (self),
      bt_ui_resources_get_accel_group ());
#endif
  
  // create main layout container
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  adw_toolbar_view_set_content (toolbar_view, box);

  GST_INFO ("before creating content, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->app));

  // add the window content pages
  self->pages = bt_main_pages_new ();
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->pages));
  // add the status bar
  self->statusbar = bt_main_statusbar_new ();
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->statusbar));

#if 0 /// GTK4
  gtk_drag_dest_set (GTK_WIDGET (self),
      (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
          GTK_DEST_DEFAULT_DROP), drop_types, n_drop_types, GDK_ACTION_COPY);
  g_signal_connect ((gpointer) self, "drag-data-received",
      G_CALLBACK (on_window_dnd_drop), (gpointer) self);
#endif
  
  GST_INFO ("content created, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->app));

#if 0 /// GTK4 still needed?
  g_signal_connect ((gpointer) self, "destroy", G_CALLBACK (on_window_destroy),
      (gpointer) self);
#endif

  g_signal_connect (self->app, "notify::unsaved",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);

  g_signal_connect (self->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self);
  
  change_log = bt_change_log_new ();
  g_signal_connect (change_log, "notify::can-undo",
      G_CALLBACK (on_song_unsaved_changed), (gpointer) self);
  g_object_unref (change_log);

  GST_INFO ("signal connected, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->app));
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
  gint w, h;

  GST_INFO ("creating a new window");

  self =
      BT_MAIN_WINDOW (g_object_new (BT_TYPE_MAIN_WINDOW, NULL));
  GST_INFO ("new main_window created");
  bt_main_window_init_ui (self);
  GST_INFO ("new main_window layouted");

  // restore last width + height
  // note that GTK4 can no longer set window position 
  g_object_get (self->app, "settings", &settings, NULL);
  g_object_get (settings, "window-width", &w, "window-height", &h, NULL);
  g_object_unref (settings);

  // use position from settings
  if (w >= 0 && h >= 0) {
    gtk_window_set_default_size (GTK_WINDOW (self), w, h);
  } else {
    gtk_window_set_default_size (GTK_WINDOW (self), 800, 600);
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
void
bt_main_window_check_unsaved_song (BtMainWindow * self,
    const gchar * title, const gchar * headline,
    BtDialogQuestionCb result_cb, gpointer user_data)
{
  BtSong *song;

  g_object_get (self->app, "song", &song, NULL);
  if (song) {
    BtChangeLog *change_log;
    gboolean unsaved;

    change_log = bt_change_log_new ();
    g_object_get (change_log, "can-undo", &unsaved, NULL);
    g_object_unref (change_log);
    if (unsaved) {
      gchar *msg;

      msg = bt_main_window_make_unsaved_changes_message (song);
      bt_dialog_question (self, title, headline, msg, result_cb, user_data);
      g_free (msg);
    }
    g_object_unref (song);
  }
}


/**
 * bt_main_window_check_quit:
 * @self: the main window instance
 *
 * Displays a dialog box, that asks the user to confirm exiting the application.
 *
 * Returns: %TRUE if the user has confirmed to exit
 */
void
bt_main_window_check_quit (BtMainWindow * self, BtDialogQuestionCb result_cb, gpointer user_data)
{
  return bt_main_window_check_unsaved_song (self, _("Really quit?"),
              _("Really quit?"), result_cb, user_data);
}

static void
bt_main_window_new_song_cb (gboolean result, gpointer data) {
  if (!result)
    return;

  BtMainWindow * self = BT_MAIN_WINDOW (data);
  
  if (!bt_edit_application_new_song (self->app)) {
    // TODO(ensonic): show error message (from where? and which error?)
  }
}

/**
 * bt_main_window_new_song:
 * @self: the main window instance
 *
 * Prepares a new song. Triggers cleaning up the old song and refreshes the ui.
 */
void
bt_main_window_new_song (BtMainWindow * self)
{
  bt_main_window_check_unsaved_song (self, _("New song?"), _("New song?"),
      bt_main_window_new_song_cb, (gpointer) self);
}

void
bt_main_window_open_song_cb (GObject* source_object, GAsyncResult* res, gpointer data) {
  GtkFileDialog* dlg = GTK_FILE_DIALOG (source_object);
  BtMainWindow* self = BT_MAIN_WINDOW (data);

  GFile* file = gtk_file_dialog_open_finish (dlg, res, NULL);

  if (file) {
    // remember last folder
    if (self->last_folder)
      g_object_unref (self->last_folder);
    self->last_folder = g_file_get_parent (file);

    gchar* file_name = g_file_get_path (file);
    load_song (self, file_name);
    g_free (file_name);

    g_object_unref (file);
  }
}

static void
bt_main_window_open_song_unsaved_cb (gboolean result, gpointer data) {
  if (!result)
    return;

  BtMainWindow * self = BT_MAIN_WINDOW (data);
  
  gchar *folder_name = NULL;
  GtkFileFilter *filter, *filter_all;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  guint ix;

  GtkFileDialog* dlg = gtk_file_dialog_new ();
  gtk_file_dialog_set_modal (dlg, TRUE);
  gtk_file_dialog_set_title (dlg, _("Open a song"));

  GListStore* store = g_list_store_new (GTK_TYPE_FILE_FILTER);

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
      g_list_store_append (store, G_OBJECT (filter));
      ix++;
    }
  }
  g_list_store_append (store, G_OBJECT (filter_all));
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "all files");
  gtk_file_filter_add_pattern (filter, "*");
  g_list_store_append (store, G_OBJECT (filter));
  // set default filter
  gtk_file_dialog_set_default_filter (dlg, filter_all);

  // set a default songs folder
  bt_child_proxy_get (self->app, "settings::song-folder", &folder_name,
      NULL);

  GFile* folder_file = g_file_new_for_path (folder_name);
  // reuse last folder (store in self, if we loaded something
  gtk_file_dialog_set_initial_folder (dlg, self->last_folder ? self->last_folder : folder_file);
  g_object_unref (folder_file);
  // TODO(ensonic): if folder != default it would be nice to show the default as well, unfortunately we can't name the shortcuts
  // - maybe we should only install real demo songs
  g_free (folder_name);

  gtk_file_dialog_open (dlg, GTK_WINDOW (self), NULL, bt_main_window_open_song_cb, self); 
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
bt_main_window_open_song (BtMainWindow * self)
{
  bt_main_window_check_unsaved_song (self, _("Load new song?"),
     _("Load new song?"), bt_main_window_open_song_unsaved_cb, (gpointer) self);
}

/**
 * bt_main_window_save_song:
 * @self: the main window instance
 *
 * Save the song to disk.
 * If it is a new song it will ask for a file_name and location.
 */
void
bt_main_window_save_song (BtMainWindow * self)
{
  gchar *file_name = NULL;

  // get songs file-name
  bt_child_proxy_get (self->app, "song::song-info::file-name", &file_name,
      NULL);

  // check the file_name of the song
  if (file_name) {
    GError *err = NULL;
    if (!bt_edit_application_save_song (self->app, file_name, &err)) {
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

typedef struct {
  BtMainWindow* self;
  gchar* old_file_name;
  gboolean check_exists;
  GFile* file;
} BtMainWindowSaveSongCbData;

void
bt_main_window_save_song_cb_data_free (BtMainWindowSaveSongCbData* cbdata) {
  g_free (cbdata->old_file_name);
  if (cbdata->file)
    g_object_unref (cbdata->file);
  g_free (cbdata);
}

static void bt_main_window_save_song_execute (BtMainWindowSaveSongCbData* cbdata);

void
bt_main_window_save_song_check_exists_cb (gboolean result, gpointer data) {
  if (result) {
    bt_main_window_save_song_execute ((BtMainWindowSaveSongCbData*) data);
  } else {
    bt_main_window_save_song_cb_data_free ((BtMainWindowSaveSongCbData*) data);
  }
}

static void
bt_main_window_save_song_execute (BtMainWindowSaveSongCbData* cbdata) {
  gchar* file_name = NULL;
  
  if (cbdata->file) {
    // remember last folder
    if (cbdata->self->last_folder)
      g_object_unref (cbdata->self->last_folder);
    cbdata->self->last_folder = g_file_get_parent (cbdata->file);

    file_name = g_file_get_path (cbdata->file);
    
    gboolean cont = TRUE;

    GST_WARNING ("song name = '%s'", file_name);

    if (cbdata->check_exists && g_file_test (file_name, G_FILE_TEST_EXISTS)) {
      GST_INFO ("file already exists");

      cbdata->check_exists = FALSE;
      
      // it already exists, ask the user what to do (do not save, choose new name, overwrite song)
      bt_dialog_question (cbdata->self,
          _("File already exists"),
          _("File already exists"),
          _("Choose 'Okay' to overwrite or 'Cancel' to abort saving the song."),
          bt_main_window_save_song_check_exists_cb,
          cbdata);
      goto AwaitQuestionCb;
    } else {
      // dbeswick: GTK4, is this logic still right? What call has set errno?
      const gchar *reason = (const gchar *) g_strerror (errno);
      gchar *msg;

      switch (errno) {
        case EACCES:           // Permission denied.
          cont = FALSE;
          msg =
              g_strdup_printf (_
              ("An error occurred while writing the file '%s': %s"), file_name,
              reason);
          bt_dialog_message (cbdata->self, _("Can't save song"), _("Can't save song"),
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
      if (!bt_edit_application_save_song (cbdata->self->app, file_name, &err)) {
        gchar *msg = g_strdup_printf (_("Can't save song '%s'."), file_name);
        bt_dialog_message (cbdata->self, _("Can't save song"), msg, err->message);
        g_free (msg);
        g_error_free (err);
      } else {
        // store recent file
        GtkRecentManager *manager = gtk_recent_manager_get_default ();
        gchar *uri;

        if (cbdata->old_file_name) {
          uri = g_filename_to_uri (cbdata->old_file_name, NULL, NULL);
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
  }

  // This is skipped over if the user needs to be asked a question.
  // cbdata will be re-used in that case and its contents shouldn't be freed.
  bt_main_window_save_song_cb_data_free (cbdata);

AwaitQuestionCb:
  g_free (file_name);
}

void
bt_main_window_save_song_cb (GObject* source_object, GAsyncResult* res, gpointer data) {
  GtkFileDialog* dlg = GTK_FILE_DIALOG (source_object);
  BtMainWindowSaveSongCbData* cbdata = (BtMainWindowSaveSongCbData*) data;

  cbdata->file = gtk_file_dialog_save_finish (dlg, res, NULL);

  bt_main_window_save_song_execute (cbdata);
}


/**
 * bt_main_window_save_song_as:
 * @self: the main window instance
 *
 * Opens a dialog box, where the user can choose a file_name and location to save
 * the song under.
 */
void
bt_main_window_save_song_as (BtMainWindow * self)
{
  BtSongInfo *song_info;
  gchar *name, *folder_name, *file_name = NULL;
  GtkFileFilter *filter, *filter_all;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  BtSongIOClass *songio_class;
  guint ix;

  GListStore* store = g_list_store_new (GTK_TYPE_FILE_FILTER);

  // set filters
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
      gtk_file_filter_add_mime_type (filter_all, info->formats[ix].mime_type);
      g_list_store_append (store, G_OBJECT (filter));
      
      ix++;
    }
  }
  g_list_store_append (store, G_OBJECT (filter_all));
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "all files");
  gtk_file_filter_add_pattern (filter, "*");
  g_list_store_append (store, G_OBJECT (filter));

  GtkFileDialog* dlg = gtk_file_dialog_new ();
  gtk_file_dialog_set_modal (dlg, TRUE);
  gtk_file_dialog_set_title (dlg, _("Save a song"));
  
  // get songs file-name
  bt_child_proxy_get (self->app,
      "song::song-info", &song_info, "settings::song-folder", &folder_name,
      NULL);
  name = bt_song_info_get_name(song_info);
  g_object_get (song_info, "file-name", &file_name, NULL);
  g_object_unref (song_info);
  if (!file_name) {
    gchar *new_file_name;

    // add extension
    info = (BtSongIOModuleInfo *) plugins->data;
    new_file_name = g_strdup_printf ("%s.%s", name, info->formats[0].extension);
    GST_DEBUG ("use defaults %s/%s", folder_name, new_file_name);
    /* the user just created a new document */
    GFile* folder_file = g_file_new_for_path (folder_name);
    gtk_file_dialog_set_initial_folder (dlg, folder_file);
    g_object_unref (folder_file);
    gtk_file_dialog_set_initial_name (dlg, new_file_name);
    g_free (new_file_name);
  } else {
#if 0 /// GTK4 still needed?
    gboolean found = FALSE;
#endif
    /* the user edited an existing document */
    gtk_file_dialog_set_initial_name (dlg, file_name);
    GST_DEBUG ("use existing %s", file_name);
#if 0 /// GTK4 still needed?
    /* select the format of this file */
    for (node = self->filters, ix = 0; node;
        node = g_list_next (node), ix++) {
      filter = node->data;
      gtk_file_dialog_set_default_filter (dlg, file_name);
      if (gtk_filter_match (GTK_FILTER (filter), &ffi)) {
        /* @bug: it matches, but this does not update the filter
         * https://bugzilla.gnome.org/show_bug.cgi?id=590941
         * fixed in gtk-2.17.X
         */
        GST_DEBUG ("use last path %s, format is '%s', filter %p", file_name,
            gtk_file_filter_get_name (filter), filter);
        gtk_file_chooser_set_filter (self->file_chooser, filter);
        gtk_combo_box_set_active (GTK_COMBO_BOX (format_chooser), ix);
        found = TRUE;
        break;
      }
    }
    if (!found) {
      ext = strrchr (file_name, '.');
      if (ext && ext[1]) {
        const GList *pnode, *fnode = self->filters;
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
              gtk_file_chooser_set_filter (self->file_chooser, filter);
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
#endif
  }
  g_free (folder_name);
  g_free (name);

  BtMainWindowSaveSongCbData* cbdata = g_malloc (sizeof (BtMainWindowSaveSongCbData));
  cbdata->self = self;
  cbdata->old_file_name = file_name;
  cbdata->check_exists = TRUE;
  cbdata->file = NULL;
  gtk_file_dialog_save (dlg, GTK_WINDOW (self), NULL, bt_main_window_save_song_cb, cbdata); 
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
bt_dialog_message (BtMainWindow * self, const gchar * title,
    const gchar * headline, const gchar * message)
{
  g_return_if_fail (BT_IS_MAIN_WINDOW (self));
  g_return_if_fail (BT_IS_STRING (title));
  g_return_if_fail (BT_IS_STRING (headline));
  g_return_if_fail (BT_IS_STRING (message));

  AdwMessageDialog* dlg = ADW_MESSAGE_DIALOG (
      adw_message_dialog_new (GTK_WINDOW (self), title, NULL));

  adw_message_dialog_format_body_markup (dlg, "<big><b>%s</b></big>\n\n%s", headline, message);
  adw_message_dialog_add_responses (dlg, "ok",  _("_Ok"), NULL);

  gtk_window_present (GTK_WINDOW (dlg));
}

static void
bt_dialog_question_cb (GObject* source_object, GAsyncResult* res, gpointer data) {
  AdwMessageDialog* dlg = ADW_MESSAGE_DIALOG (source_object);
  BtDialogQuestionCbData* cbdata = (BtDialogQuestionCbData*)data;

  (*(cbdata->cb)) (g_strcmp0 (adw_message_dialog_choose_finish (dlg, res), "ok") == 0, cbdata->user_data);

  g_free (cbdata);
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
void
bt_dialog_question (BtMainWindow * self, const gchar * title,
    const gchar * headline, const gchar * message,
    BtDialogQuestionCb result_cb, gpointer user_data)
{
  g_return_if_fail (BT_IS_MAIN_WINDOW (self));
  g_return_if_fail (BT_IS_STRING (title));
  g_return_if_fail (BT_IS_STRING (headline));
  g_return_if_fail (BT_IS_STRING (message));

  AdwMessageDialog* dlg = ADW_MESSAGE_DIALOG (adw_message_dialog_new (GTK_WINDOW (self), title, NULL));

  adw_message_dialog_format_body_markup (dlg, "<big><b>%s</b></big>\n\n%s", headline, message);

  adw_message_dialog_add_responses (
    dlg,
    "cancel", _("_Cancel"),
    "ok",  _("_Okay"),
    NULL);

  BtDialogQuestionCbData* cbdata = g_malloc (sizeof (BtDialogQuestionCbData));
  adw_message_dialog_choose (dlg, NULL, bt_dialog_question_cb, cbdata);
}

//-- wrapper

//-- class internals

static void
bt_main_window_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtMainWindow *self = BT_MAIN_WINDOW (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MAIN_WINDOW_STATUSBAR:
      g_value_set_object (value, self->statusbar);
      break;
    case MAIN_WINDOW_PAGES:
      g_value_set_object (value, self->pages);
      break;
    case MAIN_WINDOW_DIALOG:
      g_value_set_object (value, self->dialog);
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
  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

#if 0 /// GTK4
  gtk_window_remove_accel_group (GTK_WINDOW (self),
      bt_ui_resources_get_accel_group ());
#endif

  g_clear_object (&self->last_folder);

  g_object_unref (self->app);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_window_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_main_window_init (BtMainWindow * self)
{
  self = bt_main_window_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
}

static void
bt_main_window_class_init (BtMainWindowClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bt_main_window_get_property;
  gobject_class->dispose = bt_main_window_dispose;

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
