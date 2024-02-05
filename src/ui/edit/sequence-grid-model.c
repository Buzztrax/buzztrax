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

//-- structs

struct _BtSequenceGridModel
{
  GObject parent;
  
  BtSequence *sequence;
  BtSongInfo *song_info;

  gulong bars;
  gulong ticks_per_beat;

  gulong tracks;

  /* Contains BtListItemLong.
     
     Note that all GListModels must return GObjects, and all of those objects
     must be distinct instances. That's why a separate array of BtListItemLong
     objects is kept, even though it seems redundant.
   */
  GPtrArray* items;
};

//-- the class

static void bt_sequence_grid_model_g_list_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSequenceGridModel, bt_sequence_grid_model,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
        bt_sequence_grid_model_g_list_model_init));

//-- helper

static void
bt_sequence_grid_model_rows_changed (BtSequenceGridModel * model, gulong beg,
    gulong end)
{
  g_list_model_items_changed (G_LIST_MODEL (model), beg, end - beg, end - beg);
}

static void
bt_sequence_grid_model_all_rows_changed (BtSequenceGridModel * model)
{
  bt_sequence_grid_model_rows_changed (model, 0, model->items->len);
}

static void
update_length (BtSequenceGridModel * model, gulong new_length)
{
  gulong old_length = model->items->len;
  
  GST_INFO ("resize length : %lu -> %lu", old_length, new_length);

  if (old_length < new_length) {
    g_ptr_array_set_size (model->items, new_length);
    
    // trigger row-inserted
    for (gulong i = old_length; i < new_length; i++) {
      model->items->pdata[i] = bt_list_item_long_new (i);
    }

    g_list_model_items_changed (G_LIST_MODEL (model), old_length, 0, new_length);
  } else if (old_length > new_length) {
    // trigger row-deleted
    for (gulong i = old_length; i != 0; i--) {
      g_object_unref (model->items->pdata[i-1]);
    }
    
    g_ptr_array_set_size (model->items, new_length);
    g_list_model_items_changed (G_LIST_MODEL (model), new_length - 1, old_length - new_length, 0);
  }
}

//-- signal handlers

static void
on_song_info_tpb_changed (BtSongInfo * song_info, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);

  g_object_get (song_info, "tpb", &model->ticks_per_beat, NULL);
  bt_sequence_grid_model_all_rows_changed (model);
}

static void
on_sequence_length_changed (BtSequence * sequence, GParamSpec * arg,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);
  gulong old_length = model->items->len;
  gulong new_length;
  
  g_object_get ((gpointer) sequence, "len-patterns", &new_length, NULL);
  if (new_length != old_length) {
    GST_INFO ("sequence length changed: %lu -> %lu", old_length,
        new_length);

    update_length (model, new_length);
  }
}

static void
on_sequence_rows_changed (BtSequence * sequence, gulong beg, gulong end,
    gpointer user_data)
{
  BtSequenceGridModel *model = BT_SEQUENCE_GRID_MODEL (user_data);

  bt_sequence_grid_model_rows_changed (model, beg, end);
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
  
  gulong length;
  g_object_get ((gpointer) sequence, "len-patterns", &length, NULL);
  update_length (self, length);

  self->sequence = sequence;
  g_object_ref (sequence);
  self->song_info = song_info;
  g_object_ref (song_info);

  g_object_get (self->song_info, "bars", &self->bars, NULL);

  g_object_get (song_info, "tpb", &self->ticks_per_beat, NULL);
  g_signal_connect_object (song_info, "notify::tpb",
      G_CALLBACK (on_song_info_tpb_changed), (gpointer) self, 0);

  // follow sequence grid size changes
  g_signal_connect_object (sequence, "notify::len-patterns",
      G_CALLBACK (on_sequence_length_changed), (gpointer) self, 0);
  g_signal_connect_object (sequence, "rows-changed",
      G_CALLBACK (on_sequence_rows_changed), (gpointer) self, 0);

  return self;
}

//-- methods

glong
bt_sequence_grid_model_item_get_tick (BtSequenceGridModel* model, const BtListItemLong* item) {
  return item->value;
}

gboolean
bt_sequence_grid_model_item_get_shade (BtSequenceGridModel* model, const BtListItemLong* item, gulong bars)
{
  glong tick = bt_sequence_grid_model_item_get_tick (model, item);
  guint shade = (tick % (bars * 2));
  if (shade == 0) {
    return FALSE;
  } else {
    // we're only interested in shade==bars, but lets set the others too
    // even though we should not be called for those
    return TRUE;
  }
}

glong
bt_sequence_grid_model_item_get_pos (BtSequenceGridModel* model, const BtListItemLong* item)
{
  return item->value;
}

/**
 * bt_sequence_grid_model_item_get_label:
 *
 * Returns: (transfer full)
 */
gchar*
bt_sequence_grid_model_item_get_label (BtSequenceGridModel* model, const BtListItemLong* item)
{
  glong tick = bt_sequence_grid_model_item_get_tick (model, item);
  return bt_sequence_get_label (model->sequence, tick);
}

gboolean
bt_sequence_grid_model_item_has_label (BtSequenceGridModel* model, const BtListItemLong* item)
{
  glong tick = bt_sequence_grid_model_item_get_tick (model, item);
  return bt_sequence_has_label (model->sequence, tick);
}

//-- GListModel interface

static gpointer
bt_sequence_grid_model_get_item (GListModel* list, guint position)
{
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL (list);
  if (position > self->items->len)
    return NULL;

  g_assert (self->items->pdata[position]);
  
  return self->items->pdata[position];
}

static GType
bt_sequence_grid_model_get_item_type (GListModel* list)
{
  return BT_TYPE_LIST_ITEM_LONG;
}

static guint
bt_sequence_grid_model_get_n_items (GListModel* list)
{
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL (list);
  return self->items->len;
}
  
static void
bt_sequence_grid_model_g_list_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GListModelInterface *const iface = g_iface;

  iface->get_item = bt_sequence_grid_model_get_item;
  iface->get_item_type = bt_sequence_grid_model_get_item_type;
  iface->get_n_items = bt_sequence_grid_model_get_n_items;
}

//-- class internals

static void
bt_sequence_grid_model_dispose (GObject * object)
{
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL (object);
  
  g_clear_object (&self->sequence);
  g_clear_object (&self->song_info);
  g_clear_pointer (&self->items, g_ptr_array_unref);

  G_OBJECT_CLASS (bt_sequence_grid_model_parent_class)->dispose (object);
}

static void
bt_sequence_grid_model_init (BtSequenceGridModel * self)
{
  self = bt_sequence_grid_model_get_instance_private(self);
  self->items = g_ptr_array_new ();
}

static void
bt_sequence_grid_model_class_init (BtSequenceGridModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_sequence_grid_model_dispose;
}
