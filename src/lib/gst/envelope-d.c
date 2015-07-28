/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-d.c: decay envelope generator
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
 * SECTION:envelope-d
 * @title: GstBtEnvelopeD
 * @include: libgstbuzztrax/envelope-d.h
 * @short_description: decay envelope generator
 *
 * Simple decay envelope. Does a linear fade between #GstBtEnvelopeD:peak-level
 * and #GstBtEnvelopeD:floor-level by default (#GstBtEnvelopeD:curve = 0.5). For
 * smaller values of #GstBtEnvelopeD:curve the transition starts quicker and
 * then slows down and for values > than 0.5 it is the other way around.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "envelope-d.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_PEAK_LEVEL = 1,
  PROP_DECAY,
  PROP_FLOOR_LEVEL,
  PROP_CURVE,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

//-- the class

G_DEFINE_TYPE (GstBtEnvelopeD, gstbt_envelope_d, GSTBT_TYPE_ENVELOPE);

//-- constructor methods

/**
 * gstbt_envelope_d_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelopeD *
gstbt_envelope_d_new (void)
{
  return GSTBT_ENVELOPE_D (g_object_new (GSTBT_TYPE_ENVELOPE_D, NULL));
}

//-- private methods

//-- public methods

/**
 * gstbt_envelope_d_setup:
 * @self: the envelope
 * @samplerate: the audio sampling rate
 *
 * Initialize the envelope for a new cycle.
 */
void
gstbt_envelope_d_setup (GstBtEnvelopeD * self, gint samplerate)
{
  GstBtEnvelope *base = (GstBtEnvelope *) self;
  GstTimedValueControlSource *cs = base->cs;
  guint64 decay;
  gboolean use_curve = (self->curve != 0.5);

  /* reset states */
  base->value = self->peak_level;
  base->offset = G_GUINT64_CONSTANT (0);

  /* samplerate will be one second */
  decay = samplerate * self->decay;
  base->length = decay;

  /* configure envelope */
  gst_timed_value_control_source_unset_all (cs);
  g_object_set (cs, "mode",
      use_curve ? GST_INTERPOLATION_MODE_CUBIC : GST_INTERPOLATION_MODE_LINEAR,
      NULL);
  gst_timed_value_control_source_set (cs, G_GUINT64_CONSTANT (0),
      self->peak_level);
  if (use_curve) {
    gst_timed_value_control_source_set (cs, self->curve * decay,
        self->peak_level + (1.0 - self->curve) *
        (self->floor_level - self->peak_level));
  }
  gst_timed_value_control_source_set (cs, decay, self->floor_level);
}

//-- virtual methods

static void
gstbt_envelope_d_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEnvelopeD *self = GSTBT_ENVELOPE_D (object);

  switch (prop_id) {
    case PROP_PEAK_LEVEL:
      self->peak_level = g_value_get_double (value);
      break;
    case PROP_DECAY:
      self->decay = g_value_get_double (value);
      break;
    case PROP_FLOOR_LEVEL:
      self->floor_level = g_value_get_double (value);
      break;
    case PROP_CURVE:
      self->curve = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_d_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEnvelopeD *self = GSTBT_ENVELOPE_D (object);

  switch (prop_id) {
    case PROP_PEAK_LEVEL:
      g_value_set_double (value, self->peak_level);
      break;
    case PROP_DECAY:
      g_value_set_double (value, self->decay);
      break;
    case PROP_FLOOR_LEVEL:
      g_value_set_double (value, self->floor_level);
      break;
    case PROP_CURVE:
      g_value_set_double (value, self->curve);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_d_init (GstBtEnvelopeD * self)
{
  /* set base parameters */
  self->peak_level = G_MAXDOUBLE;
  self->decay = 0.5;
  self->floor_level = 0.0;
  self->curve = 0.5;
}

static void
gstbt_envelope_d_class_init (GstBtEnvelopeDClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");

  gobject_class->set_property = gstbt_envelope_d_set_property;
  gobject_class->get_property = gstbt_envelope_d_get_property;

  // register own properties

  PROP (PEAK_LEVEL) = g_param_spec_double ("peak-level", "Peak level",
      "Highest level of the envelope", 0.0, G_MAXDOUBLE, G_MAXDOUBLE,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (DECAY) = g_param_spec_double ("decay", "Decay",
      "Decay of the envelope in seconds", 0.001, 4.0, 0.5,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (FLOOR_LEVEL) = g_param_spec_double ("floor-level", "Floor level",
      "Lowest level of the envelope", 0.0, G_MAXDOUBLE, 0.0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (CURVE) = g_param_spec_double ("curve", "Curve",
      "Curve of the envelope, 0.5=linear", 0.0, 1.0, 0.5,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
