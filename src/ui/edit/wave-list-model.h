/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_WAVE_LIST_MODEL_H
#define BT_WAVE_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVE_LIST_MODEL            (bt_wave_list_model_get_type())
#define BT_WAVE_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVE_LIST_MODEL, BtWaveListModel))
#define BT_WAVE_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVE_LIST_MODEL, BtWaveListModelClass))
#define BT_IS_WAVE_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVE_LIST_MODEL))
#define BT_IS_WAVE_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVE_LIST_MODEL))
#define BT_WAVE_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVE_LIST_MODEL, BtWaveListModelClass))

/* type macros */

typedef struct _BtWaveListModel BtWaveListModel;
typedef struct _BtWaveListModelClass BtWaveListModelClass;
typedef struct _BtWaveListModelPrivate BtWaveListModelPrivate;

/**
 * BtWaveListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtWaveListModel {
  GObject parent;

  /*< private >*/
  BtWaveListModelPrivate *priv;
};

struct _BtWaveListModelClass {
  GObjectClass parent;
};

enum {
  BT_WAVE_LIST_MODEL_INDEX = 0,
  BT_WAVE_LIST_MODEL_HEX_ID,
  BT_WAVE_LIST_MODEL_NAME,
  BT_WAVE_LIST_MODEL_HAS_WAVE,
  __BT_WAVE_LIST_MODEL_N_COLUMNS
};

GType bt_wave_list_model_get_type(void) G_GNUC_CONST;

BtWaveListModel *bt_wave_list_model_new(BtWavetable *wavetable);
BtWave *bt_wave_list_model_get_object(BtWaveListModel *model,GtkTreeIter *iter);

#endif // BT_WAVE_LIST_MODEL_H
