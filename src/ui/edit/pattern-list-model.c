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

struct _BtPatternListModel
{
  GObject parent;
  
  gint stamp;
  BtMachine *machine;
  BtSequence *sequence;
  gboolean skip_internal;

  /* shortcut table */
  const char *pattern_keys;

  GSequence *seq;
};

//-- the class

static void bt_pattern_list_model_g_list_model_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtPatternListModel, bt_pattern_list_model,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
        bt_pattern_list_model_g_list_model_init));

//-- helper

//-- signal handlers

static void
on_pattern_added (BtMachine * machine, BtCmdPattern * pattern,
    gpointer user_data)
{
  GListModel *model = G_LIST_MODEL (user_data);
  guint count = g_list_model_get_n_items (model);
  g_list_model_items_changed (model, 0, count, count);
}

static void
on_pattern_removed (BtMachine * machine, BtPattern * pattern,
    gpointer user_data)
{
  on_pattern_added (machine, BT_CMD_PATTERN (pattern), user_data);
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

  self = g_object_new (BT_TYPE_PATTERN_LIST_MODEL, NULL);

  self->sequence = sequence;
  g_object_add_weak_pointer ((GObject *) sequence,
      (gpointer *) & self->sequence);
  self->machine = machine;
  g_object_add_weak_pointer ((GObject *) machine,
      (gpointer *) & self->machine);
  self->skip_internal = skip_internal;

  // shortcut keys (take skiping into account)
  self->pattern_keys =
      skip_internal ? &sink_pattern_keys[2] : sink_pattern_keys;
  if (BT_IS_PROCESSOR_MACHINE (self->machine)) {
    self->pattern_keys =
        skip_internal ? &processor_pattern_keys[3] : processor_pattern_keys;
  } else if (BT_IS_SOURCE_MACHINE (self->machine)) {
    self->pattern_keys =
        skip_internal ? &source_pattern_keys[3] : source_pattern_keys;
  }

  g_signal_connect_object (machine, "pattern-added",
      G_CALLBACK (on_pattern_added), (gpointer) self, 0);
  g_signal_connect_object (machine, "pattern-removed",
      G_CALLBACK (on_pattern_removed), (gpointer) self, 0);

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
 * Returns: (transfer full): the #BtCmdPattern at the index.
 */
BtCmdPattern *
bt_pattern_list_model_get_object (BtPatternListModel * model, guint position)
{
  gpointer result = g_list_model_get_item (G_LIST_MODEL (model), position);
  return result ? BT_CMD_PATTERN (result) : NULL;
}

/**
 * bt_pattern_list_model_get_pattern_by_key:
 * @model: the model
 * @key: a shortcut key
 *
 * Returns: (transfer full): the #BtCmdPattern having the shortcut given by @key, or NULL if none exists.
 */
BtCmdPattern *
bt_pattern_list_model_get_pattern_by_key (BtPatternListModel *model, gchar key)
{
  /// GTK4 verify this
  const gchar *c;
  guint i;
  guint count = g_list_model_get_n_items (G_LIST_MODEL (model));
  
  for (c = model->pattern_keys, i = 0; *c && i < count; ++c, ++i) {
    if (*c == key)
      return bt_machine_get_pattern_by_index (model->machine, i);
  }

  return NULL;
}

//-- GListModel interface

static gpointer
bt_pattern_list_model_get_item (GListModel* list, guint position)
{
  BtPatternListModel *self = BT_PATTERN_LIST_MODEL (list);

  g_return_val_if_fail (self->machine, NULL);

  gulong idx = (gulong) position;

  if (self->skip_internal) {
    idx += bt_machine_get_internal_patterns_count (self->machine);
  }
  
  guint count = bt_machine_get_patterns_count (self->machine);
  g_assert (count < G_MAXUINT32);
            
  if (idx < count) {
    return bt_machine_get_pattern_by_index (self->machine, idx);
  } else {
    return NULL;
  }
}

static GType
bt_pattern_list_model_get_item_type (GListModel* list)
{
  return BT_TYPE_CMD_PATTERN;
}

static guint
bt_pattern_list_model_get_n_items (GListModel* list)
{
  BtPatternListModel *self = BT_PATTERN_LIST_MODEL (list);
  
  g_return_val_if_fail (self->machine, 0);
  
  // check if pattern is internal
  if (self->skip_internal) {
    return MAX (0, bt_machine_get_patterns_count (self->machine) -
                      bt_machine_get_internal_patterns_count (self->machine));
  } else {
    return bt_machine_get_patterns_count (self->machine);
  }
}
  
static void
bt_pattern_list_model_g_list_model_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GListModelInterface *const iface = g_iface;

  iface->get_item = bt_pattern_list_model_get_item;
  iface->get_item_type = bt_pattern_list_model_get_item_type;
  iface->get_n_items = bt_pattern_list_model_get_n_items;
}

//-- class internals

static void
bt_pattern_list_model_finalize (GObject * object)
{
  BtPatternListModel *self = BT_PATTERN_LIST_MODEL (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->machine) {
    g_object_remove_weak_pointer ((GObject *) self->machine,
        (gpointer *) & self->machine);
  }
  if (self->sequence) {
    g_object_remove_weak_pointer ((GObject *) self->sequence,
        (gpointer *) & self->sequence);
  }

  g_sequence_free (self->seq);

  G_OBJECT_CLASS (bt_pattern_list_model_parent_class)->finalize (object);
}

static void
bt_pattern_list_model_init (BtPatternListModel * self)
{
  self = bt_pattern_list_model_get_instance_private(self);

  self->seq = g_sequence_new (NULL);
  // random int to check whether an iter belongs to our model
  self->stamp = g_random_int ();
}

static void
bt_pattern_list_model_class_init (BtPatternListModelClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = bt_pattern_list_model_finalize;
}
