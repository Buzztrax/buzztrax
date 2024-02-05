/* Buzztrax
 * Copyright (C) 2023 David Beswick <dlbeswick@gmail.com>
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

#include "list-item-long.h"

G_DEFINE_TYPE(BtListItemLong, bt_list_item_long, G_TYPE_OBJECT)

static void
bt_list_item_long_set_property(GObject * const obj, const guint property_id, const GValue * const value,
                                  GParamSpec * const pspec) {
  BtListItemLong* self = BT_LIST_ITEM_LONG (obj);
  if (property_id == 1) {
    self->value = g_value_get_long (value);
  } else {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
  }
}

static void
bt_list_item_long_get_property(GObject * const obj, const guint property_id, GValue * const value,
                                  GParamSpec * const pspec) {
  BtListItemLong* self = BT_LIST_ITEM_LONG (obj);
  if (property_id == 1) {
    g_value_set_long (value, self->value);
  } else {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
  }
}

static void
bt_list_item_long_class_init (BtListItemLongClass* klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_list_item_long_set_property;
  gobject_class->get_property = bt_list_item_long_get_property;

  g_object_class_install_property (gobject_class, 1,
      g_param_spec_long ("value", "", "", G_MININT64, G_MAXINT64, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
}

static void bt_list_item_long_init (BtListItemLong* self)
{
}

glong bt_list_item_long_get (BtListItemLong* self)
{
  return self->value;
}

BtListItemLong* bt_list_item_long_new (glong value)
{
  return g_object_new (BT_TYPE_LIST_ITEM_LONG, "value", value, NULL);
}
