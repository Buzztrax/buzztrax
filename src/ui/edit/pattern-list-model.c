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
 * SECTION:btpatternlistmodel
 * @short_description: data model class for widgets showing pattern of a song
 *
 * A generic model representing the patterns of a machine, suitable for
 * combo-boxes and treeview widgets.
 */

#define BT_EDIT
#define BT_PATTERN_LIST_MODEL_C

#include "bt-edit.h"

//-- defines
#define N_COLUMNS __BT_PATTERN_LIST_MODEL_N_COLUMNS

//-- globals

// keyboard shortcuts
// CLEAR       '.'
// MUTE        '-'
// BREAK       ','
// SOLO/BYPASS '_'
static const gchar sink_pattern_keys[] =
    "-,0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const gchar source_pattern_keys[] =
    "-,_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const gchar processor_pattern_keys[] =
    "-,_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

//-- property ids

//-- structs

struct _BtPatternListModelPrivate
{
  gint stamp;
  BtMachine *machine;
  BtSequence *sequence;
  gboolean skip_internal;

  /* shortcut table */
  const char *pattern_keys;

  GType param_types[N_COLUMNS];
  GSequence *seq;
};

//-- the class

static void bt_pattern_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtPatternListModel, bt_pattern_list_model,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE(BtPatternListModel)
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
        bt_pattern_list_model_tree_model_init));

//-- helper

static void on_pattern_name_changed (BtPattern * pattern, GParamSpec * arg,
    gpointer user_data);

// we are comparing by type and name
static gint
model_item_cmp (gconstpointer a, gconstpointer b, gpointer data)
{
  BtCmdPattern *pa = (BtCmdPattern *) a;
  BtCmdPattern *pb = (BtCmdPattern *) b;
  gboolean iia, iib;
  gint c;

  if (a == b)
    return 0;

  iia = !BT_IS_PATTERN (pa);
  iib = !BT_IS_PATTERN (pb);
  if (iia && iib) {
    BtPatternCmd ca, cb;
    g_object_get (pa, "command", &ca, NULL);
    g_object_get (pb, "command", &cb, NULL);
    c = (ca < cb) ? -1 : ((ca == cb) ? 0 : 1);
    GST_LOG ("comparing %d <-> %d: %d", ca, cb, c);
  } else if (iia && !iib) {
    c = -1;
    GST_LOG ("comparing cmd <-> data: -1");
  } else if (!iia && iib) {
    c = 1;
    GST_LOG ("comparing data <-> cmd: 1");
  } else {
    gchar *ida, *idb;
    g_object_get (pa, "name", &ida, NULL);
    g_object_get (pb, "name", &idb, NULL);
    c = strcmp (ida, idb);
    GST_LOG ("comparing %s <-> %s: %d", ida, idb, c);
    g_free (ida);
    g_free (idb);
  }

  return c;
}


static void
bt_pattern_list_model_add (BtPatternListModel * model, BtCmdPattern * pattern)
{
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint position;

  // check if pattern is internal
  if (model->priv->skip_internal) {
    if (!BT_IS_PATTERN (pattern)) {
      GST_INFO ("not adding internal pattern to model");
      return;
    }
  }

  GST_INFO ("add pattern to model");

  // insert new entry
  iter.stamp = model->priv->stamp;
  iter.user_data =
      g_sequence_insert_sorted (seq, pattern, model_item_cmp, NULL);
  position = g_sequence_iter_get_position (iter.user_data);

  g_signal_connect_object (pattern, "notify::name",
      G_CALLBACK (on_pattern_name_changed), (gpointer) model, 0);

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, position);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
  GST_DEBUG ("inserted pattern %p at position %d", pattern, position);
}

static void
bt_pattern_list_model_rem (BtPatternListModel * model, BtPattern * pattern)
{
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GSequenceIter *iter;
  gint position;

  GST_INFO ("removing pattern from model");

  // remove entry
  iter = g_sequence_lookup (seq, pattern, model_item_cmp, NULL);
  position = g_sequence_iter_get_position (iter);
  g_sequence_remove (iter);

  // signal to the view/app
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, position);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
  gtk_tree_path_free (path);

  GST_INFO ("removed pattern from model at pos %d ", position);
}

//-- signal handlers

static void
on_pattern_name_changed (BtPattern * pattern, GParamSpec * arg,
    gpointer user_data)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (user_data);
  GSequence *seq = model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint pos1, pos2 = -1, len;

  // find the item by pattern (cannot use model_item_cmp, as id has changed)
  iter.stamp = model->priv->stamp;
  len = g_sequence_get_length (seq);
  for (pos1 = 0; pos1 < len; pos1++) {
    iter.user_data = g_sequence_get_iter_at_pos (seq, pos1);
    if (g_sequence_get (iter.user_data) == pattern) {
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
on_pattern_added (BtMachine * machine, BtCmdPattern * pattern,
    gpointer user_data)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (user_data);

  bt_pattern_list_model_add (model, pattern);
}

static void
on_pattern_removed (BtMachine * machine, BtPattern * pattern,
    gpointer user_data)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (user_data);

  bt_pattern_list_model_rem (model, pattern);
}

static void
on_sequence_pattern_usage_changed (BtSequence * sequence, BtPattern * pattern,
    gpointer user_data)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (user_data);
  BtMachine *machine;

  g_object_get (pattern, "machine", &machine, NULL);
  if (machine) {
    if (machine == model->priv->machine) {
      GSequence *seq = model->priv->seq;
      GtkTreeIter iter;
      GtkTreePath *path;

      // find the item by pattern
      iter.stamp = model->priv->stamp;
      iter.user_data = g_sequence_lookup (seq, pattern, model_item_cmp, NULL);
      path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path,
          g_sequence_iter_get_position (iter.user_data));
      gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
      gtk_tree_path_free (path);
    }
    g_object_unref (machine);
  }
}

//-- constructor methods

/**
 * bt_pattern_list_model_new:
 * @machine: the machine
 * @sequence: the sequence
 * @skip_internal: wheter to include internal patterns or not
 *
 * Creates a list model of patterns for the @machne. The model is automatically
 * updated when patterns are added, removed or changed. The @sequence is used to
 * track the use of patterns.
 *
 * Returns: the pattern model.
 */

BtPatternListModel *
bt_pattern_list_model_new (BtMachine * machine, BtSequence * sequence,
    gboolean skip_internal)
{
  BtPatternListModel *self;
  BtCmdPattern *pattern;
  GList *node, *list;

  self = g_object_new (BT_TYPE_PATTERN_LIST_MODEL, NULL);

  self->priv->sequence = sequence;
  g_object_add_weak_pointer ((GObject *) sequence,
      (gpointer *) & self->priv->sequence);
  self->priv->machine = machine;
  g_object_add_weak_pointer ((GObject *) machine,
      (gpointer *) & self->priv->machine);
  self->priv->skip_internal = skip_internal;

  self->priv->param_types[BT_PATTERN_LIST_MODEL_LABEL] = G_TYPE_STRING;
  self->priv->param_types[BT_PATTERN_LIST_MODEL_IS_USED] = G_TYPE_BOOLEAN;
  self->priv->param_types[BT_PATTERN_LIST_MODEL_IS_UNUSED] = G_TYPE_BOOLEAN;
  self->priv->param_types[BT_PATTERN_LIST_MODEL_SHORTCUT] = G_TYPE_STRING;

  // shortcut keys (take skiping into account)
  self->priv->pattern_keys =
      skip_internal ? &sink_pattern_keys[2] : sink_pattern_keys;
  if (BT_IS_PROCESSOR_MACHINE (self->priv->machine)) {
    self->priv->pattern_keys =
        skip_internal ? &processor_pattern_keys[3] : processor_pattern_keys;
  } else if (BT_IS_SOURCE_MACHINE (self->priv->machine)) {
    self->priv->pattern_keys =
        skip_internal ? &source_pattern_keys[3] : source_pattern_keys;
  }
  // get pattern list from machine
  g_object_get ((gpointer) machine, "patterns", &list, NULL);
  // add patterns
  for (node = list; node; node = g_list_next (node)) {
    pattern = BT_CMD_PATTERN (node->data);      // we take no extra ref on the patterns here
    bt_pattern_list_model_add (self, pattern);
    g_object_unref (pattern);
  }
  g_list_free (list);

  g_signal_connect_object (machine, "pattern-added",
      G_CALLBACK (on_pattern_added), (gpointer) self, 0);
  g_signal_connect_object (machine, "pattern-removed",
      G_CALLBACK (on_pattern_removed), (gpointer) self, 0);
  g_signal_connect_object (sequence, "pattern-added",
      G_CALLBACK (on_sequence_pattern_usage_changed), (gpointer) self, 0);
  g_signal_connect_object (sequence, "pattern-removed",
      G_CALLBACK (on_sequence_pattern_usage_changed), (gpointer) self, 0);

  return self;
}

//-- methods

/**
 * bt_pattern_list_model_get_object:
 * @model: the model
 * @iter: the iter
 *
 * Lookup a pattern.
 *
 * Returns: the #BtPattern for the iter.
 */
BtPattern *
bt_pattern_list_model_get_object (BtPatternListModel * model,
    GtkTreeIter * iter)
{
  return g_sequence_get (iter->user_data);
}

/* more efficient than main-page-patterns::pattern_menu_model_get_iter_by_pattern()
gboolean
bt_pattern_list_model_get_iter (BtPatternListModel *model, GtkTreeIter *iter,
    BtPattern *pattern) {
  GSequence *seq=model->priv->seq;

  iter->stamp=model->priv->stamp;
  iter->user_data=g_sequence_lookup(seq,pattern,model_item_cmp,NULL);
  return(g_sequence_get(iter->user_data)==pattern);
}
*/

//-- tree model interface

static GtkTreeModelFlags
bt_pattern_list_model_tree_model_get_flags (GtkTreeModel * tree_model)
{
  return (GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint
bt_pattern_list_model_tree_model_get_n_columns (GtkTreeModel * tree_model)
{
  return N_COLUMNS;
}

static GType
bt_pattern_list_model_tree_model_get_column_type (GtkTreeModel * tree_model,
    gint index)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);

  g_return_val_if_fail (index < N_COLUMNS, G_TYPE_INVALID);

  return model->priv->param_types[index];
}

static gboolean
bt_pattern_list_model_tree_model_get_iter (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreePath * path)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);
  GSequence *seq = model->priv->seq;
  gint i = gtk_tree_path_get_indices (path)[0];

  if (i >= g_sequence_get_length (seq))
    return FALSE;

  iter->stamp = model->priv->stamp;
  iter->user_data = g_sequence_get_iter_at_pos (seq, i);

  return TRUE;
}

static GtkTreePath *
bt_pattern_list_model_tree_model_get_path (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);
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
bt_pattern_list_model_tree_model_get_value (GtkTreeModel * tree_model,
    GtkTreeIter * iter, gint column, GValue * value)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);
  BtPattern *pattern;

  g_return_if_fail (column < N_COLUMNS);

  g_value_init (value, model->priv->param_types[column]);

  if ((pattern = g_sequence_get (iter->user_data))) {

    switch (column) {
      case BT_PATTERN_LIST_MODEL_LABEL:
        g_object_get_property ((GObject *) pattern, "name", value);
        break;
      case BT_PATTERN_LIST_MODEL_IS_USED:{
        gboolean is_used = FALSE;

        if (!model->priv->skip_internal) {
          /* treat internal patterns as always used */
          is_used = !BT_IS_PATTERN (pattern);
        }
        if (!is_used && model->priv->sequence) {
          is_used =
              bt_sequence_is_pattern_used (model->priv->sequence, pattern);
        }
        g_value_set_boolean (value, is_used);
        break;
      }
      case BT_PATTERN_LIST_MODEL_IS_UNUSED:{
        gboolean is_used = FALSE;

        if (!model->priv->skip_internal) {
          /* treat internal patterns as always used */
          is_used = !BT_IS_PATTERN (pattern);
        }
        if (!is_used && model->priv->sequence) {
          is_used =
              bt_sequence_is_pattern_used (model->priv->sequence, pattern);
        }
        g_value_set_boolean (value, !is_used);
        break;
      }
      case BT_PATTERN_LIST_MODEL_SHORTCUT:{
        gchar key[2] = { 0, };
        gint index;

        index = g_sequence_iter_get_position (iter->user_data);
        key[0] = (index < 64) ? model->priv->pattern_keys[index] : ' ';
        g_value_set_string (value, key);
        break;
      }
    }
  }
}

static gboolean
bt_pattern_list_model_tree_model_iter_next (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);
  gboolean res;

  g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

  iter->user_data = g_sequence_iter_next (iter->user_data);
  if ((res = g_sequence_iter_is_end (iter->user_data)))
    iter->stamp = 0;

  return !res;
}

static gboolean
bt_pattern_list_model_tree_model_iter_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);

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
bt_pattern_list_model_tree_model_iter_has_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  return FALSE;
}

static gint
bt_pattern_list_model_tree_model_iter_n_children (GtkTreeModel * tree_model,
    GtkTreeIter * iter)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);

  if (iter == NULL)
    return g_sequence_get_length (model->priv->seq);

  g_return_val_if_fail (model->priv->stamp == iter->stamp, -1);
  return 0;
}

static gboolean
bt_pattern_list_model_tree_model_iter_nth_child (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
  BtPatternListModel *model = BT_PATTERN_LIST_MODEL (tree_model);
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
bt_pattern_list_model_tree_model_iter_parent (GtkTreeModel * tree_model,
    GtkTreeIter * iter, GtkTreeIter * child)
{
  iter->stamp = 0;
  return FALSE;
}

static void
bt_pattern_list_model_tree_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GtkTreeModelIface *const iface = g_iface;

  iface->get_flags = bt_pattern_list_model_tree_model_get_flags;
  iface->get_n_columns = bt_pattern_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_pattern_list_model_tree_model_get_column_type;
  iface->get_iter = bt_pattern_list_model_tree_model_get_iter;
  iface->get_path = bt_pattern_list_model_tree_model_get_path;
  iface->get_value = bt_pattern_list_model_tree_model_get_value;
  iface->iter_next = bt_pattern_list_model_tree_model_iter_next;
  iface->iter_children = bt_pattern_list_model_tree_model_iter_children;
  iface->iter_has_child = bt_pattern_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_pattern_list_model_tree_model_iter_n_children;
  iface->iter_nth_child = bt_pattern_list_model_tree_model_iter_nth_child;
  iface->iter_parent = bt_pattern_list_model_tree_model_iter_parent;
}

//-- class internals

static void
bt_pattern_list_model_finalize (GObject * object)
{
  BtPatternListModel *self = BT_PATTERN_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->machine) {
    g_object_remove_weak_pointer ((GObject *) self->priv->machine,
        (gpointer *) & self->priv->machine);
  }
  if (self->priv->sequence) {
    g_object_remove_weak_pointer ((GObject *) self->priv->sequence,
        (gpointer *) & self->priv->sequence);
  }

  g_sequence_free (self->priv->seq);

  G_OBJECT_CLASS (bt_pattern_list_model_parent_class)->finalize (object);
}

static void
bt_pattern_list_model_init (BtPatternListModel * self)
{
  self->priv = bt_pattern_list_model_get_instance_private(self);

  self->priv->seq = g_sequence_new (NULL);
  // random int to check whether an iter belongs to our model
  self->priv->stamp = g_random_int ();
}

static void
bt_pattern_list_model_class_init (BtPatternListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bt_pattern_list_model_finalize;
}
