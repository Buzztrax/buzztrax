/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_WAVELEVEL_LIST_MODEL_H
#define BT_WAVELEVEL_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>


#define BT_TYPE_WAVELEVEL_LIST_MODEL            (bt_wavelevel_list_model_get_type())
#define BT_WAVELEVEL_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVELEVEL_LIST_MODEL, BtWavelevelListModel))
#define BT_WAVELEVEL_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVELEVEL_LIST_MODEL, BtWavelevelListModelClass))
#define BT_IS_WAVELEVEL_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVELEVEL_LIST_MODEL))
#define BT_IS_WAVELEVEL_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVELEVEL_LIST_MODEL))
#define BT_WAVELEVEL_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVELEVEL_LIST_MODEL, BtWavelevelListModelClass))

/* type macros */

typedef struct _BtWavelevelListModel BtWavelevelListModel;
typedef struct _BtWavelevelListModelClass BtWavelevelListModelClass;
typedef struct _BtWavelevelListModelPrivate BtWavelevelListModelPrivate;

/**
 * BtWavelevelListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtWavelevelListModel {
  GObject parent;

  /*< private >*/
  BtWavelevelListModelPrivate *priv;
};

struct _BtWavelevelListModelClass {
  GObjectClass parent;
};

enum {
  BT_WAVELEVEL_LIST_MODEL_ROOT_NOTE = 0,
  BT_WAVELEVEL_LIST_MODEL_LENGTH,
  BT_WAVELEVEL_LIST_MODEL_RATE,
  BT_WAVELEVEL_LIST_MODEL_LOOP_START,
  BT_WAVELEVEL_LIST_MODEL_LOOP_END,
  __BT_WAVELEVEL_LIST_MODEL_N_COLUMNS
};

GType bt_wavelevel_list_model_get_type(void) G_GNUC_CONST;

BtWavelevelListModel *bt_wavelevel_list_model_new(BtWave *wave);
BtWavelevel *bt_wavelevel_list_model_get_object(BtWavelevelListModel *model,GtkTreeIter *iter);

#endif // BT_WAVELEVEL_LIST_MODEL_H
