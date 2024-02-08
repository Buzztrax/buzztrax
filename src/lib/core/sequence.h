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

#ifndef BT_SEQUENCE_H
#define BT_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>

#include "pattern.h"
#include "machine.h"

#define BT_TYPE_SEQUENCE            (bt_sequence_get_type ())
#define BT_SEQUENCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SEQUENCE, BtSequence))
#define BT_SEQUENCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SEQUENCE, BtSequenceClass))
#define BT_IS_SEQUENCE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SEQUENCE))
#define BT_IS_SEQUENCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SEQUENCE))
#define BT_SEQUENCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SEQUENCE, BtSequenceClass))

/* type macros */

typedef struct _BtSequence BtSequence;
typedef struct _BtSequenceClass BtSequenceClass;
typedef struct _BtSequencePrivate BtSequencePrivate;

/**
 * BtSequence:
 *
 * Starting point for the #BtSong timeline data-structures.
 * Holds a series of array of #BtCmdPatterns for time and tracks, which define the
 * events that are sent to a #BtMachine at a time.
 */
struct _BtSequence {
  const GObject parent;
  
  /*< private >*/
  BtSequencePrivate *priv;
};

struct _BtSequenceClass {
  const GObjectClass parent;
};

GType bt_sequence_get_type(void) G_GNUC_CONST;

BtSequence *bt_sequence_new(const BtSong * const song);

glong bt_sequence_get_track_by_machine(const BtSequence * const self,const BtMachine * const machine,gulong track);
glong bt_sequence_get_tick_by_pattern(const BtSequence * const self,gulong track, const BtCmdPattern * const pattern,gulong tick);

BtMachine *bt_sequence_get_machine(const BtSequence * const self,const gulong track);

gboolean bt_sequence_add_track(const BtSequence * const self,const BtMachine * const machine, const glong ix);
gboolean bt_sequence_remove_track_by_ix(const BtSequence * const self, const gulong ix);
gboolean bt_sequence_remove_track_by_machine(const BtSequence * const self,const BtMachine * const machine);
gboolean bt_sequence_move_track_left(const BtSequence * const self, const gulong track);
gboolean bt_sequence_move_track_right(const BtSequence * const self, const gulong track);

gchar *bt_sequence_get_label(const BtSequence * const self, const gulong time);
void bt_sequence_set_label(const BtSequence * const self, const gulong time, const gchar * const label);
BtCmdPattern *bt_sequence_get_pattern(const BtSequence * const self, const gulong time, const gulong track);
gboolean bt_sequence_set_pattern_quick(const BtSequence * const self, const gulong time, const gulong track, const BtCmdPattern * const pattern);
void bt_sequence_set_pattern(const BtSequence * const self, const gulong time, const gulong track, const BtCmdPattern * const pattern);

gulong bt_sequence_get_loop_length(const BtSequence * const self);
gulong bt_sequence_limit_play_pos(const BtSequence * const self, const gulong play_pos);

gboolean bt_sequence_is_pattern_used(const BtSequence * const self,const BtPattern * const pattern);

void bt_sequence_insert_rows(const BtSequence * const self, const gulong time, const glong track, const gulong rows);
void bt_sequence_insert_full_rows(const BtSequence * const self, const gulong time, const gulong rows);
void bt_sequence_delete_rows(const BtSequence * const self, const gulong time, const glong track, const gulong rows);
void bt_sequence_delete_full_rows(const BtSequence * const self, const gulong time, const gulong rows);

// This should only be needed by functions such as sequence data paste.
// It should ideally not be exposed at all, but this will do for now.
void bt_sequence_resize_data_length (const BtSequence * const self, const gulong length);

#endif // BT_SEQUENCE_H
