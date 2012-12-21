/* Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btsettingspagedirectories
 * @short_description: default directory configuration settings page
 *
 * Select default directories for songs, samples, recordings, etc.
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_DIRECTORIES_C

#include "bt-edit.h"

struct _BtSettingsPageDirectoriesPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE (BtSettingsPageDirectories, bt_settings_page_directories,
    GTK_TYPE_TABLE);



//-- event handler

static void
on_folder_changed (GtkFileChooser * chooser, gpointer user_data)
{
  BtSettingsPageDirectories *self = BT_SETTINGS_PAGE_DIRECTORIES (user_data);
  gchar *new_folder;

  if ((new_folder = gtk_file_chooser_get_current_folder (chooser))) {
    BtSettings *settings;
    const gchar *folder_name = gtk_widget_get_name (GTK_WIDGET (chooser));
    gchar *old_folder;

    g_object_get (self->priv->app, "settings", &settings, NULL);
    g_object_get (settings, folder_name, &old_folder, NULL);
    if (!old_folder || strcmp (old_folder, new_folder)) {
      // store in settings
      g_object_set (settings, folder_name, new_folder, NULL);
    }
    g_free (new_folder);
    g_free (old_folder);
    g_object_unref (settings);
  }
}


//-- helper methods

static void
bt_settings_page_directories_init_ui (const BtSettingsPageDirectories * self)
{
  BtSettings *settings;
  GtkWidget *label, *spacer, *widget;
  gchar *str;
  gchar *song_folder, *record_folder, *sample_folder;

  gtk_widget_set_name (GTK_WIDGET (self), "default directory settings");

  // get settings
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "song-folder", &song_folder, "record-folder",
      &record_folder, "sample-folder", &sample_folder, NULL);

  // add setting widgets
  spacer = gtk_label_new ("    ");
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Directories"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 0, 3, 0, 1, GTK_FILL | GTK_EXPAND,
      GTK_SHRINK, 2, 1);
  gtk_table_attach (GTK_TABLE (self), spacer, 0, 1, 1, 4, GTK_SHRINK,
      GTK_SHRINK, 2, 1);

  label = gtk_label_new (_("Songs"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 1, 2, GTK_FILL, GTK_SHRINK,
      2, 1);

  widget =
      gtk_file_chooser_button_new (_("Select a folder"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_widget_set_name (widget, "song-folder");
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget), song_folder);
  g_signal_connect (widget, "selection-changed", G_CALLBACK (on_folder_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (self), widget, 2, 3, 1, 2, GTK_FILL | GTK_EXPAND,
      GTK_SHRINK, 2, 1);

  label = gtk_label_new (_("Recordings"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK,
      2, 1);

  widget =
      gtk_file_chooser_button_new (_("Select a folder"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_widget_set_name (widget, "record-folder");
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
      record_folder);
  g_signal_connect (widget, "selection-changed", G_CALLBACK (on_folder_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (self), widget, 2, 3, 2, 3, GTK_FILL | GTK_EXPAND,
      GTK_SHRINK, 2, 1);

  label = gtk_label_new (_("Waveforms"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (self), label, 1, 2, 3, 4, GTK_FILL, GTK_SHRINK,
      2, 1);

  widget =
      gtk_file_chooser_button_new (_("Select a folder"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_widget_set_name (widget, "sample-folder");
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
      sample_folder);
  g_signal_connect (widget, "selection-changed", G_CALLBACK (on_folder_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (self), widget, 2, 3, 3, 4, GTK_FILL | GTK_EXPAND,
      GTK_SHRINK, 2, 1);

  g_free (song_folder);
  g_free (record_folder);
  g_free (sample_folder);
  g_object_unref (settings);
}

//-- constructor methods

/**
 * bt_settings_page_directories_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageDirectories *
bt_settings_page_directories_new (void)
{
  BtSettingsPageDirectories *self;

  self =
      BT_SETTINGS_PAGE_DIRECTORIES (g_object_new
      (BT_TYPE_SETTINGS_PAGE_DIRECTORIES, "n-rows", 4, "n-columns", 3,
          "homogeneous", FALSE, NULL));
  bt_settings_page_directories_init_ui (self);
  gtk_widget_show_all (GTK_WIDGET (self));
  return (self);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_page_directories_dispose (GObject * object)
{
  BtSettingsPageDirectories *self = BT_SETTINGS_PAGE_DIRECTORIES (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_directories_parent_class)->dispose (object);
}

static void
bt_settings_page_directories_finalize (GObject * object)
{
  BtSettingsPageDirectories *self = BT_SETTINGS_PAGE_DIRECTORIES (object);

  GST_DEBUG ("!!!! self=%p", self);

  G_OBJECT_CLASS (bt_settings_page_directories_parent_class)->finalize (object);
}

static void
bt_settings_page_directories_init (BtSettingsPageDirectories * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SETTINGS_PAGE_DIRECTORIES,
      BtSettingsPageDirectoriesPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_settings_page_directories_class_init (BtSettingsPageDirectoriesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSettingsPageDirectoriesPrivate));

  gobject_class->dispose = bt_settings_page_directories_dispose;
  gobject_class->finalize = bt_settings_page_directories_finalize;
}
