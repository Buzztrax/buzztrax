/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btsequencegridmodel
 * @short_description: data model class for widgets showing the pattern sequence
 * of a song
 *
 * A generic model representing the track x time grid of patterns of a song.
 * Can be shown by a treeview.
 *
 * The visible length can be greater then the real length of the underlying
 * sequence, by setting the BtSequenceGridModel::length property.
 */

/* design:
 * - support reordering tracks without rebuilding the model
 *   - check on_track_move_left_activated()
 *   - maybe we need to emit notify::tracks when reordering too
 * - when do we refresh+recolorize in the old code
 *   - todo in new code
 *     - move track left/right
 *     - cursor below the end-of song
 *   - already handled in new code
 *     - loop-end changed
 *     - add/remove track
 *     - machine removed (should trigger notify::tracks)
 *     - song changed (will create the model)
 *
 * - we have a property for the pos-format, when it changes we refresh all rows
 *   - main-page-sequence should build the combobox from this enum
 *     (should we have a model for enums?)
 */

#define BT_EDIT
#define BT_SEQUENCE_GRID_MODEL_C

#include "bt-edit.h"
#include <glib/gprintf.h>

//-- defines
#define N_COLUMNS __BT_SEQUENCE_GRID_MODEL_N_COLUMNS

//-- property ids

enum
{
  SEQUENCE_GRID_MODEL_POS_FORMAT = 1,
  SEQUENCE_GRID_MODEL_BARS,
  SEQUENCE_GRID_MODEL_LENGTH
};

//-- structs

struct _BtSequenceGridModelPrivate
{
  gint stamp;
  BtSequence *sequence;
  BtSongInfo *song_info;

  BtSequenceGridModelPosFormat pos_format;
  gulong bars;
  gulong ticks_per_beat;

  GType param_types[N_COLUMNS];
  gulong length, tracks;
  gulong visible_length;
};

//-- enums

GType
bt_sequence_grid_model_pos_format_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS,
          "BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS", "ticks"},
      {BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME,
          "BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME", "time"},
      {BT_SEQUENCE_GRID_MODEL_POS_FORMAT_BEATS,
          "BT_SEQUENCE_GRID_MODEL_POS_FORMAT_BEATS", "beats"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtSequenceGridModelPosFormat", values);
  }
  return type;
}

//-- the class

static void bt_sequence_grid_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSequenceGridModel, bt_sequence_grid_model,
    G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
        bt_sequence_grid_model_tree_model_init));

//-- helper

static gchar *
format_position (const BtSequenceGridModel * model, gulong pos)
{
  static gchar pos_str[20];

  switch (model->priv->pos_format) {
    case BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS:
      g_snprintf (pos_str, 5, "%lu", pos);
      break;
    case BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME:{
      gulong msec, sec, min;
      bt_song_info_tick_to_m_s_ms (model->priv->song_info, pos, &min, &sec,
          &msec);
      g_snprintf (pos_str, sizeof(pos_str), "%02lu:%02lu.%03lu", min, sec, msec);
    }
      break;
    case BT_SEQUENCE_GRID_MODEL_POS_FORMAT_BEATS:{
      gulong beat, tick;
      gint tick_chars;

      beat = pos / model->priv->ticks_per_beat;
      tick = pos - (beat * model->priv->ticks_per_beat);
      if (model->priv->ticks_per_beat >= 100)
        tick_chars = 3;
      else if (model->priv->ticks_per_beat >= 10)
        tick_chars = 2;
      else
        tick_chars = 1;
      g_snprintf (pos_str, sizeof(pos_str), "%lu.%0*lu", beat, tick_chars, tick);
    }
      break;
    default:
      *pos_str = '\0';
      GST_WARNING ("unimplemented time format %d", model->priv->pos_format);
  }
  return pos_str;
}

static void
bt_sequence_grid_model_rows_changed (BtSequenceGridModel * model, gulong beg,
    gulong end)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  gulong i;

  // trigger row-changed for all rows
  iter.stamp = model->priv->stamp;
  for (i = beg; i <= end; i++) {
    iter.user_data = GUINT_TO_POINTER (i);
    path = gtk_tree_path_new ();
    gtk_tree_path_append_index (path, i);
    gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
    gtk_tree_path_free (path);
  }
}

static void
bt_sequence_grid_model_all_rows_changed (BtSequenceGridModel * model)
{
  bt_sequence_grid_model_rows_changed (model, 0,
      model->priv->visible_length - 1);
}

static void
update_length (BtSequenceGridModel * model, gulong old_length,
    gulong new_length)
{
  GtkTreePath *path;
  glong i;

  GST_INFO ("resize length : %lu -> %lu", old_length, new_length);

  if (old_length < new_length) {
    GtkTreeIter iter;

    // trigger row-inserted
    iter.stamp = model->priv->stamp;
    for (i = old_length; i < new_length; i++) {
      iter.user_data = GUINT_TO_POINTER (i);
      path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, i);
      gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
      gtk_tree_path_free (path);
    }
  } else if (old_length > new_length) {
    // trigger row-deleted
    for (i = old_length - 1; i >= new_length; i--) {
      path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, i);
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
      gtk_tree_path_free (path);
    }
  }
}

//-- signal handlers

static void
on_pattern_name_changed (BtPattern * pattern, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);
  BtCmdPattern *that_pattern;
  GtkTreePath *path;
  GtkTreeIter iter;
  gulong i, j;
  gulong length = model->priv->length;
  gulong tracks = model->priv->tracks;

  iter.stamp = model->priv->stamp;

  // find all affected rows and signal updates
  // FIXME(ensonic): skip tracks with wrong machine (do a first run and build a list of tracks)
  for (i = 0; i < length; i++) {
    for (j = 0; j < tracks; j++) {
      if ((that_pattern =
              bt_sequence_get_pattern (model->priv->sequence, i, j))) {
        if (that_pattern == (BtCmdPattern *) pattern) {
          iter.user_data = GUINT_TO_POINTER (i);
          path = gtk_tree_path_new ();
          gtk_tree_path_append_index (path, i);
          gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
          gtk_tree_path_free (path);
          g_object_unref (that_pattern);
          break;
        } else {
          g_object_unref (that_pattern);
        }
      }
    }
  }
}

static void
on_song_info_tpb_changed (BtSongInfo * song_info, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);

  g_object_get (song_info, "tpb", &model->priv->ticks_per_beat, NULL);
  bt_sequence_grid_model_all_rows_changed (model);
}

static void
on_sequence_length_changed (BtSequence * sequence, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);
  gulong old_length = model->priv->length;

  g_object_get ((gpointer) sequence, "length", &model->priv->length, NULL);
  if (model->priv->length != old_length) {
    gulong old_visible_length = model->priv->visible_length;
    model->priv->visible_length = MAX (old_visible_length, model->priv->length);
    GST_INFO ("sequence length changed: %lu -> %lu", old_length,
        model->priv->length);

    update_length (model, old_visible_length, model->priv->visible_length);
  }
}

static void
on_sequence_tracks_changed (BtSequence * sequence, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);
  gulong old_tracks = model->priv->tracks;

  g_object_get ((gpointer) sequence, "tracks", &model->priv->tracks, NULL);
  if (model->priv->tracks != old_tracks) {
    GST_INFO ("sequence tracks changed: %lu -> %lu", old_tracks,
        model->priv->tracks);

    if (model->priv->visible_length)
      bt_sequence_grid_model_all_rows_changed (model);
  }
}

static void
on_sequence_rows_changed (BtSequence * sequence, gulong beg, gulong end,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);

  bt_sequence_grid_model_rows_changed (model, beg, end);
}

static void
on_sequence_pattern_added (BtSequence * sequence, BtPattern * pattern,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);

  g_signal_connect_object (pattern, "notify::name",
      G_CALLBACK (on_pattern_name_changed), (gpointer) model, 0);
}

//-- constructor methods

/**
 * bt_sequence_grid_model_new:
 * @sequence: the sequence
 * @song_info: the song-info
 * @bars: the intial bar-filtering for the view
 *
 * Creates a grid model for the @sequence. The model is automatically updated on
 * changes in the content. It also expands its length in sync to the sequence.
 *
 * To make the row-shading work, the application has to update
 * #BtSequenceGridModel:bars when it changed on the view.
 *
 * When setting #BtSequenceGridModel:length to a value greater than the real
 * @sequence, the model will append dummy rows. This allows the cursor to go
 * beyond the end to expand the sequence.
 *
 * Returns: the sequence model.
 */
BtSequenceGridModel *
bt_sequence_grid_model_new (BtSequence * sequence, BtSongInfo * song_info,
    gulong bars)
{
  BtSequenceGridModel *self;

  self = g_object_new (BT_TYPE_SEQUENCE_GRID_MODEL, NULL);

  self->priv->sequence = sequence;
  g_object_add_weak_pointer ((GObject *) sequence,
      (gpointer *) & self->priv->sequence);
  self->priv->song_info = song_info;
  g_object_add_weak_pointer ((GObject *) song_info,
      (gpointer *) & self->priv->song_info);
  self->priv->bars = bars;

  // static columns
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_SHADE] = G_TYPE_BOOLEAN;
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_POS] = G_TYPE_LONG;
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_POSSTR] = G_TYPE_STRING;
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_LABEL] = G_TYPE_STRING;

  // add enough rows for current length
  on_sequence_tracks_changed (sequence, NULL, (gpointer) self);
  on_sequence_length_changed (sequence, NULL, (gpointer) self);

  g_object_get (song_info, "tpb", &self->priv->ticks_per_beat, NULL);
  g_signal_connect_object (song_info, "notify::tpb",
      G_CALLBACK (on_song_info_tpb_changed), (gpointer) self, 0);

  // follow sequence grid size changes
  g_signal_connect_object (sequence, "notify::length",
      G_CALLBACK (on_sequence_length_changed), (gpointer) self, 0);
  g_signal_connect_object (sequence, "notify::tracks",
      G_CALLBACK (on_sequence_tracks_changed), (gpointer) self, 0);
  g_signal_connect_object (sequence, "rows-changed",
      G_CALLBACK (on_sequence_rows_changed), (gpointer) self, 0);
  // track pattern first additions, last removal
  g_signal_connect_object (sequence, "pattern-added",
      G_CALLBACK (on_sequence_pattern_added), (gpointer) self, 0);

  return self;
}

//-- methods

//-- tree model interface

static GtkTreeModelFlags
bt_sequence_grid_model_tree_model_get_flags (GtkTreeModel * tree_model)
{
  return (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint
bt_sequence_grid_model_tree_model_get_n_columns (GtkTreeModel * tree_model)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);

  return (N_COLUMNS + model->priv->tracks);
}

static GType
bt_sequence_grid_model_tree_model_get_column_type (GtkTreeModel * tree_model,
    gint index)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);

  g_return_val_if_fail (index < N_COLUMNS + model->priv->tracks,
      G_TYPE_INVALID);

  if (index < N_COLUMNS)
    return model->priv->param_types[index];
  else
    return G_TYPE_STRING;
}

static gboolean
bt_sequence_grid_model_tree_model_get_iter (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreePath * path)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);
  guint tick = gtk_tree_path_get_indices (path)[0];

  if (tick >= model->priv->visible_length)
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = GUINT_TO_POINTER (tick);

  return TRUE;
}

static GtkTreePath *
bt_sequence_grid_model_tree_model_get_path (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);
  GtkTreePath *path;

  g_return_val_if_fail (iter->stamp == model->priv->stamp, NULL);

  if (g_sequence_iter_is_end (iter->user_data))
    return NULL;

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, GPOINTER_TO_UINT (iter->user_data));

  return path;
}

static void
bt_sequence_grid_model_tree_model_get_value (GtkTreeModel * tree_model,
    GtkTreeIter * iter, gint column, GValue * value)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);
  guint track, tick;

  //GST_DEBUG("getting value for column=%d / (%d+%d)",column,N_COLUMNS,model->priv->tracks);

  g_return_if_fail (column < N_COLUMNS + model->priv->tracks);

  if (column < N_COLUMNS)
    g_value_init (value, model->priv->param_types[column]);
  else
    g_value_init (value, G_TYPE_STRING);
  tick = GPOINTER_TO_UINT (iter->user_data);

  switch (column) {
    case BT_SEQUENCE_GRID_MODEL_SHADE:{
      guint shade = (tick % (model->priv->bars * 2));
      if (shade == 0) {
        g_value_set_boolean (value, FALSE);
      } else {
        // we're only interested in shade==bars, but lets set the others too
        // even though we should not be called for those
        g_value_set_boolean (value, TRUE);
      }
    }
      break;
    case BT_SEQUENCE_GRID_MODEL_POS:
      g_value_set_long (value, tick);
      break;
    case BT_SEQUENCE_GRID_MODEL_POSSTR:
      g_value_set_string (value, format_position (model, tick));
      break;
    case BT_SEQUENCE_GRID_MODEL_LABEL:
      if (tick < model->priv->length) {
        g_value_take_string (value,
            bt_sequence_get_label (model->priv->sequence, tick));
      }
      break;
    default:
      if (tick < model->priv->length) {
        BtCmdPattern *pattern;

        track = column - N_COLUMNS;
        //GST_LOG("getting pattern name for tick=%u,track=%u",tick,track);
        if ((pattern =
                bt_sequence_get_pattern (model->priv->sequence, tick, track))) {
          g_object_get_property ((GObject *) pattern, "name", value);
          g_object_unref (pattern);
        }
      }
  }
}

static gboolean
bt_sequence_grid_model_tree_model_iter_next (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);
  guint tick;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

  tick = GPOINTER_TO_UINT (iter->user_data) + 1;
  if (tick < model->priv->visible_length) {
    iter->user_data = GUINT_TO_POINTER (tick);
    return TRUE;
  } else {
    iter->stamp = 0;
    return FALSE;
  }
}

static gboolean
bt_sequence_grid_model_tree_model_iter_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);

  /* this is a list, nodes have no children */
  if (!parent) {
    if (model->priv->visible_length > 0) {
      iter->stamp = model->priv->stamp;
      iter->user_data = GUINT_TO_POINTER (0);
      return TRUE;
    }
  }
  iter->stamp = 0;
  return FALSE;
}

static gboolean
bt_sequence_grid_model_tree_model_iter_has_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  return FALSE;
}

static gint
bt_sequence_grid_model_tree_model_iter_n_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);

  if (iter == NULL)
    return model->priv->visible_length;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, -1);
  return 0;
}

static gboolean
bt_sequence_grid_model_tree_model_iter_nth_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (tree_model);

  if (parent || n >= model->priv->visible_length) {
    iter->stamp = 0;
    return FALSE;
  }

  iter->stamp = model->priv->stamp;
  iter->user_data = GUINT_TO_POINTER (n);

  return TRUE;
}

static gboolean
bt_sequence_grid_model_tree_model_iter_parent (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * child)
{
  iter->stamp = 0;
  return FALSE;
}

static void
bt_sequence_grid_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GtkTreeModelIface *const iface = g_iface;

  iface->get_flags = bt_sequence_grid_model_tree_model_get_flags;
  iface->get_n_columns = bt_sequence_grid_model_tree_model_get_n_columns;
  iface->get_column_type = bt_sequence_grid_model_tree_model_get_column_type;
  iface->get_iter = bt_sequence_grid_model_tree_model_get_iter;
  iface->get_path = bt_sequence_grid_model_tree_model_get_path;
  iface->get_value = bt_sequence_grid_model_tree_model_get_value;
  iface->iter_next = bt_sequence_grid_model_tree_model_iter_next;
  iface->iter_children = bt_sequence_grid_model_tree_model_iter_children;
  iface->iter_has_child = bt_sequence_grid_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_sequence_grid_model_tree_model_iter_n_children;
  iface->iter_nth_child = bt_sequence_grid_model_tree_model_iter_nth_child;
  iface->iter_parent = bt_sequence_grid_model_tree_model_iter_parent;
}

//-- class internals

static void
bt_sequence_grid_model_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL (object);

  switch (property_id) {
    case SEQUENCE_GRID_MODEL_POS_FORMAT:
      g_value_set_enum (value, self->priv->pos_format);
      break;
    case SEQUENCE_GRID_MODEL_BARS:
      g_value_set_ulong (value, self->priv->bars);
      break;
    case SEQUENCE_GRID_MODEL_LENGTH:
      g_value_set_ulong (value, self->priv->visible_length);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sequence_grid_model_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL (object);

  switch (property_id) {
    case SEQUENCE_GRID_MODEL_POS_FORMAT:{
      BtSequenceGridModelPosFormat old_pos_format = self->priv->pos_format;
      self->priv->pos_format = g_value_get_enum (value);
      if (self->priv->pos_format != old_pos_format)
        bt_sequence_grid_model_all_rows_changed (self);
      break;
    }
    case SEQUENCE_GRID_MODEL_BARS:{
      gulong old_bars = self->priv->bars;
      self->priv->bars = g_value_get_ulong (value);
      if (self->priv->bars != old_bars)
        bt_sequence_grid_model_all_rows_changed (self);
      break;
    }
    case SEQUENCE_GRID_MODEL_LENGTH:{
      gulong old_length = self->priv->visible_length;
      self->priv->visible_length = g_value_get_ulong (value);
      self->priv->visible_length =
          MAX (self->priv->visible_length, self->priv->length);
      GST_DEBUG ("visible length changed: %lu", self->priv->visible_length);
      update_length (self, old_length, self->priv->visible_length);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sequence_grid_model_finalize (GObject * object)
{
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->sequence) {
    g_object_remove_weak_pointer ((GObject *) self->priv->sequence,
        (gpointer *) & self->priv->sequence);
  }
  if (self->priv->song_info) {
    g_object_remove_weak_pointer ((GObject *) self->priv->song_info,
        (gpointer *) & self->priv->song_info);
  }

  G_OBJECT_CLASS (bt_sequence_grid_model_parent_class)->finalize (object);
}

static void
bt_sequence_grid_model_init (BtSequenceGridModel * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SEQUENCE_GRID_MODEL,
      BtSequenceGridModelPrivate);

  // random int to check whether an iter belongs to our model
  self->priv->stamp = g_random_int ();
}

static void
bt_sequence_grid_model_class_init (BtSequenceGridModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSequenceGridModelPrivate));

  gobject_class->set_property = bt_sequence_grid_model_set_property;
  gobject_class->get_property = bt_sequence_grid_model_get_property;
  gobject_class->finalize = bt_sequence_grid_model_finalize;

  g_object_class_install_property (gobject_class, SEQUENCE_GRID_MODEL_POS_FORMAT, g_param_spec_enum ("pos-format", "position format prop", "the display format for position columns", BT_TYPE_SEQUENCE_GRID_MODEL_POS_FORMAT,   /* enum type */
          BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_GRID_MODEL_BARS,
      g_param_spec_ulong ("bars", "bars prop",
          "tick stepping for the color shading", 1, G_MAXUINT, 16,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_GRID_MODEL_LENGTH,
      g_param_spec_ulong ("length", "length prop",
          "visible length of the sequence (>= real length)", 0, G_MAXUINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
