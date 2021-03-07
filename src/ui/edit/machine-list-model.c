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
 * SECTION:btmachinelistmodel
 * @short_description: data model class for widgets showing machines of a song
 *
 * A generic model representing the machines of a song, suitable for combo-boxes
 * and treeview widgets.
 */

#define BT_EDIT
#define BT_MACHINE_LIST_MODEL_C

#include "bt-edit.h"

//-- defines
#define N_COLUMNS __BT_MACHINE_LIST_MODEL_N_COLUMNS

//-- property ids

//-- structs

struct _BtMachineListModelPrivate
{
  gint stamp;
  BtSetup *setup;

  GType param_types[N_COLUMNS];
  GSequence *seq;
};

//-- the class

static void bt_machine_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMachineListModel, bt_machine_list_model,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE(BtMachineListModel)
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
        bt_machine_list_model_tree_model_init));

//-- helper

static void on_machine_id_changed (BtMachine * machine, GParamSpec * arg,
    gpointer user_data);

// we are comparing by type and name
static gint
model_item_cmp (gconstpointer a, gconstpointer b, gpointer data)
{
  BtMachine *ma = (BtMachine *) a;
  BtMachine *mb = (BtMachine *) b;
  gint ra = 0, rb = 0;

  if (a == b)
    return 0;

  if (BT_IS_SINK_MACHINE (ma))
    ra = 2;
  else if (BT_IS_PROCESSOR_MACHINE (ma))
    ra = 4;
  else if (BT_IS_SOURCE_MACHINE (ma))
    ra = 6;
  if (BT_IS_SINK_MACHINE (mb))
    rb = 2;
  else if (BT_IS_PROCESSOR_MACHINE (mb))
    rb = 4;
  else if (BT_IS_SOURCE_MACHINE (mb))
    rb = 6;

  GST_LOG ("comparing %s <-> %s: %d <-> %d", GST_OBJECT_NAME (ma),
      GST_OBJECT_NAME (mb), ra, rb);

  if (ra == rb) {
    gchar *ida, *idb;
    gint c;

    g_object_get (ma, "id", &ida, NULL);
    g_object_get (mb, "id", &idb, NULL);

    c = strcmp (ida, idb);
    if (c > 0)
      rb += 1;
    else if (c < 0)
      ra += 1;

    GST_LOG ("comparing %s <-> %s: %d: %d <-> %d", ida, idb, c, ra, rb);
    g_free (ida);
    g_free (idb);
  }

  return (rb - ra);
}


static void
bt_machine_list_model_add (BtMachineListModel * model, BtMachine * machine)
{
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint position;

  GST_INFO_OBJECT (machine, "add machine to model");

  // insert new entry
  iter.stamp = model->priv->stamp;
  iter.user_data =
      g_sequence_insert_sorted (seq, machine, model_item_cmp, NULL);
  position = g_sequence_iter_get_position (iter.user_data);

  g_signal_connect_object (machine, "notify::id",
      G_CALLBACK (on_machine_id_changed), (gpointer) model, 0);

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, position);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
}

static void
bt_machine_list_model_rem (BtMachineListModel * model, BtMachine * machine)
{
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GSequenceIter *iter;
  gint position;

  GST_INFO_OBJECT (machine, "removing machine from model");

  // remove entry
  iter = g_sequence_lookup (seq, machine, model_item_cmp, NULL);
  position = g_sequence_iter_get_position (iter);
  g_sequence_remove (iter);

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, position);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
  gtk_tree_path_free (path);

  GST_INFO_OBJECT (machine, "removed machine from model at pos %d ", position);
}

//-- signal handlers

static void
on_machine_id_changed (BtMachine * machine, GParamSpec * arg,
    gpointer user_data)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (user_data);
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint pos1, pos2 = -1, len;

  // find the item by machine (cannot use model_item_cmp, as id has changed)
  iter.stamp = model->priv->stamp;
  len = g_sequence_get_length (seq);
  for (pos1 = 0; pos1 < len; pos1++) {
    iter.user_data = g_sequence_get_iter_at_pos (seq, pos1);
    if (g_sequence_get (iter.user_data) == machine) {
      g_sequence_sort_changed (iter.user_data, model_item_cmp, NULL);
      pos2 = g_sequence_iter_get_position (iter.user_data);
      break;
    }
  }
  if (G_UNLIKELY (pos2 == -1))
    return;

  GST_DEBUG ("pos %d -> %d", pos1, pos2);

  // signal updates
  if (pos1 != pos2) {
    path = gtk_tree_path_new ();
    gtk_tree_path_append_index (path, pos1);
    gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
    gtk_tree_path_free (path);
    path = gtk_tree_path_new ();
    gtk_tree_path_append_index (path, pos2);
    gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
    gtk_tree_path_free (path);
  } else {
    path = gtk_tree_path_new ();
    gtk_tree_path_append_index (path, pos2);
    gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
    gtk_tree_path_free (path);
  }
}


static void
on_machine_added (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (user_data);

  bt_machine_list_model_add (model, machine);
}

static void
on_machine_removed (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (user_data);

  bt_machine_list_model_rem (model, machine);
}

//-- constructor methods

/**
 * bt_machine_list_model_new:
 * @setup: the setup
 *
 * Creates a list model of machines for the @setup. The model is automatically
 * updated when machines are added, removed or changed.
 *
 * Returns: the machine model.
 */
BtMachineListModel *
bt_machine_list_model_new (BtSetup * setup)
{
  BtMachineListModel *self;
  BtMachine *machine;
  GList *node, *list;

  self = g_object_new (BT_TYPE_MACHINE_LIST_MODEL, NULL);

  self->priv->setup = setup;
  g_object_add_weak_pointer ((GObject *) setup,
      (gpointer *) & self->priv->setup);

  self->priv->param_types[BT_MACHINE_LIST_MODEL_ICON] = GDK_TYPE_PIXBUF;
  self->priv->param_types[BT_MACHINE_LIST_MODEL_LABEL] = G_TYPE_STRING;

  // get machine list from setup
  g_object_get ((gpointer) setup, "machines", &list, NULL);
  // add machines
  for (node = list; node; node = g_list_next (node)) {
    // we take no extra ref on the machines here
    machine = BT_MACHINE (node->data);
    bt_machine_list_model_add (self, machine);
  }
  g_list_free (list);

  g_signal_connect_object (setup, "machine-added",
      G_CALLBACK (on_machine_added), (gpointer) self, 0);
  g_signal_connect_object (setup, "machine-removed",
      G_CALLBACK (on_machine_removed), (gpointer) self, G_CONNECT_AFTER);

  return self;
}

//-- methods

/**
 * bt_machine_list_model_get_object:
 * @model: the model
 * @iter: the iter
 *
 * The the #BtMachine for the iter.
 */
BtMachine *
bt_machine_list_model_get_object (BtMachineListModel * model,
    GtkTreeIter * iter)
{
  return g_sequence_get (iter->user_data);
}

//-- tree model interface

static GtkTreeModelFlags
bt_machine_list_model_tree_model_get_flags (GtkTreeModel * tree_model)
{
  return (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint
bt_machine_list_model_tree_model_get_n_columns (GtkTreeModel * tree_model)
{
  return N_COLUMNS;
}

static GType
bt_machine_list_model_tree_model_get_column_type (GtkTreeModel * tree_model,
    gint index)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);

  g_return_val_if_fail (index < N_COLUMNS, G_TYPE_INVALID);

  return model->priv->param_types[index];
}

static gboolean
bt_machine_list_model_tree_model_get_iter (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreePath * path)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);
  GSequence *seq = model->priv->seq;
  gint i = gtk_tree_path_get_indices (path)[0];

  if (i >= g_sequence_get_length (seq))
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = g_sequence_get_iter_at_pos (seq, i);

  return TRUE;
}

static GtkTreePath *
bt_machine_list_model_tree_model_get_path (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);
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
bt_machine_list_model_tree_model_get_value (GtkTreeModel * tree_model,
    GtkTreeIter * iter, gint column, GValue * value)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);
  BtMachine *machine;

  g_return_if_fail (column < N_COLUMNS);

  g_value_init (value, model->priv->param_types[column]);
  if ((machine = g_sequence_get (iter->user_data))) {
    switch (column) {
      case BT_MACHINE_LIST_MODEL_ICON:
        g_value_take_object (value,
            bt_ui_resources_get_icon_pixbuf_by_machine (machine));
        break;
      case BT_MACHINE_LIST_MODEL_LABEL:
        g_object_get_property ((GObject *) machine, "id", value);
        break;
    }
  }
}

static gboolean
bt_machine_list_model_tree_model_iter_next (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);
  gboolean res;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

  iter->user_data = g_sequence_iter_next (iter->user_data);
  if ((res = g_sequence_iter_is_end (iter->user_data)))
    iter->stamp = 0;

  return !res;
}

static gboolean
bt_machine_list_model_tree_model_iter_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);

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
bt_machine_list_model_tree_model_iter_has_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  return FALSE;
}

static gint
bt_machine_list_model_tree_model_iter_n_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);

  if (iter == NULL)
    return g_sequence_get_length (model->priv->seq);

  g_return_val_if_fail (model->priv->stamp == iter->stamp, -1);
  return 0;
}

static gboolean
bt_machine_list_model_tree_model_iter_nth_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (tree_model);
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
bt_machine_list_model_tree_model_iter_parent (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * child)
{
  iter->stamp = 0;
  return FALSE;
}

static void
bt_machine_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GtkTreeModelIface *const iface = g_iface;

  iface->get_flags = bt_machine_list_model_tree_model_get_flags;
  iface->get_n_columns = bt_machine_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_machine_list_model_tree_model_get_column_type;
  iface->get_iter = bt_machine_list_model_tree_model_get_iter;
  iface->get_path = bt_machine_list_model_tree_model_get_path;
  iface->get_value = bt_machine_list_model_tree_model_get_value;
  iface->iter_next = bt_machine_list_model_tree_model_iter_next;
  iface->iter_children = bt_machine_list_model_tree_model_iter_children;
  iface->iter_has_child = bt_machine_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_machine_list_model_tree_model_iter_n_children;
  iface->iter_nth_child = bt_machine_list_model_tree_model_iter_nth_child;
  iface->iter_parent = bt_machine_list_model_tree_model_iter_parent;
}

//-- class internals

static void
bt_machine_list_model_finalize (GObject * object)
{
  BtMachineListModel *self = BT_MACHINE_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->setup) {
    g_object_remove_weak_pointer ((GObject *) self->priv->setup,
        (gpointer *) & self->priv->setup);
  }

  g_sequence_free (self->priv->seq);

  G_OBJECT_CLASS (bt_machine_list_model_parent_class)->finalize (object);
}

static void
bt_machine_list_model_init (BtMachineListModel * self)
{
  self->priv = bt_machine_list_model_get_instance_private(self);

  self->priv->seq = g_sequence_new (NULL);
  // random int to check whether an iter belongs to our model
  self->priv->stamp = g_random_int ();
}

static void
bt_machine_list_model_class_init (BtMachineListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bt_machine_list_model_finalize;
}
