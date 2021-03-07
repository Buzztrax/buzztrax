/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btpattern
 * @short_description: class for an event pattern of a #BtMachine instance
 *
 * A pattern contains a grid of events. Events are parameter changes in
 * #BtMachine objects. The events are stored as #GValues. Cells containing %NULL
 * have no event for the parameter at the time.
 *
 * Patterns can have individual lengths. The length is measured in ticks. How
 * much that is in e.g. milliseconds is determined by #BtSongInfo:bpm and
 * #BtSongInfo:tpb.
 *
 * A pattern might consist of several groups. These are mapped to the global
 * parameters of a machine and the voice parameters for each machine voice (if
 * any). The number of voices (tracks) is the same in all patterns of a machine.
 * If the voices are changed on the machine patterns resize themselves.
 *
 * The patterns are used in the #BtSequence to form the score of a song.
 */
/* TODO(ensonic): pattern editing
 *  - inc/dec cursor-cell/selection
 */
/* TODO(ensonic): cut/copy/paste api
 * - need private BtPatternFragment object (not needed with the group changes?)
 *   - copy of the data
 *   - pos and size of the region
 *   - column-types
 *   - eventually wire-pattern fragments
 * - api
 *   fragment = bt_pattern_copy_fragment (pattern, col1, col2, row1, row2);
 *     return a new fragment object, opaque for the callee
 *   fragment = bt_pattern_cut_fragment (pattern, col1, col2, row1, row2);
 *     calls copy and clears afterwards
 *   success = bt_pattern_paste_fragment(pattern, fragment, col1, row1);
 *     checks if column-types are compatible
 */
#define BT_CORE
#define BT_PATTERN_C

#include "core_private.h"

//-- signal ids

enum
{
  PARAM_CHANGED_EVENT,
  GROUP_CHANGED_EVENT,
  PATTERN_CHANGED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum
{
  PATTERN_LENGTH = 1,
  PATTERN_VOICES,
  PATTERN_COPY_SOURCE
};

struct _BtPatternPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the pattern belongs to */
  BtSong *song;

  /* the number of ticks */
  gulong length;
  /* the number of voices */
  gulong voices;
  /* the machine the pattern belongs to */
  BtMachine *machine;

  /* the pattern data */
  BtValueGroup *global_value_group;
  BtValueGroup **voice_value_groups;
  GHashTable *wire_value_groups;
  GHashTable *param_to_value_groups;

  /* temporary pointer to a pattern to copy from */
  BtPattern *_src;
};

static guint signals[LAST_SIGNAL] = { 0, };

//-- prototypes

static BtValueGroup *new_value_group (const BtPattern * const self,
    BtParameterGroup * pg);
static void del_value_group (const BtPattern * const self, BtValueGroup * vg);

//-- the class

static void bt_pattern_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtPattern, bt_pattern, BT_TYPE_CMD_PATTERN,
    G_ADD_PRIVATE(BtPattern)
    G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_pattern_persistence_interface_init));

//-- helper methods

/*
 * bt_pattern_resize_data_length:
 * @self: the pattern to resize the length
 * @length: the old length
 *
 * Resizes the event data grid to the new length. Keeps previous values.
 */
static void
bt_pattern_resize_data_length (const BtPattern * const self)
{
  gulong i;
  GList *node;
  BtWire *wire;
  BtValueGroup *vg;

  if (!self->priv->global_value_group)
    return;

  g_object_set (self->priv->global_value_group, "length", self->priv->length,
      NULL);
  for (i = 0; i < self->priv->voices; i++) {
    g_object_set (self->priv->voice_value_groups[i], "length",
        self->priv->length, NULL);
  }
  for (node = self->priv->machine->dst_wires; node; node = g_list_next (node)) {
    wire = BT_WIRE (node->data);
    vg = g_hash_table_lookup (self->priv->wire_value_groups, wire);
    g_object_set (vg, "length", self->priv->length, NULL);
  }
}

/*
 * bt_pattern_resize_data_voices:
 * @self: the pattern to resize the voice number
 * @voices: the old number of voices
 *
 * Resizes the event data grid to the new number of voices. Keeps previous values.
 */
static void
bt_pattern_resize_data_voices (const BtPattern * const self,
    const gulong voices)
{
  gulong i;

  GST_INFO ("change voices from %lu -> %lu", voices, self->priv->voices);

  // unref old voices
  for (i = self->priv->voices; i < voices; i++) {
    GST_INFO ("  del %lu", i);
    del_value_group (self, self->priv->voice_value_groups[i]);
  }
  self->priv->voice_value_groups =
      g_renew (BtValueGroup *, self->priv->voice_value_groups,
      self->priv->voices);
  // create new voices
  for (i = voices; i < self->priv->voices; i++) {
    GST_INFO ("  add %lu", i);
    self->priv->voice_value_groups[i] =
        new_value_group (self,
        bt_machine_get_voice_param_group (self->priv->machine, i));
  }
}

//-- signal handler

static void
bt_pattern_on_voices_changed (BtMachine * const machine,
    const GParamSpec * const arg, gconstpointer const user_data)
{
  const BtPattern *const self = BT_PATTERN (user_data);
  gulong old_voices = self->priv->voices;

  g_object_get ((gpointer) machine, "voices", &self->priv->voices, NULL);
  if (old_voices != self->priv->voices) {
    GST_DEBUG_OBJECT (self->priv->machine,
        "set the voices for pattern: %lu -> %lu", old_voices,
        self->priv->voices);
    bt_pattern_resize_data_voices (self, old_voices);
    g_object_notify ((GObject *) self, "voices");
  }
}

static void
bt_pattern_on_setup_wire_added (BtSetup * setup, BtWire * wire,
    gconstpointer const user_data)
{
  const BtPattern *const self = BT_PATTERN (user_data);
  BtMachine *machine;

  g_object_get (wire, "dst", &machine, NULL);
  if (machine == self->priv->machine) {
    g_hash_table_insert (self->priv->wire_value_groups, wire,
        new_value_group (self, bt_wire_get_param_group (wire)));
  }
  g_object_unref (machine);
}

static void
bt_pattern_on_setup_wire_removed (BtSetup * setup, BtWire * wire,
    gconstpointer const user_data)
{
  const BtPattern *const self = BT_PATTERN (user_data);
  BtMachine *machine;

  g_object_get (wire, "dst", &machine, NULL);
  if (machine == self->priv->machine) {
    BtValueGroup *vg;
    if ((vg = g_hash_table_lookup (self->priv->wire_value_groups, wire))) {
      g_hash_table_steal (self->priv->wire_value_groups, wire);
      del_value_group (self, vg);
    }
  }
  g_object_unref (machine);
}

static void
bt_pattern_on_param_changed (const BtValueGroup * const group,
    BtParameterGroup * param_group, const gulong tick, const gulong param,
    gpointer user_data)
{
  g_signal_emit (user_data, signals[PARAM_CHANGED_EVENT], 0, param_group, tick,
      param);
}

static void
bt_pattern_on_group_changed (const BtValueGroup * const group,
    BtParameterGroup * param_group, const gboolean intermediate,
    gpointer user_data)
{
  g_signal_emit (user_data, signals[GROUP_CHANGED_EVENT], 0, param_group,
      intermediate);
}

//-- helper methods

static BtValueGroup *
new_value_group (const BtPattern * const self, BtParameterGroup * pg)
{
  BtValueGroup *vg;
  BtPattern *_src = self->priv->_src;

  if (_src) {
    BtValueGroup *_vg =
        g_hash_table_lookup (_src->priv->param_to_value_groups, pg);
    vg = bt_value_group_copy (_vg);
  } else {
    vg = bt_value_group_new (pg, self->priv->length);
  }
  // attach signal handlers
  g_signal_connect (vg, "param-changed",
      G_CALLBACK (bt_pattern_on_param_changed), (gpointer) self);
  g_signal_connect (vg, "group-changed",
      G_CALLBACK (bt_pattern_on_group_changed), (gpointer) self);

  g_hash_table_insert (self->priv->param_to_value_groups, pg,
      g_object_ref (vg));

  return vg;
}

static void
del_value_group (const BtPattern * const self, BtValueGroup * vg)
{
  BtParameterGroup *pg;

  GST_DEBUG ("del vg %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (vg));

  g_object_get (vg, "parameter-group", &pg, NULL);
  g_hash_table_remove (self->priv->param_to_value_groups, pg);
  g_object_unref (pg);
  g_object_unref (vg);
}

//-- constructor methods

/**
 * bt_pattern_new:
 * @song: the song the new instance belongs to
 * @name: the display name of the pattern
 * @length: the number of ticks
 * @machine: the machine the pattern belongs to
 *
 * Create a new instance. It will be automatically added to the machines pattern
 * list.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPattern *
bt_pattern_new (const BtSong * const song, const gchar * const name,
    const gulong length, const BtMachine * const machine)
{
  return BT_PATTERN (g_object_new (BT_TYPE_PATTERN, "song", song, "name", name,
          "machine", machine, "length", length, NULL));
}

/**
 * bt_pattern_copy:
 * @self: the pattern to create a copy from
 *
 * Create a new instance as a copy of the given instance.
 *
 * Returns: (transfer full): the new instance or %NULL in case of an error
 */
BtPattern *
bt_pattern_copy (const BtPattern * const self)
{
  BtPattern *pattern;
  gchar *name;

  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);

  GST_INFO ("copying pattern");

  name = bt_machine_get_unique_pattern_name (self->priv->machine);
  pattern = BT_PATTERN (g_object_new (BT_TYPE_PATTERN, "song", self->priv->song,
          "name", name, "machine", self->priv->machine, "length",
          self->priv->length, "copy-source", self, NULL));
  g_free (name);
  return pattern;
}

//-- methods

/**
 * bt_pattern_get_global_event_data:
 * @self: the pattern to search for the global param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Do not modify the contents!
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *
bt_pattern_get_global_event_data (const BtPattern * const self,
    const gulong tick, const gulong param)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);

  return (bt_value_group_get_event_data (self->priv->global_value_group, tick,
          param));
}

/**
 * bt_pattern_get_voice_event_data:
 * @self: the pattern to search for the voice param
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the voice parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Do not modify the contents!
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *
bt_pattern_get_voice_event_data (const BtPattern * const self,
    const gulong tick, const gulong voice, const gulong param)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (voice < self->priv->voices, NULL);

  return (bt_value_group_get_event_data (self->priv->voice_value_groups[voice],
          tick, param));
}

/**
 * bt_pattern_get_wire_event_data:
 * @self: the pattern to search for the wire param
 * @tick: the tick (time) position starting with 0
 * @wire: the related wire object
 * @param: the number of the wire parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Do not modify the contents!
 *
 * Returns: the GValue or %NULL if out of the pattern range
 */
GValue *
bt_pattern_get_wire_event_data (const BtPattern * const self, const gulong tick,
    const BtWire * wire, const gulong param)
{
  BtValueGroup *vg;

  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (BT_IS_WIRE (wire), NULL);

  if ((vg = g_hash_table_lookup (self->priv->wire_value_groups, wire))) {
    return bt_value_group_get_event_data (vg, tick, param);
  }
  return NULL;
}

/**
 * bt_pattern_set_global_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_pattern_set_global_event (const BtPattern * const self, const gulong tick,
    const gulong param, const gchar * const value)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);

  return bt_value_group_set_event (self->priv->global_value_group, tick, param,
      value);
}

/**
 * bt_pattern_set_voice_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the voice parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_pattern_set_voice_event (const BtPattern * const self, const gulong tick,
    const gulong voice, const gulong param, const gchar * const value)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);
  g_return_val_if_fail (voice < self->priv->voices, FALSE);

  return bt_value_group_set_event (self->priv->voice_value_groups[voice], tick,
      param, value);
}

/**
 * bt_pattern_set_wire_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @wire: the related wire object
 * @param: the number of the wire parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_pattern_set_wire_event (const BtPattern * const self, const gulong tick,
    const BtWire * wire, const gulong param, const gchar * const value)
{
  BtValueGroup *vg;

  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);
  g_return_val_if_fail (BT_IS_WIRE (wire), FALSE);

  if ((vg = g_hash_table_lookup (self->priv->wire_value_groups, wire))) {
    return bt_value_group_set_event (vg, tick, param, value);
  }
  return FALSE;
}

/**
 * bt_pattern_get_global_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *
bt_pattern_get_global_event (const BtPattern * const self, const gulong tick,
    const gulong param)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);

  return (bt_value_group_get_event (self->priv->global_value_group, tick,
          param));
}

/**
 * bt_pattern_get_voice_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the voice parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *
bt_pattern_get_voice_event (const BtPattern * const self, const gulong tick,
    const gulong voice, const gulong param)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (voice < self->priv->voices, NULL);

  return (bt_value_group_get_event (self->priv->voice_value_groups[voice], tick,
          param));
}

/**
 * bt_pattern_get_wire_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @wire: the related wire object
 * @param: the number of the wire parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *
bt_pattern_get_wire_event (const BtPattern * const self, const gulong tick,
    const BtWire * wire, const gulong param)
{
  BtValueGroup *vg;

  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (BT_IS_WIRE (wire), NULL);

  if ((vg = g_hash_table_lookup (self->priv->wire_value_groups, wire))) {
    return bt_value_group_get_event (vg, tick, param);
  }
  return NULL;
}

/**
 * bt_pattern_test_global_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the global parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 */
gboolean
bt_pattern_test_global_event (const BtPattern * const self, const gulong tick,
    const gulong param)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);

  return (bt_value_group_test_event (self->priv->global_value_group, tick,
          param));
}

/**
 * bt_pattern_test_voice_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @voice: the voice number starting with 0
 * @param: the number of the voice parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 */
gboolean
bt_pattern_test_voice_event (const BtPattern * const self, const gulong tick,
    const gulong voice, const gulong param)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);
  g_return_val_if_fail (voice < self->priv->voices, FALSE);

  return (bt_value_group_test_event (self->priv->voice_value_groups[voice],
          tick, param));
}

/**
 * bt_pattern_test_wire_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @wire: the related wire object
 * @param: the number of the wire parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 */
gboolean
bt_pattern_test_wire_event (const BtPattern * const self, const gulong tick,
    const BtWire * wire, const gulong param)
{
  BtValueGroup *vg;

  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);
  g_return_val_if_fail (BT_IS_WIRE (wire), FALSE);

  if ((vg = g_hash_table_lookup (self->priv->wire_value_groups, wire))) {
    return bt_value_group_test_event (vg, tick, param);
  }
  return FALSE;
}

/**
 * bt_pattern_test_tick:
 * @self: the pattern to check
 * @tick: the tick index in the pattern
 *
 * Check if there are any event in the given pattern-row.
 *
 * Returns: %TRUE is there are events, %FALSE otherwise
 */
gboolean
bt_pattern_test_tick (const BtPattern * const self, const gulong tick)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), FALSE);

  if (bt_value_group_test_tick (self->priv->global_value_group, tick)) {
    return TRUE;
  }

  const gulong voices = self->priv->voices;
  gulong j;
  for (j = 0; j < voices; j++) {
    if (bt_value_group_test_tick (self->priv->voice_value_groups[j], tick)) {
      return TRUE;
    }
  }
  return FALSE;
}

#if 0
BtValueGroup **
bt_pattern_get_groups (const BtPattern * const self)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);

  // FIXME(ensonic): get an array of all groups
  // this can be useful in main-page-patterns
  return NULL;
}
#endif

/**
 * bt_pattern_get_global_group:
 * @self: the pattern
 *
 * Get the #BtValueGroup for global parameters.
 *
 * Returns: (transfer none): the group owned by the pattern.
 */
BtValueGroup *
bt_pattern_get_global_group (const BtPattern * const self)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);

  return self->priv->global_value_group;
}

/**
 * bt_pattern_get_voice_group:
 * @self: the pattern
 * @voice: the voice to get the group for
 *
 * Get the #BtValueGroup for voice parameters.
 *
 * Returns: (transfer none): the group owned by the pattern.
 */
BtValueGroup *
bt_pattern_get_voice_group (const BtPattern * const self, const gulong voice)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (voice < self->priv->voices, NULL);

  return self->priv->voice_value_groups[voice];
}

/**
 * bt_pattern_get_wire_group:
 * @self: the pattern
 * @wire: the #BtWire to get the group for
 *
 * Get the #BtValueGroup for wire parameters.
 *
 * Returns: (transfer none): the group owned by the pattern.
 */
BtValueGroup *
bt_pattern_get_wire_group (const BtPattern * const self, const BtWire * wire)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (BT_IS_WIRE (wire), NULL);

  return g_hash_table_lookup (self->priv->wire_value_groups, wire);
}

/**
 * bt_pattern_get_group_by_parameter_group:
 * @self: the pattern
 * @param_group: the #BtParameterGroup to get the group for
 *
 * Get the #BtValueGroup for the given @param_group.
 *
 * Returns: (transfer none): the group owned by the pattern.
 */
BtValueGroup *
bt_pattern_get_group_by_parameter_group (const BtPattern * const self,
    BtParameterGroup * param_group)
{
  g_return_val_if_fail (BT_IS_PATTERN (self), NULL);
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (param_group), NULL);

  return g_hash_table_lookup (self->priv->param_to_value_groups, param_group);
}

/**
 * bt_pattern_insert_row:
 * @self: the pattern
 * @tick: the position to insert at
 *
 * Insert one empty row for all parameters.
 *
 * Since: 0.3
 */
void
bt_pattern_insert_row (const BtPattern * const self, const gulong tick)
{
  g_return_if_fail (BT_IS_PATTERN (self));

  GST_DEBUG ("insert full-row at %lu", tick);

  g_signal_emit ((gpointer) self, signals[PATTERN_CHANGED_EVENT], 0, TRUE);
  bt_value_group_insert_full_row (self->priv->global_value_group, tick);
  const gulong voices = self->priv->voices;
  gulong j;
  for (j = 0; j < voices; j++) {
    bt_value_group_insert_full_row (self->priv->voice_value_groups[j], tick);
  }
  GList *node;
  for (node = self->priv->machine->dst_wires; node; node = g_list_next (node)) {
    bt_value_group_insert_full_row (g_hash_table_lookup (self->priv->
            wire_value_groups, node->data), tick);
  }
  g_signal_emit ((gpointer) self, signals[PATTERN_CHANGED_EVENT], 0, FALSE);
}

/**
 * bt_pattern_delete_row:
 * @self: the pattern
 * @tick: the position to delete
 *
 * Delete row for all parameters.
 *
 * Since: 0.3
 */
void
bt_pattern_delete_row (const BtPattern * const self, const gulong tick)
{
  g_return_if_fail (BT_IS_PATTERN (self));

  GST_DEBUG ("delete full-row at %lu", tick);

  g_signal_emit ((gpointer) self, signals[PATTERN_CHANGED_EVENT], 0, TRUE);
  bt_value_group_delete_full_row (self->priv->global_value_group, tick);
  const gulong voices = self->priv->voices;
  gulong j;
  for (j = 0; j < voices; j++) {
    bt_value_group_delete_full_row (self->priv->voice_value_groups[j], tick);
  }
  GList *node;
  for (node = self->priv->machine->dst_wires; node; node = g_list_next (node)) {
    bt_value_group_delete_full_row (g_hash_table_lookup (self->priv->
            wire_value_groups, node->data), tick);
  }
  g_signal_emit ((gpointer) self, signals[PATTERN_CHANGED_EVENT], 0, FALSE);
}

/**
 * bt_pattern_transform_colums:
 * @self: the pattern
 * @op: the transform operation
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 *
 * Applies @op to values from @start_tick to @end_tick to all parameters in the
 * pattern.
 *
 * Since: 0.11
 */
void
bt_pattern_transform_colums (const BtPattern * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick)
{
  g_return_if_fail (BT_IS_PATTERN (self));

  g_signal_emit ((gpointer) self, signals[PATTERN_CHANGED_EVENT], 0, TRUE);
  bt_value_group_transform_colums (self->priv->global_value_group,
      BT_VALUE_GROUP_OP_CLEAR, start_tick, end_tick);
  const gulong voices = self->priv->voices;
  gulong j;
  for (j = 0; j < voices; j++) {
    bt_value_group_transform_colums (self->priv->voice_value_groups[j], op,
        start_tick, end_tick);
  }
  GList *node;
  for (node = self->priv->machine->dst_wires; node; node = g_list_next (node)) {
    bt_value_group_transform_colums (g_hash_table_lookup (self->
            priv->wire_value_groups, node->data), op, start_tick, end_tick);
  }
  g_signal_emit ((gpointer) self, signals[PATTERN_CHANGED_EVENT], 0, FALSE);
}


/**
 * bt_pattern_serialize_columns:
 * @self: the pattern
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for all params into @data.
 *
 * Since: 0.6
 */
void
bt_pattern_serialize_columns (const BtPattern * const self,
    const gulong start_tick, const gulong end_tick, GString * data)
{
  g_return_if_fail (BT_IS_PATTERN (self));

  GList *node;
  for (node = self->priv->machine->dst_wires; node; node = g_list_next (node)) {
    bt_value_group_serialize_columns (g_hash_table_lookup (self->priv->
            wire_value_groups, node->data), start_tick, end_tick, data);
  }
  bt_value_group_serialize_columns (self->priv->global_value_group, start_tick,
      end_tick, data);
  const gulong voices = self->priv->voices;
  gulong j;
  for (j = 0; j < voices; j++) {
    bt_value_group_serialize_columns (self->priv->voice_value_groups[j],
        start_tick, end_tick, data);
  }
}

//-- io interface

static xmlNodePtr
bt_pattern_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node)
{
  const BtPattern *const self = BT_PATTERN (persistence);;
  xmlNodePtr node = NULL;
  xmlNodePtr child_node, child_node2;

  GST_DEBUG ("PERSISTENCE::pattern");

  if ((node = xmlNewChild (parent_node, NULL, XML_CHAR_PTR ("pattern"), NULL))) {
    const gulong length = self->priv->length;
    const gulong voices = self->priv->voices;
    gulong global_params, voice_params;
    BtParameterGroup *pg;
    gulong i, j, k;
    gchar *value, *name;

    g_object_get ((GObject *) self, "name", &name, NULL);
    g_object_get ((gpointer) (self->priv->machine), "global-params",
        &global_params, "voice-params", &voice_params, NULL);

    xmlNewProp (node, XML_CHAR_PTR ("name"), XML_CHAR_PTR (name));
    xmlNewProp (node, XML_CHAR_PTR ("length"),
        XML_CHAR_PTR (bt_str_format_ulong (length)));
    g_free (name);

    // save pattern data
    for (i = 0; i < length; i++) {
      // check if there are any GValues stored ?
      if (bt_pattern_test_tick (self, i)) {
        child_node = xmlNewChild (node, NULL, XML_CHAR_PTR ("tick"), NULL);
        xmlNewProp (child_node, XML_CHAR_PTR ("time"),
            XML_CHAR_PTR (bt_str_format_ulong (i)));
        // save tick data
        pg = bt_machine_get_global_param_group (self->priv->machine);
        for (k = 0; k < global_params; k++) {
          if ((value = bt_pattern_get_global_event (self, i, k))) {
            child_node2 =
                xmlNewChild (child_node, NULL, XML_CHAR_PTR ("globaldata"),
                NULL);
            xmlNewProp (child_node2, XML_CHAR_PTR ("name"),
                XML_CHAR_PTR (bt_parameter_group_get_param_name (pg, k)));
            xmlNewProp (child_node2, XML_CHAR_PTR ("value"),
                XML_CHAR_PTR (value));
            g_free (value);
          }
        }
        for (j = 0; j < voices; j++) {
          const gchar *const voice_str = bt_str_format_ulong (j);
          pg = bt_machine_get_voice_param_group (self->priv->machine, j);
          for (k = 0; k < voice_params; k++) {
            if ((value = bt_pattern_get_voice_event (self, i, j, k))) {
              child_node2 =
                  xmlNewChild (child_node, NULL, XML_CHAR_PTR ("voicedata"),
                  NULL);
              xmlNewProp (child_node2, XML_CHAR_PTR ("voice"),
                  XML_CHAR_PTR (voice_str));
              xmlNewProp (child_node2, XML_CHAR_PTR ("name"),
                  XML_CHAR_PTR (bt_parameter_group_get_param_name (pg, k)));
              xmlNewProp (child_node2, XML_CHAR_PTR ("value"),
                  XML_CHAR_PTR (value));
              g_free (value);
            }
          }
        }
      }
    }
  }
  return node;
}

static BtPersistence *
bt_pattern_persistence_load (const GType type,
    const BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  BtPattern *self;
  BtPersistence *result;
  xmlChar *id, *name, *length_str, *tick_str, *value, *voice_str;
  glong tick, voice, param;
  gulong length;
  xmlNodePtr child_node;

  GST_DEBUG ("PERSISTENCE::pattern");
  g_assert (node);

  id = xmlGetProp (node, XML_CHAR_PTR ("id"));
  name = xmlGetProp (node, XML_CHAR_PTR ("name"));
  length_str = xmlGetProp (node, XML_CHAR_PTR ("length"));
  length = length_str ? atol ((char *) length_str) : 0;

  if (!persistence) {
    BtSong *song = NULL;
    BtMachine *machine = NULL;
    gchar *param_name;

    // we need to get parameters from var_args (need to handle all baseclass params
    param_name = va_arg (var_args, gchar *);
    while (param_name) {
      if (!strcmp (param_name, "song")) {
        song = va_arg (var_args, gpointer);
      } else if (!strcmp (param_name, "machine")) {
        machine = va_arg (var_args, gpointer);
      } else {
        GST_WARNING ("unhandled argument: %s", param_name);
        break;
      }
      param_name = va_arg (var_args, gchar *);
    }

    self = bt_pattern_new (song, (gchar *) name, length, machine);
    result = BT_PERSISTENCE (self);
  } else {
    self = BT_PATTERN (persistence);
    result = BT_PERSISTENCE (self);

    g_object_set (self, "name", name, "length", length, NULL);
  }
  if (id) {
    GST_INFO ("have legacy pattern id '%s'", id);
    g_object_set_data_full ((GObject *) self, "BtPattern::id", id,
        (GDestroyNotify) xmlFree);
  }
  xmlFree (name);
  xmlFree (length_str);

  GST_DEBUG ("PERSISTENCE::pattern loading data");

  // load pattern data
  for (node = node->children; node; node = node->next) {
    if ((!xmlNodeIsText (node))
        && (!strncmp ((gchar *) node->name, "tick\0", 5))) {
      tick_str = xmlGetProp (node, XML_CHAR_PTR ("time"));
      tick = atoi ((char *) tick_str);
      // iterate over children
      for (child_node = node->children; child_node;
          child_node = child_node->next) {
        if (!xmlNodeIsText (child_node)) {
          name = xmlGetProp (child_node, XML_CHAR_PTR ("name"));
          value = xmlGetProp (child_node, XML_CHAR_PTR ("value"));
          //GST_LOG("     \"%s\" -> \"%s\"",safe_string(name),safe_string(value));
          if (!strncmp ((char *) child_node->name, "globaldata\0", 11)) {
            param =
                bt_parameter_group_get_param_index
                (bt_machine_get_global_param_group (self->priv->machine),
                (gchar *) name);
            if (param != -1) {
              bt_pattern_set_global_event (self, tick, param, (gchar *) value);
            } else {
              GST_WARNING
                  ("error while loading global pattern data at tick %ld, param %ld",
                  tick, param);
            }
          } else if (!strncmp ((char *) child_node->name, "voicedata\0", 10)) {
            voice_str = xmlGetProp (child_node, XML_CHAR_PTR ("voice"));
            voice = atol ((char *) voice_str);
            if (voice < self->priv->voices) {
              param =
                  bt_parameter_group_get_param_index
                  (bt_machine_get_voice_param_group (self->priv->machine,
                      voice), (gchar *) name);
              if (param != -1) {
                bt_pattern_set_voice_event (self, tick, voice, param,
                    (gchar *) value);
              } else {
                GST_WARNING
                    ("error while loading voice pattern data at tick %ld, param %ld, voice %ld",
                    tick, param, voice);
              }
            } else {
              GST_WARNING ("voice %ld > max_voices %lu", voice,
                  self->priv->voices);
            }
            xmlFree (voice_str);
          }
          xmlFree (name);
          xmlFree (value);
        }
      }
      xmlFree (tick_str);
    }
  }

  return result;
}

static void
bt_pattern_persistence_interface_init (gpointer g_iface, gpointer iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_pattern_persistence_load;
  iface->save = bt_pattern_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void
bt_pattern_constructed (GObject * object)
{
  BtPattern *self = BT_PATTERN (object);
  BtSetup *setup;
  GList *node;
  BtWire *wire;

  G_OBJECT_CLASS (bt_pattern_parent_class)->constructed (object);

  /* fetch pointers from base-class */
  g_object_get (self, "song", &self->priv->song, "machine",
      &self->priv->machine, NULL);
  g_object_try_weak_ref (self->priv->song);
  g_object_unref (self->priv->song);
  g_object_try_weak_ref (self->priv->machine);
  g_object_unref (self->priv->machine);

  g_return_if_fail (BT_IS_SONG (self->priv->song));
  g_return_if_fail (BT_IS_MACHINE (self->priv->machine));

  GST_DEBUG ("set the machine for pattern: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->machine));

  self->priv->global_value_group =
      new_value_group (self,
      bt_machine_get_global_param_group (self->priv->machine));
  // add initial voices
  bt_pattern_on_voices_changed (self->priv->machine, NULL, (gpointer) self);
  // add initial wires
  for (node = self->priv->machine->dst_wires; node; node = g_list_next (node)) {
    wire = BT_WIRE (node->data);
    g_hash_table_insert (self->priv->wire_value_groups, wire,
        new_value_group (self, bt_wire_get_param_group (wire)));
  }
  self->priv->_src = NULL;

  g_signal_connect_object (self->priv->machine, "notify::voices",
      G_CALLBACK (bt_pattern_on_voices_changed), (gpointer) self, 0);
  g_object_get (self->priv->song, "setup", &setup, NULL);
  g_signal_connect_object (setup, "wire-added",
      G_CALLBACK (bt_pattern_on_setup_wire_added), (gpointer) self, 0);
  g_signal_connect_object (setup, "wire-removed",
      G_CALLBACK (bt_pattern_on_setup_wire_removed), (gpointer) self,
      G_CONNECT_AFTER);
  g_object_unref (setup);

  GST_DEBUG ("add pattern to machine");
  // add the pattern to the machine
  bt_machine_add_pattern (self->priv->machine, (BtCmdPattern *) self);
}

static void
bt_pattern_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtPattern *const self = BT_PATTERN (object);

  return_if_disposed ();
  switch (property_id) {
    case PATTERN_LENGTH:
      g_value_set_ulong (value, self->priv->length);
      break;
    case PATTERN_VOICES:
      g_value_set_ulong (value, self->priv->voices);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  BtPattern *const self = BT_PATTERN (object);

  return_if_disposed ();
  switch (property_id) {
    case PATTERN_LENGTH:{
      const gulong length = self->priv->length;
      self->priv->length = g_value_get_ulong (value);
      if (length != self->priv->length) {
        GST_DEBUG ("set the length for pattern: %lu", self->priv->length);
        bt_pattern_resize_data_length (self);
      }
      break;
    }
    case PATTERN_COPY_SOURCE:
      self->priv->_src = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_dispose (GObject * const object)
{
  const BtPattern *const self = BT_PATTERN (object);
  gulong i;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->global_value_group);
  for (i = 0; i < self->priv->voices; i++) {
    g_object_try_unref (self->priv->voice_value_groups[i]);
  }
  g_hash_table_destroy (self->priv->wire_value_groups);
  g_hash_table_destroy (self->priv->param_to_value_groups);

  g_object_try_weak_unref (self->priv->song);
  g_object_try_weak_unref (self->priv->machine);

  G_OBJECT_CLASS (bt_pattern_parent_class)->dispose (object);
}

static void
bt_pattern_finalize (GObject * const object)
{
  const BtPattern *const self = BT_PATTERN (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->voice_value_groups);

  G_OBJECT_CLASS (bt_pattern_parent_class)->finalize (object);
}

//-- class internals

static void
bt_pattern_init (BtPattern * self)
{
  self->priv = bt_pattern_get_instance_private(self);
  self->priv->wire_value_groups =
      g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify) g_object_unref);
  self->priv->param_to_value_groups =
      g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify) g_object_unref);
}

static void
bt_pattern_class_init (BtPatternClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bt_pattern_constructed;
  gobject_class->set_property = bt_pattern_set_property;
  gobject_class->get_property = bt_pattern_get_property;
  gobject_class->dispose = bt_pattern_dispose;
  gobject_class->finalize = bt_pattern_finalize;

  /**
   * BtPattern::param-changed:
   * @self: the pattern object that emitted the signal
   * @param_group: the parameter group
   * @tick: the tick position inside the pattern
   * @param: the parameter index
   *
   * Signals that a param of this pattern has been changed.
   */
  signals[PARAM_CHANGED_EVENT] =
      g_signal_new ("param-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, bt_marshal_VOID__OBJECT_ULONG_ULONG, G_TYPE_NONE, 3,
      BT_TYPE_PARAMETER_GROUP, G_TYPE_ULONG, G_TYPE_ULONG);

  /**
   * BtPattern::group-changed:
   * @self: the pattern object that emitted the signal
   * @param_group: the parameter group
   * @intermediate: flag that is %TRUE to signal that more change are coming
   *
   * Signals that a group of this pattern has been changed (more than in one
   * place).
   * When doing e.g. line inserts, one will receive two updates, one before and
   * one after. The first will have @intermediate=%TRUE. Applications can use
   * that to defer change-consolidation.
   */
  signals[GROUP_CHANGED_EVENT] =
      g_signal_new ("group-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, bt_marshal_VOID__OBJECT_BOOLEAN, G_TYPE_NONE, 2,
      BT_TYPE_PARAMETER_GROUP, G_TYPE_BOOLEAN);

  /**
   * BtPattern::pattern-changed:
   * @self: the pattern object that emitted the signal
   * @intermediate: flag that is %TRUE to signal that more change are coming
   *
   * Signals that this pattern has been changed (more than in one place).
   * When doing e.g. line inserts, one will receive two updates, one before and
   * one after. The first will have @intermediate=%TRUE. Applications can use
   * that to defer change-consolidation.
   */
  signals[PATTERN_CHANGED_EVENT] =
      g_signal_new ("pattern-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  g_object_class_install_property (gobject_class, PATTERN_LENGTH,
      g_param_spec_ulong ("length",
          "length prop",
          "length of the pattern in ticks",
          0,
          G_MAXULONG,
          1, G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PATTERN_VOICES,
      g_param_spec_ulong ("voices",
          "voices prop",
          "number of voices in the pattern",
          0, G_MAXULONG, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PATTERN_COPY_SOURCE,
      g_param_spec_object ("copy-source",
          "pattern copy-source prop",
          "pattern to copy data from",
          BT_TYPE_PATTERN,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}
