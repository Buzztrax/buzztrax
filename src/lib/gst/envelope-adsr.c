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
 * @attack_time: the attack time in sec
 * @decay_time: the decay time in sec
 * @note_time: the duration of the note in sec
 * @release_time: the decay time in sec
 * @peak_level: peak volume level (0.0 -> 1.0)
 * @sustain_level: sustain volume level (0.0 -> 1.0)
 *
 * Initialize the envelope for a new cycle. @note_time is the length of the
 * note. @attack_time + @decay_time must be < @note_time otherwise they get
 * scaled down.
 */
void
gstbt_envelope_adsr_setup (GstBtEnvelopeADSR * self, gint samplerate,
    gdouble attack_time, gdouble decay_time, gdouble note_time,
    gdouble release_time, gdouble peak_level, gdouble sustain_level)
{
  GstBtEnvelope *base = (GstBtEnvelope *) self;
  GstTimedValueControlSource *cs = base->cs;
  guint64 attack, decay, sustain, release;

  /* reset states */
  base->value = 0.001;
  base->offset = G_GUINT64_CONSTANT (0);

  /* ensure a+d < s */
  if ((attack_time + decay_time) > note_time) {
    gdouble fc = note_time / (attack_time + decay_time);
    attack_time *= fc;
    decay_time *= fc;
  }

  /* samplerate will be one second */
  attack = samplerate * attack_time;
  decay = attack + samplerate * decay_time;
  sustain = samplerate * note_time;
  release = sustain + samplerate * release_time;
  base->length = release;

  /* configure envelope */
  gst_timed_value_control_source_unset_all (cs);
  gst_timed_value_control_source_set (cs, G_GUINT64_CONSTANT (0), 0.0);
  gst_timed_value_control_source_set (cs, attack, peak_level);
  gst_timed_value_control_source_set (cs, decay, sustain_level);
  gst_timed_value_control_source_set (cs, sustain, sustain_level);
  gst_timed_value_control_source_set (cs, release, 0.0);
}

//-- virtual methods

static void
gstbt_envelope_adsr_init (GstBtEnvelopeADSR * self)
{
}

static void
gstbt_envelope_adsr_class_init (GstBtEnvelopeADSRClass * klass)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");
}
