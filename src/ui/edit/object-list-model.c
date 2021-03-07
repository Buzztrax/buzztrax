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
/**
 * SECTION:btobjectlistmodel
 * @short_description: data model class for some widgets
 *
 * Allows to bind gobject properties to model columns. Does not copy the data
 * and thus keeps the widget always up-to-date.
 */
/* background:
 *
 * http://scentric.net/tutorial/sec-custom-models.html
 * http://pvanhoof.be/blog/index.php/2009/08/17/treemodel-zero-a-taste-of-life-as-it-should-be
 */

#define BT_EDIT
#define BT_OBJECT_LIST_MODEL_C

#include "bt-edit.h"

//-- property ids

//-- structs

struct _BtObjectListModelPrivate
{
  gint n_columns;
  gint stamp;
  GType object_type;

  GParamSpec **params;
  GSequence *seq;
};

//-- the class

static void bt_object_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtObjectListModel, bt_object_list_model, G_TYPE_OBJECT,
    G_ADD_PRIVATE(BtObjectListModel)
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
        bt_object_list_model_tree_model_init));

//-- helper

//-- constructor methods

/**
 * bt_object_list_model_new:
 * @n_columns: number of columns
 * @object_type: the #GType of the objects in the model
 * @...: property names for the columns
 *
 * Creates a list model mapping object properties to model columns.
 *
 * Returns: the model.
 */
BtObjectListModel *
bt_object_list_model_new (gint n_columns, GType object_type, ...)
{
  BtObjectListModel *self;
  GObjectClass *klass;
  va_list args;
  GParamSpec **params;
  gint i;

  self = g_object_new (BT_TYPE_OBJECT_LIST_MODEL, NULL);

  self->priv->n_columns = n_columns;
  self->priv->object_type = object_type;

  // allocate an array for the properties
  self->priv->params = g_new (GParamSpec *, n_columns);

  // look up the param-specs
  klass = g_type_class_ref (object_type);
  va_start (args, object_type);
  params = self->priv->params;
  for (i = 0; i < n_columns; i++) {
    params[i] = g_object_class_find_property (klass, va_arg (args, gchar *));
  }
  va_end (args);

  return self;
}

//-- methods

static void
_object_list_model_insert (BtObjectListModel * model, GObject * object,
    gint position)
{
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  GSequenceIter *seq_iter;

  seq_iter = g_sequence_get_iter_at_pos (seq, position);
  if (!g_sequence_iter_is_end (seq_iter) &&
      (g_sequence_get (seq_iter) == object))
    return;

  iter.stamp = model->priv->stamp;
  iter.user_data = g_sequence_insert_before (seq_iter, g_object_ref (object));

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, position);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
}

/**
 * bt_object_list_model_insert:
 * @model: the model
 * @object: the object to insert
 * @position: the position of the new row
 *
 * Insert a new row to the @model. The @object has to have the same type as
 * given to bt_object_list_model_new().
 */
void
bt_object_list_model_insert (BtObjectListModel * model, GObject * object,
    gint position)
{
  g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (object),
          model->priv->object_type));

  _object_list_model_insert (model, object, position);
}

/**
 * bt_object_list_model_append:
 * @model: the model
 * @object: the object to append
 *
 * Append a new row to the @model. The @object has to have the same type as
 * given to bt_object_list_model_new().
 */
void
bt_object_list_model_append (BtObjectListModel * model, GObject * object)
{
  GSequence *seq = model->priv->seq;
  gint position;

  g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (object),
          model->priv->object_type));

  position = g_sequence_get_length (seq);
  _object_list_model_insert (model, object, position);
}

#if 0
void
bt_object_list_model_prepend (BtObjectListModel * model, GObject * object)
{
  g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (object),
          model->priv->object_type));

  _object_list_model_insert (model, object, 0);
}

void
bt_object_list_model_clear (BtObjectListModel * model)
{
}

#endif

/**
 * bt_object_list_model_get_object:
 * @model: the model
 * @iter: the iter
 *
 * The the #GObject for the iter.
 */
GObject *
bt_object_list_model_get_object (BtObjectListModel * model, GtkTreeIter * iter)
{
  return g_sequence_get (iter->user_data);
}

//-- tree model interface

static GtkTreeModelFlags
bt_object_list_model_tree_model_get_flags (GtkTreeModel * tree_model)
{
  return (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint
bt_object_list_model_tree_model_get_n_columns (GtkTreeModel * tree_model)
{
  return BT_OBJECT_LIST_MODEL (tree_model)->priv->n_columns;
}

static GType
bt_object_list_model_tree_model_get_column_type (GtkTreeModel * tree_model,
    gint index)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);

  g_return_val_if_fail (index < model->priv->n_columns, G_TYPE_INVALID);

  return model->priv->params[index]->value_type;
}

static gboolean
bt_object_list_model_tree_model_get_iter (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreePath * path)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);
  GSequence *seq = model->priv->seq;
  gint i = gtk_tree_path_get_indices (path)[0];

  if (i >= g_sequence_get_length (seq))
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = g_sequence_get_iter_at_pos (seq, i);

  return TRUE;
}

static GtkTreePath *
bt_object_list_model_tree_model_get_path (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);
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
bt_object_list_model_tree_model_get_value (GtkTreeModel * tree_model,
    GtkTreeIter * iter, gint column, GValue * value)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);
  GParamSpec *pspec;
  GObject *obj;

  g_return_if_fail (column < model->priv->n_columns);

  pspec = model->priv->params[column];
  g_value_init (value, pspec->value_type);
  if ((obj = g_sequence_get (iter->user_data))) {
    g_object_get_property (obj, pspec->name, value);
  } else {
    g_param_value_set_default (pspec, value);
  }
}

static gboolean
bt_object_list_model_tree_model_iter_next (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);
  gboolean res;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

  iter->user_data = g_sequence_iter_next (iter->user_data);
  if ((res = g_sequence_iter_is_end (iter->user_data)))
    iter->stamp = 0;

  return !res;
}

static gboolean
bt_object_list_model_tree_model_iter_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);

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
bt_object_list_model_tree_model_iter_has_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  return FALSE;
}

static gint
bt_object_list_model_tree_model_iter_n_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);

  if (iter == NULL)
    return g_sequence_get_length (model->priv->seq);

  g_return_val_if_fail (model->priv->stamp == iter->stamp, -1);
  return 0;
}

static gboolean
bt_object_list_model_tree_model_iter_nth_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
  BtObjectListModel *model = BT_OBJECT_LIST_MODEL (tree_model);
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
bt_object_list_model_tree_model_iter_parent (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * child)
{
  iter->stamp = 0;
  return FALSE;
}

static void
bt_object_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GtkTreeModelIface *const iface = g_iface;

  iface->get_flags = bt_object_list_model_tree_model_get_flags;
  iface->get_n_columns = bt_object_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_object_list_model_tree_model_get_column_type;
  iface->get_iter = bt_object_list_model_tree_model_get_iter;
  iface->get_path = bt_object_list_model_tree_model_get_path;
  iface->get_value = bt_object_list_model_tree_model_get_value;
  iface->iter_next = bt_object_list_model_tree_model_iter_next;
  iface->iter_children = bt_object_list_model_tree_model_iter_children;
  iface->iter_has_child = bt_object_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_object_list_model_tree_model_iter_n_children;
  iface->iter_nth_child = bt_object_list_model_tree_model_iter_nth_child;
  iface->iter_parent = bt_object_list_model_tree_model_iter_parent;
}

//-- class internals

static void
bt_object_list_model_dispose (GObject * object)
{
  BtObjectListModel *self = BT_OBJECT_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_sequence_foreach (self->priv->seq, (GFunc) g_object_unref, NULL);

  G_OBJECT_CLASS (bt_object_list_model_parent_class)->dispose (object);
}

static void
bt_object_list_model_finalize (GObject * object)
{
  BtObjectListModel *self = BT_OBJECT_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_sequence_free (self->priv->seq);
  g_free (self->priv->params);

  G_OBJECT_CLASS (bt_object_list_model_parent_class)->finalize (object);
}

static void
bt_object_list_model_init (BtObjectListModel * self)
{
  self->priv = bt_object_list_model_get_instance_private(self);

  self->priv->seq = g_sequence_new (NULL);
  // random int to check whether an iter belongs to our model
  self->priv->stamp = g_random_int ();
}

static void
bt_object_list_model_class_init (BtObjectListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_object_list_model_dispose;
  gobject_class->finalize = bt_object_list_model_finalize;
}
