/* $Id$
 *
 * Buzztard
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

#ifndef BT_WIRE_PATTERN_H
#define BT_WIRE_PATTERN_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WIRE_PATTERN            (bt_wire_pattern_get_type ())
#define BT_WIRE_PATTERN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE_PATTERN, BtWirePattern))
#define BT_WIRE_PATTERN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE_PATTERN, BtWirePatternClass))
#define BT_IS_WIRE_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE_PATTERN))
#define BT_IS_WIRE_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE_PATTERN))
#define BT_WIRE_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE_PATTERN, BtWirePatternClass))

/* type macros */

typedef struct _BtWirePattern BtWirePattern;
typedef struct _BtWirePatternClass BtWirePatternClass;
typedef struct _BtWirePatternPrivate BtWirePatternPrivate;

/**
 * BtWirePattern:
 *
 * Class that holds a sequence of automation events for a #BtWire.
 */
struct _BtWirePattern {
  const GObject parent;
  
  /*< private >*/
  BtWirePatternPrivate *priv;
};

struct _BtWirePatternClass {
  const GObjectClass parent;
};

GType bt_wire_pattern_get_type(void) G_GNUC_CONST;

#include "song.h"
#include "wire.h"

BtWirePattern *bt_wire_pattern_new(const BtSong * const song, const BtWire * const wire, const BtPattern * const pattern);

BtWirePattern *bt_wire_pattern_copy(const BtWirePattern * const self, const BtPattern * const pattern);

GValue *bt_wire_pattern_get_event_data(const BtWirePattern * const self, const gulong tick, const gulong param);

gboolean bt_wire_pattern_set_event(const BtWirePattern * const self, const gulong tick, const gulong param, const gchar * const value);
gchar *bt_wire_pattern_get_event(const BtWirePattern * const self, const gulong tick, const gulong param);

gboolean bt_wire_pattern_test_event(const BtWirePattern * const self, const gulong tick, const gulong param);
gboolean bt_wire_pattern_tick_has_data(const BtWirePattern * const self, const gulong tick);

void bt_wire_pattern_insert_row(const BtWirePattern * const self, const gulong tick, const gulong param);
void bt_wire_pattern_insert_full_row(const BtWirePattern * const self, const gulong tick);
void bt_wire_pattern_delete_row(const BtWirePattern * const self, const gulong tick, const gulong param);
void bt_wire_pattern_delete_full_row(const BtWirePattern * const self, const gulong tick);

void bt_wire_pattern_delete_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_wire_pattern_delete_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);

void bt_wire_pattern_blend_column(const BtWirePattern * const self, const gulong start_tick,const gulong end_tick, const gulong param); 
void bt_wire_pattern_blend_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);
void bt_wire_pattern_flip_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_wire_pattern_flip_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);
void bt_wire_pattern_randomize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_wire_pattern_randomize_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);

void bt_wire_pattern_serialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data);
void bt_wire_pattern_serialize_columns(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, GString *data);
gboolean bt_wire_pattern_deserialize_column(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param, const gchar *data);

#endif /* BT_WIRE_PATTERN_H */
