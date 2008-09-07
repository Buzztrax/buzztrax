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

#ifndef BT_UI_RESOURCES_METHODS_H
#define BT_UI_RESOURCES_METHODS_H

#include "ui-resources.h"
#include <libbuzztard-core/core.h>

extern BtUIResources *bt_ui_resources_new(void);

extern GdkPixbuf *bt_ui_resources_get_icon_pixbuf_by_machine(const BtMachine *machine);
extern GdkPixbuf *bt_ui_resources_get_machine_graphics_pixbuf_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_resources_get_icon_image_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_resources_get_icon_image_by_machine_type(GType machine_type);

extern GdkColor *bt_ui_resources_get_gdk_color(BtUIResourcesColors color_type);
extern guint32 bt_ui_resources_get_color_by_machine(const BtMachine *machine,BtUIResourcesMachineColors color_type);

extern GtkAccelGroup *bt_ui_resources_get_accel_group(void);

#endif // BT_UI_RESOURCES_METHDOS_H
