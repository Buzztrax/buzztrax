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

#ifndef BT_MAIN_PAGE_PATTERNS_H
#define BT_MAIN_PAGE_PATTERNS_H

#include <glib.h>
#include <glib-object.h>

G_DECLARE_FINAL_TYPE(BtMainPagePatterns, bt_main_page_patterns, BT, MAIN_PAGE_PATTERNS, GtkBox);

#define BT_TYPE_MAIN_PAGE_PATTERNS            (bt_main_page_patterns_get_type ())

/**
 * BtMainPagePatterns:
 *
 * the pattern page for the editor application
 */
struct _BtMainPagePatternsClass {
  GtkBoxClass parent;
  
};

GType bt_main_page_patterns_get_type(void) G_GNUC_CONST;

#include "main-pages.h"

BtMainPagePatterns *bt_main_page_patterns_new(const BtMainPages *pages);

void bt_main_page_patterns_show_pattern(BtMainPagePatterns *self,BtPattern *pattern);
void bt_main_page_patterns_show_machine(BtMainPagePatterns *self,BtMachine *machine);

void bt_main_page_patterns_delete_selection(BtMainPagePatterns *self);
void bt_main_page_patterns_cut_selection(BtMainPagePatterns *self);
void bt_main_page_patterns_copy_selection(BtMainPagePatterns *self);
void bt_main_page_patterns_paste_selection(BtMainPagePatterns *self);

#endif // BT_MAIN_PAGE_PATTERNS_H
