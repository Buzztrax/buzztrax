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

#ifndef BT_LIST_ITEM_LONG_H
#define BT_LIST_ITEM_LONG_H

#include <glib-object.h>

/**
 * BtListItemLong:
 *
 * A simple GObject wrapper around a glong value. Intended to ease the transition to using GTK4 lists that currently
 * use opaque simple structs, as GTK4 expects all list elements to be GObject-derived.
 */
G_DECLARE_FINAL_TYPE(BtListItemLong, bt_list_item_long, BT, LIST_ITEM_LONG, GObject);

#define BT_TYPE_LIST_ITEM_LONG (bt_list_item_long_get_type())

// Public access is allowed for speed. Use appropriate caution!
struct _BtListItemLong
{
  GObject parent;
  glong value;
};


BtListItemLong* bt_list_item_long_new (glong value);

glong bt_list_item_long_get (BtListItemLong* self);

#endif
