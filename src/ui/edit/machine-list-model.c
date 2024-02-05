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

struct _BtMachineListModel
{
  GObject parent;
  
  BtSetup *setup;

  GType param_types[N_COLUMNS];
  GSequence *seq;
};

//-- the class

static void
bt_machine_list_model_g_list_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMachineListModel, bt_machine_list_model,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
        bt_machine_list_model_g_list_model_init));

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
  GSequence *seq = model->seq;
  gint position;

  GST_INFO_OBJECT (machine, "add machine to model");

  // insert new entry
  GSequenceIter* iter = g_sequence_insert_sorted (seq, machine, model_item_cmp, NULL);
  position = g_sequence_iter_get_position (iter);

  g_signal_connect_object (machine, "notify::id",
      G_CALLBACK (on_machine_id_changed), (gpointer) model, 0);

  g_list_model_items_changed (G_LIST_MODEL (model), position, 0, 1);
}

static void
bt_machine_list_model_rem (BtMachineListModel * model, BtMachine * machine)
{
  GSequence *seq = model->seq;
  GSequenceIter *iter;
  gint position;

  GST_INFO_OBJECT (machine, "removing machine from model");

  // remove entry
  iter = g_sequence_lookup (seq, machine, model_item_cmp, NULL);
  position = g_sequence_iter_get_position (iter);
  g_sequence_remove (iter);

  g_list_model_items_changed (G_LIST_MODEL (model), position, 1, 0);

  GST_INFO_OBJECT (machine, "removed machine from model at pos %d ", position);
}

//-- signal handlers

static void
on_machine_id_changed (BtMachine * machine, GParamSpec * arg,
    gpointer user_data)
{
  BtMachineListModel *model = BT_MACHINE_LIST_MODEL (user_data);
  GSequence *seq = model->seq;
  GtkTreeIter iter;
  gint pos1, pos2 = -1, len;

  // find the item by machine (cannot use model_item_cmp, as id has changed)
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
    g_list_model_items_changed (G_LIST_MODEL (model), pos1, 1, 0);
    g_list_model_items_changed (G_LIST_MODEL (model), pos2, 0, 1);
  } else {
    g_list_model_items_changed (G_LIST_MODEL (model), pos2, 0, 1);
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
 * Creates a list model of machine instances for the @setup. The model is automatically
 * updated when machines are added to, removed from or changed in the song.
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

  self->setup = setup;
  g_object_add_weak_pointer ((GObject *) setup,
      (gpointer *) & self->setup);

  self->param_types[BT_MACHINE_LIST_MODEL_ICON] = GDK_TYPE_PIXBUF;
  self->param_types[BT_MACHINE_LIST_MODEL_LABEL] = G_TYPE_STRING;

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

//-- list model interface


static gpointer
bt_machine_list_model_get_item (GListModel* list, guint position)
{
  BtMachineListModel *self = BT_MACHINE_LIST_MODEL (list);
  
  g_assert (position < G_MAXINT64);
  
  GSequenceIter* it = g_sequence_get_iter_at_pos (self->seq, (gint)position);
  if (g_sequence_iter_compare (it, g_sequence_get_end_iter (self->seq)) == 0) {
    return NULL;
  } else {
    return g_sequence_get (it);
  }
}

static GType
bt_machine_list_model_get_item_type (GListModel* list)
{
  return BT_TYPE_MACHINE;
}

static guint
bt_machine_list_model_get_n_items (GListModel* list)
{
  BtMachineListModel *self = BT_MACHINE_LIST_MODEL (list);
  return g_sequence_get_length (self->seq);
}
  
static void
bt_machine_list_model_g_list_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GListModelInterface *const iface = g_iface;

  iface->get_item = bt_machine_list_model_get_item;
  iface->get_item_type = bt_machine_list_model_get_item_type;
  iface->get_n_items = bt_machine_list_model_get_n_items;
}

//-- class internals

static void
bt_machine_list_model_finalize (GObject * object)
{
  BtMachineListModel *self = BT_MACHINE_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->setup) {
    g_object_remove_weak_pointer ((GObject *) self->setup,
        (gpointer *) & self->setup);
  }

  g_sequence_free (self->seq);

  G_OBJECT_CLASS (bt_machine_list_model_parent_class)->finalize (object);
}

static void
bt_machine_list_model_init (BtMachineListModel * self)
{
  self = bt_machine_list_model_get_instance_private(self);

  self->seq = g_sequence_new (NULL);
}

static void
bt_machine_list_model_class_init (BtMachineListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bt_machine_list_model_finalize;
}
