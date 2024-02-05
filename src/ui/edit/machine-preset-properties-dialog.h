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

#ifndef BT_MACHINE_PRESET_PROPERTIES_DIALOG_H
#define BT_MACHINE_PRESET_PROPERTIES_DIALOG_H

#include <adwaita.h>

AdwMessageDialog* bt_machine_preset_properties_dialog_new(
    GstPreset* presets, gchar* existing_preset_name);

gchar* bt_machine_preset_properties_dialog_get_existing_preset_name(AdwMessageDialog* dlg);

#endif // BT_MACHINE_PRESET_PROPERTIES_DIALOG_H
