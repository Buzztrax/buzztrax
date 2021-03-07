/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btrenderdialog
 * @short_description: class for the editor render dialog
 *
 * Provides UI to access the song recording
 */
/* TODO(ensonic):
 * - use chooserbutton to choose name too
 *   (arg, the chooser button cannot do save_as)
 * - use song-name based file-name by default
 */
/* TODO(ensonic): more options
 * - have dithering and noise shaping options here
 */
/* TODO(ensonic): project file export
 * - consider MXF as one option
 */
/* TODO(ensonic): kick the plugin installer when one selects a format where we
 * miss some plugins
 */
/* TODO(ensonic):
 * - remember the format, should we store this with song or as a setting or
 *   just remember for the session
 */

#define BT_EDIT
#define BT_RENDER_DIALOG_C

#include "bt-edit.h"
#include <glib/gprintf.h>
#include <glib/gstdio.h>

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
  GtkWidget *info_bar;
  GtkWidget *info_bar_label;

  /* dialog data */
  gchar *folder, *base_file_name;
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
  gboolean saved_loop;
  gboolean saved_follow_playback;
};

static void on_format_menu_changed (GtkComboBox * menu, gpointer user_data);

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtRenderDialog, bt_render_dialog, GTK_TYPE_DIALOG, 
    G_ADD_PRIVATE(BtRenderDialog));


//-- enums

GType
bt_render_mode_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_RENDER_MODE_MIXDOWN, "BT_RENDER_MODE_MIXDOWN",
          N_("mix to one track")},
      {BT_RENDER_MODE_SINGLE_TRACKS, "BT_RENDER_MODE_SINGLE_TRACKS",
          N_("record one track for each source")},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtRenderMode", values);
  }
  return type;
}

//-- helper

static void
bt_render_dialog_enable_level_meters (BtSetup * setup, gboolean enable)
{
  GList *machines, *node;
  BtMachine *machine;
  GstElement *in_pre_level, *in_post_level, *out_pre_level, *out_post_level;

  g_object_get (setup, "machines", &machines, NULL);
  for (node = machines; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    g_object_get (machine, "input-pre-level", &in_pre_level, "input-post-level",
        &in_post_level, "output-pre-level", &out_pre_level, "output-post-level",
        &out_post_level, NULL);
    if (in_pre_level) {
      g_object_set (in_pre_level, "post-messages", enable, NULL);
      gst_object_unref (in_pre_level);
    }
    if (in_post_level) {
      g_object_set (in_post_level, "post-messages", enable, NULL);
      gst_object_unref (in_post_level);
    }
    if (out_pre_level) {
      g_object_set (out_pre_level, "post-messages", enable, NULL);
      gst_object_unref (out_pre_level);
    }
    if (out_post_level) {
      g_object_set (out_post_level, "post-messages", enable, NULL);
      gst_object_unref (out_post_level);
    }
  }
  g_list_free (machines);
}

//-- event handler helper

static gchar *
bt_render_dialog_make_file_name (const BtRenderDialog * self, gint track)
{
  gchar *file_name =
      g_build_filename (self->priv->folder, self->priv->base_file_name, NULL);
  gchar track_str[4];
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  if (self->priv->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    g_snprintf (track_str, sizeof(track_str), ".%03u", track);
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

  return file_name;
}

static void
bt_render_dialog_record (const BtRenderDialog * self)
{
  gchar *info;

  GST_INFO ("recording to '%s'", self->priv->file_name);
  g_object_set (self->priv->sink_bin, "record-file-name", self->priv->file_name,
      // TODO(ensonic): BT_SINK_BIN_MODE_PLAY_AND_RECORD hangs
      "mode", BT_SINK_BIN_MODE_RECORD,
      "record-format", self->priv->format, NULL);
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
  BtMainPageSequence *sequence_page;
  BtSetup *setup;
  BtSequence *sequence;
  BtMachine *machine;

  // get current settings and override
  bt_child_proxy_get (self->priv->app, "unsaved", &self->priv->unsaved,
      "main-window::pages::sequence-page", &sequence_page, NULL);
  g_object_get (self->priv->song, "setup", &setup, "sequence", &sequence, NULL);
  g_object_get (sequence, "loop", &self->priv->saved_loop, NULL);
  g_object_set (sequence, "loop", FALSE, NULL);
  g_object_unref (sequence);
  g_object_get (sequence_page, "follow-playback",
      &self->priv->saved_follow_playback, NULL);
  g_object_set (sequence_page, "follow-playback", FALSE, NULL);
  g_object_unref (sequence_page);

  // disable all level-meters
  bt_render_dialog_enable_level_meters (setup, FALSE);

  // lookup the audio-sink machine and change mode
  if ((machine = bt_setup_get_machine_by_type (setup, BT_TYPE_SINK_MACHINE))) {
    g_object_get (machine, "machine", &self->priv->sink_bin, "adder-convert",
        &self->priv->convert, NULL);

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
  self->priv->has_error = FALSE;
  if (self->priv->mode == BT_RENDER_MODE_MIXDOWN) {
    self->priv->track = -1;
    self->priv->tracks = 0;
  } else {
    self->priv->list =
        bt_setup_get_machines_by_type (setup, BT_TYPE_SOURCE_MACHINE);
    self->priv->track = 0;
    self->priv->tracks = g_list_length (self->priv->list);

    bt_child_proxy_get (self->priv->song, "song-info::name",
        &self->priv->song_name, NULL);
  }
  g_object_unref (setup);
}

/* Cleanup the rendering */
static void
bt_render_dialog_record_done (const BtRenderDialog * self)
{
  BtSetup *setup;

  /* reset play mode */
  if (self->priv->sink_bin) {
    g_object_set (self->priv->sink_bin, "mode", BT_SINK_BIN_MODE_PLAY, NULL);
    gst_object_replace ((GstObject **) & self->priv->sink_bin, NULL);
  }

  /* reset dithering/noise-shaping */
  if (self->priv->convert) {
    g_object_set (self->priv->convert, "dithering", 0, "noise-shaping", 0,
        NULL);
    gst_object_replace ((GstObject **) & self->priv->convert, NULL);
  }

  /* reset loop and follow-playback */
  bt_child_proxy_set (self->priv->song, "sequence::loop",
      self->priv->saved_loop, NULL);
  bt_child_proxy_set (self->priv->app,
      "main-window::pages::sequence-page::follow-playback",
      self->priv->saved_follow_playback, NULL);

  g_object_set (self->priv->app, "unsaved", self->priv->unsaved, NULL);

  // re-enable all level-meters
  g_object_get (self->priv->song, "setup", &setup, NULL);
  bt_render_dialog_enable_level_meters (setup, TRUE);
  g_object_unref (setup);

  if (self->priv->list) {
    g_list_free (self->priv->list);
    self->priv->list = NULL;
  }
  if (self->priv->song_name) {
    bt_child_proxy_set (self->priv->song, "song-info::name",
        self->priv->song_name, NULL);
    g_free (self->priv->song_name);
  }
  // close the dialog
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}

/* Run the rendering. */
static void
bt_render_dialog_record_next (const BtRenderDialog * self)
{
  static BtMachine *machine = NULL;

  /* cleanup from previous run */
  if (self->priv->has_error && self->priv->file_name) {
    GST_INFO ("delete output file '%s' due to errors", self->priv->file_name);
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
    gchar *track_name, *id;

    machine =
        BT_MACHINE (g_list_nth_data (self->priv->list, self->priv->track));
    g_object_set (machine, "state", BT_MACHINE_STATE_SOLO, NULL);

    g_object_get (machine, "id", &id, NULL);
    track_name = g_strdup_printf ("%s : %s", self->priv->song_name, id);
    bt_child_proxy_set (self->priv->song, "song-info::name", track_name, NULL);
    GST_INFO ("recording to '%s'", track_name);
    g_free (track_name);
    g_free (id);
  }
  bt_render_dialog_record (self);
  self->priv->track++;
}

static void
bt_render_check_existing_output_files (const BtRenderDialog * self)
{
  // we're not fully constructed yet
  if (!self->priv->info_bar) {
    return;
  }

  gchar *file_name = bt_render_dialog_make_file_name (self, 0);
  if (g_file_test (file_name, G_FILE_TEST_EXISTS)) {
    gtk_label_set_text (GTK_LABEL (self->priv->info_bar_label),
        _("File already exists!\nIt will be overwritten if you continue."));
    gtk_info_bar_set_message_type (GTK_INFO_BAR (self->priv->info_bar),
        GTK_MESSAGE_WARNING);
    gtk_widget_show (self->priv->info_bar);
  } else {
    gtk_widget_hide (self->priv->info_bar);
  }
  g_free (file_name);
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
on_base_file_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_free (self->priv->base_file_name);
  self->priv->base_file_name =
      g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));

  // update format
  if (self->priv->base_file_name) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;

    enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
      if ((enum_value = g_enum_get_value (enum_class, i))) {
        if (g_str_has_suffix (self->priv->base_file_name,
                enum_value->value_name)) {
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
  bt_render_check_existing_output_files (self);
}

static void
on_format_menu_changed (GtkComboBox * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->priv->format = gtk_combo_box_get_active (menu);

  // update base_file_name
  if (self->priv->base_file_name) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;
    gchar *fn = self->priv->base_file_name;

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
    self->priv->base_file_name =
        g_strdup_printf ("%s%s", fn, enum_value->value_name);
    GST_INFO ("set new fn to: %s", self->priv->base_file_name);
    g_free (fn);
    g_signal_handlers_block_matched (self->priv->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_base_file_name_changed, (gpointer) user_data);
    gtk_entry_set_text (GTK_ENTRY (self->priv->file_name_entry),
        self->priv->base_file_name);
    g_signal_handlers_unblock_matched (self->priv->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_base_file_name_changed, (gpointer) user_data);
  }
  bt_render_check_existing_output_files (self);
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
  // hide info_bar
  gtk_widget_hide (self->priv->info_bar);
  // start the rendering
  bt_render_dialog_record_init (self);
  bt_render_dialog_record_next (self);
}

static void
on_song_play_pos_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);
  BtSongInfo *song_info;
  gulong pos, length;
  gdouble progress;
  // the +4 is not really needed, but I get a stack smashing error on ubuntu without
  gchar str[3 + 2 * (2 + 2 + 3 + 3) + 4];
  gulong cmsec, csec, cmin, tmsec, tsec, tmin;

  bt_child_proxy_get ((gpointer) song, "sequence::length", &length, "song-info",
      &song_info, "play-pos", &pos, NULL);

  progress = (gdouble) pos / (gdouble) length;
  if (progress >= 1.0) {
    progress = 1.0;
  }
  GST_INFO ("progress %ld/%ld=%lf", pos, length, progress);

  bt_song_info_tick_to_m_s_ms (song_info, length, &tmin, &tsec, &tmsec);
  bt_song_info_tick_to_m_s_ms (song_info, pos, &cmin, &csec, &cmsec);
  // format
  g_snprintf (str, sizeof(str), "%02lu:%02lu.%03lu / %02lu:%02lu.%03lu", cmin, csec, cmsec,
      tmin, tsec, tmsec);

  g_object_set (self->priv->track_progress, "fraction", progress, "text", str,
      NULL);

  g_object_unref (song_info);
}

static void
on_song_error (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "received %s bus message",
      GST_MESSAGE_TYPE_NAME (message));

  self->priv->has_error = TRUE;
  bt_render_dialog_record_next (self);
}

static void
on_song_eos (const GstBus * const bus, const GstMessage * const message,
    gconstpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  bt_song_stop (self->priv->song);
  // trigger next file / done
  bt_render_dialog_record_next (self);
}

static void
on_info_bar_realize (GtkWidget * widget, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);
  GtkRequisition requisition;

  gtk_widget_get_preferred_size (widget, NULL, &requisition);
  GST_DEBUG ("#### info_bar size req %d x %d",
      requisition.width, requisition.height);

  gtk_widget_set_size_request (GTK_WIDGET (self), requisition.width + 24, -1);
}

//-- helper methods

static void
bt_render_dialog_init_ui (const BtRenderDialog * self)
{
  GtkWidget *overlay, *label, *widget, *table;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter iter;
  guint i;
  GstBin *bin;
  GstBus *bus;
  gchar *full_file_name = NULL;

  GST_DEBUG ("read settings");

  bt_child_proxy_get (self->priv->app, "settings::record-folder",
      &self->priv->folder, NULL);
  bt_child_proxy_get (self->priv->song, "song-info::file-name", &full_file_name,
      "bin", &bin, NULL);

  // set default base name
  if (full_file_name) {
    gchar *file_name, *ext;

    // cut off extension from file_name
    if ((ext = strrchr (full_file_name, '.')))
      *ext = '\0';
    if ((file_name = strrchr (full_file_name, G_DIR_SEPARATOR))) {
      self->priv->base_file_name = g_strdup (&file_name[1]);
      g_free (full_file_name);
    } else {
      self->priv->base_file_name = full_file_name;
    }
  } else {
    self->priv->base_file_name = g_strdup ("");
  }
  GST_INFO ("initial base filename: '%s'", self->priv->base_file_name);

  GST_DEBUG ("prepare render dialog");

  gtk_widget_set_name (GTK_WIDGET (self), "song rendering");

  gtk_window_set_title (GTK_WINDOW (self), _("song rendering"));

  // add dialog commision widgets (okay, cancel)
  self->priv->okay_button = gtk_dialog_add_button (GTK_DIALOG (self), _("OK"),
      GTK_RESPONSE_NONE);
  g_signal_connect (self->priv->okay_button, "clicked",
      G_CALLBACK (on_okay_clicked), (gpointer) self);
  gtk_dialog_add_button (GTK_DIALOG (self), _("_Cancel"), GTK_RESPONSE_REJECT);

  // add widgets to the dialog content area
  overlay = gtk_overlay_new ();
  gtk_container_set_border_width (GTK_CONTAINER (overlay), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      overlay, TRUE, TRUE, 0);

  table = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (overlay), table);

  label = gtk_label_new (_("Folder"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

  self->priv->dir_chooser = widget =
      gtk_file_chooser_button_new (_("Select a folder"),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
      self->priv->folder);
  g_signal_connect (widget, "selection-changed", G_CALLBACK (on_folder_changed),
      (gpointer) self);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 0, 1, 1);


  label = gtk_label_new (_("Filename"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

  self->priv->file_name_entry = widget = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (widget), self->priv->base_file_name);
  gtk_entry_set_activates_default (GTK_ENTRY (self->priv->file_name_entry),
      TRUE);
  g_signal_connect (widget, "changed", G_CALLBACK (on_base_file_name_changed),
      (gpointer) self);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 1, 1, 1);


  label = gtk_label_new (_("Format"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);

  // query supported formats from sinkbin
  self->priv->format_menu = widget = gtk_combo_box_new ();
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "foreground", "gray", NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer, "text", 0,
      "foreground-set", 1, NULL);
  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  enum_class = g_type_class_ref (BT_TYPE_SINK_BIN_RECORD_FORMAT);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, 0, enum_value->value_nick, 1,
          !bt_sink_bin_is_record_format_supported (i), -1);
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
      BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS);
  g_signal_connect (widget, "changed", G_CALLBACK (on_format_menu_changed),
      (gpointer) self);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 2, 1, 1);


  label = gtk_label_new (_("Mode"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);

  // query supported modes from sinkbin
  self->priv->mode_menu = widget = gtk_combo_box_text_new ();
  enum_class = g_type_class_ref (BT_TYPE_RENDER_MODE);
  for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
    if ((enum_value = g_enum_get_value (enum_class, i))) {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
          gettext (enum_value->value_nick));
    }
  }
  g_type_class_unref (enum_class);
  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), BT_RENDER_MODE_MIXDOWN);
  g_signal_connect (widget, "changed", G_CALLBACK (on_mode_menu_changed),
      (gpointer) self);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 3, 1, 1);

  /* TODO(ensonic): add more widgets
     o write project file
     o none, jokosher, ardour, ...
   */

  label = gtk_label_new ("");
  self->priv->info = GTK_LABEL (label);
  g_object_set (label, "hexpand", TRUE, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 4, 2, 1);

  self->priv->track_progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  g_object_set (self->priv->track_progress, "hexpand", TRUE, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->track_progress),
      0, 5, 2, 1);

  label = gtk_label_new ("");
  g_object_set (label, "hexpand", TRUE, "vexpand", TRUE, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 6, 2, 1);

  // info_bar for messages
  self->priv->info_bar = gtk_info_bar_new ();
  gtk_widget_set_no_show_all (self->priv->info_bar, TRUE);
  gtk_widget_set_valign (self->priv->info_bar, GTK_ALIGN_END);
#if GTK_CHECK_VERSION (3,10,0)
  gtk_info_bar_set_show_close_button (GTK_INFO_BAR (self->priv->info_bar),
      TRUE);
#endif
  self->priv->info_bar_label = gtk_label_new ("");
  gtk_widget_show (self->priv->info_bar_label);
  gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR
              (self->priv->info_bar))), self->priv->info_bar_label);
  g_signal_connect (self->priv->info_bar, "response",
      G_CALLBACK (gtk_widget_hide), NULL);
  g_signal_connect (self->priv->info_bar, "realize",
      G_CALLBACK (on_info_bar_realize), (gpointer) self);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), self->priv->info_bar);

  // set initial filename:
  on_format_menu_changed (GTK_COMBO_BOX (widget), (gpointer) self);

  // connect signal handlers
  g_signal_connect_object (self->priv->song, "notify::play-pos",
      G_CALLBACK (on_song_play_pos_notify), (gpointer) self, 0);
  bus = gst_element_get_bus (GST_ELEMENT (bin));
  g_signal_connect_object (bus, "message::error", G_CALLBACK (on_song_error),
      (gpointer) self, 0);
  g_signal_connect_object (bus, "message::eos", G_CALLBACK (on_song_eos),
      (gpointer) self, 0);

  gst_object_unref (bus);
  gst_object_unref (bin);
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
  return self;
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
    self->priv->file_name = NULL;
  }

  g_object_try_unref (self->priv->song);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->dispose (object);
}

static void
bt_render_dialog_finalize (GObject * object)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_free (self->priv->folder);
  g_free (self->priv->base_file_name);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->finalize (object);
}

static void
bt_render_dialog_init (BtRenderDialog * self)
{
  self->priv = bt_render_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_render_dialog_class_init (BtRenderDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_render_dialog_dispose;
  gobject_class->finalize = bt_render_dialog_finalize;
}
