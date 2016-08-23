/* Buzztrax
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
 *
 * One can attach #GstControlSources to some of the patameters to modulate them.
 *
 * # Waveforms
 *
 * ![Sine wave](lt-bt_gst_osc-synth_sine.svg) ![Square wave](lt-bt_gst_osc-synth_square.svg)
 * ![Saw wave](lt-bt_gst_osc-synth_saw.svg) ![Triangle wave](lt-bt_gst_osc-synth_triangle.svg)
 * ![Silence](lt-bt_gst_osc-synth_silence.svg) ![White noise](lt-bt_gst_osc-synth_white_noise.svg)
 * ![Pink noise](lt-bt_gst_osc-synth_pink_noise.svg) ![White Gaussian noise](lt-bt_gst_osc-synth_white_gaussian_noise.svg)
 * ![Red (brownian) noise](lt-bt_gst_osc-synth_red__brownian__noise.svg) ![Blue noise](lt-bt_gst_osc-synth_blue_noise.svg)
 * ![Violet noise](lt-bt_gst_osc-synth_violet_noise.svg) ![Sample &amp; Hold](lt-bt_gst_osc-synth_sample_and_hold.svg)
 * ![Spikes](lt-bt_gst_osc-synth_spikes.svg) ![Sample &amp; Glide](lt-bt_gst_osc-synth_sample_and_glide.svg)
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

#include <gst/audio/audio.h>

#include "osc-synth.h"

#define GST_CAT_DEFAULT osc_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define M_PI_M2 ( M_PI + M_PI )
#define INNER_LOOP 64

enum
{
  // static class properties
  PROP_SAMPLERATE = 1,
  // dynamic class properties
  PROP_WAVE,
  PROP_VOLUME,
  PROP_FREQUENCY,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

//-- the class

G_DEFINE_TYPE (GstBtOscSynth, gstbt_osc_synth, GST_TYPE_OBJECT);

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
    {GSTBT_OSC_SYNTH_WAVE_S_AND_H, "Sample and Hold", "sample-and-hold"},
    {GSTBT_OSC_SYNTH_WAVE_SPIKES, "Spikes", "spikes"},
    {GSTBT_OSC_SYNTH_WAVE_S_AND_G, "Sample and Glide", "sample-and-glide"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtOscSynthWave", enums);
  }
  return type;
}

GType
gstbt_osc_synth_tonal_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_OSC_SYNTH_WAVE_SILENCE, "Silence", "silence"},
    {GSTBT_OSC_SYNTH_WAVE_SINE, "Sine", "sine"},
    {GSTBT_OSC_SYNTH_WAVE_SQUARE, "Square", "square"},
    {GSTBT_OSC_SYNTH_WAVE_SAW, "Saw", "saw"},
    {GSTBT_OSC_SYNTH_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GSTBT_OSC_SYNTH_WAVE_S_AND_H, "Sample and Hold", "sample-and-hold"},
    {GSTBT_OSC_SYNTH_WAVE_SPIKES, "Spikes", "spikes"},
    {GSTBT_OSC_SYNTH_WAVE_S_AND_G, "Sample and Glide", "sample-and-glide"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtOscSynthTonalWave", enums);
  }
  return type;
}

GType
gstbt_osc_synth_noise_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
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
    type = g_enum_register_static ("GstBtOscSynthNoiseWave", enums);
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

#define UPDATE_INNER_LOOP(c,r) do { \
  if (INNER_LOOP < r) {             \
    c = INNER_LOOP; r-= INNER_LOOP; \
  } else {                          \
    c = r; r = 0;                   \
  }                                 \
} while (0)

static void
gstbt_osc_synth_create_sine (GstBtOscSynth * self, guint ct, gint16 * samples)
{
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp, step;
  gdouble accumulator = self->accumulator;
  gdouble period = self->period;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * 32767.0;
    step = self->freq * period;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
      accumulator += step;
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
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp, step;
  gdouble accumulator = self->accumulator;
  gdouble period = self->period;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * 32767.0;
    step = self->freq * period;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
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
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp, step;
  gdouble ampf = 32767.0 / M_PI;
  gdouble accumulator = self->accumulator;
  gdouble period = self->period;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * ampf;
    step = self->freq * period;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
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
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp, step;
  gdouble ampf = 65535.0 / M_PI;
  gdouble accumulator = self->accumulator;
  gdouble period = self->period;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * ampf;
    step = self->freq * period;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
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
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
      samples[i] =
          (gint16) (amp * (32768.0 - (65535.0 * rand () / (RAND_MAX + 1.0))));
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
  /* calculate maximum possible signed random value.
   * Extra 1 for white noise always added. */
  self->pink.scalar = 1.0f / ((num_rows + 1) * (1 << 15));
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
    new_random = 32768.0 - (65536.0 * rand () / (RAND_MAX + 1.0));
    pink->running_sum += new_random;
    pink->rows[num_zeros] = new_random;
  }

  /* Add extra white noise value. */
  new_random = 32768.0 - (65536.0 * rand () / (RAND_MAX + 1.0));
  sum = pink->running_sum + new_random;

  /* Scale to range of -1.0 to 0.9999. */
  return (pink->scalar * sum);
}

static void
gstbt_osc_synth_create_pink_noise (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  guint i = 0, j, c, r = ct;
  GstBtPinkNoise *pink = &self->pink;
  guint64 offset = self->offset;
  gdouble amp;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * 32767.0;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
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
  gint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * 32767.0;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j += 2) {
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
  gint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp;
  gdouble state = self->red.state;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol * 32767.0;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
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

static void
gstbt_osc_synth_create_s_and_h (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp, step;
  gint count = self->sh.count;
  gint samplerate = self->samplerate;
  gdouble smpl = self->sh.smpl;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
      if (G_UNLIKELY (count <= 0)) {
        gst_object_sync_values ((GstObject *) self, offset + i);
        // if freq = 100, we want 100 changes per second
        // = 100 changes per samplingrate samples
        step = self->freq;
        step = CLAMP (step, 1, samplerate);
        count = samplerate / step;
        smpl = 32768 - (65535.0 * rand () / (RAND_MAX + 1.0));
        amp = self->vol;
      }
      samples[i] = (gint16) (amp * smpl);
      count--;
    }
  }
  self->sh.count = count;
  self->sh.smpl = smpl;
}

static void
gstbt_osc_synth_create_spikes (GstBtOscSynth * self, guint ct, gint16 * samples)
{
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble step, smpl;
  gint count = self->sh.count;
  gint samplerate = self->samplerate;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
      if (G_UNLIKELY (count <= 0)) {
        gst_object_sync_values ((GstObject *) self, offset + i);
        // if freq = 100, we want 100 spikes per second
        // = 100 changes per samplingrate samples
        step = self->freq;
        step = CLAMP (step, 1, samplerate);
        count = samplerate / step;
        smpl = 32768 - (65535.0 * rand () / (RAND_MAX + 1.0));
        samples[i] = (gint16) (self->vol * smpl);
      } else {
        samples[i] = (gint16) 0;
      }
      count--;
    }
  }
  self->sh.count = count;
}

static void
gstbt_osc_synth_create_s_and_g (GstBtOscSynth * self, guint ct,
    gint16 * samples)
{
  guint i = 0, j, c, r = ct;
  guint64 offset = self->offset;
  gdouble amp, step;
  gint count = self->sh.count;
  gint samplerate = self->samplerate;
  gdouble smpl = self->sh.smpl;
  gdouble next = self->sh.next;

  while (i < ct) {
    gst_object_sync_values ((GstObject *) self, offset + i);
    amp = self->vol;
    UPDATE_INNER_LOOP (c, r);
    for (j = 0; j < c; j++, i++) {
      if (G_UNLIKELY (count <= 0)) {
        gst_object_sync_values ((GstObject *) self, offset + i);
        // if freq = 100, we want 100 changes per second
        // = 100 changes per samplingrate samples
        step = self->freq;
        step = CLAMP (step, 1, samplerate);
        count = samplerate / step;
        next = 32768 - (65535.0 * rand () / (RAND_MAX + 1.0));
        next = (next - smpl) / (gdouble) count;
        amp = self->vol;
      }
      samples[i] = (gint16) (amp * smpl);
      count--;
      smpl += next;
    }
  }
  self->sh.count = count;
  self->sh.smpl = smpl;
  self->sh.next = next;
}

/*
 * gstbt_osc_synth_change_wave:
 * @self: the oscillator
 *
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
      self->process = gstbt_osc_synth_create_pink_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE:
      self->process = gstbt_osc_synth_create_gaussian_white_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_RED_NOISE:
      self->process = gstbt_osc_synth_create_red_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE:
      self->process = gstbt_osc_synth_create_blue_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE:
      self->process = gstbt_osc_synth_create_violet_noise;
      break;
    case GSTBT_OSC_SYNTH_WAVE_S_AND_H:
      self->process = gstbt_osc_synth_create_s_and_h;
      break;
    case GSTBT_OSC_SYNTH_WAVE_SPIKES:
      self->process = gstbt_osc_synth_create_spikes;
      break;
    case GSTBT_OSC_SYNTH_WAVE_S_AND_G:
      self->process = gstbt_osc_synth_create_s_and_g;
      break;
    default:
      GST_ERROR ("invalid wave-form: %d", self->wave);
      break;
  }
  gstbt_osc_synth_trigger (self);
}

//-- public methods

/**
 * gstbt_osc_synth_trigger:
 * @self: the oscillator
 *
 * Reset oscillator state. Typically called for new notes.
 */
void
gstbt_osc_synth_trigger (GstBtOscSynth * self)
{
  self->offset = 0;
  self->accumulator = 0.0;
  self->flip = 1.0;
  switch (self->wave) {
    case GSTBT_OSC_SYNTH_WAVE_SINE:
    case GSTBT_OSC_SYNTH_WAVE_SQUARE:
    case GSTBT_OSC_SYNTH_WAVE_SAW:
    case GSTBT_OSC_SYNTH_WAVE_TRIANGLE:
    case GSTBT_OSC_SYNTH_WAVE_SILENCE:
    case GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE:
    case GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE:
      break;
    case GSTBT_OSC_SYNTH_WAVE_PINK_NOISE:
    case GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE:
      gstbt_osc_synth_init_pink_noise (self);
      break;
    case GSTBT_OSC_SYNTH_WAVE_RED_NOISE:
    case GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE:
      self->red.state = 0.0;
      break;
    case GSTBT_OSC_SYNTH_WAVE_S_AND_H:
      self->sh.count = 0;
      self->sh.smpl = 0.0;
      break;
    case GSTBT_OSC_SYNTH_WAVE_SPIKES:
      self->sh.count = 0;
      break;
    case GSTBT_OSC_SYNTH_WAVE_S_AND_G:
      self->sh.count = 0;
      self->sh.smpl = 0.0;
      self->sh.next = 32768 - (65535.0 * rand () / (RAND_MAX + 1.0));
      break;
    default:
      GST_ERROR ("invalid wave-form: %d", self->wave);
      break;
  }
}

/**
 * gstbt_osc_synth_process:
 * @self: the oscillator
 * @size: number of sample to generate
 * @data: buffer to hold the audio
 *
 * Generate @size samples of audio and store them into @data.
 */
void
gstbt_osc_synth_process (GstBtOscSynth * self, guint size, gint16 * data)
{
  self->process (self, size, data);
  self->offset += size;
}

//-- virtual methods

static void
gstbt_osc_synth_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtOscSynth *self = GSTBT_OSC_SYNTH (object);

  switch (prop_id) {
    case PROP_SAMPLERATE:
      self->samplerate = g_value_get_int (value);
      self->period = M_PI_M2 / self->samplerate;
      break;
    case PROP_WAVE:
      //GST_INFO("change wave %d <- %d",g_value_get_enum (value),self->wave);
      self->wave = g_value_get_enum (value);
      gstbt_osc_synth_change_wave (self);
      break;
    case PROP_VOLUME:
      //GST_INFO("change volume %lf <- %lf",g_value_get_double (value),self->vol);
      self->vol = g_value_get_double (value);
      break;
    case PROP_FREQUENCY:
      //GST_INFO("change frequency %lf <- %lf",g_value_get_double (value),self->freq);
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
    case PROP_WAVE:
      g_value_set_enum (value, self->wave);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_osc_synth_init (GstBtOscSynth * self)
{
  self->wave = GSTBT_OSC_SYNTH_WAVE_SINE;
  self->vol = 1.0;
  self->freq = 0.0;
  self->flip = 1.0;
  self->samplerate = GST_AUDIO_DEF_RATE;
  self->period = M_PI_M2 / self->samplerate;
  gstbt_osc_synth_change_wave (self);
}

static void
gstbt_osc_synth_class_init (GstBtOscSynthClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "osc-synth",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "synthetic waveform oscillator");

  gobject_class->set_property = gstbt_osc_synth_set_property;
  gobject_class->get_property = gstbt_osc_synth_get_property;

  // register own properties

  PROP (SAMPLERATE) = g_param_spec_int ("sample-rate", "Sample Rate",
      "Sampling rate", 1, G_MAXINT, GST_AUDIO_DEF_RATE,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  PROP (WAVE) = g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
      GSTBT_TYPE_OSC_SYNTH_WAVE, GSTBT_OSC_SYNTH_WAVE_SINE,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (VOLUME) = g_param_spec_double ("volume", "Volume",
      "Volume of tone", 0.0, 1.0, 0.0,
      G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (FREQUENCY) = g_param_spec_double ("frequency", "Frequency",
      "Frequency of tone", 0.0, G_MAXDOUBLE, 0.0,
      G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
