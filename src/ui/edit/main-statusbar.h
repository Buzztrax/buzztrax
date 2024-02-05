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

#ifndef BT_MAIN_STATUSBAR_H
#define BT_MAIN_STATUSBAR_H

#include <glib.h>
#include <glib-object.h>

/**
 * BtMainStatusbar:
 *
 * the root window for the editor application
 */
G_DECLARE_FINAL_TYPE(BtMainStatusbar, bt_main_statusbar, BT, MAIN_STATUSBAR, GtkBox);

#define BT_TYPE_MAIN_STATUSBAR            (bt_main_statusbar_get_type ())

/**
 * BT_MAIN_STATUSBAR_DEFAULT:
 *
 * Default text to display when idle.
 */
#define BT_MAIN_STATUSBAR_DEFAULT _("Ready to rock!")

BtMainStatusbar *bt_main_statusbar_new(void);

#endif // BT_MAIN_STATUSBAR_H
