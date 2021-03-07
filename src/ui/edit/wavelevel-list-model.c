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
/**
 * SECTION:btwavelevellistmodel
 * @short_description: data model class for widgets showing wavelevel of a wave
 *
 * A generic model representing the wavelevels of a wave, suitable for
 * combo-boxes and treeview widgets.
 */

#define BT_EDIT
#define BT_WAVELEVEL_LIST_MODEL_C

#include "bt-edit.h"
#include "gst/toneconversion.h"

//-- defines
#define N_COLUMNS __BT_WAVELEVEL_LIST_MODEL_N_COLUMNS

//-- globals

//-- property ids

//-- structs

struct _BtWavelevelListModelPrivate
{
  gint stamp;
  BtWave *wave;

  GType param_types[N_COLUMNS];
  GSequence *seq;
};

//-- the class

static void bt_wavelevel_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtWavelevelListModel, bt_wavelevel_list_model,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE(BtWavelevelListModel)
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
        bt_wavelevel_list_model_tree_model_init));

//-- signal handlers

static void
on_wavelevel_property_changed (BtWavelevel * wavelevel, GParamSpec * arg,
    gpointer user_data)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (user_data);
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint pos, len;

  // find the item by pattern (cannot use model_item_cmp, as id has changed)
  iter.stamp = model->priv->stamp;
  len = g_sequence_get_length (seq);
  for (pos = 0; pos < len; pos++) {
    iter.user_data = g_sequence_get_iter_at_pos (seq, pos);
    if (g_sequence_get (iter.user_data) == wavelevel) {
      break;
    }
  }
  if (G_UNLIKELY (pos == -1))
    return;

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, pos);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
}

//-- helper

static void
bt_wavelevel_list_model_add (BtWavelevelListModel * model,
    BtWavelevel * wavelevel)
{
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint position;

  GST_INFO ("add pattern to model");

  // insert new entry
  iter.stamp = model->priv->stamp;
  iter.user_data = g_sequence_append (seq, wavelevel);
  position = g_sequence_iter_get_position (iter.user_data);

  g_signal_connect_object (wavelevel, "notify::root-note",
      G_CALLBACK (on_wavelevel_property_changed), (gpointer) model, 0);
  g_signal_connect_object (wavelevel, "notify::rate",
      G_CALLBACK (on_wavelevel_property_changed), (gpointer) model, 0);
  g_signal_connect_object (wavelevel, "notify::loop-start",
      G_CALLBACK (on_wavelevel_property_changed), (gpointer) model, 0);
  g_signal_connect_object (wavelevel, "notify::loop-end",
      G_CALLBACK (on_wavelevel_property_changed), (gpointer) model, 0);

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, position);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
  GST_DEBUG ("inserted wavelevel %p at position %d", wavelevel, position);
}

//-- constructor methods

/**
 * bt_wavelevel_list_model_new:
 * @wave: the wave
 *
 * Creates a list model of wave-levels for the @wave. The model is automatically
 * updated when wave-levels are changed.
 *
 * Returns: the wave-level model.
 */

BtWavelevelListModel *
bt_wavelevel_list_model_new (BtWave * wave)
{
  BtWavelevelListModel *self;

  self = g_object_new (BT_TYPE_WAVELEVEL_LIST_MODEL, NULL);

  self->priv->param_types[BT_WAVELEVEL_LIST_MODEL_ROOT_NOTE] = G_TYPE_STRING;
  self->priv->param_types[BT_WAVELEVEL_LIST_MODEL_LENGTH] = G_TYPE_ULONG;
  self->priv->param_types[BT_WAVELEVEL_LIST_MODEL_RATE] = G_TYPE_ULONG;
  self->priv->param_types[BT_WAVELEVEL_LIST_MODEL_LOOP_START] = G_TYPE_ULONG;
  self->priv->param_types[BT_WAVELEVEL_LIST_MODEL_LOOP_END] = G_TYPE_ULONG;

  if (wave) {
    GList *node, *list;

    self->priv->wave = wave;
    g_object_add_weak_pointer ((GObject *) wave,
        (gpointer *) & self->priv->wave);

    // get wavelevel list from wave
    g_object_get ((gpointer) wave, "wavelevels", &list, NULL);
    for (node = list; node; node = g_list_next (node)) {
      bt_wavelevel_list_model_add (self, BT_WAVELEVEL (node->data));
    }
    g_list_free (list);
  }

  return self;
}

//-- methods

/**
 * bt_wavelevel_list_model_get_object:
 * @model: the model
 * @iter: the iter
 *
 * Lookup a wavelevel.
 *
 * Returns: the #BtWavelevel for the iter.
 */
BtWavelevel *
bt_wavelevel_list_model_get_object (BtWavelevelListModel * model,
    GtkTreeIter * iter)
{
  return g_object_ref (g_sequence_get (iter->user_data));
}

//-- tree model interface

static GtkTreeModelFlags
bt_wavelevel_list_model_tree_model_get_flags (GtkTreeModel * tree_model)
{
  return (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint
bt_wavelevel_list_model_tree_model_get_n_columns (GtkTreeModel * tree_model)
{
  return N_COLUMNS;
}

static GType
bt_wavelevel_list_model_tree_model_get_column_type (GtkTreeModel * tree_model,
    gint index)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);

  g_return_val_if_fail (index < N_COLUMNS, G_TYPE_INVALID);

  return model->priv->param_types[index];
}

static gboolean
bt_wavelevel_list_model_tree_model_get_iter (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreePath * path)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);
  GSequence *seq = model->priv->seq;
  gint i = gtk_tree_path_get_indices (path)[0];

  if (i >= g_sequence_get_length (seq))
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = g_sequence_get_iter_at_pos (seq, i);

  return TRUE;
}

static GtkTreePath *
bt_wavelevel_list_model_tree_model_get_path (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);
  GtkTreePath *path;

  g_return_val_if_fail (iter->stamp == model->priv->stamp, NULL);

  if (g_sequence_iter_is_end (iter->user_data))
    return NULL;

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path,
      g_sequence_iter_get_position (iter->user_data));

  return path;
}

static void
bt_wavelevel_list_model_tree_model_get_value (GtkTreeModel * tree_model,
    GtkTreeIter * iter, gint column, GValue * value)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);
  BtWavelevel *wavelevel;

  g_return_if_fail (column < N_COLUMNS);

  g_value_init (value, model->priv->param_types[column]);

  if (model->priv->wave && (wavelevel = g_sequence_get (iter->user_data))) {
    switch (column) {
      case BT_WAVELEVEL_LIST_MODEL_ROOT_NOTE:{
        GstBtNote root_note;
        g_object_get ((GObject *) wavelevel, "root-note", &root_note, NULL);
        g_value_set_string (value,
            gstbt_tone_conversion_note_number_2_string (root_note));
        break;
      }
      case BT_WAVELEVEL_LIST_MODEL_LENGTH:
        g_object_get_property ((GObject *) wavelevel, "length", value);
        break;
      case BT_WAVELEVEL_LIST_MODEL_RATE:
        g_object_get_property ((GObject *) wavelevel, "rate", value);
        break;
      case BT_WAVELEVEL_LIST_MODEL_LOOP_START:
        g_object_get_property ((GObject *) wavelevel, "loop-start", value);
        break;
      case BT_WAVELEVEL_LIST_MODEL_LOOP_END:
        g_object_get_property ((GObject *) wavelevel, "loop-end", value);
        break;
    }
  }
}

static gboolean
bt_wavelevel_list_model_tree_model_iter_next (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);
  gboolean res;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

  iter->user_data = g_sequence_iter_next (iter->user_data);
  if ((res = g_sequence_iter_is_end (iter->user_data)))
    iter->stamp = 0;

  return !res;
}

static gboolean
bt_wavelevel_list_model_tree_model_iter_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);

  /* this is a list, nodes have no children */
  if (!parent) {
    if (g_sequence_get_length (model->priv->seq) > 0) {
      iter->stamp = model->priv->stamp;
      iter->user_data = g_sequence_get_begin_iter (model->priv->seq);
      return TRUE;
    }
  }
  iter->stamp = 0;
  return FALSE;
}

static gboolean
bt_wavelevel_list_model_tree_model_iter_has_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  return FALSE;
}

static gint
bt_wavelevel_list_model_tree_model_iter_n_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);

  if (iter == NULL)
    return g_sequence_get_length (model->priv->seq);

  g_return_val_if_fail (model->priv->stamp == iter->stamp, -1);
  return 0;
}

static gboolean
bt_wavelevel_list_model_tree_model_iter_nth_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
  BtWavelevelListModel *model = BT_WAVELEVEL_LIST_MODEL (tree_model);
  GSequenceIter *child;

  iter->stamp = 0;

  if (parent)
    return FALSE;

  child = g_sequence_get_iter_at_pos (model->priv->seq, n);

  if (g_sequence_iter_is_end (child))
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = child;

  return TRUE;
}

static gboolean
bt_wavelevel_list_model_tree_model_iter_parent (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * child)
{
  iter->stamp = 0;
  return FALSE;
}

static void
bt_wavelevel_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GtkTreeModelIface *const iface = g_iface;

  iface->get_flags = bt_wavelevel_list_model_tree_model_get_flags;
  iface->get_n_columns = bt_wavelevel_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_wavelevel_list_model_tree_model_get_column_type;
  iface->get_iter = bt_wavelevel_list_model_tree_model_get_iter;
  iface->get_path = bt_wavelevel_list_model_tree_model_get_path;
  iface->get_value = bt_wavelevel_list_model_tree_model_get_value;
  iface->iter_next = bt_wavelevel_list_model_tree_model_iter_next;
  iface->iter_children = bt_wavelevel_list_model_tree_model_iter_children;
  iface->iter_has_child = bt_wavelevel_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_wavelevel_list_model_tree_model_iter_n_children;
  iface->iter_nth_child = bt_wavelevel_list_model_tree_model_iter_nth_child;
  iface->iter_parent = bt_wavelevel_list_model_tree_model_iter_parent;
}

//-- class internals

static void
bt_wavelevel_list_model_finalize (GObject * object)
{
  BtWavelevelListModel *self = BT_WAVELEVEL_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->wave) {
    g_object_remove_weak_pointer ((GObject *) self->priv->wave,
        (gpointer *) & self->priv->wave);
  }

  g_sequence_free (self->priv->seq);

  G_OBJECT_CLASS (bt_wavelevel_list_model_parent_class)->finalize (object);
}

static void
bt_wavelevel_list_model_init (BtWavelevelListModel * self)
{
  self->priv = bt_wavelevel_list_model_get_instance_private(self);

  self->priv->seq = g_sequence_new (NULL);
  // random int to check whether an iter belongs to our model
  self->priv->stamp = g_random_int ();
}

static void
bt_wavelevel_list_model_class_init (BtWavelevelListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bt_wavelevel_list_model_finalize;
}
