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

#ifndef BT_LIST_ITEM_POINTER_H
#define BT_LIST_ITEM_POINTER_H

#include <glib-object.h>

/**
 * BtListItemPointer:
 *
 * A simple GObject wrapper around a gpointer value. Intended to ease the
 * transition to using GTK4 lists that currently use opaque simple structs, as
 * GTK4 expects all list elements to be GObject-derived.
 */
G_DECLARE_FINAL_TYPE(BtListItemPointer, bt_list_item_pointer, BT, LIST_ITEM_POINTER, GObject);

#define BT_TYPE_LIST_ITEM_POINTER (bt_list_item_pointer_get_type())

// Public access is allowed for speed. Use appropriate caution!
struct _BtListItemPointer
{
  GObject parent;
  gpointer value;
};


gpointer *bt_list_item_pointer_get (BtListItemPointer* self);
BtListItemPointer *bt_list_item_pointer_new (gpointer value);

#endif
