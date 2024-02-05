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

#ifndef BT_MACHINE_PREFERENCES_DIALOG_H
#define BT_MACHINE_PREFERENCES_DIALOG_H

#include <glib.h>
#include <glib-object.h>

G_DECLARE_FINAL_TYPE(BtMachinePreferencesDialog, bt_machine_preferences_dialog, BT, MACHINE_PREFERENCES_DIALOG,
     AdwWindow);

#define BT_TYPE_MACHINE_PREFERENCES_DIALOG bt_machine_preferences_dialog_get_type()

/**
 * BtMachinePreferencesDialog:
 *
 * the root window for the editor application
 */
struct _BtMachinePreferencesDialogClass {
  GtkWindowClass parent;
};

GType bt_machine_preferences_dialog_get_type(void) G_GNUC_CONST;

BtMachinePreferencesDialog *bt_machine_preferences_dialog_new(const BtMachine *machine);

#endif // BT_MACHINE_PREFERENCES_DIALOG_H
