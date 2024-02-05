/* $Id: sequence-grid-model.h 3349 2011-05-02 20:35:54Z ensonic $
 *
 * Buzztrax
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

#ifndef BT_SEQUENCE_GRID_MODEL_H
#define BT_SEQUENCE_GRID_MODEL_H

#include <glib.h>
#include <glib-object.h>
#include "core/sequence.h"
#include "list-item-long.h"


/**
 * BtSequenceGridModel:
 *
 * List model containing song sequence data and metadata used for display by UI elements.
 */
G_DECLARE_FINAL_TYPE(BtSequenceGridModel, bt_sequence_grid_model, BT, SEQUENCE_GRID_MODEL, GObject);

#define BT_TYPE_SEQUENCE_GRID_MODEL            (bt_sequence_grid_model_get_type())


glong bt_sequence_grid_model_item_get_tick (BtSequenceGridModel* model, const BtListItemLong* item);
gboolean bt_sequence_grid_model_item_get_shade (BtSequenceGridModel* model, const BtListItemLong* item, gulong bars);
glong bt_sequence_grid_model_item_get_pos (BtSequenceGridModel* model, const BtListItemLong* item);
gchar* bt_sequence_grid_model_item_get_label (BtSequenceGridModel* model, const BtListItemLong* item);
gboolean bt_sequence_grid_model_item_has_label (BtSequenceGridModel* model, const BtListItemLong* item);


G_DECLARE_FINAL_TYPE(BtSequenceGridModelItem, bt_sequence_grid_model_item, BT, SEQUENCE_GRID_MODEL_ITEM, GObject);

BtSequenceGridModel *bt_sequence_grid_model_new(BtSequence *sequence, BtSongInfo *song_info, gulong bars);


#endif // BT_SEQUENCE_GRID_MODEL_H
