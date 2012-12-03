/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

#include <glib.h>
#include <glib-object.h>

#include "cmd-pattern.h"

#define BT_TYPE_PATTERN             (bt_pattern_get_type ())
#define BT_PATTERN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN, BtPattern))
#define BT_PATTERN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN, BtPatternClass))
#define BT_IS_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN))
#define BT_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN))
#define BT_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN, BtPatternClass))

/* type macros */

typedef struct _BtPattern BtPattern;
typedef struct _BtPatternClass BtPatternClass;
typedef struct _BtPatternPrivate BtPatternPrivate;

/**
 * BtPattern:
 *
 * Holds a sequence of events for a #BtMachine.
 */
struct _BtPattern {
  const BtCmdPattern parent;
  
  /*< private >*/
  BtPatternPrivate *priv;
};

struct _BtPatternClass {
  const BtCmdPatternClass parent;
};

GType bt_pattern_get_type(void) G_GNUC_CONST;

#include "machine.h"
#include "song.h"
#include "value-group.h"
#include "wire.h"

BtPattern *bt_pattern_new(const BtSong * const song, const gchar * const name, const gulong length, const BtMachine * const machine);

BtPattern *bt_pattern_copy(const BtPattern * const self);

GValue *bt_pattern_get_global_event_data(const BtPattern * const self, const gulong tick, const gulong param);
GValue *bt_pattern_get_voice_event_data(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param);
GValue *bt_pattern_get_wire_event_data(const BtPattern * const self, const gulong tick, const BtWire *wire, const gulong param);

gboolean bt_pattern_set_global_event(const BtPattern * const self, const gulong tick, const gulong param, const gchar * const value);
gboolean bt_pattern_set_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param, const gchar * const value);
gboolean bt_pattern_set_wire_event(const BtPattern * const self, const gulong tick, const BtWire *wire, const gulong param, const gchar * const value);
gchar *bt_pattern_get_global_event(const BtPattern * const self, const gulong tick, const gulong param);
gchar *bt_pattern_get_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param);
gchar *bt_pattern_get_wire_event(const BtPattern * const self, const gulong tick, const BtWire *wire, const gulong param);
gboolean bt_pattern_test_global_event(const BtPattern * const self, const gulong tick, const gulong param);
gboolean bt_pattern_test_voice_event(const BtPattern * const self, const gulong tick, const gulong voice, const gulong param);
gboolean bt_pattern_test_wire_event(const BtPattern * const self, const gulong tick, const BtWire *wire, const gulong param);
gboolean bt_pattern_test_tick(const BtPattern * const self, const gulong tick);

//BtValueGroup **bt_pattern_get_groups(const BtPattern * const self);
BtValueGroup *bt_pattern_get_global_group(const BtPattern * const self);
BtValueGroup *bt_pattern_get_voice_group(const BtPattern * const self, const gulong voice);
BtValueGroup *bt_pattern_get_wire_group(const BtPattern * const self, const BtWire *wire);
BtValueGroup *bt_pattern_get_group_by_parameter_group(const BtPattern * const self, BtParameterGroup *param_group);

void bt_pattern_insert_row(const BtPattern * const self, const gulong tick);
void bt_pattern_delete_row(const BtPattern * const self, const gulong tick);

void bt_pattern_clear_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick);
void bt_pattern_blend_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick);
void bt_pattern_flip_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick);
void bt_pattern_randomize_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick);
void bt_pattern_range_randomize_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick);
void bt_pattern_serialize_columns(const BtPattern * const self, const gulong start_tick, const gulong end_tick, GString *data);

#endif /* BT_PATTERN_H */
