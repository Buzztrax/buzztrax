/* $Id: ui-ressources-methods.h,v 1.3 2006-08-31 19:57:57 ensonic Exp $
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

#ifndef BT_UI_RESSOURCES_METHODS_H
#define BT_UI_RESSOURCES_METHODS_H

#include "ui-ressources.h"

extern BtUIRessources *bt_ui_ressources_new(void);

extern GdkPixbuf *bt_ui_ressources_get_pixbuf_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_ressources_get_image_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_ressources_get_image_by_machine_type(GType machine_type);

extern GdkColor *bt_ui_ressources_get_gdk_color(BtUIRessourcesColors color_type);
extern guint32 bt_ui_ressources_get_color_by_machine(const BtMachine *machine,BtUIRessourcesMachineColors color_type);

#endif // BT_UI_RESSOURCES_METHDOS_H
