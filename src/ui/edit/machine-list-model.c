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
#define N_COLUMNS 2

//-- property ids

//-- structs

struct _BtMachineListModelPrivate {
  gint stamp;
  BtSetup *setup;

  GType param_types[N_COLUMNS];
  GSequence *seq;
};

//-- the class

static void bt_machine_list_model_tree_model_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMachineListModel, bt_machine_list_model, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
						bt_machine_list_model_tree_model_init));

//-- helper

static void on_machine_id_changed(BtMachine *machine,GParamSpec *arg,gpointer user_data);

static gint model_item_cmp(gconstpointer a,gconstpointer b,gpointer data)
{
  return ((gint)b-(gint)a);
}


static void bt_machine_list_model_add(BtMachineListModel *model,BtMachine *machine) {
  GSequence *seq=model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint position;

  // add new entry
  position=g_sequence_get_length(seq);
  iter.stamp=model->priv->stamp;
  iter.user_data=g_sequence_append(seq,machine);

  g_signal_connect(machine,"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)model);

  // signal to the view/app
  path=gtk_tree_path_new();
  gtk_tree_path_append_index(path,position);
  gtk_tree_model_row_inserted(GTK_TREE_MODEL(model),path,&iter);
  gtk_tree_path_free(path);
}

static void bt_machine_list_model_rem(BtMachineListModel *model,BtMachine *machine) {
  GSequence *seq=model->priv->seq;
  GtkTreePath *path;
  GSequenceIter *iter;
  gint position;

  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_machine_id_changed,(gpointer)model);

#if GLIB_CHECK_VERSION(2,28,0)
  iter=g_sequence_lookup(seq,machine,model_item_cmp,NULL);
#else
  iter=g_sequence_search(seq,machine,model_item_cmp,NULL);
#endif
  position=g_sequence_iter_get_position(iter);

  // signal to the view/app
  path=gtk_tree_path_new();
  gtk_tree_path_append_index(path,position);
  gtk_tree_model_row_deleted(GTK_TREE_MODEL(model),path);
  gtk_tree_path_free(path);

  // remove entry
  g_sequence_remove(iter);
}

//-- signal handlers

static void on_machine_id_changed(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(user_data);
  GSequence *seq=model->priv->seq;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint position;

  // look the iter in model
  iter.stamp=model->priv->stamp;
#if GLIB_CHECK_VERSION(2,28,0)
  iter.user_data=g_sequence_lookup(seq,machine,model_item_cmp,NULL);
#else
  iter.user_data=g_sequence_search(seq,machine,model_item_cmp,NULL);
#endif
  position=g_sequence_iter_get_position(iter.user_data);

  // -> gtk_tree_model_row_changed
  path=gtk_tree_path_new();
  gtk_tree_path_append_index(path,position);
  gtk_tree_model_row_changed(GTK_TREE_MODEL(model),path,&iter);
  gtk_tree_path_free(path);
}


static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(user_data);

  bt_machine_list_model_add(model,machine);
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(user_data);

  bt_machine_list_model_rem(model,machine);
}

//-- constructor methods

BtMachineListModel *bt_machine_list_model_new(BtSetup *setup) {
  BtMachineListModel *self;
  BtMachine *machine;
  GList *node,*list;

  self=g_object_new(BT_TYPE_MACHINE_LIST_MODEL, NULL);

  self->priv->setup=setup;
  g_object_add_weak_pointer((GObject *)setup,(gpointer *)&self->priv->setup);

  self->priv->param_types[0]=GDK_TYPE_PIXBUF;
  self->priv->param_types[1]=G_TYPE_STRING;

  // get machine list from setup
  g_object_get((gpointer)setup,"machines",&list,NULL);
  // add machines
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data); // we take no extra ref on the machines here
    bt_machine_list_model_add(self,machine);
  }
  g_list_free(list);

  g_signal_connect(setup,"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(setup,"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);

  return(self);
}

//-- methods

BtMachine *bt_machine_list_model_get_object(BtMachineListModel *model,GtkTreeIter *iter) {
  return(g_sequence_get(iter->user_data));
}

//-- tree model interface

static GtkTreeModelFlags bt_machine_list_model_tree_model_get_flags(GtkTreeModel *tree_model) {
  return(GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint bt_machine_list_model_tree_model_get_n_columns(GtkTreeModel *tree_model) {
  return(N_COLUMNS);
}

static GType bt_machine_list_model_tree_model_get_column_type(GtkTreeModel *tree_model,gint index) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);

  g_return_val_if_fail(index<N_COLUMNS,G_TYPE_INVALID);

  return(model->priv->param_types[index]);
}

static gboolean bt_machine_list_model_tree_model_get_iter(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreePath *path) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);
  GSequence *seq=model->priv->seq;
  gint i=gtk_tree_path_get_indices (path)[0];

  if(i>=g_sequence_get_length(seq))
    return(FALSE);

  iter->stamp=model->priv->stamp;
  iter->user_data=g_sequence_get_iter_at_pos(seq,i);

  return(TRUE);
}

static GtkTreePath *bt_machine_list_model_tree_model_get_path(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);
  GtkTreePath *path;

  g_return_val_if_fail(iter->stamp==model->priv->stamp,NULL);

  if(g_sequence_iter_is_end(iter->user_data))
    return(NULL);

  path=gtk_tree_path_new();
  gtk_tree_path_append_index(path,g_sequence_iter_get_position(iter->user_data));

  return(path);
}

static void bt_machine_list_model_tree_model_get_value(GtkTreeModel *tree_model,GtkTreeIter *iter,gint column,GValue *value) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);
  BtMachine *machine;

  g_return_if_fail(column<N_COLUMNS);

  g_value_init(value,model->priv->param_types[column]);
  if((machine=g_sequence_get(iter->user_data))) {
    switch(column) {
      case 0:
        g_value_set_object(value,bt_ui_resources_get_icon_pixbuf_by_machine(machine));
        break;
      case 1:
        g_object_get_property((GObject *)machine,"id",value);
        break;
    }
  }
}

static gboolean bt_machine_list_model_tree_model_iter_next(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);
  gboolean res;

  g_return_val_if_fail(model->priv->stamp==iter->stamp,FALSE);

  iter->user_data=g_sequence_iter_next(iter->user_data);
  if((res=g_sequence_iter_is_end(iter->user_data)))
    iter->stamp=0;

  return !res;
}

static gboolean bt_machine_list_model_tree_model_iter_children(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);

  /* this is a list, nodes have no children */
  if (!parent) {
    if (g_sequence_get_length(model->priv->seq)>0) {
      iter->stamp=model->priv->stamp;
      iter->user_data=g_sequence_get_begin_iter(model->priv->seq);
      return(TRUE);
    }
  }
  iter->stamp=0;
  return(FALSE);
}

static gboolean bt_machine_list_model_tree_model_iter_has_child(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  return(FALSE);
}

static gint bt_machine_list_model_tree_model_iter_n_children(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);

  if (iter == NULL)
    return g_sequence_get_length(model->priv->seq);

  g_return_val_if_fail(model->priv->stamp==iter->stamp,-1);
  return(0);
}

static gboolean  bt_machine_list_model_tree_model_iter_nth_child(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent,gint n) {
  BtMachineListModel *model=BT_MACHINE_LIST_MODEL(tree_model);
  GSequenceIter *child;

  iter->stamp=0;

  if (parent)
    return(FALSE);

  child=g_sequence_get_iter_at_pos(model->priv->seq,n);

  if(g_sequence_iter_is_end(child))
    return(FALSE);

  iter->stamp=model->priv->stamp;
  iter->user_data=child;

  return(TRUE);
}

static gboolean bt_machine_list_model_tree_model_iter_parent(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *child) {
  iter->stamp=0;
  return(FALSE);
}

static void bt_machine_list_model_tree_model_init(gpointer const g_iface, gpointer const iface_data) {
  GtkTreeModelIface * const iface = g_iface;

  iface->get_flags       = bt_machine_list_model_tree_model_get_flags;
  iface->get_n_columns   = bt_machine_list_model_tree_model_get_n_columns;
  iface->get_column_type = bt_machine_list_model_tree_model_get_column_type;
  iface->get_iter        = bt_machine_list_model_tree_model_get_iter;
  iface->get_path        = bt_machine_list_model_tree_model_get_path;
  iface->get_value       = bt_machine_list_model_tree_model_get_value;
  iface->iter_next       = bt_machine_list_model_tree_model_iter_next;
  iface->iter_children   = bt_machine_list_model_tree_model_iter_children;
  iface->iter_has_child  = bt_machine_list_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_machine_list_model_tree_model_iter_n_children;
  iface->iter_nth_child  = bt_machine_list_model_tree_model_iter_nth_child;
  iface->iter_parent     = bt_machine_list_model_tree_model_iter_parent;
}

//-- class internals

static void bt_machine_list_model_finalize(GObject *object) {
  BtMachineListModel *self = BT_MACHINE_LIST_MODEL(object);

  GST_DEBUG("!!!! self=%p",self);

  if(self->priv->setup) {
    g_signal_handlers_disconnect_matched(self->priv->setup,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_machine_added,(gpointer)self);
    g_signal_handlers_disconnect_matched(self->priv->setup,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_machine_removed,(gpointer)self);
  }

  g_sequence_free(self->priv->seq);

  G_OBJECT_CLASS(bt_machine_list_model_parent_class)->finalize(object);
}

static void bt_machine_list_model_init(BtMachineListModel *self) {
  self->priv=G_TYPE_INSTANCE_GET_PRIVATE(self,BT_TYPE_MACHINE_LIST_MODEL,BtMachineListModelPrivate);

  self->priv->seq=g_sequence_new (NULL);
  // random int to check whether an iter belongs to our model
  self->priv->stamp=g_random_int();
}

static void bt_machine_list_model_class_init(BtMachineListModelClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtMachineListModelPrivate));

  gobject_class->finalize     = bt_machine_list_model_finalize;
}

