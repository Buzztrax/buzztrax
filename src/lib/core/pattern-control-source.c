/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
  PATTERN_CONTROL_SOURCE_SONG_INFO,
  PATTERN_CONTROL_SOURCE_MACHINE,
  PATTERN_CONTROL_SOURCE_PARAMETER_GROUP,
  PATTERN_CONTROL_SOURCE_DEFAULT_VALUE
};

struct _BtPatternControlSourcePrivate
{
  /* used to validate if dipose has run */
  gboolean dispose_has_run;

  /* parameters */
  BtSequence *sequence;
  BtSongInfo *song_info;
  BtMachine *machine;
  BtParameterGroup *param_group;
  /* type of the handled property */
  GType type, base;

  glong param_index;
  GValue def_value;
  gboolean is_trigger;

  GstClockTime tick_duration;
};

//-- the class
G_DEFINE_TYPE_WITH_CODE (BtPatternControlSource, bt_pattern_control_source,
    GST_TYPE_CONTROL_BINDING,
    G_ADD_PRIVATE(BtPatternControlSource));

/**
 * bt_pattern_control_source_new:
 * @object: the object of the property
 * @property_name: the property-name to attach the control source
 * @sequence: the songs sequence
 * @song_info: the song info
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
bt_pattern_control_source_new (GstObject * object, const gchar * property_name,
    BtSequence * sequence, const BtSongInfo * song_info,
    const BtMachine * machine, BtParameterGroup * param_group)
{
  return g_object_new (BT_TYPE_PATTERN_CONTROL_SOURCE,
      "object", object, "name", property_name, "sequence", sequence,
      "song-info", song_info, "machine", machine, "parameter-group",
      param_group, NULL);
}

/* bt_pattern_control_source_get_value:
 *
 * Determine the property value for the given timestamp.
 *
 * Returns: the value for the given timestamp or %NULL
 */
static GValue *
bt_pattern_control_source_get_value (GstControlBinding * self_,
    GstClockTime timestamp)
{
  BtPatternControlSource *self = (BtPatternControlSource *) self_;
  BtSequence *sequence = self->priv->sequence;
  BtSongInfo *song_info = self->priv->song_info;
  BtMachine *machine = self->priv->machine;
  BtParameterGroup *pg = self->priv->param_group;
  glong param_index = self->priv->param_index;
  GValue *res = NULL;
  gulong tick = bt_song_info_time_to_tick (song_info, timestamp);
  GstClockTime ts = bt_song_info_tick_to_time (song_info, tick);

  GST_LOG_OBJECT (machine, "get control_value for param %ld at tick%4lu,"
      " %1d: %" G_GUINT64_FORMAT " == %" G_GUINT64_FORMAT,
      param_index, tick, (ts == timestamp), timestamp, ts);

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
      //tick = (length > 0) ? (length - 1) : 0;
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
      g_object_try_unref (pattern);
    }
  } else {
    GST_LOG_OBJECT (self->priv->machine, "skipping subtick");
  }
  // if we found a value, set it
  if (res) {
    GST_LOG_OBJECT (self->priv->machine,
        "tick %lu: Set value for %s", tick,
        bt_parameter_group_get_param_name (pg, param_index));

    return res;
  } else {
    if (self->priv->is_trigger || !timestamp) {
      // set defaults value for triggers or for all paramters at ts=0, for
      // triggers the default value should be the no-value
      GST_LOG_OBJECT (self->priv->machine,
          "tick %lu: Set default for %s", tick,
          bt_parameter_group_get_param_name (pg, param_index));

      return &self->priv->def_value;
    }
  }
  return NULL;
}

// we need to fill an array as the volume element is applying sample based changes
static gboolean
bt_pattern_control_source_get_value_array (GstControlBinding * self_,
    GstClockTime timestamp, GstClockTime interval, guint n_values,
    gpointer values_)
{
  BtPatternControlSource *self = (BtPatternControlSource *) self_;
  gint i;
  GValue *value;
  return_val_if_disposed (FALSE);

  if (!(value = bt_pattern_control_source_get_value (self_, timestamp))) {
    return FALSE;
  }
  switch (self->priv->base) {
    case G_TYPE_BOOLEAN:{
      gboolean val = g_value_get_boolean (value);
      gboolean *values = (gboolean *) values_;
      GST_INFO ("filling array with boolean: %d", val);
      for (i = 0; i < n_values; i++)
        *values++ = val;
      break;
    }
    case G_TYPE_FLOAT:{
      gfloat val = g_value_get_float (value);
      gfloat *values = (gfloat *) values_;
      GST_INFO ("filling array with floats: %f", val);
      for (i = 0; i < n_values; i++)
        *values++ = val;
      break;
    }
    case G_TYPE_DOUBLE:{
      gdouble val = g_value_get_double (value);
      gdouble *values = (gdouble *) values_;
      GST_INFO ("filling array with doubles: %lf", val);
      for (i = 0; i < n_values; i++)
        *values++ = val;
      break;
    }
    default:
      GST_WARNING ("implement me for type %s", g_type_name (self->priv->base));
      break;
  }
  return TRUE;
}

// we need to fill an array as the volume element is applying sample based changes
static gboolean
bt_pattern_control_source_get_g_value_array (GstControlBinding * self_,
    GstClockTime timestamp, GstClockTime interval, guint n_values,
    GValue * values)
{
  BtPatternControlSource *self = (BtPatternControlSource *) self_;
  gint i;
  GValue *value;
  return_val_if_disposed (FALSE);

  if (!(value = bt_pattern_control_source_get_value (self_, timestamp))) {
    return FALSE;
  }
  for (i = 0; i < n_values; i++) {
    g_value_init (&values[i], self->priv->type);
    g_value_copy (value, &values[i]);
  }
  return TRUE;
}

static gboolean
gst_pattern_control_source_sync_values (GstControlBinding * self_,
    GstObject * object, GstClockTime timestamp, GstClockTime last_sync)
{
  BtPatternControlSource *self = (BtPatternControlSource *) self_;
  GValue *value;
  return_val_if_disposed (FALSE);

  if ((value = bt_pattern_control_source_get_value (self_, timestamp))) {
    g_object_set_property ((GObject *) object, self_->name, value);
    return TRUE;
  } else {
    GST_LOG_OBJECT (object, "at %" GST_TIME_FORMAT
        ", no control value for param %s", GST_TIME_ARGS (timestamp),
        self_->name);
    return FALSE;
  }
}

// -- g_object overrides

static GObject *
bt_pattern_control_source_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  BtPatternControlSource *self;
  GParamSpec *pspec;

  self =
      BT_PATTERN_CONTROL_SOURCE (G_OBJECT_CLASS
      (bt_pattern_control_source_parent_class)->constructor (type,
          n_construct_params, construct_params));

  pspec = GST_CONTROL_BINDING_PSPEC (self);
  if (pspec) {
    BtParameterGroup *pg = self->priv->param_group;
    GType type;

    /* get the fundamental base type */
    self->priv->type = type = G_PARAM_SPEC_VALUE_TYPE (pspec);
    self->priv->base = bt_g_type_get_base_type (type);

    GST_DEBUG_OBJECT (self->priv->machine, "%s: type %s, base %s", pspec->name,
        g_type_name (type), g_type_name (self->priv->base));

    /* assign param index from pspec */
    self->priv->param_index =
        bt_parameter_group_get_param_index (pg, pspec->name);
    self->priv->is_trigger =
        bt_parameter_group_is_param_trigger (pg, self->priv->param_index);

    if (!G_IS_VALUE (&self->priv->def_value)) {
      g_value_init (&self->priv->def_value, type);
      if (self->priv->is_trigger) {
        g_value_copy (bt_parameter_group_get_param_no_value (pg,
                self->priv->param_index), &self->priv->def_value);
      } else {
        g_param_value_set_default (pspec, &self->priv->def_value);
      }
    }
  }
  return (GObject *) self;
}

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
    case PATTERN_CONTROL_SOURCE_SONG_INFO:
      g_value_set_object (value, self->priv->song_info);
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
    case PATTERN_CONTROL_SOURCE_SONG_INFO:
      self->priv->song_info = BT_SONG_INFO (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song_info);
      GST_DEBUG ("set the song-info for the controlsource: %p",
          self->priv->song_info);
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
      GST_INFO ("%s -> %s", G_VALUE_TYPE_NAME (new_value),
          G_VALUE_TYPE_NAME (&self->priv->def_value));
      GST_INFO ("%s -> %s", g_strdup_value_contents (new_value),
          g_strdup_value_contents (&self->priv->def_value));
      g_value_copy (new_value, &self->priv->def_value);
      GST_DEBUG ("set the def_value for the controlsource");
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
  self->priv->machine = NULL;
  g_object_try_weak_unref (self->priv->param_group);
  self->priv->param_group = NULL;
  g_object_try_weak_unref (self->priv->song_info);
  self->priv->song_info = NULL;
  g_object_try_weak_unref (self->priv->sequence);
  self->priv->sequence = NULL;

  GST_DEBUG (" chaining up");
  G_OBJECT_CLASS (bt_pattern_control_source_parent_class)->dispose (object);
  GST_DEBUG (" done");
}

static void
bt_pattern_control_source_finalize (GObject * const object)
{
  BtPatternControlSource *self = BT_PATTERN_CONTROL_SOURCE (object);

  if (G_IS_VALUE (&self->priv->def_value)) {
    g_value_unset (&self->priv->def_value);
  }

  G_OBJECT_CLASS (bt_pattern_control_source_parent_class)->finalize (object);
}

// -- class internals

static void
bt_pattern_control_source_init (BtPatternControlSource * self)
{
  self->priv = bt_pattern_control_source_get_instance_private(self);
}

static void
bt_pattern_control_source_class_init (BtPatternControlSourceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  GstControlBindingClass *control_binding_class =
      (GstControlBindingClass *) klass;

  control_binding_class->sync_values = gst_pattern_control_source_sync_values;
  control_binding_class->get_value = bt_pattern_control_source_get_value;
  control_binding_class->get_value_array =
      bt_pattern_control_source_get_value_array;
  control_binding_class->get_g_value_array =
      bt_pattern_control_source_get_g_value_array;

  gobject_class->constructor = bt_pattern_control_source_constructor;
  gobject_class->get_property = bt_pattern_control_source_get_property;
  gobject_class->set_property = bt_pattern_control_source_set_property;
  gobject_class->dispose = bt_pattern_control_source_dispose;
  gobject_class->finalize = bt_pattern_control_source_finalize;

  g_object_class_install_property (gobject_class,
      PATTERN_CONTROL_SOURCE_SEQUENCE, g_param_spec_object ("sequence",
          "sequence construct prop",
          "the sequence object",
          BT_TYPE_SEQUENCE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_CONTROL_SOURCE_SONG_INFO, g_param_spec_object ("song-info",
          "song-info construct prop",
          "the song-info object",
          BT_TYPE_SONG_INFO,
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
