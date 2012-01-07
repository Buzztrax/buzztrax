/* Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_OBJECT_LIST_MODEL_H
#define BT_OBJECT_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_OBJECT_LIST_MODEL            (bt_object_list_model_get_type())
#define BT_OBJECT_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_OBJECT_LIST_MODEL, BtObjectListModel))
#define BT_OBJECT_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_OBJECT_LIST_MODEL, BtObjectListModelClass))
#define BT_IS_OBJECT_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_OBJECT_LIST_MODEL))
#define BT_IS_OBJECT_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_OBJECT_LIST_MODEL))
#define BT_OBJECT_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_OBJECT_LIST_MODEL, BtObjectListModelClass))

/* type macros */

typedef struct _BtObjectListModel BtObjectListModel;
typedef struct _BtObjectListModelClass BtObjectListModelClass;
typedef struct _BtObjectListModelPrivate BtObjectListModelPrivate;

/**
 * BtObjectListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtObjectListModel {
  GObject parent;

  /*< private >*/
  BtObjectListModelPrivate *priv;
};

struct _BtObjectListModelClass {
  GObjectClass parent;
};

GType bt_object_list_model_get_type(void) G_GNUC_CONST;

BtObjectListModel *bt_object_list_model_new(gint n_columns,GType object_type,...);
void bt_object_list_model_append(BtObjectListModel *model,GObject *object);
GObject *bt_object_list_model_get_object(BtObjectListModel *model,GtkTreeIter *iter);

#endif // BT_OBJECT_LIST_MODEL_H
