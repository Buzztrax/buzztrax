/* Buzztrax
 * Copyright (C) 2014 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_PRESET_LIST_MODEL_H
#define BT_PRESET_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PRESET_LIST_MODEL            (bt_preset_list_model_get_type())
#define BT_PRESET_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PRESET_LIST_MODEL, BtPresetListModel))
#define BT_PRESET_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PRESET_LIST_MODEL, BtPresetListModelClass))
#define BT_IS_PRESET_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PRESET_LIST_MODEL))
#define BT_IS_PRESET_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PRESET_LIST_MODEL))
#define BT_PRESET_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PRESET_LIST_MODEL, BtPresetListModelClass))

/* type macros */

typedef struct _BtPresetListModel BtPresetListModel;
typedef struct _BtPresetListModelClass BtPresetListModelClass;
typedef struct _BtPresetListModelPrivate BtPresetListModelPrivate;

/**
 * BtPresetListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtPresetListModel {
  GObject parent;

  /*< private >*/
  BtPresetListModelPrivate *priv;
};

struct _BtPresetListModelClass {
  GObjectClass parent;
};

enum {
  BT_PRESET_LIST_MODEL_LABEL=0,
  BT_PRESET_LIST_MODEL_COMMENT,
  __BT_PRESET_LIST_MODEL_N_COLUMNS
};

GType bt_preset_list_model_get_type(void) G_GNUC_CONST;

BtPresetListModel *bt_preset_list_model_new(GstElement * machine);
void bt_preset_list_model_add(BtPresetListModel * model, gchar *preset);
void bt_preset_list_model_remove(BtPresetListModel * model, gchar *preset);
void bt_preset_list_model_rename(BtPresetListModel * model, gchar *o_preset, gchar *n_preset);
gboolean bt_preset_list_model_find_iter(BtPresetListModel * model, gchar *preset, GtkTreeIter *iter);

#endif // BT_PRESET_LIST_MODEL_H
