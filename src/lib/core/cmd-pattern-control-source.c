/* Buzztrax
 * Copyright (C) 2015 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btcmdpatterncontrolsource
 * @short_description: Custom controlsource based on repeated event blocks
 * (#BtCmdPatterns).
 *
 * The control source will update machine parameters over time, based on the
 * events from the sequences and the patterns. One control-source will handle
 * one single parameter. It implements the logic of computing the parameter
 * value for a given time, taking multiple tracks and overlapping patterns into
 * account.
 *
 * At the begin of the timeline (ts==0) all parameters that have no value in the
 * sequence will be initialized from #BtCmdPatternControlSource:default-value.
 * This is the last value one has set in the ui or via interaction controller.
 */

#define BT_CORE
#define BT_CMD_PATTERN_CONTROL_SOURCE_C

#include "core_private.h"

// -- property ids

enum
{
  CMD_PATTERN_CONTROL_SOURCE_SEQUENCE = 1,
  CMD_PATTERN_CONTROL_SOURCE_SONG_INFO,
  CMD_PATTERN_CONTROL_SOURCE_MACHINE,
  CMD_PATTERN_CONTROL_SOURCE_DEFAULT_VALUE
};

struct _BtCmdPatternControlSourcePrivate
{
  /* used to validate if dipose has run */
  gboolean dispose_has_run;

  /* parameters */
  BtSequence *sequence;
  BtSongInfo *song_info;
  BtMachine *machine;

  GValue cur_value;
  BtMachineState def_state;

  GstClockTime tick_duration;
};

//-- the class
G_DEFINE_TYPE_WITH_CODE (BtCmdPatternControlSource, bt_cmd_pattern_control_source,
    GST_TYPE_CONTROL_BINDING,
    G_ADD_PRIVATE(BtCmdPatternControlSource));

/**
 * bt_cmd_pattern_control_source_new:
 * @object: the object of the property
 * @property_name: the property-name to attach the control source
 * @sequence: the songs sequence
 * @song_info: the song info
 * @machine: the machine
 *
 * Create a cmd-pattern control source for the given @machine. Use
 * gst_control_source_bind() to attach it to the related parameter of the
 * machine.
 *
 * Returns: the new cmd-pattern control source
 */
BtCmdPatternControlSource *
bt_cmd_pattern_control_source_new (GstObject * object,
    const gchar * property_name, BtSequence * sequence,
    const BtSongInfo * song_info, const BtMachine * machine)
{
  return g_object_new (BT_TYPE_CMD_PATTERN_CONTROL_SOURCE,
      "object", object, "name", property_name, "sequence", sequence,
      "song-info", song_info, "machine", machine, NULL);
}

/* get_value:
 *
 * Determine the property value for the given timestamp.
 *
 * Returns: the value for the given timestamp or -1
 */
static BtMachineState
get_value (GstControlBinding * self_, GstClockTime timestamp)
{
  BtCmdPatternControlSource *self = (BtCmdPatternControlSource *) self_;
  BtSequence *sequence = self->priv->sequence;
  BtSongInfo *song_info = self->priv->song_info;
  BtMachine *machine = self->priv->machine;
  gulong tick = bt_song_info_time_to_tick (song_info, timestamp);
  GstClockTime ts = bt_song_info_tick_to_time (song_info, tick);
  BtPatternCmd ret = -1, cmd;

  /* If we manually mute/solo/bypass from the UI, bypass the controller */
  if (self->priv->def_state != BT_MACHINE_STATE_NORMAL)
    return -1;

  GST_LOG_OBJECT (machine, "get control_value for pattern cmd at tick%4lu,"
      " %1d: %" G_GUINT64_FORMAT " == %" G_GUINT64_FORMAT,
      tick, (ts == timestamp), timestamp, ts);

  // Don't check patterns on a subtick
  if (ts == timestamp) {
    glong i = -1, l, length, min_l = 0;
    gulong len;
    BtCmdPattern *pattern;

    g_object_get (sequence, "length", &length, NULL);
    if (tick >= length) {
      // no need to search for positions > length
      // when we do idle-play, do we want to have the settings from the:
      // - end of the song
      // - start of the song  (now, as it is faster)
      // - last stop position (not available right now)
      //tick = (length > 0) ? (length - 1) : 0;
      tick = 0;
      GST_DEBUG_OBJECT (machine, "idle mode detected, using defaults");
    }
    // look up track machines that match this controlsource machine
    while ((i = bt_sequence_get_track_by_machine (sequence, machine, i + 1))
        != -1) {
      // check what pattern plays at tick or upwards
      for (l = tick, pattern = NULL; l >= min_l; l--) {
        if ((pattern = bt_sequence_get_pattern (sequence, l, i)))
          break;
      }
      if (!pattern)
        continue;
      if (BT_IS_PATTERN (pattern)) {
        g_object_get (pattern, "command", &cmd, "length", &len, NULL);
      } else {
        g_object_get (pattern, "command", &cmd, NULL);
        len = 1;
      }
      g_object_unref (pattern);
      if ((l + len) > min_l) {
        ret = cmd;
      }
      // in the next track we don't need to search any further
      min_l = l;
    }
  } else {
    GST_LOG_OBJECT (self->priv->machine, "skipping subtick");
  }
  if (ret != -1) {
    GST_LOG_OBJECT (self->priv->machine,
        "tick %lu: Set value for machine::state to %d", tick, ret);
    switch (ret) {
      case BT_PATTERN_CMD_NORMAL:
      case BT_PATTERN_CMD_BREAK:
        return BT_MACHINE_STATE_NORMAL;
      case BT_PATTERN_CMD_MUTE:
        return BT_MACHINE_STATE_MUTE;
      case BT_PATTERN_CMD_SOLO:
        return BT_MACHINE_STATE_SOLO;
      case BT_PATTERN_CMD_BYPASS:
        return BT_MACHINE_STATE_BYPASS;
      default:
        g_assert_not_reached ();
    }
  } else {
    if (!timestamp) {
      // set defaults value for paramters at ts=0
      GST_DEBUG_OBJECT (self->priv->machine,
          "tick %lu: Set default for machine:.state", tick);
      /* FIXME: need def-value handling:
       * - when changing the state from the UI, we need to store this as the
       *   default in main-page-sequence.c and machine-canvas-item.c
       * - see:
       *     machine-properties-dialog:bt_machine_update_default_param_value()
       *   this essentially calls
       *     bt_parameter_group_set_param_default(pg, param)
       *   which will copy the current value into the the def_value of the
       *   control_binding
       */
      return self->priv->def_state;
    }
  }
  return -1;
}

static GValue *
bt_cmd_pattern_control_source_get_value (GstControlBinding * self_,
    GstClockTime timestamp)
{
  BtCmdPatternControlSource *self = (BtCmdPatternControlSource *) self_;
  BtMachineState state = get_value (self_, timestamp);

  if (state != -1) {
    g_value_set_enum (&self->priv->cur_value, state);
    return &self->priv->cur_value;
  } else {
    return NULL;
  }
}

static gboolean
bt_cmd_pattern_control_source_get_value_array (GstControlBinding * self_,
    GstClockTime timestamp, GstClockTime interval, guint n_values,
    gpointer values_)
{
  return FALSE;
}

static gboolean
bt_cmd_pattern_control_source_get_g_value_array (GstControlBinding * self_,
    GstClockTime timestamp, GstClockTime interval, guint n_values,
    GValue * values)
{
  return FALSE;
}

static gboolean
gst_cmd_pattern_control_source_sync_values (GstControlBinding * self_,
    GstObject * object, GstClockTime timestamp, GstClockTime last_sync)
{
  BtCmdPatternControlSource *self = (BtCmdPatternControlSource *) self_;
  BtMachineState new_state, old_state;
  return_val_if_disposed (FALSE);

  if ((new_state = get_value (self_, timestamp)) != -1) {
    /* we only attach the cb to volume::mute to be called regularilly,
     * of couse for solo, this is not perfect :/ */
    g_object_get (self->priv->machine, "state", &old_state, NULL);
    /* avoid notify:: emissions */
    if (old_state != new_state) {
      g_object_set (self->priv->machine, "state", new_state, NULL);
    }
    return TRUE;
  } else {
    GST_LOG_OBJECT (object, "at %" GST_TIME_FORMAT
        ", no control value for machine::state", GST_TIME_ARGS (timestamp));
    return FALSE;
  }
}

// -- g_object overrides

static GObject *
bt_cmd_pattern_control_source_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  BtCmdPatternControlSource *self;

  self =
      BT_CMD_PATTERN_CONTROL_SOURCE (G_OBJECT_CLASS
      (bt_cmd_pattern_control_source_parent_class)->constructor (type,
          n_construct_params, construct_params));

  self->priv->def_state = BT_MACHINE_STATE_NORMAL;
  g_value_init (&self->priv->cur_value, BT_TYPE_MACHINE_STATE);
  return (GObject *) self;
}

static void
bt_cmd_pattern_control_source_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtCmdPatternControlSource *self = BT_CMD_PATTERN_CONTROL_SOURCE (object);
  return_if_disposed ();

  switch (property_id) {
    case CMD_PATTERN_CONTROL_SOURCE_SEQUENCE:
      g_value_set_object (value, self->priv->sequence);
      break;
    case CMD_PATTERN_CONTROL_SOURCE_SONG_INFO:
      g_value_set_object (value, self->priv->song_info);
      break;
    case CMD_PATTERN_CONTROL_SOURCE_MACHINE:
      g_value_set_object (value, self->priv->machine);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_cmd_pattern_control_source_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  BtCmdPatternControlSource *self = BT_CMD_PATTERN_CONTROL_SOURCE (object);
  return_if_disposed ();

  switch (property_id) {
    case CMD_PATTERN_CONTROL_SOURCE_SEQUENCE:
      self->priv->sequence = BT_SEQUENCE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->sequence);
      GST_DEBUG ("set the sequence for the controlsource: %p",
          self->priv->sequence);
      break;
    case CMD_PATTERN_CONTROL_SOURCE_SONG_INFO:
      self->priv->song_info = BT_SONG_INFO (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song_info);
      GST_DEBUG ("set the song-info for the controlsource: %p",
          self->priv->song_info);
      break;
    case CMD_PATTERN_CONTROL_SOURCE_MACHINE:
      self->priv->machine = BT_MACHINE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->machine);
      GST_DEBUG ("set the machine for the controlsource: %p",
          self->priv->machine);
      break;
    case CMD_PATTERN_CONTROL_SOURCE_DEFAULT_VALUE:{
      GValue *new_value = g_value_get_pointer (value);
      GST_INFO ("%d -> %d", self->priv->def_state,
          g_value_get_enum (new_value));
      self->priv->def_state = g_value_get_enum (new_value);
      GST_DEBUG ("set the def_value for the controlsource");
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_cmd_pattern_control_source_dispose (GObject * const object)
{
  BtCmdPatternControlSource *self = BT_CMD_PATTERN_CONTROL_SOURCE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  g_object_try_weak_unref (self->priv->machine);
  self->priv->machine = NULL;
  g_object_try_weak_unref (self->priv->song_info);
  self->priv->song_info = NULL;
  g_object_try_weak_unref (self->priv->sequence);
  self->priv->sequence = NULL;

  GST_DEBUG (" chaining up");
  G_OBJECT_CLASS (bt_cmd_pattern_control_source_parent_class)->dispose (object);
  GST_DEBUG (" done");
}

static void
bt_cmd_pattern_control_source_finalize (GObject * const object)
{
  BtCmdPatternControlSource *self = BT_CMD_PATTERN_CONTROL_SOURCE (object);

  if (G_IS_VALUE (&self->priv->cur_value)) {
    g_value_unset (&self->priv->cur_value);
  }

  G_OBJECT_CLASS (bt_cmd_pattern_control_source_parent_class)->finalize
      (object);
}

// -- class internals

static void
bt_cmd_pattern_control_source_init (BtCmdPatternControlSource * self)
{
  self->priv = bt_cmd_pattern_control_source_get_instance_private(self);
}

static void
bt_cmd_pattern_control_source_class_init (BtCmdPatternControlSourceClass *
    const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  GstControlBindingClass *control_binding_class =
      (GstControlBindingClass *) klass;

  control_binding_class->sync_values =
      gst_cmd_pattern_control_source_sync_values;
  control_binding_class->get_value = bt_cmd_pattern_control_source_get_value;
  control_binding_class->get_value_array =
      bt_cmd_pattern_control_source_get_value_array;
  control_binding_class->get_g_value_array =
      bt_cmd_pattern_control_source_get_g_value_array;

  gobject_class->constructor = bt_cmd_pattern_control_source_constructor;
  gobject_class->get_property = bt_cmd_pattern_control_source_get_property;
  gobject_class->set_property = bt_cmd_pattern_control_source_set_property;
  gobject_class->dispose = bt_cmd_pattern_control_source_dispose;
  gobject_class->finalize = bt_cmd_pattern_control_source_finalize;

  g_object_class_install_property (gobject_class,
      CMD_PATTERN_CONTROL_SOURCE_SEQUENCE, g_param_spec_object ("sequence",
          "sequence construct prop",
          "the sequence object",
          BT_TYPE_SEQUENCE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      CMD_PATTERN_CONTROL_SOURCE_SONG_INFO, g_param_spec_object ("song-info",
          "song-info construct prop",
          "the song-info object",
          BT_TYPE_SONG_INFO,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      CMD_PATTERN_CONTROL_SOURCE_MACHINE, g_param_spec_object ("machine",
          "machine construct prop",
          "the machine object, the controlsource belongs to",
          BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      CMD_PATTERN_CONTROL_SOURCE_DEFAULT_VALUE,
      g_param_spec_pointer ("default-value", "default value prop",
          "pointer to value to use if no other found",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}
