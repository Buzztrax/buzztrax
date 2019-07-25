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
 * SECTION:btwavelistmodel
 * @short_description: data model class for widgets showing waves of a wavetable
 *
 * A generic model representing the waves of a wavetable, suitable for
 * combo-boxes and treeview widgets.
 */

#define BT_EDIT
#define BT_WAVE_LIST_MODEL_C

#include "bt-edit.h"

//-- defines
#define N_COLUMNS __BT_WAVE_LIST_MODEL_N_COLUMNS
#define N_ROWS 200

//-- globals

//-- property ids

//-- structs

struct _BtWaveListModelPrivate
{
  gint stamp;
  BtWavetable *wavetable;

  GType param_types[N_COLUMNS];
  GPtrArray *seq;
};

//-- the class

static void bt_wave_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtWaveListModel, bt_wave_list_model,
    G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
        bt_wave_list_model_tree_model_init));

//-- helper

static void
bt_wave_list_model_add (BtWaveListModel * model, BtWave * wave)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  gulong pos;

  GST_INFO ("%p: update wave-model", model);
  g_object_get (wave, "index", &pos, NULL);
  pos--;

  // insert new entry
  iter.stamp = model->priv->stamp;
  iter.user_data = GINT_TO_POINTER (pos);
  g_ptr_array_index (model->priv->seq, pos) = wave;

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, pos);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
  GST_DEBUG ("%p: notified wave %p at position %lu", model, wave, pos);
}

static void
bt_wave_list_model_rem (BtWaveListModel * model, BtWave * wave)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  gulong pos;

  GST_INFO ("%p: rem wave from model", model);
  g_object_get (wave, "index", &pos, NULL);
  pos--;

  // release entry
  iter.stamp = model->priv->stamp;
  iter.user_data = GINT_TO_POINTER (pos);
  g_ptr_array_index (model->priv->seq, pos) = NULL;

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, pos);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
  GST_DEBUG ("%p: deleted wave %p at position %lu", model, wave, pos);
}

//-- signal handlers

static void
on_wave_added (BtWavetable * wavetable, BtWave * wave, gpointer user_data)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (user_data);

  bt_wave_list_model_add (model, wave);
}

static void
on_wave_removed (BtWavetable * wavetable, BtWave * wave, gpointer user_data)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (user_data);

  bt_wave_list_model_rem (model, wave);
}

//-- constructor methods

/**
 * bt_wave_list_model_new:
 * @wavetable: the wavetable
 *
 * Creates a list model of waves for the @wavetable. The model is automatically
 * updated when waves are added, removed or changed.
 *
 * Returns: the wave-list model.
 */

BtWaveListModel *
bt_wave_list_model_new (BtWavetable * wavetable)
{
  BtWaveListModel *self;
  BtWave *wave;
  GList *node, *list;

  self = g_object_new (BT_TYPE_WAVE_LIST_MODEL, NULL);

  self->priv->wavetable = wavetable;
  g_object_add_weak_pointer ((GObject *) wavetable,
      (gpointer *) & self->priv->wavetable);

  self->priv->param_types[BT_WAVE_LIST_MODEL_INDEX] = G_TYPE_ULONG;
  self->priv->param_types[BT_WAVE_LIST_MODEL_HEX_ID] = G_TYPE_STRING;
  self->priv->param_types[BT_WAVE_LIST_MODEL_NAME] = G_TYPE_STRING;
  self->priv->param_types[BT_WAVE_LIST_MODEL_HAS_WAVE] = G_TYPE_BOOLEAN;

  // get wave list from wavetable
  g_object_get ((gpointer) wavetable, "waves", &list, NULL);
  // add waves
  for (node = list; node; node = g_list_next (node)) {
    wave = BT_WAVE (node->data);
    bt_wave_list_model_add (self, wave);
  }
  g_list_free (list);

  g_signal_connect_object (wavetable, "wave-added", G_CALLBACK (on_wave_added),
      (gpointer) self, 0);
  g_signal_connect_object (wavetable, "wave-removed",
      G_CALLBACK (on_wave_removed), (gpointer) self, 0);

  return self;
}

//-- methods

/**
 * bt_wave_list_model_get_object:
 * @model: the model
 * @iter: the iter
 *
 * Lookup a wave.
 *
 * Returns: the #BtWave for the iter. Unref when done.
 */
BtWave *
bt_wave_list_model_get_object (BtWaveListModel * model, GtkTreeIter * iter)
{
  BtWave *wave = g_ptr_array_index (model->priv->seq,
      GPOINTER_TO_INT (iter->user_data));
  return (wave ? g_object_ref (wave) : NULL);
}

//-- tree model interface

static GtkTreeModelFlags
bt_wave_list_model_tree_model_get_flags (GtkTreeModel * tree_model)
{
  return (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint
bt_wave_list_model_tree_model_get_n_columns (GtkTreeModel * tree_model)
{
  return N_COLUMNS;
}

static GType
bt_wave_list_model_tree_model_get_column_type (GtkTreeModel * tree_model,
    gint index)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);

  g_return_val_if_fail (index < N_COLUMNS, G_TYPE_INVALID);

  return model->priv->param_types[index];
}

static gboolean
bt_wave_list_model_tree_model_get_iter (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreePath * path)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);
  gint i = gtk_tree_path_get_indices (path)[0];

  if (i >= N_ROWS)
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = GINT_TO_POINTER (i);

  return TRUE;
}

static GtkTreePath *
bt_wave_list_model_tree_model_get_path (GtkTreeModel *
    tree_model, GtkTreeIter * iter)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);
  GtkTreePath *path;

  g_return_val_if_fail (iter->stamp == model->priv->stamp, NULL);

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, GPOINTER_TO_UINT (iter->user_data));

  return path;
}

static void
bt_wave_list_model_tree_model_get_value (GtkTreeModel * tree_model,
    GtkTreeIter * iter, gint column, GValue * value)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);
  BtWave *wave = NULL;
  gint pos;

  g_return_if_fail (column < N_COLUMNS);

  g_value_init (value, model->priv->param_types[column]);

  pos = GPOINTER_TO_UINT (iter->user_data);
  if (pos < N_ROWS)
    wave = g_ptr_array_index (model->priv->seq, pos);

  GST_DEBUG ("get data for wave %p at position %d, column %d",
      wave, pos, column);

  switch (column) {
    case BT_WAVE_LIST_MODEL_INDEX:
      g_value_set_ulong (value, pos + 1);
      break;
    case BT_WAVE_LIST_MODEL_HEX_ID:{
      gchar hstr[3];

      snprintf (hstr, sizeof(hstr), "%02x", pos + 1);
      g_value_set_string (value, hstr);
      break;
    }
    case BT_WAVE_LIST_MODEL_NAME:
      if (wave) {
        g_object_get_property ((GObject *) wave, "name", value);
      }
      break;
    case BT_WAVE_LIST_MODEL_HAS_WAVE:
      g_value_set_boolean (value, (wave != NULL));
      break;
  }
}

static gboolean
bt_wave_list_model_tree_model_iter_next (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);
  gint pos;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

  pos = GPOINTER_TO_UINT (iter->user_data);
  if (pos == N_ROWS) {
    iter->stamp = 0;
    return FALSE;
  } else {
    iter->user_data = GINT_TO_POINTER (pos + 1);
    return TRUE;
  }
}

static gboolean
bt_wave_list_model_tree_model_iter_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);

  /* this is a list, nodes have no children */
  if (!parent) {
    iter->stamp = model->priv->stamp;
    iter->user_data = GINT_TO_POINTER (0);
    return TRUE;
  }
  iter->stamp = 0;
  return FALSE;
}

static gboolean
bt_wave_list_model_tree_model_iter_has_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  return FALSE;
}

static gint
bt_wave_list_model_tree_model_iter_n_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);

  if (iter == NULL)
    return N_ROWS;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, -1);
  return 0;
}

static gboolean
bt_wave_list_model_tree_model_iter_nth_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
  BtWaveListModel *model = BT_WAVE_LIST_MODEL (tree_model);

  iter->stamp = 0;

  if (parent)
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = GINT_TO_POINTER (n);

  return TRUE;
}

static gboolean
bt_wave_list_model_tree_model_iter_parent (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * child)
{
  iter->stamp = 0;
  return FALSE;
}

static void
bt_wave_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GtkTreeModelIface *const iface = g_iface;

  iface->get_flags = bt_wave_list_model_tree_model_get_flags;
  iface->get_n_columns = bt_wave_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_wave_list_model_tree_model_get_column_type;
  iface->get_iter = bt_wave_list_model_tree_model_get_iter;
  iface->get_path = bt_wave_list_model_tree_model_get_path;
  iface->get_value = bt_wave_list_model_tree_model_get_value;
  iface->iter_next = bt_wave_list_model_tree_model_iter_next;
  iface->iter_children = bt_wave_list_model_tree_model_iter_children;
  iface->iter_has_child = bt_wave_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_wave_list_model_tree_model_iter_n_children;
  iface->iter_nth_child = bt_wave_list_model_tree_model_iter_nth_child;
  iface->iter_parent = bt_wave_list_model_tree_model_iter_parent;
}

//-- class internals

static void
bt_wave_list_model_finalize (GObject * object)
{
  BtWaveListModel *self = BT_WAVE_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->wavetable) {
    g_object_remove_weak_pointer ((GObject *) self->priv->wavetable,
        (gpointer *) & self->priv->wavetable);
  }
  g_ptr_array_free (self->priv->seq, TRUE);

  G_OBJECT_CLASS (bt_wave_list_model_parent_class)->finalize (object);
}

static void
bt_wave_list_model_init (BtWaveListModel * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_WAVE_LIST_MODEL,
      BtWaveListModelPrivate);

  self->priv->seq = g_ptr_array_sized_new (N_ROWS);
  g_ptr_array_set_size (self->priv->seq, N_ROWS);
  // random int to check whether an iter belongs to our model
  self->priv->stamp = g_random_int ();
}

static void
bt_wave_list_model_class_init (BtWaveListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtWaveListModelPrivate));

  gobject_class->finalize = bt_wave_list_model_finalize;
}
