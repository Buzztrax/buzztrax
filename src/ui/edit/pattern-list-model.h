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

#ifndef BT_PATTERN_LIST_MODEL_H
#define BT_PATTERN_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PATTERN_LIST_MODEL            (bt_pattern_list_model_get_type())
#define BT_PATTERN_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN_LIST_MODEL, BtPatternListModel))
#define BT_PATTERN_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN_LIST_MODEL, BtPatternListModelClass))
#define BT_IS_PATTERN_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN_LIST_MODEL))
#define BT_IS_PATTERN_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN_LIST_MODEL))
#define BT_PATTERN_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN_LIST_MODEL, BtPatternListModelClass))

/* type macros */

typedef struct _BtPatternListModel BtPatternListModel;
typedef struct _BtPatternListModelClass BtPatternListModelClass;
typedef struct _BtPatternListModelPrivate BtPatternListModelPrivate;

/**
 * BtPatternListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtPatternListModel {
  GObject parent;

  /*< private >*/
  BtPatternListModelPrivate *priv;
};

struct _BtPatternListModelClass {
  GObjectClass parent;
};

enum {
  BT_PATTERN_MODEL_LABEL=0,
  BT_PATTERN_MODEL_IS_USED,
  BT_PATTERN_MODEL_IS_UNUSED,
  BT_PATTERN_MODEL_SHORTCUT,
  __BT_PATTERN_MODEL_N_COLUMNS
};

GType bt_pattern_list_model_get_type(void) G_GNUC_CONST;

BtPatternListModel *bt_pattern_list_model_new(BtMachine *machine,BtSequence *sequence,gboolean skip_internal);
BtPattern *bt_pattern_list_model_get_object(BtPatternListModel *model,GtkTreeIter *iter);

#endif // BT_PATTERN_LIST_MODEL_H
