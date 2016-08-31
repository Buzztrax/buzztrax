/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_VALUE_GROUP_H
#define BT_VALUE_GROUP_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_VALUE_GROUP            (bt_value_group_get_type ())
#define BT_VALUE_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_VALUE_GROUP, BtValueGroup))
#define BT_VALUE_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_VALUE_GROUP, BtValueGroupClass))
#define BT_IS_VALUE_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_VALUE_GROUP))
#define BT_IS_VALUE_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_VALUE_GROUP))
#define BT_VALUE_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_VALUE_GROUP, BtValueGroupClass))

/* type macros */

typedef struct _BtValueGroup BtValueGroup;
typedef struct _BtValueGroupClass BtValueGroupClass;
typedef struct _BtValueGroupPrivate BtValueGroupPrivate;

/**
 * BtValueGroup:
 *
 * A group of parameters, such as used in machines or wires.
 */
struct _BtValueGroup {
  const GObject parent;

  /*< private >*/
  BtValueGroupPrivate *priv;
};

struct _BtValueGroupClass {
  const GObjectClass parent;

};

/**
 * BtValueGroupOp:
 * @BT_VALUE_GROUP_OP_CLEAR: clear values
 * @BT_VALUE_GROUP_OP_BLEND: linear blend from start to end
 * @BT_VALUE_GROUP_OP_FLIP: swap values from start to end
 * @BT_VALUE_GROUP_OP_RANDOMIZE: fill with random values
 * @BT_VALUE_GROUP_OP_RANGE_RANDOMIZE: fill with random values using the first
 *  and last value as bounds for the random values
 * @BT_VALUE_GROUP_OP_TRANSPOSE_FINE_UP: transposes values in single steps up
 * @BT_VALUE_GROUP_OP_TRANSPOSE_FINE_DOWN: transposes values in single steps
 *  down
 * @BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_UP: transposes values in large steps up
 * @BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_DOWN: transposes values in large steps
 *  down
 *
 * Operations that can be applied to pattern values.
 */
typedef enum {
  BT_VALUE_GROUP_OP_CLEAR=0,
  BT_VALUE_GROUP_OP_BLEND,
  BT_VALUE_GROUP_OP_FLIP,
  BT_VALUE_GROUP_OP_RANDOMIZE,
  BT_VALUE_GROUP_OP_RANGE_RANDOMIZE,
  BT_VALUE_GROUP_OP_TRANSPOSE_FINE_UP,
  BT_VALUE_GROUP_OP_TRANSPOSE_FINE_DOWN,
  BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_UP,
  BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_DOWN,
  // TODO: mirror
  // TODO: mirror_range
  /*< private >*/
  BT_VALUE_GROUP_OP_COUNT
} BtValueGroupOp;


#include "parameter-group.h"

BtValueGroup *bt_value_group_new(const BtParameterGroup * const param_group, const gulong length);

BtValueGroup *bt_value_group_copy(const BtValueGroup * const self);

GValue *bt_value_group_get_event_data(const BtValueGroup * const self, const gulong tick, const gulong param);

gboolean bt_value_group_set_event(const BtValueGroup * const self, const gulong tick, const gulong param, const gchar * const value);
gchar *bt_value_group_get_event(const BtValueGroup * const self, const gulong tick, const gulong param);
gboolean bt_value_group_test_event(const BtValueGroup * const self, const gulong tick, const gulong param);
gboolean bt_value_group_test_tick(const BtValueGroup * const self, const gulong tick);
// FIXME(ensonic): add _test_param()?

void bt_value_group_insert_row(const BtValueGroup * const self, const gulong tick, const gulong param);
void bt_value_group_insert_full_row(const BtValueGroup * const self, const gulong tick);
void bt_value_group_delete_row(const BtValueGroup * const self, const gulong tick, const gulong param);
void bt_value_group_delete_full_row(const BtValueGroup * const self, const gulong tick);

void bt_value_group_transform_colum(const BtValueGroup * const self, BtValueGroupOp op, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_transform_colums(const BtValueGroup * const self, BtValueGroupOp op, const gulong start_tick, const gulong end_tick);

void bt_value_group_serialize_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data);
void bt_value_group_serialize_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, GString *data);
gboolean bt_value_group_deserialize_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param, const gchar *data);

GType bt_value_group_get_type(void) G_GNUC_CONST;

#endif // BT_VALUE_GROUP_H
