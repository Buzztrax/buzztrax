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

#include "list-item-pointer.h"

G_DEFINE_TYPE(BtListItemPointer, bt_list_item_pointer, G_TYPE_OBJECT)

static void
bt_list_item_pointer_set_property(GObject * const obj, const guint property_id, const GValue * const value,
                                  GParamSpec * const pspec) {
  BtListItemPointer* self = BT_LIST_ITEM_POINTER (obj);
  if (property_id == 1) {
    self->value = g_value_get_pointer (value);
  } else {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
  }
}

static void
bt_list_item_pointer_get_property(GObject * const obj, const guint property_id, GValue * const value,
                                  GParamSpec * const pspec) {
  BtListItemPointer* self = BT_LIST_ITEM_POINTER (obj);
  if (property_id == 1) {
    g_value_set_pointer (value, self->value);
  } else {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
  }
}

static void
bt_list_item_pointer_class_init (BtListItemPointerClass* klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_list_item_pointer_set_property;
  gobject_class->get_property = bt_list_item_pointer_get_property;

  g_object_class_install_property (gobject_class, 1,
      g_param_spec_pointer ("value", "", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
}

static void bt_list_item_pointer_init (BtListItemPointer* self)
{
}

gpointer *bt_list_item_pointer_get (BtListItemPointer* self)
{
  return self->value;
}

BtListItemPointer *bt_list_item_pointer_new (gpointer value) {
  return g_object_new (BT_TYPE_LIST_ITEM_POINTER, "value", value, NULL);
}
