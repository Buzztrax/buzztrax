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

#ifndef BT_MAIN_PAGE_MACHINES_METHODS_H
#define BT_MAIN_PAGE_MACHINES_METHODS_H

#include "main-pages.h"
#include "main-page-machines.h"
#include "edit-application.h"
#include "machine-canvas-item.h"
#include "wire-canvas-item.h"

extern BtMainPageMachines *bt_main_page_machines_new(const BtMainPages *pages);

extern void machine_view_get_machine_position(GHashTable *properties, gdouble *pos_x,gdouble *pos_y);

extern gboolean bt_main_page_machines_wire_volume_popup(const BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos);
extern gboolean bt_main_page_machines_wire_panorama_popup(const BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos);


#endif // BT_MAIN_PAGE_MACHINES_METHDOS_H
