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

void bt_value_group_clear_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_clear_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);

void bt_value_group_blend_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_blend_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_flip_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_flip_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_randomize_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_randomize_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_range_randomize_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_range_randomize_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_transpose_fine_up_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_transpose_fine_up_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_transpose_fine_down_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_transpose_fine_down_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_transpose_coarse_up_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_transpose_coarse_up_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);
void bt_value_group_transpose_coarse_down_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param);
void bt_value_group_transpose_coarse_down_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick);

void bt_value_group_serialize_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param, GString *data);
void bt_value_group_serialize_columns(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, GString *data);
gboolean bt_value_group_deserialize_column(const BtValueGroup * const self, const gulong start_tick, const gulong end_tick, const gulong param, const gchar *data);

GType bt_value_group_get_type(void) G_GNUC_CONST;

#endif // BT_VALUE_GROUP_H
