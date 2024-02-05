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

#ifndef BT_MAIN_PAGE_SEQUENCE_H
#define BT_MAIN_PAGE_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

/**
 * BtMainPageSequence:
 *
 * the sequence page for the editor application
 */
G_DECLARE_FINAL_TYPE(BtMainPageSequence, bt_main_page_sequence, BT, MAIN_PAGE_SEQUENCE, GtkBox);

#define BT_TYPE_MAIN_PAGE_SEQUENCE            (bt_main_page_sequence_get_type ())

#include "main-pages.h"

void bt_main_page_sequence_delete_selection(BtMainPageSequence *self);
void bt_main_page_sequence_cut_selection(BtMainPageSequence *self);
void bt_main_page_sequence_copy_selection(BtMainPageSequence *self);
void bt_main_page_sequence_paste_selection(BtMainPageSequence *self);

#endif // BT_MAIN_PAGE_SEQUENCE_H
