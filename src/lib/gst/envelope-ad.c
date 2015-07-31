/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-ad.c: attack-decay envelope generator
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
 * SECTION:envelope-ad
 * @title: GstBtEnvelopeAD
 * @include: libgstbuzztrax/envelope-ad.h
 * @short_description: attack-decay envelope generator
 *
 * Simple attack-decay envelope.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "envelope-ad.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_ATTACK = 1,
  PROP_PEAK_LEVEL,
  PROP_DECAY,
  PROP_FLOOR_LEVEL,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

//-- the class

G_DEFINE_TYPE (GstBtEnvelopeAD, gstbt_envelope_ad, GSTBT_TYPE_ENVELOPE);

//-- constructor methods

/**
 * gstbt_envelope_ad_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelopeAD *
gstbt_envelope_ad_new (void)
{
  return GSTBT_ENVELOPE_AD (g_object_new (GSTBT_TYPE_ENVELOPE_AD, NULL));
}

//-- private methods

//-- public methods

/**
 * gstbt_envelope_ad_setup:
 * @self: the envelope
 * @samplerate: the audio sampling rate
 *
 * Initialize the envelope for a new cycle.
 */
void
gstbt_envelope_ad_setup (GstBtEnvelopeAD * self, gint samplerate)
{
  GstTimedValueControlSource *cs = (GstTimedValueControlSource *) self;
  gdouble attack_time = self->attack;
  guint64 attack, decay;

  /* ensure a < d */
  if (attack_time > self->decay) {
    attack_time = self->decay / 2.0;
  }

  /* samplerate will be one second */
  attack = samplerate * attack_time;
  decay = samplerate * self->decay;
  g_object_set (self, "length", decay, NULL);

  /* configure envelope */
  gst_timed_value_control_source_unset_all (cs);
  gst_timed_value_control_source_set (cs, G_GUINT64_CONSTANT (0),
      self->floor_level);
  gst_timed_value_control_source_set (cs, attack, self->peak_level);
  gst_timed_value_control_source_set (cs, decay, self->floor_level);
}

//-- virtual methods

static void
gstbt_envelope_ad_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEnvelopeAD *self = GSTBT_ENVELOPE_AD (object);

  switch (prop_id) {
    case PROP_ATTACK:
      self->attack = g_value_get_double (value);
      break;
    case PROP_PEAK_LEVEL:
      self->peak_level = g_value_get_double (value);
      break;
    case PROP_DECAY:
      self->decay = g_value_get_double (value);
      break;
    case PROP_FLOOR_LEVEL:
      self->floor_level = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_ad_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEnvelopeAD *self = GSTBT_ENVELOPE_AD (object);

  switch (prop_id) {
    case PROP_ATTACK:
      g_value_set_double (value, self->attack);
      break;
    case PROP_PEAK_LEVEL:
      g_value_set_double (value, self->peak_level);
      break;
    case PROP_DECAY:
      g_value_set_double (value, self->decay);
      break;
    case PROP_FLOOR_LEVEL:
      g_value_set_double (value, self->floor_level);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_ad_init (GstBtEnvelopeAD * self)
{
  /* set base parameters */
  self->attack = 0.001;
  self->peak_level = G_MAXDOUBLE;
  self->decay = 0.5;
  self->floor_level = 0.0;
}

static void
gstbt_envelope_ad_class_init (GstBtEnvelopeADClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");

  gobject_class->set_property = gstbt_envelope_ad_set_property;
  gobject_class->get_property = gstbt_envelope_ad_get_property;

  // register own properties

  PROP (ATTACK) = g_param_spec_double ("attack", "Attack",
      "Attack of the envelope in seconds", 0.001, 4.0, 0.001,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (PEAK_LEVEL) = g_param_spec_double ("peak-level", "Peak level",
      "Highest level of the envelope", 0.0, G_MAXDOUBLE, G_MAXDOUBLE,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (DECAY) = g_param_spec_double ("decay", "Decay",
      "Decay of the envelope in seconds", 0.001, 4.0, 0.5,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (FLOOR_LEVEL) = g_param_spec_double ("floor-level", "Floor level",
      "Lowest level of the envelope", 0.0, G_MAXDOUBLE, 0.0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
