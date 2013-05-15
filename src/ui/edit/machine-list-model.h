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

#define BT_TYPE_MACHINE_LIST_MODEL            (bt_machine_list_model_get_type())
#define BT_MACHINE_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE_LIST_MODEL, BtMachineListModel))
#define BT_MACHINE_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE_LIST_MODEL, BtMachineListModelClass))
#define BT_IS_MACHINE_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE_LIST_MODEL))
#define BT_IS_MACHINE_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE_LIST_MODEL))
#define BT_MACHINE_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE_LIST_MODEL, BtMachineListModelClass))

/* type macros */

typedef struct _BtMachineListModel BtMachineListModel;
typedef struct _BtMachineListModelClass BtMachineListModelClass;
typedef struct _BtMachineListModelPrivate BtMachineListModelPrivate;

/**
 * BtMachineListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtMachineListModel {
  GObject parent;

  /*< private >*/
  BtMachineListModelPrivate *priv;
};

struct _BtMachineListModelClass {
  GObjectClass parent;
};

enum {
  BT_MACHINE_MODEL_ICON=0,
  BT_MACHINE_MODEL_LABEL,
  __BT_MACHINE_MODEL_N_COLUMNS
};

GType bt_machine_list_model_get_type(void) G_GNUC_CONST;

BtMachineListModel *bt_machine_list_model_new(BtSetup *setup);
BtMachine *bt_machine_list_model_get_object(BtMachineListModel *model,GtkTreeIter *iter);

#endif // BT_MACHINE_LIST_MODEL_H
