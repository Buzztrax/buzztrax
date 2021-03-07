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
 * SECTION:bteditapplication
 * @short_description: class for a gtk based buzztrax editor application
 * @see_also: #BtMainWindow
 *
 * Opens the #BtMainWindow and provide application level function like load,
 * save, run and exit.
 *
 * It also provides functions to invoke some dialogs like about and tips.
 *
 * The application instance will have exactly one active
 * #BtEditApplication:song at a time. I tracks undo/redo-able changes to the
 * song via #BtChangeLog and simple flagged changes via the
 * #BtEditApplication:unsaved property.
 */
/* TODO(ensonic): unsaved flagging
 * - various parts in the UI call bt_edit_application_set_song_unsaved()
 * - this sometimes happens as a side effect from doing something undo/redoable
 * - can we include flagging unsaved in the changelog group, so hat if we undo
 *   it the flagging gets undone?
 */

#define BT_EDIT
#define BT_EDIT_APPLICATION_C

#include "bt-edit.h"
#include <glib/gstdio.h>

//-- signal ids

//-- property ids

enum
{
  EDIT_APPLICATION_SONG = 1,
  EDIT_APPLICATION_MAIN_WINDOW,
  EDIT_APPLICATION_IC_REGISTRY,
  EDIT_APPLICATION_UNSAVED
};

// this needs to be here because of gtk-doc and unit-tests
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

struct _BtEditApplicationPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the currently loaded song */
  BtSong *song;
  /* shared ui resources */
  BtUIResources *ui_resources;
  /* the top-level window of our app */
  BtMainWindow *main_window;

  /* editor change log for undo/redo */
  BtChangeLog *change_log;
  /* flag for smaller changes to make the song-saveable */
  gboolean unsaved;
  gboolean need_dts_reset;

  /* remote playback controllers */
  BtPlaybackControllerSocket *pbc_socket;
  BtPlaybackControllerIc *pbc_ic;

  /* interaction controller registry */
  BtIcRegistry *ic_registry;

  /* persistent audio session */
  BtAudioSession *audio_session;

  /* error from loading a song via commandline args or NULL */
  GError *init_err;
  gchar *init_file_name;
};

static BtEditApplication *singleton = NULL;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtEditApplication, bt_edit_application, BT_TYPE_APPLICATION, 
    G_ADD_PRIVATE(BtEditApplication));

//-- event handler

static void
on_songio_status_changed (BtSongIO * songio, GParamSpec * arg,
    gpointer user_data)
{
  BtEditApplication *self = BT_EDIT_APPLICATION (user_data);
  gchar *str;

  /* IDEA(ensonic): bind properties */
  g_object_get (songio, "status", &str, NULL);
  GST_INFO ("songio_status has changed : \"%s\"", safe_string (str));
  bt_child_proxy_set (self->priv->main_window, "statusbar::status", str, NULL);
  g_free (str);
}

static void
on_changelog_can_undo_changed (BtChangeLog * change_log, GParamSpec * arg,
    gpointer user_data)
{
  BtEditApplication *self = BT_EDIT_APPLICATION (user_data);

  if (self->priv->need_dts_reset) {
    self->priv->need_dts_reset = FALSE;
    // this updates the time-stamp (we need that, to show the since when we have
    // unsaved changes, if someone closes the song)
    bt_child_proxy_set (self->priv->song, "song-info::change-dts", NULL, NULL);
  }
}

//-- helper methods

/*
 * bt_edit_application_check_missing:
 * @self: the edit application
 *
 * Run gstreamer element checks. If elements are missing, if shows a dialog with
 * element-names and brief description what will not work.
 *
 * Returns: %TRUE is no critical elements are missing
 */
static gboolean
bt_edit_application_check_missing (const BtEditApplication * self)
{
  GList *missing_core_elements, *missing_edit_elements =
      NULL, *missing_elements;
  GList *edit_elements = NULL;
  gboolean res = TRUE, missing = FALSE;

  if ((missing_core_elements = bt_gst_check_core_elements ())) {
    missing = TRUE;
    res = FALSE;
  }
  // TODO(ensonic): check recording 'formats' -> use GstEncodeBin
  edit_elements = g_list_prepend (NULL, "level");
  if ((missing_elements = bt_gst_check_elements (edit_elements))) {
    missing_edit_elements =
        g_list_concat (missing_edit_elements, g_list_copy (missing_elements));
    missing_edit_elements =
        g_list_append (missing_edit_elements,
        _("-> You will not see any level-meters."));
    missing = TRUE;
  }
  g_list_free (edit_elements);
  edit_elements = g_list_prepend (NULL, "spectrum");
  if ((missing_elements = bt_gst_check_elements (edit_elements))) {
    missing_edit_elements =
        g_list_concat (missing_edit_elements, g_list_copy (missing_elements));
    missing_edit_elements =
        g_list_append (missing_edit_elements,
        _
        ("-> You will not see the frequency spectrum in the analyzer window."));
    missing = TRUE;
  }
  g_list_free (edit_elements);
  edit_elements = g_list_prepend (NULL, "playbin");
  if ((missing_elements = bt_gst_check_elements (edit_elements))) {
    missing_edit_elements =
        g_list_concat (missing_edit_elements, g_list_copy (missing_elements));
    missing_edit_elements =
        g_list_append (missing_edit_elements,
        _
        ("-> Sample playback previews from the wavetable page will not work."));
    missing = TRUE;
  }
  g_list_free (edit_elements);
  // show missing dialog
  if (missing) {
    GtkWidget *dialog;

    if ((dialog =
            GTK_WIDGET (bt_missing_framework_elements_dialog_new
                (missing_core_elements, missing_edit_elements)))) {
      bt_edit_application_attach_child_window (self, GTK_WINDOW (dialog));
      gtk_widget_show_all (dialog);

      gtk_dialog_run (GTK_DIALOG (dialog));
      bt_missing_framework_elements_dialog_apply
          (BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
  }
  // don't free. its static
  //g_list_free(missing_core_elements);
  g_list_free (missing_edit_elements);
  return res;
}

static gboolean
bt_edit_application_run_ui_idle (gpointer user_data)
{
  BtEditApplication *self = BT_EDIT_APPLICATION (user_data);
  BtSettings *settings;
  guint version;
  gboolean res, show_tips;

  g_assert (self->priv->main_window);

  GST_INFO ("application.run_ui launched");

  g_object_get ((gpointer) self, "settings", &settings, NULL);
  g_object_get (settings, "news-seen", &version, "show-tips", &show_tips, NULL);

  if (PACKAGE_VERSION_NUMBER > version) {
    // show about
    bt_edit_application_show_about (self);
    // store new version
    version = PACKAGE_VERSION_NUMBER;
    g_object_set (settings, "news-seen", version, NULL);
  } else if (show_tips) {
    // show tip-of-the-day
    bt_edit_application_show_tip (self);
  }
  g_object_unref (settings);

  // check for missing elements
  if (!(res = bt_edit_application_check_missing (self)))
    gtk_main_quit ();

  // show error from a song we loaded
  if (self->priv->init_err && self->priv->init_file_name) {
    gchar *msg = g_strdup_printf (_("Can't load song '%s'."),
        self->priv->init_file_name);
    bt_dialog_message (self->priv->main_window, _("Can't load song"), msg,
        self->priv->init_err->message);
    g_free (msg);
    g_free (self->priv->init_file_name);
    g_error_free (self->priv->init_err);
  }
  // check for recoverable songs
  bt_edit_application_crash_log_recover (self);

  return FALSE;
}

/*
 * bt_edit_application_run_ui:
 * @self: the edit application
 *
 * Run the user interface. This checks if the users runs a new version for the
 * first time. In this case it show the about dialog. It also runs the gstreamer
 * element checks.
 *
 * Returns: %TRUE for success
 */
static gboolean
bt_edit_application_run_ui (const BtEditApplication * self)
{
  g_idle_add (bt_edit_application_run_ui_idle, (gpointer) self);

  GST_INFO ("before running the UI");
  gtk_main ();
  GST_INFO ("after running the UI");

  return TRUE;
}

//-- constructor methods

/**
 * bt_edit_application_new:
 *
 * Create a new instance on first call and return a reference later on.
 *
 * Returns: the new singleton instance
 */
BtEditApplication *
bt_edit_application_new (void)
{
  return BT_EDIT_APPLICATION (g_object_new (BT_TYPE_EDIT_APPLICATION, NULL));
}

//-- methods

/**
 * bt_edit_application_new_song:
 * @self: the application instance to create a new song in
 *
 * Creates a new blank song instance. If there is a previous song instance it
 * will be freed.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_edit_application_new_song (const BtEditApplication * self)
{
  gboolean res = FALSE;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  gchar *id;
  gulong bars;
  GError *err = NULL;

  g_return_val_if_fail (BT_IS_EDIT_APPLICATION (self), FALSE);

  // create new song
  song = bt_song_new (BT_APPLICATION (self));

  bt_child_proxy_get (song, "setup", &setup, "song-info::bars", &bars, NULL);
  // make initial song length 4 timelines
  bt_child_proxy_set (song, "sequence::length", bars * 4, NULL);
  // add audiosink
  id = bt_setup_get_unique_machine_id (setup, "master");
  machine = BT_MACHINE (bt_sink_machine_new (song, id, &err));
  if (err == NULL) {
    GHashTable *properties;

    GST_DEBUG ("sink-machine=%" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    g_object_get (machine, "properties", &properties, NULL);
    if (properties) {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      g_hash_table_insert (properties, g_strdup ("xpos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, 0.0)));
      g_hash_table_insert (properties, g_strdup ("ypos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, 0.0)));
    }
    if (bt_machine_enable_input_post_level (machine)) {
      GST_DEBUG ("sink-machine=%" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (machine));
      // set new song in application
      g_object_set ((gpointer) self, "song", song, NULL);
      res = TRUE;
    } else {
      GST_WARNING ("Can't add input level/gain element in sink machine");
    }
    GST_DEBUG ("sink-machine=%" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
  } else {
    GST_WARNING ("Can't create sink machine: %s", err->message);
    g_error_free (err);
    gst_object_unref (machine);
  }
  g_free (id);

  self->priv->unsaved = FALSE;
  g_object_notify (G_OBJECT (self), "unsaved");

  // release references
  g_object_unref (setup);
  g_object_unref (song);
  return res;
}

/**
 * bt_edit_application_load_song:
 * @self: the application instance to load a new song in
 * @file_name: the song filename to load
 * @err: where to store the error message in case of an error, or %NULL
 *
 * Loads a new song. If there is a previous song instance it will be freed.
 *
 * Returns: true for success
 */
gboolean
bt_edit_application_load_song (const BtEditApplication * self,
    const char *file_name, GError ** err)
{
  gboolean res = FALSE;
  BtSongIO *loader;
  BtSong *song;

  g_return_val_if_fail (BT_IS_EDIT_APPLICATION (self), FALSE);

  GST_INFO ("song name = %s", file_name);

  if ((loader = bt_song_io_from_file (file_name, err))) {
    BtSetup *setup;
    GList *missing_machines, *missing_waves;

    bt_edit_application_ui_lock (self);
    g_signal_connect (loader, "notify::status",
        G_CALLBACK (on_songio_status_changed), (gpointer) self);

    // create new song and release the previous one
    song = bt_song_new (BT_APPLICATION (self));
    g_object_set ((gpointer) self, "song", NULL, NULL);

#ifdef USE_DEBUG
    // do sanity check that bin is empty
    {
      GstBin *bin;
      g_object_get ((gpointer) self, "bin", &bin, NULL);

      if (GST_BIN_NUMCHILDREN (bin)) {
        GList *node = GST_BIN_CHILDREN (bin);
        GST_WARNING ("bin.num_children=%d has left-overs",
            GST_BIN_NUMCHILDREN (bin));

        while (node) {
          GST_WARNING_OBJECT (node->data, "removing object %"
              G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (node->data));
          gst_bin_remove (bin, GST_ELEMENT (node->data));
          node = GST_BIN_CHILDREN (bin);
        }
      }
      gst_object_unref (bin);
    }
#endif

    // this is synchronous execution
    // https://github.com/Buzztrax/buzztrax/issues/52
    // if we bump glib from 2.32 -> 2.36 we can use GTask and
    // g_task_run_in_thread()
    if (bt_song_io_load (loader, song, err)) {
      BtMachine *machine;

      // get sink-machine
      g_object_get (song, "setup", &setup, NULL);
      if ((machine =
              bt_setup_get_machine_by_type (setup, BT_TYPE_SINK_MACHINE))) {
        if (bt_machine_enable_input_post_level (machine)) {
          // DEBUG
          //bt_song_write_to_highlevel_dot_file(song);
          // DEBUG
          // set new song
          g_object_set ((gpointer) self, "song", song, NULL);
          res = TRUE;
          GST_INFO ("new song activated");
        } else {
          GST_WARNING ("Can't add input level/gain element in sink machine");
        }
        GST_DEBUG ("unreffing stuff after loading");
        g_object_unref (machine);
      } else {
        GST_WARNING ("Can't look up sink machine");
      }
      g_object_unref (setup);
    } else {
      GST_WARNING ("could not load song \"%s\"", file_name);
    }
    self->priv->unsaved = FALSE;
    g_object_notify (G_OBJECT (self), "unsaved");

    bt_edit_application_ui_unlock (self);

    // get missing element info
    bt_child_proxy_get (song, "setup::missing-machines", &missing_machines,
        "wavetable::missing-waves", &missing_waves, NULL);
    // tell about missing machines and/or missing waves
    if (missing_machines || missing_waves) {
      GtkWidget *dialog;

      if ((dialog =
              GTK_WIDGET (bt_missing_song_elements_dialog_new (missing_machines,
                      missing_waves)))) {
        bt_edit_application_attach_child_window (self, GTK_WINDOW (dialog));
        gtk_widget_show_all (dialog);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
      }
    }
    g_object_unref (song);
    g_object_unref (loader);
  } else {
    GST_WARNING ("Unknown extension \"%s\"", file_name);
  }
  return res;
}

/**
 * bt_edit_application_save_song:
 * @self: the application instance to save a song from
 * @file_name: the song filename to save
 * @err: where to store the error message in case of an error, or %NULL
 *
 * Saves a song.
 *
 * Returns: true for success
 */
gboolean
bt_edit_application_save_song (const BtEditApplication * self,
    const char *file_name, GError ** err)
{
  gboolean res = FALSE;
  BtSongIO *saver;

  g_return_val_if_fail (BT_IS_EDIT_APPLICATION (self), FALSE);

  GST_INFO ("song name = %s", file_name);

  if ((saver = bt_song_io_from_file (file_name, err))) {
    gchar *old_file_name = NULL, *bak_file_name = NULL;

    bt_edit_application_ui_lock (self);
    g_signal_connect (saver, "notify::status",
        G_CALLBACK (on_songio_status_changed), (gpointer) self);

    // update the time-stamp
    bt_child_proxy_set (self->priv->song, "song-info::change-dts", NULL, NULL);

    bt_child_proxy_get (self->priv->song, "song-info::file-name",
        &old_file_name, NULL);

    /* save file saving (bak files)
     * save
     *   new file (!old_file_name)
     *     chosen file-name already exist
     *       - move to <existing>.bak
     *       - save newfile
     *       - if saving failed, move <existing>.bak back
     *       - if saving worked, delete <existing>.bak
     *     chosen file-name does not exist
     *       - save newfile
     *   existing file
     *     - move to <existing>.bak
     *       - save newfile
     *       - if saving failed, move <existing>.bak back
     * save-as
     *   new file (!old_file_name)
     *     like save of a new-file
     *   existing file
     *     chosen file-name already exist
     *       - like save of an existing file
     *     chosen file-name does not exist
     *       - save newfile
     *
     * - check how other apps do it (check if inodes change if various scenarios)
     * - when loading a file, should we keep the file-handle open, so then when
     *   saving, we can just update it?
     *   - this can help with the wavetable (only updated changed wavetable slots)
     *   - if we can't update it, we can use gsf_input_copy() for external files
     *     (if unchanged)
     * - if the user deletes the file that is currently open -> user error
     */
    if (g_file_test (file_name, G_FILE_TEST_EXISTS)) {
      bak_file_name = g_strconcat (file_name, ".bak", NULL);
      g_rename (file_name, bak_file_name);
    }
    if (bt_song_io_save (saver, self->priv->song, err)) {
      res = TRUE;
      if (!old_file_name || strcmp (old_file_name, file_name)) {
        // saving worked, we remove the bak file as
        // - there was no old_file_name and/or
        // - user has chosen to overwrite this file
        g_unlink (bak_file_name);
      }
    } else {
      GST_WARNING ("could not save song \"%s\"", file_name);
      if (bak_file_name) {
        // saving failed, so move a file we renamed to .bak back
        g_rename (bak_file_name, file_name);
      }
    }
    GST_INFO ("saving done");
    self->priv->unsaved = FALSE;
    g_object_notify (G_OBJECT (self), "unsaved");
    self->priv->need_dts_reset = TRUE;

    bt_edit_application_ui_unlock (self);

    g_free (old_file_name);
    g_free (bak_file_name);
    g_object_unref (saver);
  } else {
    GST_WARNING ("Unknown extension \"%s\"", file_name);
  }
  return res;
}

/**
 * bt_edit_application_run:
 * @self: the application instance to run
 *
 * Start the gtk based editor application
 *
 * Returns: %TRUE for success
 */
gboolean
bt_edit_application_run (const BtEditApplication * self)
{
  gboolean res = FALSE;

  g_return_val_if_fail (BT_IS_EDIT_APPLICATION (self), FALSE);

  GST_INFO ("application.run launched");

  if (bt_edit_application_new_song (self)) {
    res = bt_edit_application_run_ui (self);
  }
  GST_INFO ("application.run finished");
  return res;
}

/**
 * bt_edit_application_load_and_run:
 * @self: the application instance to run
 * @input_file_name: the file to load initially
 *
 * load the file of the supplied name and start the gtk based editor application
 *
 * Returns: true for success
 */
gboolean
bt_edit_application_load_and_run (const BtEditApplication * self,
    const gchar * input_file_name)
{
  gboolean res = FALSE;

  g_return_val_if_fail (BT_IS_EDIT_APPLICATION (self), FALSE);

  GST_INFO ("application.load_and_run launched");

  if (bt_edit_application_load_song (self, input_file_name,
          &self->priv->init_err)) {
    res = bt_edit_application_run_ui (self);
  } else {
    GST_WARNING ("loading song '%s' failed", input_file_name);
    self->priv->init_file_name = g_strdup (input_file_name);
    // start normaly
    bt_edit_application_run (self);
  }
  GST_INFO ("application.load_and_run finished");
  return res;
}

/**
 * bt_edit_application_quit:
 * @self: the application instance to quit
 *
 * End the application. Eventually asks the user for confirmation.
 *
 * Returns: %TRUE it ending the application was confirmed
 */
gboolean
bt_edit_application_quit (const BtEditApplication * self)
{
  if (bt_main_window_check_quit (self->priv->main_window)) {
    BtSettings *settings;
    gint x, y, w, h;

    // remember window size
    g_object_get ((gpointer) self, "settings", &settings, NULL);
    gtk_window_get_position (GTK_WINDOW (self->priv->main_window), &x, &y);
    gtk_window_get_size (GTK_WINDOW (self->priv->main_window), &w, &h);
    g_object_set (settings, "window-xpos", x, "window-ypos", y, "window-width",
        w, "window-height", h, NULL);
    g_object_unref (settings);

    return TRUE;
  }
  return FALSE;
}

/**
 * bt_edit_application_show_about:
 * @self: the application instance
 *
 * Shows the applications about window
 */
void
bt_edit_application_show_about (const BtEditApplication * self)
{
  GtkWidget *dialog;

  if ((dialog = GTK_WIDGET (bt_about_dialog_new ()))) {
    bt_edit_application_attach_child_window (self, GTK_WINDOW (dialog));
    gtk_widget_show_all (dialog);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
}

/**
 * bt_edit_application_show_tip:
 * @self: the application instance
 *
 * Shows the tip of the day window
 *
 * Since: 0.6
 */
void
bt_edit_application_show_tip (const BtEditApplication * self)
{
  GtkWidget *dialog;

  if ((dialog = GTK_WIDGET (bt_tip_dialog_new ()))) {
    bt_edit_application_attach_child_window (self, GTK_WINDOW (dialog));
    gtk_widget_show_all (dialog);

    while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_NONE);
    gtk_widget_destroy (dialog);
  }
}

/**
 * bt_edit_application_crash_log_recover:
 * @self: the application instance
 *
 * Shows the crash-log recover window if we have pending crash logs.
 *
 * Since: 0.6
 */
void
bt_edit_application_crash_log_recover (const BtEditApplication * self)
{
  GList *crash_logs;

  g_object_get (self->priv->change_log, "crash-logs", &crash_logs, NULL);
  if (crash_logs) {
    GtkWidget *dialog;

    GST_INFO ("have found crash logs");
    if ((dialog = GTK_WIDGET (bt_crash_recover_dialog_new (crash_logs)))) {
      bt_edit_application_attach_child_window (self, GTK_WINDOW (dialog));
      gtk_widget_show_all (dialog);

      while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_NONE);
      gtk_widget_destroy (dialog);
    }
  }
}


/**
 * bt_edit_application_attach_child_window:
 * @self: the application instance
 * @window: a child window (e.g. dialog)
 *
 * The parent and transient relation ship to the applications main-window.
 *
 * Since: 0.6
 */
void
bt_edit_application_attach_child_window (const BtEditApplication * self,
    GtkWindow * window)
{
  gtk_window_set_transient_for (window, GTK_WINDOW (self->priv->main_window));
  gtk_window_set_destroy_with_parent (window, TRUE);
}


/**
 * bt_edit_application_ui_lock:
 * @self: the application instance
 *
 * Sets the main window insensitive and show a wait cursor.
 */
void
bt_edit_application_ui_lock (const BtEditApplication * self)
{
  GdkCursor *cursor =
      gdk_cursor_new_for_display (gdk_display_get_default (), GDK_WATCH);

  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self->
              priv->main_window)), cursor);
  g_object_unref (cursor);
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->main_window), FALSE);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

/**
 * bt_edit_application_ui_unlock:
 * @self: the application instance
 *
 * Sets the main window sensitive again and unset the wait cursor.
 */
void
bt_edit_application_ui_unlock (const BtEditApplication * self)
{
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->main_window), TRUE);
  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (self->
              priv->main_window)), NULL);
}

/**
 * bt_edit_application_is_song_unsaved:
 * @self: the application instance
 *
 * Check if the song has unsaved changes.
 *
 * Returns: %TRUE if there are pending changes
 */
gboolean
bt_edit_application_is_song_unsaved (const BtEditApplication * self)
{
  gboolean unsaved = self->priv->unsaved;

  if (!unsaved)
    g_object_get (self->priv->change_log, "can-undo", &unsaved, NULL);
  return unsaved;
}

/**
 * bt_edit_application_set_song_unsaved:
 * @self: the application instance
 *
 * Flag unsaved changes in the applications song.
 */
void
bt_edit_application_set_song_unsaved (const BtEditApplication * self)
{
  /* this is not successfully preventing setting the flag when we create an
   * undo-able change
   gboolean can_undo;

   g_object_get(self->priv->change_log,"can-undo",&can_undo,NULL);
   if((!self->priv->unsaved) && (!can_undo)) {
   */
  if (!self->priv->unsaved) {
    self->priv->unsaved = TRUE;
    g_object_notify (G_OBJECT (self), "unsaved");
  }

}

//-- wrapper

//-- default signal handler

//-- class internals

static void
bt_edit_application_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtEditApplication *self = BT_EDIT_APPLICATION (object);
  return_if_disposed ();
  switch (property_id) {
    case EDIT_APPLICATION_SONG:
      g_value_set_object (value, self->priv->song);
      break;
    case EDIT_APPLICATION_MAIN_WINDOW:
      g_value_set_object (value, self->priv->main_window);
      break;
    case EDIT_APPLICATION_IC_REGISTRY:
      g_value_set_object (value, self->priv->ic_registry);
      break;
    case EDIT_APPLICATION_UNSAVED:
      g_value_set_boolean (value, self->priv->unsaved);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_edit_application_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtEditApplication *self = BT_EDIT_APPLICATION (object);
  return_if_disposed ();
  switch (property_id) {
    case EDIT_APPLICATION_SONG:
#ifdef USE_DEBUG
      if (G_OBJECT_REF_COUNT (self->priv->song) != 1) {
        GST_DEBUG ("old song: %" G_OBJECT_REF_COUNT_FMT,
            G_OBJECT_LOG_REF_COUNT (self->priv->song));
      }
#endif
      g_object_try_unref (self->priv->song);
      self->priv->song = BT_SONG (g_value_dup_object (value));
      GST_DEBUG ("new song: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (self->priv->song));
      break;
    case EDIT_APPLICATION_UNSAVED:
      self->priv->unsaved = g_value_get_boolean (value);
      GST_INFO ("set the unsaved flag to %d for the song", self->priv->unsaved);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static GObject *
bt_edit_application_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  GObject *object;

  if (G_UNLIKELY (!singleton)) {
    object =
        G_OBJECT_CLASS (bt_edit_application_parent_class)->constructor (type,
        n_construct_params, construct_params);
    singleton = BT_EDIT_APPLICATION (object);
    g_object_add_weak_pointer (object, (gpointer *) (gpointer) & singleton);

    //GST_DEBUG("<<<");
    GST_INFO ("new edit app instantiated");
    // create or ref the shared ui resources
    singleton->priv->ui_resources = bt_ui_resources_new ();

    // create the interaction controller registry
    singleton->priv->ic_registry = btic_registry_new ();
    btic_registry_start_discovery ();

    // create the playback controllers (we need to create them all as they watch
    // the settings them self)
    singleton->priv->pbc_socket = bt_playback_controller_socket_new ();
    singleton->priv->pbc_ic = bt_playback_controller_ic_new ();

    // create the editor change log
    singleton->priv->change_log = bt_change_log_new ();
    g_signal_connect (singleton->priv->change_log, "notify::can-undo",
        G_CALLBACK (on_changelog_can_undo_changed), (gpointer) singleton);
    // create the audio_session
    singleton->priv->audio_session = bt_audio_session_new ();

    // create main window
    GST_INFO ("new edit app created, app: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (singleton));
    singleton->priv->main_window = bt_main_window_new ();
    gtk_widget_show_all (GTK_WIDGET (singleton->priv->main_window));
    GST_INFO ("new main_window shown");

    // warning: dereferencing type-punned pointer will break strict-aliasing rules
    g_object_add_weak_pointer (G_OBJECT (singleton->priv->main_window),
        (gpointer *) (gpointer) & singleton->priv->main_window);
    GST_INFO ("new edit app created, app: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (singleton));
    //GST_DEBUG(">>>");
  } else {
    object = g_object_ref ((gpointer) singleton);
  }
  return object;
}


static void
bt_edit_application_dispose (GObject * object)
{
  BtEditApplication *self = BT_EDIT_APPLICATION (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  /* This should destroy the window as this is a child of the app.
   * Problem 1: On the other hand, this *NEVER* gets called as long as the window keeps its
   * strong reference to the app.
   * Solution 1: Only use weak refs when reffing upstream objects
   */
  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->song) {
    GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->song));
    bt_song_stop (self->priv->song);
    g_object_unref (self->priv->song);
    self->priv->song = NULL;
  }

  if (self->priv->main_window) {
    GST_INFO ("main_window: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->main_window));
    //main-menu.c::on_menu_quit_activate
    gtk_widget_destroy (GTK_WIDGET (self->priv->main_window));
  }

  GST_DEBUG ("  more unrefs");
  g_object_try_unref (self->priv->ui_resources);
  g_object_try_unref (self->priv->pbc_socket);
  g_object_try_unref (self->priv->pbc_ic);
  g_object_try_unref (self->priv->ic_registry);
  g_object_try_unref (self->priv->change_log);
  g_object_try_unref (self->priv->audio_session);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_edit_application_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_edit_application_init (BtEditApplication * self)
{
  self->priv = bt_edit_application_get_instance_private(self);
}

static void
bt_edit_application_class_init (BtEditApplicationClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = bt_edit_application_constructor;
  gobject_class->set_property = bt_edit_application_set_property;
  gobject_class->get_property = bt_edit_application_get_property;
  gobject_class->dispose = bt_edit_application_dispose;

  klass->song_changed = NULL;

  g_object_class_install_property (gobject_class, EDIT_APPLICATION_SONG,
      g_param_spec_object ("song", "song prop", "the current song object",
          BT_TYPE_SONG, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, EDIT_APPLICATION_MAIN_WINDOW,
      g_param_spec_object ("main-window", "main window prop",
          "the main window of this application", BT_TYPE_MAIN_WINDOW,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, EDIT_APPLICATION_IC_REGISTRY,
      g_param_spec_object ("ic-registry", "ic registry prop",
          "the interaction controller registry of this application",
          BTIC_TYPE_REGISTRY, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, EDIT_APPLICATION_UNSAVED,
      g_param_spec_boolean ("unsaved",
          "unsaved prop",
          "tell whether the current state of the song has been saved",
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}
