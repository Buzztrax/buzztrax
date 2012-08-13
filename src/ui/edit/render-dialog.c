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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btrenderdialog
 * @short_description: class for the editor render dialog
 *
 * Provides UI to access the song recording
 */
/* TODO(ensonic):
 * - use chooserbutton to choose name too
 *   (arg, the chooser button cannot do save_as)
 * - use song-name based file-name by default
 *
 * TODO(ensonic): more options
 * - have dithering and noise shaping options here
 *
 * TODO(ensonic): include the progressbar here and remove render-progress.{c,h}
 */

#define BT_EDIT
#define BT_RENDER_DIALOG_C

#include "bt-edit.h"

struct _BtRenderDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* dialog widgets */
  GtkWidget *okay_button;
  GtkWidget *dir_chooser;
  GtkWidget *file_name_entry;
  GtkWidget *format_menu;
  GtkWidget *mode_menu;
  GtkProgressBar *track_progress;
  GtkLabel *info;

  /* dialog data */
  gchar *folder, *filename;
  BtSinkBinRecordFormat format;
  BtRenderMode mode;

  /* render state data (incl. save song state data) */
  BtSong *song;
  BtSinkBin *sink_bin;
  GstElement *convert;
  GList *list;
  gint track, tracks;
  gchar *song_name, *file_name;
  gboolean unsaved, has_error;
};

static void on_format_menu_changed (GtkComboBox * menu, gpointer user_data);

//-- the class

G_DEFINE_TYPE (BtRenderDialog, bt_render_dialog, GTK_TYPE_DIALOG);


//-- enums

GType
bt_render_mode_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_RENDER_MODE_MIXDOWN, "BT_RENDER_MODE_MIXDOWN", "mix to one track"},
      {BT_RENDER_MODE_SINGLE_TRACKS, "BT_RENDER_MODE_SINGLE_TRACKS",
            "record one track for each source"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtRenderMode", values);
  }
  return type;
}

//-- event handler helper

static gchar *
bt_render_dialog_make_file_name (const BtRenderDialog * self, gint track)
{
  gchar *file_name =
      g_build_filename (self->priv->folder, self->priv->filename, NULL);
  gchar track_str[4];
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  if (self->priv->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    g_sprintf (track_str, ".%03u", track);
  } else {
    track_str[0] = '\0';
  }

  // add file suffix if not yet there
  enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
  enum_value = g_enum_get_value (enum_class, self->priv->format);
  if (!g_str_has_suffix (file_name, enum_value->value_name)) {
    gchar *tmp_file_name;

    // append extension
    tmp_file_name = g_strdup_printf ("%s%s%s",
        file_name, track_str, enum_value->value_name);
    g_free (file_name);
    file_name = tmp_file_name;
  } else {
    gchar *tmp_file_name;

    // reuse extension
    file_name[strlen (file_name) - strlen (enum_value->value_name)] = '\0';
    tmp_file_name = g_strdup_printf ("%s%s%s",
        file_name, track_str, enum_value->value_name);
    g_free (file_name);
    file_name = tmp_file_name;
  }
  GST_INFO ("record file template: '%s'", file_name);

  return (file_name);
}

static void
bt_render_dialog_record (const BtRenderDialog * self)
{
  gchar *info;

  GST_INFO ("recording to '%s'", self->priv->file_name);
  g_object_set (self->priv->sink_bin, "record-file-name", self->priv->file_name,
      NULL);
  info = g_strdup_printf (_("Recording to: %s"), self->priv->file_name);
  gtk_label_set_text (self->priv->info, info);
  g_free (info);

  bt_song_play (self->priv->song);
  bt_song_write_to_lowlevel_dot_file (self->priv->song);
}

/* Prepare the rendering. */
static void
bt_render_dialog_record_init (const BtRenderDialog * self)
{
  BtSetup *setup;
  BtMachine *machine;

  g_object_get (self->priv->app, "unsaved", &self->priv->unsaved, NULL);
  g_object_get (self->priv->song, "setup", &setup, NULL);
  // lookup the audio-sink machine and change mode
  if ((machine = bt_setup_get_machine_by_type (setup, BT_TYPE_SINK_MACHINE))) {
    g_object_get (machine, "machine", &self->priv->sink_bin, "adder-convert",
        &self->priv->convert, NULL);

    g_object_set (self->priv->sink_bin, "mode", BT_SINK_BIN_MODE_RECORD,
        // TODO(ensonic): this hangs :/
        //"mode",BT_SINK_BIN_MODE_PLAY_AND_RECORD,
        "record-format", self->priv->format, NULL);

    /* TODO(ensonic): configure dithering/noise-shaping
     * - should sink-bin do it so that we get this also when recording from
     *   the commandline (need some extra cmdline options for it :/
     * - we could also put it to the options
     * - sink-machine could also set this (hard-coded) when going to record mode
     */
    g_object_set (self->priv->convert, "dithering", 2, "noise-shaping", 3,
        NULL);

    g_object_unref (machine);
  }
  if (self->priv->mode == BT_RENDER_MODE_MIXDOWN) {
    self->priv->track = -1;
    self->priv->tracks = 0;
  } else {
    BtSongInfo *song_info;

    g_object_get (self->priv->song, "song-info", &song_info, NULL);

    self->priv->list =
        bt_setup_get_machines_by_type (setup, BT_TYPE_SOURCE_MACHINE);
    self->priv->track = 0;
    self->priv->tracks = g_list_length (self->priv->list);

    g_object_get (song_info, "name", &self->priv->song_name, NULL);
    g_object_unref (song_info);
  }
  g_object_unref (setup);
}

/* Cleanup the rendering */
static void
bt_render_dialog_record_done (const BtRenderDialog * self)
{
  /* reset play mode */
  g_object_set (self->priv->sink_bin, "mode", BT_SINK_BIN_MODE_PLAY, NULL);

  /* reset dithering/noise-shaping */
  g_object_set (self->priv->convert, "dithering", 0, "noise-shaping", 0, NULL);

  g_object_set (self->priv->app, "unsaved", self->priv->unsaved, NULL);

  if (self->priv->list) {
    g_list_free (self->priv->list);
    self->priv->list = NULL;
  }
  if (self->priv->song_name) {
    BtSongInfo *song_info;

    g_object_get (self->priv->song, "song-info", &song_info, NULL);
    g_object_set (song_info, "name", self->priv->song_name, NULL);
    g_object_unref (song_info);
    g_free (self->priv->song_name);
  }
  gst_object_replace ((GstObject **) & self->priv->convert, NULL);
  gst_object_replace ((GstObject **) & self->priv->sink_bin, NULL);

  // close the dialog
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_NONE);
}

/* Run the rendering. */
static void
bt_render_dialog_record_next (const BtRenderDialog * self)
{
  static BtMachine *machine = NULL;

  /* cleanup from previous run */
  if (self->priv->has_error && self->priv->file_name) {
    g_unlink (self->priv->file_name);
  }
  if (self->priv->tracks && machine) {
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  if (self->priv->file_name) {
    g_free (self->priv->file_name);
    self->priv->file_name = NULL;
  }
  /* if there was an error or we're done */
  if (self->priv->has_error || self->priv->track == self->priv->tracks) {
    bt_render_dialog_record_done (self);
    return;
  }

  self->priv->file_name =
      bt_render_dialog_make_file_name (self, self->priv->track);

  if (self->priv->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    BtSongInfo *song_info;
    gchar *track_name, *id;

    machine =
        BT_MACHINE (g_list_nth_data (self->priv->list, self->priv->track));
    g_object_set (machine, "state", BT_MACHINE_STATE_SOLO, NULL);

    g_object_get (machine, "id", &id, NULL);
    track_name = g_strdup_printf ("%s : %s", self->priv->song_name, id);
    g_object_get (self->priv->song, "song-info", &song_info, NULL);
    g_object_set (song_info, "name", track_name, NULL);
    g_object_unref (song_info);
    g_free (track_name);
    g_free (id);
  }
  bt_render_dialog_record (self);
  self->priv->track++;
}

//-- event handler

static void
on_folder_changed (GtkFileChooser * chooser, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_free (self->priv->folder);
  self->priv->folder = gtk_file_chooser_get_current_folder (chooser);

  GST_DEBUG ("folder changed '%s'", self->priv->folder);
}

static void
on_filename_changed (GtkEditable * editable, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_free (self->priv->filename);
  self->priv->filename = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));

  // update format
  if (self->priv->filename) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;

    enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
      if ((enum_value = g_enum_get_value (enum_class, i))) {
        if (g_str_has_suffix (self->priv->filename, enum_value->value_name)) {
          g_signal_handlers_block_matched (self->priv->format_menu,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_format_menu_changed, (gpointer) user_data);
          gtk_combo_box_set_active (GTK_COMBO_BOX (self->priv->format_menu), i);
          g_signal_handlers_unblock_matched (self->priv->format_menu,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_format_menu_changed, (gpointer) user_data);
          break;
        }
      }
    }
  }
}

static void
on_format_menu_changed (GtkComboBox * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->priv->format = gtk_combo_box_get_active (menu);

  // update filename
  if (self->priv->filename) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;
    gchar *fn = self->priv->filename;

    enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
      if ((enum_value = g_enum_get_value (enum_class, i))) {
        if (g_str_has_suffix (fn, enum_value->value_name)) {
          GST_INFO ("found matching ext: %s", enum_value->value_name);
          // replace this suffix, with the new
          fn[strlen (fn) - strlen (enum_value->value_name)] = '\0';
          GST_INFO ("cut fn to: %s", fn);
          break;
        }
      }
    }
    enum_value = g_enum_get_value (enum_class, self->priv->format);
    self->priv->filename = g_strdup_printf ("%s%s", fn, enum_value->value_name);
    GST_INFO ("set new fn to: %s", self->priv->filename);
    g_free (fn);
    g_signal_handlers_block_matched (self->priv->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_filename_changed, (gpointer) user_data);
    gtk_entry_set_text (GTK_ENTRY (self->priv->file_name_entry),
        self->priv->filename);
    g_signal_handlers_unblock_matched (self->priv->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_filename_changed, (gpointer) user_data);
  }
}

static void
on_mode_menu_changed (GtkComboBox * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->priv->mode = gtk_combo_box_get_active (menu);
}

static void
on_okay_clicked (GtkButton * button, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  // gray the okay button and the action area
  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  gtk_widget_set_sensitive (self->priv->dir_chooser, FALSE);
  gtk_widget_set_sensitive (self->priv->file_name_entry, FALSE);
  gtk_widget_set_sensitive (self->priv->format_menu, FALSE);
  gtk_widget_set_sensitive (self->priv->mode_menu, FALSE);
  // start the rendering
  bt_render_dialog_record_init (self);
  bt_render_dialog_record_next (self);
}

static void
on_song_play_pos_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);
  BtSequence *sequence;
  gulong pos, length;
  gdouble progress;
  // the +4 is not really needed, but I get a stack smashing error on ubuntu without
  gchar str[3 + 2 * (2 + 2 + 3 + 3) + 4];
  gulong msec1, sec1, min1, msec2, sec2, min2;
  GstClockTime bar_time;

  g_object_get ((gpointer) song, "sequence", &sequence, "play-pos", &pos, NULL);
  g_object_get (sequence, "length", &length, NULL);
  bar_time = bt_sequence_get_bar_time (sequence);

  progress = (gdouble) pos / (gdouble) length;
  if (progress >= 1.0) {
    progress = 1.0;
    bt_song_stop (song);
    // trigger next file / done
    bt_render_dialog_record_next (self);
  }
  GST_INFO ("progress %ld/%ld=%lf", pos, length, progress);

  msec1 = (gulong) ((pos * bar_time) / G_USEC_PER_SEC);
  min1 = (gulong) (msec1 / 60000);
  msec1 -= (min1 * 60000);
  sec1 = (gulong) (msec1 / 1000);
  msec1 -= (sec1 * 1000);
  msec2 = (gulong) ((length * bar_time) / G_USEC_PER_SEC);
  min2 = (gulong) (msec2 / 60000);
  msec2 -= (min2 * 60000);
  sec2 = (gulong) (msec2 / 1000);
  msec2 -= (sec2 * 1000);
  // format
  g_sprintf (str, "%02lu:%02lu.%03lu / %02lu:%02lu.%03lu", min1, sec1, msec1,
      min2, sec2, msec2);

  g_object_set (self->priv->track_progress, "fraction", progress, "text", str,
      NULL);

  g_object_unref (sequence);
}

static void
on_song_error (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  GST_WARNING ("received %s bus message from %s",
      GST_MESSAGE_TYPE_NAME (message),
      GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));

  self->priv->has_error = TRUE;
  bt_render_dialog_record_next (self);
}


//-- helper methods

static void
bt_render_dialog_init_ui (const BtRenderDialog * self)
{
  BtSettings *settings;
  GtkWidget *box, *label, *widget, *table;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint i;
  BtSongInfo *song_info;
  GstBin *bin;
  GstBus *bus;
  gchar *full_file_name = NULL;

  GST_DEBUG ("read settings");

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "record-folder", &self->priv->folder, NULL);
  g_object_unref (settings);
  g_object_get (self->priv->song, "song-info", &song_info, "bin", &bin, NULL);

  GST_DEBUG ("prepare render dialog");

  gtk_widget_set_name (GTK_WIDGET (self), "song rendering");

  gtk_window_set_title (GTK_WINDOW (self), _("song rendering"));

  // add dialog commision widgets (okay, cancel)
  self->priv->okay_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG
              (self))), self->priv->okay_button);
  gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL,
      GTK_RESPONSE_REJECT);

  //gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_NONE);
  g_signal_connect (self->priv->okay_button, "clicked",
      G_CALLBACK (on_okay_clicked), (gpointer) self);

  // add widgets to the dialog content area
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG
              (self))), box);

  table =
      gtk_table_new ( /*rows= */ 7, /*columns= */ 2, /*homogenous= */ FALSE);
  gtk_container_add (GTK_CONTAINER (box), table);


  label = gtk_label_new (_("Folder"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_SHRINK,
      2, 1);

  self->priv->dir_chooser = widget =
      gtk_file_chooser_button_new (_("Select a folder"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
      self->priv->folder);
  g_signal_connect (widget, "selection-changed", G_CALLBACK (on_folder_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 0, 1,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);


  label = gtk_label_new (_("Filename"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_SHRINK,
      2, 1);

  // set deault name
  g_object_get (song_info, "file-name", &full_file_name, NULL);
  if (full_file_name) {
    gchar *file_name, *ext;

    // cut off extension from file_name
    if ((ext = strrchr (full_file_name, '.')))
      *ext = '\0';
    if ((file_name = strrchr (full_file_name, G_DIR_SEPARATOR)))
      file_name = &file_name[1];
    else
      file_name = full_file_name;
    self->priv->filename = g_strdup_printf ("%s.ogg", file_name);
    g_free (full_file_name);
  } else {
    self->priv->filename = g_strdup_printf (".ogg");
  }

  self->priv->file_name_entry = widget = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (widget), self->priv->filename);
  gtk_entry_set_activates_default (GTK_ENTRY (self->priv->file_name_entry),
      TRUE);
  g_signal_connect (widget, "changed", G_CALLBACK (on_filename_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 1, 2,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);


  label = gtk_label_new (_("Format"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_SHRINK,
      2, 1);

  // query supported formats from sinkbin
  self->priv->format_menu = widget = gtk_combo_box_text_new ();
  enum_class = g_type_class_ref (BT_TYPE_SINK_BIN_RECORD_FORMAT);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
          enum_value->value_nick);
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
      BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS);
  g_signal_connect (widget, "changed", G_CALLBACK (on_format_menu_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 2, 3,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);


  label = gtk_label_new (_("Mode"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_SHRINK,
      2, 1);

  // query supported formats from sinkbin
  self->priv->mode_menu = widget = gtk_combo_box_text_new ();
  enum_class = g_type_class_ref (BT_TYPE_RENDER_MODE);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
          enum_value->value_nick);
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), BT_RENDER_MODE_MIXDOWN);
  g_signal_connect (widget, "changed", G_CALLBACK (on_mode_menu_changed),
      (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 3, 4,
      GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

  /* TODO(ensonic): add more widgets
     o write project file
     o none, jokosher, ardour, ...
   */

  self->priv->info = GTK_LABEL (gtk_label_new (""));
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (self->priv->info), 0, 2, 4,
      5, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

  self->priv->track_progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (self->priv->track_progress),
      0, 2, 5, 6, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 2, 1);

  // connect signal handlers    
  g_signal_connect (self->priv->song, "notify::play-pos",
      G_CALLBACK (on_song_play_pos_notify), (gpointer) self);
  bus = gst_element_get_bus (GST_ELEMENT (bin));
  g_signal_connect (bus, "message::error", G_CALLBACK (on_song_error),
      (gpointer) self);

  gst_object_unref (bus);
  gst_object_unref (bin);
  g_object_unref (song_info);
  GST_DEBUG ("done");
}

//-- constructor methods

/**
 * bt_render_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtRenderDialog *
bt_render_dialog_new (void)
{
  BtRenderDialog *self;

  self = BT_RENDER_DIALOG (g_object_new (BT_TYPE_RENDER_DIALOG, NULL));
  g_object_get (self->priv->app, "song", &self->priv->song, NULL);
  bt_render_dialog_init_ui (self);
  return (self);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_render_dialog_dispose (GObject * object)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->file_name) {
    GST_INFO ("canceled recording, removing partial file");
    g_unlink (self->priv->file_name);
    g_free (self->priv->file_name);
  }

  if (self->priv->song) {
    GstBin *bin;

    g_object_get (self->priv->song, "bin", &bin, NULL);
    if (bin) {
      GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
      g_signal_handlers_disconnect_matched (bus, G_SIGNAL_MATCH_FUNC, 0, 0,
          NULL, on_song_error, NULL);
      gst_object_unref (bus);
    }
    g_signal_handlers_disconnect_matched (self->priv->song, G_SIGNAL_MATCH_FUNC,
        0, 0, NULL, on_song_play_pos_notify, NULL);
    gst_object_unref (bin);
    g_object_unref (self->priv->song);
  }
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->dispose (object);
}

static void
bt_render_dialog_finalize (GObject * object)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_free (self->priv->folder);
  g_free (self->priv->filename);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->finalize (object);
}

static void
bt_render_dialog_init (BtRenderDialog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_RENDER_DIALOG,
      BtRenderDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_render_dialog_class_init (BtRenderDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtRenderDialogPrivate));

  gobject_class->dispose = bt_render_dialog_dispose;
  gobject_class->finalize = bt_render_dialog_finalize;
}
