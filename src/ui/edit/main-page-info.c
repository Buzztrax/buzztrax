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
 * SECTION:btmainpageinfo
 * @short_description: the editor main info page
 * @see_also: #BtSongInfo
 *
 * Provides an editor for the song meta data.
 */

/* TODO(ensonic):
 * - use this tab as the first one?
 * - add choice for metre (in german -> takt): beats (beats = bars / tpb)
 *   or at least show the resulting metre
 */
/* TODO(ensonic): undo/redo
 * - for text_view we would need to connect to insert_at_cursor and delete_from_cursor
 *   signals to track changes
 * - for text_enry the signals are inserted_text and deleted_text
 * - optionally we just store the complete before/after
 */
/* TODO(ensonic): changing tpb, beats
 * - when changing tpb and beats the playback-speed of the current song is
 *   affected, we might want to offer a tool to resample the patterns.
 * - it would probably be easier to have some buttons for
 *   1/4x 1/3x 1/2x 2x, 3x, 4x tick resolution
 * - when lowering the tick resolution, we need to handle loss of notes
 *   when doing 1/4 we would merge 4 rows into one keeping the first note found
 */
/* TODO(ensonic): song-name generator
 * - add a magic-wand buton behind the song-name
 * - when clicked generate a song name
 */

#define BT_EDIT
#define BT_MAIN_PAGE_INFO_C

#include "bt-edit.h"

struct _BtMainPageInfoPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the sequence we are showing */
  BtSongInfo *song_info;

  /* name, genre, author of the song */
  GtkEntry *name, *genre, *author;

  /* ro date stamps of the song */
  GtkEntry *date_created, *date_changed;

  /* bpm,tpb,beats of the song */
  GtkSpinButton *bpm, *tpb, *beats;

  /* freeform info about the song */
  GtkTextView *info;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMainPageInfo, bt_main_page_info, GTK_TYPE_BOX, 
    G_ADD_PRIVATE(BtMainPageInfo));


//-- event handler

static void
on_page_mapped (GtkWidget * widget, gpointer user_data)
{
  GTK_WIDGET_GET_CLASS (widget)->focus (widget, GTK_DIR_TAB_FORWARD);
}

static void
on_page_switched (GtkNotebook * notebook, GParamSpec * arg, gpointer user_data)
{
  //BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  guint page_num;
  static gint prev_page_num = -1;

  g_object_get (notebook, "page", &page_num, NULL);

  if (page_num == BT_MAIN_PAGES_INFO_PAGE) {
    // only do this if the page really has changed
    if (prev_page_num != BT_MAIN_PAGES_INFO_PAGE) {
      GST_DEBUG ("enter info page");
    }
  } else {
    // only do this if the page was BT_MAIN_PAGES_INFO_PAGE
    if (prev_page_num == BT_MAIN_PAGES_INFO_PAGE) {
      GST_DEBUG ("leave info page");
    }
  }
  prev_page_num = page_num;
}

static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);

  GST_INFO ("name changed : self=%p -> \"%s\"", self,
      gtk_entry_get_text (GTK_ENTRY (editable)));
  // update info fields
  g_object_set (self->priv->song_info, "name",
      g_strdup (gtk_entry_get_text (GTK_ENTRY (editable))), NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_genre_changed (GtkEditable * editable, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);

  GST_INFO ("genre changed : self=%p -> \"%s\"", self,
      gtk_entry_get_text (GTK_ENTRY (editable)));
  // update info fields
  g_object_set (self->priv->song_info, "genre",
      g_strdup (gtk_entry_get_text (GTK_ENTRY (editable))), NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_author_changed (GtkEditable * editable, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);

  GST_INFO ("author changed : self=%p -> \"%s\"", self,
      gtk_entry_get_text (GTK_ENTRY (editable)));
  // update info fields
  g_object_set (self->priv->song_info, "author",
      g_strdup (gtk_entry_get_text (GTK_ENTRY (editable))), NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_bpm_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);

  GST_INFO ("bpm changed : self=%p -> %d", self,
      gtk_spin_button_get_value_as_int (spinbutton));
  // update info fields
  g_object_set (self->priv->song_info, "bpm",
      gtk_spin_button_get_value_as_int (spinbutton), NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_tpb_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);
  gulong tpb, beats, bars;
  GParamSpecULong *pspec;
  GObjectClass *song_info_class;

  GST_INFO ("tpb changed : self=%p -> %d", self,
      gtk_spin_button_get_value_as_int (spinbutton));

  song_info_class = g_type_class_ref (BT_TYPE_SONG_INFO);
  pspec =
      (GParamSpecULong *) g_object_class_find_property (song_info_class,
      "bars");

  // update info fields
  g_object_get (self->priv->song_info, "bars", &bars, "tpb", &tpb, NULL);
  beats = bars / tpb;
  tpb = gtk_spin_button_get_value_as_int (spinbutton);
  bars = beats * tpb;
  bars = CLAMP (bars, pspec->minimum, pspec->maximum);
  tpb = bars / beats;
  gtk_spin_button_set_value (spinbutton, tpb);

  g_object_set (self->priv->song_info, "tpb", tpb, "bars", bars, NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_beats_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);
  gulong tpb, beats, bars;
  GParamSpecULong *pspec;
  GObjectClass *song_info_class;

  song_info_class = g_type_class_ref (BT_TYPE_SONG_INFO);
  pspec =
      (GParamSpecULong *) g_object_class_find_property (song_info_class,
      "bars");

  GST_INFO ("beats changed : self=%p -> %d", self,
      gtk_spin_button_get_value_as_int (spinbutton));
  // update info fields
  g_object_get (self->priv->song_info, "tpb", &tpb, NULL);
  beats = gtk_spin_button_get_value_as_int (spinbutton);
  bars = beats * tpb;
  bars = CLAMP (bars, pspec->minimum, pspec->maximum);
  beats = bars / tpb;
  gtk_spin_button_set_value (spinbutton, beats);

  g_object_set (self->priv->song_info, "bars", bars, NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_info_changed (GtkTextBuffer * textbuffer, gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);
  gchar *str;
  GtkTextIter beg_iter, end_iter;

  GST_INFO ("info changed : self=%p", self);
  // update info fields
  // get begin and end textiters :(, then get text
  gtk_text_buffer_get_iter_at_offset (textbuffer, &beg_iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuffer, &end_iter, -1);
  str = gtk_text_buffer_get_text (textbuffer, &beg_iter, &end_iter, FALSE);
  g_object_set (self->priv->song_info, "info", str, NULL);
  g_free (str);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_name_notify (const BtSongInfo * song_info, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);
  gchar *name;

  g_object_get ((gpointer) song_info, "name", &name, NULL);
  g_signal_handlers_block_matched (self->priv->name,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_name_changed,
      (gpointer) self);
  gtk_entry_set_text (self->priv->name, safe_string (name));
  g_free (name);
  g_signal_handlers_unblock_matched (self->priv->name,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_name_changed,
      (gpointer) self);
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (user_data);
  BtSong *song;
  GtkTextBuffer *buffer;
  gchar *name, *genre, *author, *create_dts, *change_dts;
  gulong bpm, tpb, bars;
  gchar *info;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  g_object_try_unref (self->priv->song_info);

  // get song from app
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song) {
    self->priv->song_info = NULL;
    return;
  }
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_get (song, "song-info", &self->priv->song_info, NULL);
  // update info fields
  g_object_get (self->priv->song_info,
      "name", &name, "genre", &genre, "author", &author, "info", &info,
      "bpm", &bpm, "tpb", &tpb, "bars", &bars,
      "create-dts", &create_dts, "change-dts", &change_dts, NULL);
  g_signal_handlers_block_matched (self->priv->name,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_name_changed,
      (gpointer) self);
  gtk_entry_set_text (self->priv->name, safe_string (name));
  g_free (name);
  g_signal_handlers_unblock_matched (self->priv->name,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_name_changed,
      (gpointer) self);

  g_signal_handlers_block_matched (self->priv->genre,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_genre_changed,
      (gpointer) self);
  gtk_entry_set_text (self->priv->genre, safe_string (genre));
  g_free (genre);
  g_signal_handlers_unblock_matched (self->priv->genre,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_genre_changed,
      (gpointer) self);

  g_signal_handlers_block_matched (self->priv->author,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_author_changed,
      (gpointer) self);
  gtk_entry_set_text (self->priv->author, safe_string (author));
  g_free (author);
  g_signal_handlers_unblock_matched (self->priv->author,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_author_changed,
      (gpointer) self);

  g_signal_handlers_block_matched (self->priv->bpm,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_bpm_changed,
      (gpointer) self);
  gtk_spin_button_set_value (self->priv->bpm, (gdouble) bpm);
  g_signal_handlers_unblock_matched (self->priv->bpm,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_bpm_changed,
      (gpointer) self);

  g_signal_handlers_block_matched (self->priv->tpb,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_tpb_changed,
      (gpointer) self);
  gtk_spin_button_set_value (self->priv->tpb, (gdouble) tpb);
  g_signal_handlers_unblock_matched (self->priv->tpb,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_tpb_changed,
      (gpointer) self);

  g_signal_handlers_block_matched (self->priv->beats,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_beats_changed,
      (gpointer) self);
  gtk_spin_button_set_value (self->priv->beats, (gdouble) (bars / tpb));
  g_signal_handlers_unblock_matched (self->priv->beats,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_beats_changed,
      (gpointer) self);

  /* the iso date is not nice for the user
   * struct tm tm;
   * char dts[255];
   *
   * strptime(create_dts, "%FT%TZ", &tm);
   * strftime(dts, sizeof(buf), "%F %T", &tm);
   *
   * but the code below is simpler and works too :)
   */
  create_dts[10] = ' ';
  create_dts[19] = '\0';
  gtk_entry_set_text (self->priv->date_created, create_dts);
  g_free (create_dts);
  change_dts[10] = ' ';
  change_dts[19] = '\0';
  gtk_entry_set_text (self->priv->date_changed, change_dts);
  g_free (change_dts);

  buffer = gtk_text_view_get_buffer (self->priv->info);
  g_signal_handlers_block_matched (buffer,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_info_changed,
      (gpointer) self);
  gtk_text_buffer_set_text (buffer, safe_string (info), -1);
  g_free (info);
  g_signal_handlers_unblock_matched (buffer,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, on_info_changed,
      (gpointer) self);

  g_signal_connect (self->priv->song_info, "notify::name",
      G_CALLBACK (on_name_notify), (gpointer) self);

  // release the references
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

//-- helper methods

static GtkWidget *
make_heading (const gchar * txt)
{
  GtkWidget *label;
  GtkStyleContext *context;
  gchar *str;

  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<b>%s</b>", txt);
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);

  context = gtk_widget_get_style_context (label);
  gtk_style_context_add_class (context, "heading");

  return label;
}

static void
bt_main_page_info_init_ui (const BtMainPageInfo * self,
    const BtMainPages * pages)
{
  GtkWidget *label, *frame, *box;
  GtkWidget *table;
  GtkWidget *scrolledwindow;
  GtkAdjustment *spin_adjustment;
  GParamSpecULong *pspec, *pspec2;
  GObjectClass *song_info_class;
  gulong def, min, max;

  GST_DEBUG ("!!!! self=%p", self);

  gtk_widget_set_name (GTK_WIDGET (self), "song information");

  song_info_class = g_type_class_ref (BT_TYPE_SONG_INFO);

  // first row of vbox
  frame = gtk_frame_new (NULL);
  label = make_heading (_("song meta data"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BOX_BORDER);
  gtk_widget_set_name (frame, "song meta data");
  gtk_box_pack_start (GTK_BOX (self), frame, FALSE, TRUE, 0);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_BORDER);
  gtk_container_add (GTK_CONTAINER (frame), box);

  /* left side padding */
  gtk_box_pack_start (GTK_BOX (box), gtk_label_new ("    "), FALSE, TRUE, 0);

  // first column
  table = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);

  label = gtk_label_new (_("name"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  self->priv->name = GTK_ENTRY (gtk_entry_new ());
  g_object_set (self->priv->name, "hexpand", TRUE, "margin-left", LABEL_PADDING,
      NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->name), 1, 0, 1, 1);
  g_signal_connect (self->priv->name, "changed", G_CALLBACK (on_name_changed),
      (gpointer) self);

  label = gtk_label_new (_("genre"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
  self->priv->genre = GTK_ENTRY (gtk_entry_new ());
  g_object_set (self->priv->genre, "hexpand", TRUE, "margin-left",
      LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->genre), 1, 1, 1,
      1);
  g_signal_connect (self->priv->genre, "changed", G_CALLBACK (on_genre_changed),
      (gpointer) self);

  label = gtk_label_new (_("author"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);
  self->priv->author = GTK_ENTRY (gtk_entry_new ());
  g_object_set (self->priv->author, "hexpand", TRUE, "margin-left",
      LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->author), 1, 2, 1,
      1);
  g_signal_connect (self->priv->author, "changed",
      G_CALLBACK (on_author_changed), (gpointer) self);

  label = gtk_label_new (_("created"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);
  self->priv->date_created = GTK_ENTRY (gtk_entry_new ());
  gtk_editable_set_editable (GTK_EDITABLE (self->priv->date_created), FALSE);
  gtk_widget_set_can_focus (GTK_WIDGET (self->priv->date_created), FALSE);
  g_object_set (self->priv->date_created, "hexpand", TRUE, "margin-left",
      LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->date_created), 1,
      3, 1, 1);

  // second column
  table = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (box), table, TRUE, TRUE, 0);

  label = gtk_label_new (_("beats per minute"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  pspec =
      (GParamSpecULong *) g_object_class_find_property (song_info_class, "bpm");
  spin_adjustment = gtk_adjustment_new (pspec->default_value, pspec->minimum,
      pspec->maximum, 1.0, 5.0, 0.0);
  self->priv->bpm =
      GTK_SPIN_BUTTON (gtk_spin_button_new (spin_adjustment, 1.0, 0));
  g_object_set (self->priv->bpm, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->bpm), 1, 0, 1, 1);
  g_signal_connect (self->priv->bpm, "value-changed",
      G_CALLBACK (on_bpm_changed), (gpointer) self);

  label = gtk_label_new (_("beats"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
  pspec =
      (GParamSpecULong *) g_object_class_find_property (song_info_class, "tpb");
  pspec2 =
      (GParamSpecULong *) g_object_class_find_property (song_info_class,
      "bars");
  def = pspec2->default_value / pspec->default_value;
  min = pspec2->minimum / pspec->maximum;
  max = pspec2->maximum / pspec->minimum;
  if (min < 1)
    min = 1;
  spin_adjustment = gtk_adjustment_new (def, min, max, 1.0, 4.0, 0.0);
  self->priv->beats =
      GTK_SPIN_BUTTON (gtk_spin_button_new (spin_adjustment, 1.0, 0));
  g_object_set (self->priv->beats, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->beats), 1, 1, 1,
      1);
  g_signal_connect (self->priv->beats, "value-changed",
      G_CALLBACK (on_beats_changed), (gpointer) self);

  label = gtk_label_new (_("ticks per beat"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);
  spin_adjustment = gtk_adjustment_new (pspec->default_value, pspec->minimum,
      pspec->maximum, 1.0, 4.0, 0.0);
  self->priv->tpb =
      GTK_SPIN_BUTTON (gtk_spin_button_new (spin_adjustment, 1.0, 0));
  g_object_set (self->priv->tpb, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->tpb), 1, 2, 1, 1);
  g_signal_connect (self->priv->tpb, "value-changed",
      G_CALLBACK (on_tpb_changed), (gpointer) self);

  label = gtk_label_new (_("last saved"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);
  self->priv->date_changed = GTK_ENTRY (gtk_entry_new ());
  gtk_editable_set_editable (GTK_EDITABLE (self->priv->date_changed), FALSE);
  gtk_widget_set_can_focus (GTK_WIDGET (self->priv->date_changed), FALSE);
  g_object_set (self->priv->date_changed, "hexpand", TRUE, "margin-left",
      LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET (self->priv->date_changed), 1,
      3, 1, 1);

  // @idea show tick and subtick interval as time (s:ms)

  // second row of hbox
  frame = gtk_frame_new (NULL);
  label = make_heading (_("free text info"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BOX_BORDER);
  gtk_box_pack_start (GTK_BOX (self), frame, TRUE, TRUE, 0);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_BORDER);
  gtk_container_add (GTK_CONTAINER (frame), box);

  /* left side padding */
  gtk_box_pack_start (GTK_BOX (box), gtk_label_new ("    "), FALSE, TRUE, 0);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
      GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), scrolledwindow, TRUE, TRUE, 0);

  self->priv->info = GTK_TEXT_VIEW (gtk_text_view_new ());
  gtk_widget_set_name (GTK_WIDGET (self->priv->info), "free text info");
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self->priv->info), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolledwindow),
      GTK_WIDGET (self->priv->info));
  g_signal_connect (gtk_text_view_get_buffer (self->priv->info), "changed",
      G_CALLBACK (on_info_changed), (gpointer) self);

  // register event handlers
  g_signal_connect_object (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self, 0);
  // listen to page changes
  g_signal_connect ((gpointer) pages, "notify::page",
      G_CALLBACK (on_page_switched), (gpointer) self);
  g_signal_connect ((gpointer) self, "map",
      G_CALLBACK (on_page_mapped), (gpointer) self);

  GST_DEBUG ("  done");
}

//-- constructor methods

/**
 * bt_main_page_info_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainPageInfo *
bt_main_page_info_new (const BtMainPages * pages)
{
  BtMainPageInfo *self;

  self =
      BT_MAIN_PAGE_INFO (g_object_new (BT_TYPE_MAIN_PAGE_INFO, "spacing", 6,
          NULL));
  bt_main_page_info_init_ui (self, pages);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static gboolean
bt_main_page_info_focus (GtkWidget * widget, GtkDirectionType direction)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (widget);

  GST_DEBUG ("focusing default widget");
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->priv->info));
  return FALSE;
}

static void
bt_main_page_info_dispose (GObject * object)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->song_info);

  g_object_unref (self->priv->app);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_info_parent_class)->dispose (object);
}

static void
bt_main_page_info_init (BtMainPageInfo * self)
{
  self->priv = bt_main_page_info_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
      GTK_ORIENTATION_VERTICAL);
}

static void
bt_main_page_info_class_init (BtMainPageInfoClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->dispose = bt_main_page_info_dispose;

  gtkwidget_class->focus = bt_main_page_info_focus;
}
