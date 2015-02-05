/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-adsr.c: attack-decay-sustain-release envelope generator
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
 * SECTION:envelope-adsr
 * @title: GstBtEnvelopeASDR
 * @include: libgstbuzztrax/envelope-adsr.h
 * @short_description: attack-decay-sustain-release envelope generator
 *
 * Classic attack-decay-sustain-release envelope.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "envelope-adsr.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  PROP_NOTE_LENGTH = 1,
  PROP_ATTACK,
  PROP_PEAK_VOLUME,
  PROP_DECAY,
  PROP_SUSTAIN_VOLUME,
  PROP_RELEASE
};

//-- the class

G_DEFINE_TYPE (GstBtEnvelopeADSR, gstbt_envelope_adsr, GSTBT_TYPE_ENVELOPE);

//-- constructor methods

/**
 * gstbt_envelope_adsr_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtEnvelopeADSR *
gstbt_envelope_adsr_new (void)
{
  return GSTBT_ENVELOPE_ADSR (g_object_new (GSTBT_TYPE_ENVELOPE_ADSR, NULL));
}

//-- private methods

//-- public methods

/**
 * gstbt_envelope_adsr_setup:
 * @self: the envelope
 * @samplerate: the audio sampling rate
 * @ticktime: clocktime for one tick
 *
 * Initialize the envelope for a new cycle.
 */
void
gstbt_envelope_adsr_setup (GstBtEnvelopeADSR * self, gint samplerate,
    GstClockTime ticktime)
{
  GstBtEnvelope *base = (GstBtEnvelope *) self;
  GstTimedValueControlSource *cs = base->cs;
  guint64 attack, decay, sustain, release;
  gdouble note_time = (gdouble) (self->note_length * ticktime) /
      (gdouble) GST_SECOND;
  gdouble fc = 1.0;

  /* reset states */
  base->value = 0.001;
  base->offset = G_GUINT64_CONSTANT (0);

  /* ensure a+d < s */
  if ((self->attack + self->decay) > note_time) {
    fc = note_time / (self->attack + self->decay);
  }

  /* samplerate will be one second */
  attack = samplerate * (self->attack * fc);
  decay = attack + samplerate * (self->decay * fc);
  sustain = samplerate * note_time;
  release = sustain + samplerate * self->release;
  base->length = release;

  /* configure envelope */
  gst_timed_value_control_source_unset_all (cs);
  gst_timed_value_control_source_set (cs, G_GUINT64_CONSTANT (0), 0.0);
  gst_timed_value_control_source_set (cs, attack, self->peak_volume);
  gst_timed_value_control_source_set (cs, decay, self->sustain_volume);
  gst_timed_value_control_source_set (cs, sustain, self->sustain_volume);
  gst_timed_value_control_source_set (cs, release, 0.0);
}

//-- virtual methods

static void
gstbt_envelope_adsr_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEnvelopeADSR *self = GSTBT_ENVELOPE_ADSR (object);

  switch (prop_id) {
    case PROP_NOTE_LENGTH:
      self->note_length = g_value_get_uint (value);
      break;
    case PROP_ATTACK:
      self->attack = g_value_get_double (value);
      break;
    case PROP_PEAK_VOLUME:
      self->peak_volume = g_value_get_double (value);
      break;
    case PROP_DECAY:
      self->decay = g_value_get_double (value);
      break;
    case PROP_SUSTAIN_VOLUME:
      self->sustain_volume = g_value_get_double (value);
      break;
    case PROP_RELEASE:
      self->release = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_adsr_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEnvelopeADSR *self = GSTBT_ENVELOPE_ADSR (object);

  switch (prop_id) {
    case PROP_NOTE_LENGTH:
      g_value_set_uint (value, self->note_length);
      break;
    case PROP_ATTACK:
      g_value_set_double (value, self->attack);
      break;
    case PROP_PEAK_VOLUME:
      g_value_set_double (value, self->peak_volume);
      break;
    case PROP_DECAY:
      g_value_set_double (value, self->decay);
      break;
    case PROP_SUSTAIN_VOLUME:
      g_value_set_double (value, self->sustain_volume);
      break;
    case PROP_RELEASE:
      g_value_set_double (value, self->release);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_adsr_init (GstBtEnvelopeADSR * self)
{
  /* set base parameters */
  self->note_length = 1;
  self->attack = 0.1;
  self->peak_volume = 0.8;
  self->decay = 0.5;
  self->sustain_volume = 0.4;
  self->release = 0.5;
}

static void
gstbt_envelope_adsr_class_init (GstBtEnvelopeADSRClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gstbt_envelope_adsr_set_property;
  gobject_class->get_property = gstbt_envelope_adsr_get_property;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");

  g_object_class_install_property (gobject_class, PROP_NOTE_LENGTH,
      g_param_spec_uint ("length", "Note length", "Note length in ticks",
          1, 255, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ATTACK,
      g_param_spec_double ("attack", "Attack",
          "Volume attack of the tone in seconds", 0.001, 4.0, 0.1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PEAK_VOLUME,
      g_param_spec_double ("peak-volume", "Peak Volume", "Peak volume of tone",
          0.0, 1.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DECAY,
      g_param_spec_double ("decay", "Decay",
          "Volume decay of the tone in seconds", 0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SUSTAIN_VOLUME,
      g_param_spec_double ("sustain-volume", "Sustain Volume",
          "Sustain volume of tone", 0.0, 1.0, 0.4,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RELEASE,
      g_param_spec_double ("release", "Release",
          "Volume release of the tone in seconds", 0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}
