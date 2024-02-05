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

#ifndef BT_SEQUENCE_VIEW_H
#define BT_SEQUENCE_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

typedef struct _BtMachine BtMachine;
typedef struct _BtSequence BtSequence;
typedef struct _BtSong BtSong;
typedef struct _BtSongInfo BtSongInfo;
typedef struct _BtSequenceGridModel BtSequenceGridModel;

/**
 * BtSequenceView:
 *
 * the sequence widget view
 */
G_DECLARE_FINAL_TYPE (BtSequenceView, bt_sequence_view, BT, SEQUENCE_VIEW, GtkWidget);

#define BT_TYPE_SEQUENCE_VIEW            (bt_sequence_view_get_type ())

#define BT_TYPE_SEQUENCE_GRID_MODEL_POS_FORMAT   (bt_sequence_grid_model_pos_format_get_type())

/**
 * BtSequenceGridModelPosFormat:
 * @BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS: show as number of ticks
 * @BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME: show as "min:sec.msec"
 * @BT_SEQUENCE_GRID_MODEL_POS_FORMAT_BEATS: show as "beats.ticks"
 *
 * Format type for time values in the sequencer.
 */
typedef enum {
  BT_SEQUENCE_VIEW_POS_FORMAT_TICKS=0,
  BT_SEQUENCE_VIEW_POS_FORMAT_TIME,
  BT_SEQUENCE_VIEW_POS_FORMAT_BEATS
} BtSequenceViewPosFormat;

GType bt_sequence_view_pos_format_get_type(void) G_GNUC_CONST;

BtSequenceView *bt_sequence_view_new(BtSong* song);

BtMachine *bt_sequence_view_get_current_machine(BtSequenceView *self);
gboolean bt_sequence_view_get_current_pos(BtSequenceView *self, gulong *time, gulong *track);
BtSequenceGridModel *bt_sequence_view_get_model(BtSequenceView *self);
void bt_sequence_view_set_song(BtSequenceView *self, BtSong *song);

#endif // BT_SEQUENCE_VIEW_H
