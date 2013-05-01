/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#ifndef BT_PARAMETER_GROUP_H
#define BT_PARAMETER_GROUP_H

#include <glib.h>
#include <glib-object.h>
#include "song.h"
#include "machine.h"

#define BT_TYPE_PARAMETER_GROUP            (bt_parameter_group_get_type ())
#define BT_PARAMETER_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PARAMETER_GROUP, BtParameterGroup))
#define BT_PARAMETER_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PARAMETER_GROUP, BtParameterGroupClass))
#define BT_IS_PARAMETER_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PARAMETER_GROUP))
#define BT_IS_PARAMETER_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PARAMETER_GROUP))
#define BT_PARAMETER_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PARAMETER_GROUP, BtParameterGroupClass))

/* type macros */

typedef struct _BtParameterGroup BtParameterGroup;
typedef struct _BtParameterGroupClass BtParameterGroupClass;
typedef struct _BtParameterGroupPrivate BtParameterGroupPrivate;

/**
 * BtParameterGroup:
 *
 * A group of parameters, such as used in machines or wires.
 */
struct _BtParameterGroup {
  const GObject parent;

  /*< private >*/
  BtParameterGroupPrivate *priv;
};

struct _BtParameterGroupClass {
  const GObjectClass parent;

};

BtParameterGroup *bt_parameter_group_new(gulong num_params, GObject ** parents, GParamSpec ** params, BtSong * song, const BtMachine * machine);

//-- parameter access

gboolean bt_parameter_group_is_param_trigger(const BtParameterGroup * const self, const gulong index);
gboolean bt_parameter_group_is_param_no_value(const BtParameterGroup * const self, const gulong index, GValue * const value);

glong bt_parameter_group_get_param_index(const BtParameterGroup * const self, const gchar * const name);

GParamSpec *bt_parameter_group_get_param_spec(const BtParameterGroup * const self, const gulong index);
GObject *bt_parameter_group_get_param_parent(const BtParameterGroup * const self, const gulong index);
void bt_parameter_group_get_param_details(const BtParameterGroup * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val);
GType bt_parameter_group_get_param_type(const BtParameterGroup * const self, const gulong index);
const gchar *bt_parameter_group_get_param_name(const BtParameterGroup * const self, const gulong index);
GValue *bt_parameter_group_get_param_no_value(const BtParameterGroup * const self, const gulong index);
glong bt_parameter_group_get_trigger_param_index(const BtParameterGroup * const self);
glong bt_parameter_group_get_wave_param_index(const BtParameterGroup * const self);

void bt_parameter_group_set_param_default(const BtParameterGroup * const self, const gulong index);

void bt_parameter_group_set_param_value(const BtParameterGroup * const self, const gulong index, GValue * const event);
gchar *bt_parameter_group_describe_param_value(const BtParameterGroup * const self, const gulong index, GValue * const event);

//-- group changes

void bt_parameter_group_set_param_defaults(const BtParameterGroup * const self);
void bt_parameter_group_randomize_values(const BtParameterGroup * const self);
void bt_parameter_group_reset_values(const BtParameterGroup * const self);

GType bt_parameter_group_get_type(void) G_GNUC_CONST;

#endif // BT_PARAMETER_GROUP_H
