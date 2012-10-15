/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btpatterncontrolsource
 * @short_description: Custom controlsource based on repeated event blocks
 * (#BtPatterns).
 *
 * The control source will update machine parameters over time, based on the
 * events from the sequences and the patterns. One control-source will handle
 * one single parameter. It implements the logic of computing the parameter
 * value for a given time, taking multiple tracks and overlapping patterns into
 * account.
 *
 * At the begin of the timeline (ts==0) all parameters that have no value in the
 * sequence will be initialized from #BtPatternControlSource:default-value. For
 * trigger parameter this usualy is the no-value. For other parameters it is the
 * last value one has set in the ui or via interaction controller.
 */
/* TODO(ensonic): create a variant for trigger parameters?
 * - these don't search in the sequence and they have different default_values
 */
/* TODO(ensonic): only create controllers on machines that have tracks
 * - parameter-group could connect to sequence::track-{added,removed}?
 */

#define BT_CORE
#define BT_PATTERN_CONTROL_SOURCE_C

#include "core_private.h"

// -- property ids

enum
{
  PATTERN_CONTROL_SOURCE_SEQUENCE = 1,
  PATTERN_CONTROL_SOURCE_MACHINE,
  PATTERN_CONTROL_SOURCE_PARAMETER_GROUP,
  PATTERN_CONTROL_SOURCE_DEFAULT_VALUE
};

struct _BtPatternControlSourcePrivate
{
  /* used to validate if dipose has run */
  gboolean dispose_has_run;

  /* Sequence this controlsource is bound to */
    G_POINTER_ALIAS (BtSequence *, sequence);
  /* Machine this controlsource is bound to */
    G_POINTER_ALIAS (BtMachine *, machine);
  /* parameter specific data */
    G_POINTER_ALIAS (BtParameterGroup *, param_group);
  /* type of the handled property */
  GType type;
  /* base-type of the handled property */
  GType base;

  glong param_index;
  GValue last_value, def_value;
  gboolean is_trigger;
};

//-- prototypes
static gboolean bt_pattern_control_source_bind (GstControlSource * self,
    GParamSpec * pspec);

static gboolean bt_pattern_control_source_get_value (BtPatternControlSource *
    self, GstClockTime timestamp, GValue * value);

static gboolean
bt_pattern_control_source_get_value_array (BtPatternControlSource * self,
    GstClockTime timestamp, GstValueArray * value_array);

//-- the class
G_DEFINE_TYPE (BtPatternControlSource, bt_pattern_control_source,
    GST_TYPE_CONTROL_SOURCE);

// -- g_object overrides
static void
bt_pattern_control_source_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtPatternControlSource *self = BT_PATTERN_CONTROL_SOURCE (object);
  return_if_disposed ();

  switch (property_id) {
    case PATTERN_CONTROL_SOURCE_SEQUENCE:
      g_value_set_object (value, self->priv->sequence);
      break;
    case PATTERN_CONTROL_SOURCE_MACHINE:
      g_value_set_object (value, self->priv->machine);
      break;
    case PATTERN_CONTROL_SOURCE_PARAMETER_GROUP:
      g_value_set_object (value, self->priv->param_group);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_control_source_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  BtPatternControlSource *self = BT_PATTERN_CONTROL_SOURCE (object);
  return_if_disposed ();

  switch (property_id) {
    case PATTERN_CONTROL_SOURCE_SEQUENCE:
      self->priv->sequence = BT_SEQUENCE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->sequence);
      GST_DEBUG ("set the sequence for the controlsource: %p",
          self->priv->sequence);
      break;
    case PATTERN_CONTROL_SOURCE_MACHINE:
      self->priv->machine = BT_MACHINE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->machine);
      GST_DEBUG ("set the machine for the controlsource: %p",
          self->priv->machine);
      break;
    case PATTERN_CONTROL_SOURCE_PARAMETER_GROUP:
      self->priv->param_group = BT_PARAMETER_GROUP (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->param_group);
      GST_DEBUG ("set the param group for the controlsource: %p",
          self->priv->param_group);
      break;
    case PATTERN_CONTROL_SOURCE_DEFAULT_VALUE:{
      GValue *new_value = g_value_get_pointer (value);
      g_value_copy (new_value, &self->priv->def_value);
      GST_DEBUG ("set the def_value for the controlsource: %p",
          self->priv->def_value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_control_source_dispose (GObject * const object)
{
  BtPatternControlSource *self = BT_PATTERN_CONTROL_SOURCE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  g_object_try_weak_unref (self->priv->machine);
  g_object_try_weak_unref (self->priv->param_group);
  g_object_try_weak_unref (self->priv->sequence);

  GST_DEBUG (" chaining up");
  G_OBJECT_CLASS (bt_pattern_control_source_parent_class)->dispose (object);
  GST_DEBUG (" done");
}

static void
bt_pattern_control_source_finalize (GObject * const object)
{
  BtPatternControlSource *self = BT_PATTERN_CONTROL_SOURCE (object);

  g_value_unset (&self->priv->last_value);
  g_value_unset (&self->priv->def_value);

  G_OBJECT_CLASS (bt_pattern_control_source_parent_class)->finalize (object);
}

/**
 * bt_pattern_control_source_new:
 * @sequence: the songs sequence
 * @machine: the machine
 * @param_group: the parameter group
 *
 * Create a pattern control source for the given @machine and @param_group. Use
 * gst_control_source_bind() to attach it to one specific parameter of the
 * @param_group.
 *
 * Returns: the new pattern control source
 */
BtPatternControlSource *
bt_pattern_control_source_new (BtSequence * sequence, const BtMachine * machine,
    BtParameterGroup * param_group)
{
  return g_object_new (BT_TYPE_PATTERN_CONTROL_SOURCE, "sequence", sequence,
      "machine", machine, "parameter-group", param_group, NULL);
}

// -- class internals
static void
bt_pattern_control_source_init (BtPatternControlSource * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_PATTERN_CONTROL_SOURCE,
      BtPatternControlSourcePrivate);
}

static void
bt_pattern_control_source_class_init (BtPatternControlSourceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  GstControlSourceClass *control_source_class = (GstControlSourceClass *) klass;

  g_type_class_add_private (klass, sizeof (BtPatternControlSourcePrivate));

  control_source_class->bind = bt_pattern_control_source_bind;

  gobject_class->get_property = bt_pattern_control_source_get_property;
  gobject_class->set_property = bt_pattern_control_source_set_property;
  gobject_class->dispose = bt_pattern_control_source_dispose;
  gobject_class->finalize = bt_pattern_control_source_finalize;

  g_object_class_install_property (gobject_class,
      PATTERN_CONTROL_SOURCE_SEQUENCE, g_param_spec_object ("sequence",
          "sequence construct prop",
          "the sequence object, the controlsource belongs to",
          BT_TYPE_SEQUENCE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_CONTROL_SOURCE_MACHINE, g_param_spec_object ("machine",
          "machine construct prop",
          "the machine object, the controlsource belongs to",
          BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_CONTROL_SOURCE_PARAMETER_GROUP,
      g_param_spec_object ("parameter-group", "parameter group construct prop",
          "the parameter group", BT_TYPE_PARAMETER_GROUP,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_CONTROL_SOURCE_DEFAULT_VALUE,
      g_param_spec_pointer ("default-value", "default value prop",
          "pointer to value to use if no other found",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}

static gboolean
bt_pattern_control_source_bind (GstControlSource * source, GParamSpec * pspec)
{
  GType type, base;
  BtPatternControlSource *self = BT_PATTERN_CONTROL_SOURCE (source);
  BtParameterGroup *pg = self->priv->param_group;

  /* get the fundamental base type */
  self->priv->type = base = type = G_PARAM_SPEC_VALUE_TYPE (pspec);
  while ((type = g_type_parent (type)))
    base = type;
  self->priv->base = base;

  GST_DEBUG_OBJECT (self->priv->machine, "type %s, base %s",
      g_type_name (self->priv->type), g_type_name (self->priv->base));

  source->get_value =
      (GstControlSourceGetValue) bt_pattern_control_source_get_value;
  source->get_value_array =
      (GstControlSourceGetValueArray) bt_pattern_control_source_get_value_array;

  /* assign param index from pspec */
  self->priv->param_index =
      bt_parameter_group_get_param_index (pg, pspec->name);
  self->priv->is_trigger =
      bt_parameter_group_is_param_trigger (pg, self->priv->param_index);

  g_value_init (&self->priv->last_value, self->priv->type);
  g_value_init (&self->priv->def_value, self->priv->type);
  if (self->priv->is_trigger) {
    g_value_copy (bt_parameter_group_get_param_no_value (pg,
            self->priv->param_index), &self->priv->def_value);
  } else {
    g_param_value_set_default (pspec, &self->priv->def_value);
  }
  g_value_copy (&self->priv->def_value, &self->priv->last_value);

  return TRUE;
}

/* bt_pattern_control_source_get_value:
 *
 * Determine the property value for the given timestamp.
 *
 * Returns: %TRUE if the value was successfully calculated
 */
static gboolean
bt_pattern_control_source_get_value (BtPatternControlSource * self,
    GstClockTime timestamp, GValue * value)
{
  BtSequence *sequence = self->priv->sequence;
  BtMachine *machine = self->priv->machine;
  BtParameterGroup *pg = self->priv->param_group;
  glong param_index = self->priv->param_index;
  gboolean ret = FALSE;
  GValue *res = NULL;

  GstClockTime bar_time = bt_sequence_get_bar_time (sequence);
  GstClockTime tick = timestamp / bar_time;
  GstClockTime ts = tick * bar_time;

  // Don't check patterns on a subtick
  if (ts == timestamp) {
    glong i = -1, l, length;
    BtCmdPattern *pattern;
    BtValueGroup *vg;
    GValue *cur;

    g_object_get (sequence, "length", &length, NULL);
    if (tick >= length) {
      // no need to search for positions > length
      // when we do idle-play, do we want to have the settings from the:
      // - end of the song
      // - start of the song  (now, as it is faster)
      // - last stop position (not available right now)
      tick = (length > 0) ? (length - 1) : 0;
      tick = 0;
      GST_DEBUG_OBJECT (machine, "idle mode detected, using defaults");
    }
    // look up track machines that match this controlsource machine
    while ((i = bt_sequence_get_track_by_machine (sequence, machine, i + 1))
        != -1) {
      // check what pattern plays at tick or upwards
      for (l = tick, pattern = NULL; l >= 0; l--) {
        if ((pattern = bt_sequence_get_pattern (sequence, l, i)))
          break;
      }
      // check if valid pattern
      if (BT_IS_PATTERN (pattern)) {
        gulong len, pos = (glong) tick - l;
        // get length of pattern
        g_object_get (pattern, "length", &len, NULL);
        if (pos < len) {
          // store pattern and position
          vg = bt_pattern_get_group_by_parameter_group (
              (BtPattern *) pattern, pg);
          // get value at tick if set
          if ((cur = bt_value_group_get_event_data (vg, pos, param_index))
              && BT_IS_GVALUE (cur)) {
            res = cur;
          }
        }
      }
    }
  } else {
    GST_LOG_OBJECT (self->priv->machine, "skipping subtick");
  }
  // if we found a value, set it
  if (res) {
    GST_LOG_OBJECT (self->priv->machine,
        "tick %lld: Set value for %s", tick,
        bt_parameter_group_get_param_name (pg, param_index));

    g_value_copy (res, value);
    g_value_copy (value, &self->priv->last_value);
    ret = TRUE;
  } else {
    if (self->priv->is_trigger || !timestamp) {
      // set defaults value for triggers (no value) or the default at ts=0
      GST_LOG_OBJECT (self->priv->machine,
          "tick %lld: Set default for %s", tick,
          bt_parameter_group_get_param_name (pg, param_index));

      g_value_copy (&self->priv->def_value, value);
      g_value_copy (value, &self->priv->last_value);
      ret = TRUE;
    }
  }
  return ret;
}

static gboolean
bt_pattern_control_source_get_value_array (BtPatternControlSource * self,
    GstClockTime timestamp, GstValueArray * value_array)
{
  // we need to fill an array as the volume element is applying sample based
  // volume changes
  gint i;
  GValue value = { 0, };

  g_value_init (&value, self->priv->type);
  if (!bt_pattern_control_source_get_value (self, timestamp, &value)) {
    g_value_copy (&self->priv->last_value, &value);
  }
  switch (self->priv->base) {
    case G_TYPE_BOOLEAN:{
      gboolean val = g_value_get_boolean (&value);
      gboolean *values = (gboolean *) value_array->values;
      for (i = 0; i < value_array->nbsamples; i++)
        *values++ = val;
      break;
    }
    case G_TYPE_FLOAT:{
      gfloat val = g_value_get_float (&value);
      gfloat *values = (gfloat *) value_array->values;
      for (i = 0; i < value_array->nbsamples; i++)
        *values++ = val;
      break;
    }
    case G_TYPE_DOUBLE:{
      gdouble val = g_value_get_double (&value);
      gdouble *values = (gdouble *) value_array->values;
      for (i = 0; i < value_array->nbsamples; i++)
        *values++ = val;
      break;
    }
    default:
      GST_WARNING ("implement me for type %s", g_type_name (self->priv->base));
      break;
  }
  g_value_unset (&value);
  return TRUE;
}
