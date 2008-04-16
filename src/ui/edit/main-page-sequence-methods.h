/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_MAIN_PAGE_SEQUENCE_METHODS_H
#define BT_MAIN_PAGE_SEQUENCE_METHODS_H

#include "main-pages.h"
#include "main-page-sequence.h"
#include "edit-application.h"

extern BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app,const BtMainPages *pages);

extern BtMachine *bt_main_page_sequence_get_current_machine(const BtMainPageSequence *self);

extern void bt_main_page_sequence_delete_selection(const BtMainPageSequence *self);
extern void bt_main_page_sequence_cut_selection(const BtMainPageSequence *self);
extern void bt_main_page_sequence_copy_selection(const BtMainPageSequence *self);
extern void bt_main_page_sequence_paste_selection(const BtMainPageSequence *self);

#endif // BT_MAIN_PAGE_SEQUENCE_METHDOS_H
