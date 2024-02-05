/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_MACHINE_LIST_MODEL_H
#define BT_MACHINE_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>

/**
 * BtMachineListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
G_DECLARE_FINAL_TYPE(BtMachineListModel, bt_machine_list_model, BT, MACHINE_LIST_MODEL, GObject);

#define BT_TYPE_MACHINE_LIST_MODEL            (bt_machine_list_model_get_type())


enum {
  BT_MACHINE_LIST_MODEL_ICON=0,
  BT_MACHINE_LIST_MODEL_LABEL,
  __BT_MACHINE_LIST_MODEL_N_COLUMNS
};

GType bt_machine_list_model_get_type(void) G_GNUC_CONST;

BtMachineListModel *bt_machine_list_model_new(BtSetup *setup);
BtMachine *bt_machine_list_model_get_object(BtMachineListModel *model,GtkTreeIter *iter);

#endif // BT_MACHINE_LIST_MODEL_H
