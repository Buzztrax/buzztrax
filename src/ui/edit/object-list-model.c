/* $Id$
 *
 * Buzztard
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

struct _BtObjectListModelPrivate {
  gint n_columns;
  gint stamp;

  gchar **property_names;
};

//-- the class

static void bt_object_list_model_tree_model_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtObjectListModel, bt_object_list_model, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
						bt_object_list_model_tree_model_init));

//-- helper

//-- constructor methods

BtObjectListModel *bt_object_list_model_new(gint n_columns,...) {
  BtObjectListModel *self;
  va_list args;
  gint i;
  gchar **names;

  self=g_object_new(BT_TYPE_OBJECT_LIST_MODEL, NULL);

  // allocate an array for the properties
  self->priv->property_names=g_new(gchar*,n_columns);
  // FIXME: supply the list item GType too, then we can lookup the paramspecs
  // and from that the property GTypes

  va_start (args, n_columns);
  names=self->priv->property_names;
  for(i=0;i<n_columns;i++) {
    names[i]=va_arg(args, gpointer);
  }
  va_end (args);
  self->priv->n_columns=n_columns;

  return(self);
}

//-- methods

#if 0
void bt_object_list_model_clear(BtObjectListModel *model) {
}

void bt_object_list_model_prepend(BtObjectListModel *model,GObject *object) {

}

void bt_object_list_model_append(BtObjectListModel *model,GObject *object) {

}

#endif

//-- tree model interface

static GtkTreeModelFlags bt_object_list_model_tree_model_get_flags(GtkTreeModel *tree_model) {
  return(GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint bt_object_list_model_tree_model_get_n_columns(GtkTreeModel *tree_model) {
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);

#if 0
  model->columns_dirty=TRUE;
#endif

  return(model->priv->n_columns);
}

static GType bt_object_list_model_tree_model_get_column_type(GtkTreeModel *tree_model,gint index) {
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);

  g_return_val_if_fail(index<model->priv->n_columns,G_TYPE_INVALID);

#if 0
  model->columns_dirty=TRUE;

  return(model->column_headers[index]);
#endif
  return(G_TYPE_INVALID);
}

static gboolean bt_object_list_model_tree_model_get_iter(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreePath *path) {
#if 0
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);
  GSequence *seq;
  gint i;

  model->columns_dirty=TRUE;
  seq=model->seq;
  i = gtk_tree_path_get_indices (path)[0];

  if (i >= g_sequence_get_length (seq))
    return FALSE;

  iter->stamp = model->stamp;
  iter->user_data = g_sequence_get_iter_at_pos (seq, i);

  return(TRUE);
#endif
  return(FALSE);
}

static GtkTreePath *bt_object_list_model_tree_model_get_path(GtkTreeModel *tree_model,GtkTreeIter *iter) {
#if 0
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);
  GtkTreePath *path;

  g_return_val_if_fail(iter->stamp==model->priv->stamp,NULL);

  if(g_sequence_iter_is_end(iter->user_data))
    return(NULL);

  path=gtk_tree_path_new();
  gtk_tree_path_append_index(path,g_sequence_iter_get_position(iter->user_data));

  return(path);
#endif
  return(NULL);
}

static void bt_object_list_model_tree_model_get_value(GtkTreeModel *tree_model,GtkTreeIter *iter,gint column,GValue *value) {
#if 0
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);
  GtkTreeDataList *list;
  gint tmp_column=column;

  g_return_if_fail(column<model->n_columns);
  g_return_if_fail(VALID_ITER(iter,model));

  list=g_sequence_get(iter->user_data);

  while(tmp_column-- > 0 && list)
    list=list->next;

  if(list==NULL)
    g_value_init(value,model->column_headers[column]);
  else
    _gtk_tree_data_list_node_to_value (list,
				       model->column_headers[column],
				       value);
  #endif
}

static gboolean bt_object_list_model_tree_model_iter_next(GtkTreeModel *tree_model,GtkTreeIter *iter) {
#if 0
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);
  gboolean res;

  g_return_val_if_fail(model->priv->stamp==iter->stamp,FALSE);

  iter->user_data=g_sequence_iter_next(iter->user_data);
  if(res=g_sequence_iter_is_end(iter->user_data))
    iter->stamp=0;

  return !res;
#endif
  return(FALSE);
}

static gboolean bt_object_list_model_tree_model_iter_children(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent) {
#if 0
  BtObjectListModel *model=BT_OBJECT_LIST_MODEL(tree_model);

  /* this is a list, nodes have no children */
  if (!parent) {
    if (g_sequence_get_length(model->seq)>0) {
      iter->stamp=model->stamp;
      iter->user_data=g_sequence_get_begin_iter(model->seq);
      return(TRUE);
    }
  }
#endif
  iter->stamp=0;
  return(FALSE);
}

static gboolean bt_object_list_model_tree_model_iter_has_child(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  return(FALSE);
}

static gint bt_object_list_model_tree_model_iter_n_children(GtkTreeModel *tree_model,GtkTreeIter *iter) {
#if 0
  BtObjectListModel *model=(BtObjectListModel *)tree_model;

  if (iter == NULL)
    return g_sequence_get_length(model->seq);

  g_return_val_if_fail(model->stamp==iter->stamp,-1);
#endif
  return(0);
}

static gboolean  bt_object_list_model_tree_model_iter_nth_child(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent,gint n) {
#if 0
  BtObjectListModel *model=(BtObjectListModel *)tree_model;
  GSequenceIter *child;

  iter->stamp = 0;

  if (parent)
    return(FALSE);

  child = g_sequence_get_iter_at_pos (model->seq, n);

  if (g_sequence_iter_is_end (child))
    return(FALSE);

  iter->stamp = model->stamp;
  iter->user_data = child;

  return(TRUE);
#endif
  return(FALSE);
}

static gboolean bt_object_list_model_tree_model_iter_parent(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *child) {
  iter->stamp=0;
  return(FALSE);
}

static void bt_object_list_model_tree_model_init(gpointer const g_iface, gpointer const iface_data) {
  GtkTreeModelIface * const iface = g_iface;

  iface->get_flags       = bt_object_list_model_tree_model_get_flags;
  iface->get_n_columns   = bt_object_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_object_list_model_tree_model_get_column_type;
  iface->get_iter        = bt_object_list_model_tree_model_get_iter;
  iface->get_path        = bt_object_list_model_tree_model_get_path;
  iface->get_value       = bt_object_list_model_tree_model_get_value;
  iface->iter_next       = bt_object_list_model_tree_model_iter_next;
  iface->iter_children   = bt_object_list_model_tree_model_iter_children;
  iface->iter_has_child  = bt_object_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_object_list_model_tree_model_iter_n_children;
  iface->iter_nth_child  = bt_object_list_model_tree_model_iter_nth_child;
  iface->iter_parent     = bt_object_list_model_tree_model_iter_parent;
}

//-- class internals

static void bt_object_list_model_dispose(GObject *object) {
  BtObjectListModel *self = BT_OBJECT_LIST_MODEL(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(bt_object_list_model_parent_class)->dispose(object);
}

static void bt_object_list_model_finalize(GObject *object) {
  BtObjectListModel *self = BT_OBJECT_LIST_MODEL(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv->property_names);

  G_OBJECT_CLASS(bt_object_list_model_parent_class)->finalize(object);
}

static void bt_object_list_model_init(BtObjectListModel *self) {
  self->priv=G_TYPE_INSTANCE_GET_PRIVATE(self,BT_TYPE_OBJECT_LIST_MODEL,BtObjectListModelPrivate);
}

static void bt_object_list_model_class_init(BtObjectListModelClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtObjectListModelPrivate));

  gobject_class->dispose      = bt_object_list_model_dispose;
  gobject_class->finalize     = bt_object_list_model_finalize;

}

