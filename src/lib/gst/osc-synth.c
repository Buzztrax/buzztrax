/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * osc-synth.c: synthetic waveform oscillator
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
 * SECTION:osc-synth
 * @title: GstBtOscSynth
 * @include: libgstbuzztrax/osc-synth.h
 * @short_description: synthetic waveform oscillator
 *
 * An audio generator producing classic oscillator waveforms.
 */
/* TODO(ensonic): we should do a linear fade down in the last inner_loop block as an
 * anticlick messure
 *   if(note_count+INNER_LOOP>=note_length) {
 *     ac_f=1.0;
 *     ac_s=1.0/INNER_LOOP;
 *   }
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "osc-synth.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define M_PI_M2 ( M_PI + M_PI )
#define INNER_LOOP 64

enum
{
  // static class properties
  PROP_SAMPLERATE = 1,
  PROP_VOLUME_ENVELOPE,
  // dynamic class properties
  PROP_WAVE,
  PROP_FREQUENCY
};

//-- the class

G_DEFINE_TYPE (GstBtOscSynth, gstbt_osc_synth, G_TYPE_OBJECT);

//-- enums

GType
gstbt_osc_synth_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_OSC_SYNTH_WAVE_SINE, "Sine", "sine"},
    {GSTBT_OSC_SYNTH_WAVE_SQUARE, "Square", "square"},
    {GSTBT_OSC_SYNTH_WAVE_SAW, "Saw", "saw"},
    {GSTBT_OSC_SYNTH_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GSTBT_OSC_SYNTH_WAVE_SILENCE, "Silence", "silence"},
    {GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE, "White noise", "white-noise"},
    {GSTBT_OSC_SYNTH_WAVE_PINK_NOISE, "Pink noise", "pink-noise"},
    {GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE, "White Gaussian noise",
        "gaussian-noise"},
    {GSTBT_OSC_SYNTH_WAVE_RED_NOISE, "Red (brownian) noise", "red-noise"},
    {GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE, "Blue noise", "blue-noise"},
    {GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE, "Violet noise", "violet-noise"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtOscSynthWave", enums);
  }
  return type;
}

//-- constructor methods

/**
 * gstbt_osc_synth_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
GstBtOscSynth *
gstbt_osc_synth_new (void)
{
  return GSTBT_OSC_SYNTH (g_object_new (GSTBT_TYPE_OSC_SYNTH, NULL));
}

//-- private methods

static gdouble
get_volume (GstBtOscSynth * self, gdouble ampf, guint size)
{
  if (self->volenv) {
    return gstbt_envelope_get (self->volenv, MIN (INNER_LOOP, size)) * ampf;
  } else {
    return ampf;
  }
}

static void
gstbt_osc_synth_create_sine (GstBtOscSynth * self, guint ct, gint16 * samples)
{
  guint i = 0, j;
  gdouble amp;
  gdouble accumulator = self->accumulator;
  gdouble step = M_PI_M2 * self->freq / self->samplerate;

  while (i < ct) {
    amp = get_volume (self, 32767.0, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      /* TODO(ensonic): move out of inner loop? */
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      samples[i] = (gint16) (sin (accumulator) * amp);
    }
  }
  self->accumulator = accumulator;
}

static void
gstbt_osc_synth_create_square (GstBtOscSynth * self, guint ct, gint16 * samples)
{
  guint i = 0, j;
  gdouble amp;
  gdouble accumulator = self->accumulator;
  gdouble step = M_PI_M2 * self->freq / self->samplerate;

  while (i < ct) {
    amp = get_volume (self, 32767.0, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      samples[i] = (gint16) ((accumulator < M_PI) ? amp : -amp);
    }
  }
  self->accumulator = accumulator;
}

static void
gstbt_osc_synth_create_saw (GstBtOscSynth * self, guint ct, gint16 * samples)
{
  guint i = 0, j;
  gdouble amp, ampf = 32767.0 / M_PI;
  gdouble accumulator = self->accumulator;
  gdouble step = M_PI_M2 * self->freq / self->samplerate;

  while (i < ct) {
    amp = get_volume (self, ampf, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      if (accumulator < M_PI) {
        samples[i] = (gint16) (accumulator * amp);
      } else {
        samples[i] = (gint16) ((M_PI_M2 - accumulator) * -amp);
      }
    }
  }
  self->accumulator = accumulator;
}

static void
gstbt_osc_synth_create_triangle (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  guint i = 0, j;
  gdouble amp, ampf = 32767.0 / M_PI;
  gdouble accumulator = self->accumulator;
  gdouble step = M_PI_M2 * self->freq / self->samplerate;

  while (i < ct) {
    amp = get_volume (self, ampf, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      accumulator += step;
      if (G_UNLIKELY (accumulator >= M_PI_M2))
        accumulator -= M_PI_M2;

      if (accumulator < (M_PI * 0.5)) {
        samples[i] = (gint16) (accumulator * amp);
      } else if (accumulator < (M_PI * 1.5)) {
        samples[i] = (gint16) ((accumulator - M_PI) * -amp);
      } else {
        samples[i] = (gint16) ((M_PI_M2 - accumulator) * -amp);
      }
    }
  }
  self->accumulator = accumulator;
}

static void
gstbt_osc_synth_create_silence (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  memset (samples, 0, ct * sizeof (gint16));
}

static void
gstbt_osc_synth_create_white_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  guint i = 0, j;
  gdouble amp;

  while (i < ct) {
    amp = get_volume (self, 65535.0, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      samples[i] = (gint16) (32768 - (amp * rand () / (RAND_MAX + 1.0)));
    }
  }
}

/* pink noise calculation is based on
 * http://www.firstpr.com.au/dsp/pink-noise/phil_burk_19990905_patest_pink.c
 * which has been released under public domain
 * Many thanks Phil!
 */
static void
gstbt_osc_synth_init_pink_noise (GstBtOscSynth * self)
{
  guint num_rows = 12;          /* arbitrary: 1 .. _PINK_MAX_RANDOM_ROWS */

  self->pink.index = 0;
  self->pink.index_mask = (1 << num_rows) - 1;
  self->pink.scalar = 1.0f / (RAND_MAX + 1.0);
  /* Initialize rows. */
  memset (self->pink.rows, 0, sizeof (self->pink.rows));
  self->pink.running_sum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
static gfloat
gstbt_osc_synth_generate_pink_noise_value (GstBtPinkNoise * pink)
{
  glong new_random;
  glong sum;

  /* Increment and mask index. */
  pink->index = (pink->index + 1) & pink->index_mask;

  /* If index is zero, don't update any random values. */
  if (pink->index != 0) {
    /* Determine how many trailing zeros in PinkIndex. */
    /* This algorithm will hang if n==0 so test first. */
    gint num_zeros = 0;
    gint n = pink->index;

    while ((n & 1) == 0) {
      n = n >> 1;
      num_zeros++;
    }

    /* Replace the indexed ROWS random value.
     * Subtract and add back to RunningSum instead of adding all the random
     * values together. Only one changes each time.
     */
    pink->running_sum -= pink->rows[num_zeros];
    new_random = 32768.0 - (65536.0 * (gulong) rand () / (RAND_MAX + 1.0));
    pink->running_sum += new_random;
    pink->rows[num_zeros] = new_random;
  }

  /* Add extra white noise value. */
  new_random = 32768.0 - (65536.0 * (gulong) rand () / (RAND_MAX + 1.0));
  sum = pink->running_sum + new_random;

  /* Scale to range of -1.0 to 0.9999. */
  return (pink->scalar * sum);
}

static void
gstbt_osc_synth_create_pink_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  guint i = 0, j;
  GstBtPinkNoise *pink = &self->pink;
  gdouble amp;

  while (i < ct) {
    amp = get_volume (self, 32767.0, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      samples[i] =
          (gint16) (gstbt_osc_synth_generate_pink_noise_value (pink) * amp);
    }
  }
}

/* Gaussian white noise using Box-Muller algorithm.  unit variance
 * normally-distributed random numbers are generated in pairs as the real
 * and imaginary parts of a compex random variable with
 * uniformly-distributed argument and \chi^{2}-distributed modulus.
 */
static void
gstbt_osc_synth_create_gaussian_white_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  gint i = 0, j;
  gdouble amp;

  while (i < ct) {
    amp = get_volume (self, 32767.0, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j += 2) {
      gdouble mag = sqrt (-2 * log (1.0 - rand () / (RAND_MAX + 1.0)));
      gdouble phs = M_PI_M2 * rand () / (RAND_MAX + 1.0);

      samples[i++] = (gint16) (amp * mag * cos (phs));
      if (i < ct)
        samples[i++] = (gint16) (amp * mag * sin (phs));
    }
  }
}

static void
gstbt_osc_synth_create_red_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  gint i = 0, j;
  gdouble amp;
  gdouble state = self->red.state;

  while (i < ct) {
    amp = get_volume (self, 32767.0, ct - i);
    for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
      while (TRUE) {
        gdouble r = 1.0 - (2.0 * rand () / (RAND_MAX + 1.0));
        state += r;
        if (state < -8.0f || state > 8.0f)
          state -= r;
        else
          break;
      }
      samples[i] = (gint16) (amp * state * 0.0625f);    /* /16.0 */
    }
  }
  self->red.state = state;
}

static void
gstbt_osc_synth_create_blue_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  gint i;
  gdouble flip = self->flip;

  gstbt_osc_synth_create_pink_noise (self, ct, samples);
  for (i = 0; i < ct; i++) {
    samples[i] *= flip;
    flip *= -1.0;
  }
  self->flip = flip;
}

static void
gstbt_osc_synth_create_violet_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  gint i;
  gdouble flip = self->flip;

  gstbt_osc_synth_create_red_noise (self, ct, samples);
  for (i = 0; i < ct; i++) {
    samples[i] *= flip;
    flip *= -1.0;
  }
  self->flip = flip;
}

/*
 * gstbt_osc_synth_change_wave:
 * Assign function pointer of wave genrator.
 */
static void
gstbt_osc_synth_change_wave (GstBtOscSynth * self)
{
  switch (self->wave) {
    case GSTBT_OSC_SYNTH_WAVE_SINE:
      self->process = gstbt_osc_synth_create_sine;
      break;
    case GSTBT_OSC_SYNTH_WAVE_SQUARE:
      self->process = gstbt_osc_synth_create_square;
      break;
    case GSTBT_OSC_SYNTH_WAVE_SAW:
      self->process = gstbt_osc_synth_create_saw;
      break;
    case GSTBT_OSC_SYNTH_WAVE_TRIANGLE:
      self->process = gstbt_osc_synth_create_triangle;
      break;
    case GSTBT_OSC_SYNTH_WAVE_SILENCE:
      self->process = gstbt_osc_synth_create_silence;
      break;
    case GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE:
      self->process = gstbt_osc_synth_create_white_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_PINK_NOISE:
      gstbt_osc_synth_init_pink_noise (self);
      self->process = gstbt_osc_synth_create_pink_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE:
      self->process = gstbt_osc_synth_create_gaussian_white_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_RED_NOISE:
      self->red.state = 0.0;
      self->process = gstbt_osc_synth_create_red_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE:
      gstbt_osc_synth_init_pink_noise (self);
      self->flip = 1.0;
      self->process = gstbt_osc_synth_create_blue_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE:
      self->red.state = 0.0;
      self->flip = 1.0;
      self->process = gstbt_osc_synth_create_violet_noise;
      break;
    default:
      GST_ERROR ("invalid wave-form: %d", self->wave);
      break;
  }
}

//-- public methods

//-- virtual methods

static void
gstbt_osc_synth_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtOscSynth *self = GSTBT_OSC_SYNTH (object);

  switch (prop_id) {
    case PROP_SAMPLERATE:
      self->samplerate = g_value_get_int (value);
      break;
    case PROP_VOLUME_ENVELOPE:
      self->volenv = GSTBT_ENVELOPE (g_value_get_object (value));
      g_object_add_weak_pointer (G_OBJECT (self->volenv),
          (gpointer *) & self->volenv);
      break;
    case PROP_WAVE:
      //GST_INFO("change wave %d -> %d",g_value_get_enum (value),self->wave);
      self->wave = g_value_get_enum (value);
      gstbt_osc_synth_change_wave (self);
      break;
    case PROP_FREQUENCY:
      //GST_INFO("change frequency %lf -> %lf",g_value_get_double (value),self->freq);
      self->freq = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_osc_synth_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtOscSynth *self = GSTBT_OSC_SYNTH (object);

  switch (prop_id) {
    case PROP_SAMPLERATE:
      g_value_set_int (value, self->samplerate);
      break;
    case PROP_VOLUME_ENVELOPE:
      g_value_set_object (value, self->volenv);
      break;
    case PROP_WAVE:
      g_value_set_enum (value, self->wave);
      break;
    case PROP_FREQUENCY:
      g_value_set_double (value, self->freq);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_osc_synth_dispose (GObject * const object)
{
  GstBtOscSynth *self = GSTBT_OSC_SYNTH (object);

  if (self->volenv) {
    g_object_remove_weak_pointer (G_OBJECT (self->volenv),
        (gpointer *) & self->volenv);
  }

  G_OBJECT_CLASS (gstbt_osc_synth_parent_class)->dispose (object);
}

static void
gstbt_osc_synth_init (GstBtOscSynth * self)
{
  self->wave = GSTBT_OSC_SYNTH_WAVE_SINE;
  self->freq = 0.0;
  gstbt_osc_synth_change_wave (self);
  self->flip = 1.0;
  self->samplerate = 44100;
}

static void
gstbt_osc_synth_class_init (GstBtOscSynthClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "osc-synth",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "synthetic waveform oscillator");

  gobject_class->set_property = gstbt_osc_synth_set_property;
  gobject_class->get_property = gstbt_osc_synth_get_property;
  gobject_class->dispose = gstbt_osc_synth_dispose;

  // register own properties

  g_object_class_install_property (gobject_class, PROP_SAMPLERATE,
      g_param_spec_int ("sample-rate", "Sample Rate", "Sampling rate",
          1, G_MAXINT, 44100, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VOLUME_ENVELOPE,
      g_param_spec_object ("volume-envelope", "Volume envelope",
          "Volume envelope of tone", GSTBT_TYPE_ENVELOPE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GSTBT_TYPE_OSC_SYNTH_WAVE, GSTBT_OSC_SYNTH_WAVE_SINE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FREQUENCY,
      g_param_spec_double ("frequency", "Frequency", "Frequency of tone",
          0.0, G_MAXDOUBLE, 0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}
