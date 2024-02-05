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
#include "main-page-sequence.h"
#include <glib/gprintf.h>
#include <glib/gstdio.h>

struct _BtRenderDialog
{
  AdwWindow parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* dialog widgets */
  GtkWidget *okay_button;
  GtkWidget *dir_chooser;
  GtkWidget *file_name_entry;
  GtkDropDown *format_menu;
  GtkDropDown *mode_menu;
  GtkProgressBar *track_progress;
  GtkLabel *info;
  GtkRevealer *info_bar;
  GtkLabel *info_bar_label;

  /* dialog data */
  GFile *folder;
  gchar *base_file_name;
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

static void on_format_menu_changed (GtkDropDown * menu, gpointer user_data);

//-- the class

G_DEFINE_TYPE (BtRenderDialog, bt_render_dialog, ADW_TYPE_WINDOW);


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
bt_render_dialog_make_file_name (BtRenderDialog * self, gint track)
{
  GFile *file = g_file_get_child (self->folder, self->base_file_name);
  gchar track_str[4];
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  if (self->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    g_snprintf (track_str, sizeof(track_str), ".%03u", track);
  } else {
    track_str[0] = '\0';
  }

  // add file suffix if not yet there
  enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
  enum_value = g_enum_get_value (enum_class, self->format);

  gchar *file_name = g_file_get_path (file);
  
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

  g_free (file);
  
  return file_name;
}

static void
bt_render_dialog_record (BtRenderDialog * self)
{
  gchar *info;

  GST_INFO ("recording to '%s'", self->file_name);
  g_object_set (self->sink_bin, "record-file-name", self->file_name,
      // TODO(ensonic): BT_SINK_BIN_MODE_PLAY_AND_RECORD hangs
      "mode", BT_SINK_BIN_MODE_RECORD,
      "record-format", self->format, NULL);
  info = g_strdup_printf (_("Recording to: %s"), self->file_name);
  gtk_label_set_text (self->info, info);
  g_free (info);

  bt_song_play (self->song);
  bt_song_write_to_lowlevel_dot_file (self->song);
}

/* Prepare the rendering. */
static void
bt_render_dialog_record_init (BtRenderDialog * self)
{
  BtMainPageSequence *sequence_page;
  BtSetup *setup;
  BtSequence *sequence;
  BtMachine *machine;

  // get current settings and override
  bt_child_proxy_get (self->app, "unsaved", &self->unsaved,
      "main-window::pages::sequence-page", &sequence_page, NULL);
  g_object_get (self->song, "setup", &setup, "sequence", &sequence, NULL);
  g_object_get (sequence, "loop", &self->saved_loop, NULL);
  g_object_set (sequence, "loop", FALSE, NULL);
  g_object_unref (sequence);
  g_object_get (sequence_page, "follow-playback",
      &self->saved_follow_playback, NULL);
  g_object_set (sequence_page, "follow-playback", FALSE, NULL);
  g_object_unref (sequence_page);

  // disable all level-meters
  bt_render_dialog_enable_level_meters (setup, FALSE);

  // lookup the audio-sink machine and change mode
  if ((machine = bt_setup_get_machine_by_type (setup, BT_TYPE_SINK_MACHINE))) {
    g_object_get (machine, "machine", &self->sink_bin, "adder-convert",
        &self->convert, NULL);

    /* TODO(ensonic): configure dithering/noise-shaping
     * - should sink-bin do it so that we get this also when recording from
     *   the commandline (need some extra cmdline options for it :/
     * - we could also put it to the options
     * - sink-machine could also set this (hard-coded) when going to record mode
     */
    g_object_set (self->convert, "dithering", 2, "noise-shaping", 3,
        NULL);

    g_object_unref (machine);
  }
  self->has_error = FALSE;
  if (self->mode == BT_RENDER_MODE_MIXDOWN) {
    self->track = -1;
    self->tracks = 0;
  } else {
    self->list =
        bt_setup_get_machines_by_type (setup, BT_TYPE_SOURCE_MACHINE);
    self->track = 0;
    self->tracks = g_list_length (self->list);

    bt_child_proxy_get (self->song, "song-info::name",
        &self->song_name, NULL);
  }
  g_object_unref (setup);
}

/* Cleanup the rendering */
static void
bt_render_dialog_record_done (BtRenderDialog * self)
{
  BtSetup *setup;

  /* reset play mode */
  if (self->sink_bin) {
    g_object_set (self->sink_bin, "mode", BT_SINK_BIN_MODE_PLAY, NULL);
    gst_object_replace ((GstObject **) & self->sink_bin, NULL);
  }

  /* reset dithering/noise-shaping */
  if (self->convert) {
    g_object_set (self->convert, "dithering", 0, "noise-shaping", 0,
        NULL);
    gst_object_replace ((GstObject **) & self->convert, NULL);
  }

  /* reset loop and follow-playback */
  bt_child_proxy_set (self->song, "sequence::loop",
      self->saved_loop, NULL);
  bt_child_proxy_set (self->app,
      "main-window::pages::sequence-page::follow-playback",
      self->saved_follow_playback, NULL);

  g_object_set (self->app, "unsaved", self->unsaved, NULL);

  // re-enable all level-meters
  g_object_get (self->song, "setup", &setup, NULL);
  bt_render_dialog_enable_level_meters (setup, TRUE);
  g_object_unref (setup);

  if (self->list) {
    g_list_free (self->list);
    self->list = NULL;
  }
  if (self->song_name) {
    bt_child_proxy_set (self->song, "song-info::name",
        self->song_name, NULL);
    g_free (self->song_name);
  }

  gtk_window_close (GTK_WINDOW (self));
}

/* Run the rendering. */
static void
bt_render_dialog_record_next (BtRenderDialog * self)
{
  static BtMachine *machine = NULL;

  /* cleanup from previous run */
  if (self->has_error && self->file_name) {
    GST_INFO ("delete output file '%s' due to errors", self->file_name);
    g_unlink (self->file_name);
  }
  if (self->tracks && machine) {
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  if (self->file_name) {
    g_free (self->file_name);
    self->file_name = NULL;
  }
  /* if there was an error or we're done */
  if (self->has_error || self->track == self->tracks) {
    bt_render_dialog_record_done (self);
    return;
  }

  self->file_name =
      bt_render_dialog_make_file_name (self, self->track);

  if (self->mode == BT_RENDER_MODE_SINGLE_TRACKS) {
    gchar *track_name, *id;

    machine =
        BT_MACHINE (g_list_nth_data (self->list, self->track));
    g_object_set (machine, "state", BT_MACHINE_STATE_SOLO, NULL);

    g_object_get (machine, "id", &id, NULL);
    track_name = g_strdup_printf ("%s : %s", self->song_name, id);
    bt_child_proxy_set (self->song, "song-info::name", track_name, NULL);
    GST_INFO ("recording to '%s'", track_name);
    g_free (track_name);
    g_free (id);
  }
  bt_render_dialog_record (self);
  self->track++;
}

static void
bt_render_check_existing_output_files (BtRenderDialog * self)
{
  // we're not fully constructed yet
  if (!self->info_bar) {
    return;
  }

  gchar *file_name = bt_render_dialog_make_file_name (self, 0);
  if (g_file_test (file_name, G_FILE_TEST_EXISTS)) {
    gtk_label_set_text (GTK_LABEL (self->info_bar_label),
        _("File already exists!\nIt will be overwritten if you continue."));
    gtk_revealer_set_reveal_child (self->info_bar, TRUE);
  } else {
    gtk_revealer_set_reveal_child (self->info_bar, FALSE);
  }
  
  g_free (file_name);
}

//-- event handler

static void
on_folder_changed (GtkFileChooser * chooser, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_object_try_unref (self->folder);
  self->folder = gtk_file_chooser_get_current_folder (chooser);
}

static void
on_base_file_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  g_free (self->base_file_name);
  self->base_file_name =
      g_strdup (gtk_editable_get_text (editable));

  // update format
  if (self->base_file_name) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;

    enum_class = g_type_class_peek_static (BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for (i = enum_class->minimum; i <= enum_class->maximum; i++) {
      if ((enum_value = g_enum_get_value (enum_class, i))) {
        if (g_str_has_suffix (self->base_file_name,
                enum_value->value_name)) {
          g_signal_handlers_block_matched (self->format_menu,
              G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
              on_format_menu_changed, (gpointer) user_data);
          gtk_drop_down_set_selected (GTK_DROP_DOWN (self->format_menu), i);
          g_signal_handlers_unblock_matched (self->format_menu,
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
on_format_menu_changed (GtkDropDown * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->format = gtk_drop_down_get_selected (menu);

  // update base_file_name
  if (self->base_file_name) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;
    gchar *fn = self->base_file_name;

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
    enum_value = g_enum_get_value (enum_class, self->format);
    self->base_file_name =
        g_strdup_printf ("%s%s", fn, enum_value->value_name);
    GST_INFO ("set new fn to: %s", self->base_file_name);
    g_free (fn);
    g_signal_handlers_block_matched (self->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_base_file_name_changed, (gpointer) user_data);
    gtk_editable_set_text (GTK_EDITABLE (self->file_name_entry),
        self->base_file_name);
    g_signal_handlers_unblock_matched (self->file_name_entry,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_base_file_name_changed, (gpointer) user_data);
  }
  bt_render_check_existing_output_files (self);
}

static void
on_mode_menu_changed (GtkDropDown * menu, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  self->mode = gtk_drop_down_get_selected (menu);
}

static void on_cancel_clicked(GtkButton * widget, gpointer user_data)
{
  gtk_window_close (GTK_WINDOW (user_data));
}

static void
on_okay_clicked (GtkButton * button, gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  // gray the okay button and the action area
  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
  gtk_widget_set_sensitive (self->dir_chooser, FALSE);
  gtk_widget_set_sensitive (self->file_name_entry, FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (self->format_menu), FALSE);
  gtk_widget_set_sensitive (GTK_WIDGET (self->mode_menu), FALSE);
  // hide info_bar
  gtk_revealer_set_reveal_child (self->info_bar, FALSE);
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

  g_object_set (self->track_progress, "fraction", progress, "text", str,
      NULL);

  g_object_unref (song_info);
}

static void
on_song_error (const GstBus * const bus, GstMessage * message,
    gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "received %s bus message",
      GST_MESSAGE_TYPE_NAME (message));

  self->has_error = TRUE;
  bt_render_dialog_record_next (self);
}

static void
on_song_eos (const GstBus * const bus, const GstMessage * const message,
    gpointer user_data)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (user_data);

  bt_song_stop (self->song);
  // trigger next file / done
  bt_render_dialog_record_next (self);
}

//-- helper methods

static void
bt_render_dialog_init_ui (BtRenderDialog * self)
{
  GstBin *bin;
  GstBus *bus;
  gchar *full_file_name = NULL;

  GST_DEBUG ("read settings");

  bt_child_proxy_get (self->app, "settings::record-folder",
      &self->folder, NULL);
  bt_child_proxy_get (self->song, "song-info::file-name", &full_file_name,
      "bin", &bin, NULL);

  // set default base name
  if (full_file_name) {
    gchar *file_name, *ext;

    // cut off extension from file_name
    if ((ext = strrchr (full_file_name, '.')))
      *ext = '\0';
    if ((file_name = strrchr (full_file_name, G_DIR_SEPARATOR))) {
      self->base_file_name = g_strdup (&file_name[1]);
      g_free (full_file_name);
    } else {
      self->base_file_name = full_file_name;
    }
  } else {
    self->base_file_name = g_strdup ("");
  }
  GST_INFO ("initial base filename: '%s'", self->base_file_name);

  GST_DEBUG ("prepare render dialog");

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (self->dir_chooser),
      self->folder, NULL);

  // supported formats come from sinkbin
  gtk_drop_down_set_selected (self->format_menu, BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS);
  
  gtk_drop_down_set_selected (self->mode_menu, BT_RENDER_MODE_MIXDOWN);

  /* TODO(ensonic): add more widgets
     o write project file
     o none, jokosher, ardour, ...
   */

  // set initial filename:
  on_format_menu_changed (GTK_DROP_DOWN (self->format_menu), (gpointer) self);

  // connect signal handlers
  g_signal_connect_object (self->song, "notify::play-pos",
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
  g_object_get (self->app, "song", &self->song, NULL);
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
  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->file_name) {
    GST_INFO ("canceled recording, removing partial file");
    g_unlink (self->file_name);
    g_free (self->file_name);
    self->file_name = NULL;
  }

  g_object_try_unref (self->song);
  g_object_unref (self->app);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_RENDER_DIALOG);
  
  G_OBJECT_CLASS (bt_render_dialog_parent_class)->dispose (object);
}

static void
bt_render_dialog_finalize (GObject * object)
{
  BtRenderDialog *self = BT_RENDER_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_free (self->folder);
  g_free (self->base_file_name);

  G_OBJECT_CLASS (bt_render_dialog_parent_class)->finalize (object);
}

static void
bt_render_dialog_init (BtRenderDialog * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_render_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
}

static void
bt_render_dialog_class_init (BtRenderDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_render_dialog_dispose;
  gobject_class->finalize = bt_render_dialog_finalize;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/render-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, dir_chooser);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, file_name_entry);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, format_menu);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, info);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, info_bar);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, info_bar_label);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, mode_menu);
  gtk_widget_class_bind_template_child (widget_class, BtRenderDialog, track_progress);
  
  gtk_widget_class_bind_template_callback (widget_class, on_base_file_name_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_cancel_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_folder_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_format_menu_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_okay_clicked);
}
