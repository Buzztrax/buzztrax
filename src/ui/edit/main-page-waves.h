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

#ifndef BT_MAIN_PAGE_WAVES_H
#define BT_MAIN_PAGE_WAVES_H

#include <glib.h>
#include <glib-object.h>

/**
 * BtMainPageWaves:
 *
 * the pattern page for the editor application
 */
G_DECLARE_FINAL_TYPE(BtMainPageWaves, bt_main_page_waves, BT, MAIN_PAGE_WAVES, GtkBox);

#define BT_TYPE_MAIN_PAGE_WAVES             (bt_main_page_waves_get_type ())

#include "main-pages.h"

BtMainPageWaves *bt_main_page_waves_new(const BtMainPages *pages);

#endif // BT_MAIN_PAGE_WAVES_H
