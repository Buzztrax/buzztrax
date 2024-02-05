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

#ifndef BT_PATTERN_LIST_MODEL_H
#define BT_PATTERN_LIST_MODEL_H

#include <glib.h>
#include <glib-object.h>

/**
 * BtPatternListModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
G_DECLARE_FINAL_TYPE(BtPatternListModel, bt_pattern_list_model, BT, PATTERN_LIST_MODEL, GObject);

#define BT_TYPE_PATTERN_LIST_MODEL            (bt_pattern_list_model_get_type())

BtPatternListModel *bt_pattern_list_model_new(BtMachine *machine,BtSequence *sequence,gboolean skip_internal);
BtCmdPattern *bt_pattern_list_model_get_object(BtPatternListModel *model, guint position);
BtCmdPattern *bt_pattern_list_model_get_pattern_by_key(BtPatternListModel *model, gchar key);

#endif // BT_PATTERN_LIST_MODEL_H
