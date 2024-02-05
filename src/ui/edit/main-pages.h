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

#ifndef BT_MAIN_PAGES_H
#define BT_MAIN_PAGES_H

#include <glib.h>
#include <glib-object.h>
#include <adwaita.h>

/**
 * BtMainPages:
 *
 * the root window for the editor application
 */
G_DECLARE_FINAL_TYPE (BtMainPages, bt_main_pages, BT, MAIN_PAGES, GtkWidget);

#define BT_TYPE_MAIN_PAGES            (bt_main_pages_get_type ())

enum {
  BT_MAIN_PAGES_MACHINES_PAGE=0,
  BT_MAIN_PAGES_PATTERNS_PAGE,
  BT_MAIN_PAGES_SEQUENCE_PAGE,
  BT_MAIN_PAGES_WAVES_PAGE,
  BT_MAIN_PAGES_INFO_PAGE,
  BT_MAIN_PAGES_COUNT
};

GType bt_main_pages_get_type(void) G_GNUC_CONST;

BtMainPages *bt_main_pages_new(void);

#endif // BT_MAIN_PAGES_H
