/* Buzztrax
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * osc-synth.h: synthetic waveform oscillator
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

#ifndef __GSTBT_OSC_SYNTH_H__
#define __GSTBT_OSC_SYNTH_H__

#include <gst/gst.h>
#include "gst/envelope.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_OSC_SYNTH_WAVE (gstbt_osc_synth_wave_get_type())
#define GSTBT_TYPE_OSC_SYNTH_TONAL_WAVE (gstbt_osc_synth_tonal_wave_get_type())
#define GSTBT_TYPE_OSC_SYNTH_NOISE_WAVE (gstbt_osc_synth_noise_wave_get_type())


/**
 * GstBtOscSynthWave:
 * @GSTBT_OSC_SYNTH_WAVE_SINE: sine wave
 * @GSTBT_OSC_SYNTH_WAVE_SQUARE: square wave
 * @GSTBT_OSC_SYNTH_WAVE_SAW: saw wave
 * @GSTBT_OSC_SYNTH_WAVE_TRIANGLE: triangle wave
 * @GSTBT_OSC_SYNTH_WAVE_SILENCE: silence
 * @GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE: white noise
 * @GSTBT_OSC_SYNTH_WAVE_PINK_NOISE: pink noise
 * @GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE: white (zero mean) Gaussian noise;
 *   volume sets the standard deviation of the noise in units of the range of
 *   values of the sample type, e.g. volume=0.1 produces noise with a standard
 *   deviation of 0.1*32767=3277 with 16-bit integer samples, or 0.1*1.0=0.1
 *   with floating-point samples.
 * @GSTBT_OSC_SYNTH_WAVE_RED_NOISE: red (brownian) noise
 * @GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE: spectraly inverted pink noise
 * @GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE: spectraly inverted red (brownian) noise
 * @GSTBT_OSC_SYNTH_WAVE_S_AND_H: sample and hold. Create a random value and
 * hold it for a time specified by #GstBtOscSynth:frequency.
 * @GSTBT_OSC_SYNTH_WAVE_SPIKES: spikes. Create a random level spikes at a rate
 * specified by #GstBtOscSynth:frequency.
 * @GSTBT_OSC_SYNTH_WAVE_S_AND_G: sample and glide. Create an random values and
 *  blend between them over a time specified by #GstBtOscSynth:frequency.
 * @GSTBT_OSC_SYNTH_WAVE_COUNT: number of waves, this can change with new
 * releases
 *
 * Oscillator wave forms.
 */
typedef enum
{
  GSTBT_OSC_SYNTH_WAVE_SINE,
  GSTBT_OSC_SYNTH_WAVE_SQUARE,
  GSTBT_OSC_SYNTH_WAVE_SAW,
  GSTBT_OSC_SYNTH_WAVE_TRIANGLE,
  GSTBT_OSC_SYNTH_WAVE_SILENCE,
  GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE,
  GSTBT_OSC_SYNTH_WAVE_PINK_NOISE,
  GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE,
  GSTBT_OSC_SYNTH_WAVE_RED_NOISE,
  GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE,
  GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE,
  GSTBT_OSC_SYNTH_WAVE_S_AND_H,
  GSTBT_OSC_SYNTH_WAVE_SPIKES,
  GSTBT_OSC_SYNTH_WAVE_S_AND_G,
  GSTBT_OSC_SYNTH_WAVE_COUNT
} GstBtOscSynthWave;

/**
 * GstBtOscSynthTonalWave:
 * @GSTBT_OSC_SYNTH_WAVE_SILENCE: silence
 * @GSTBT_OSC_SYNTH_WAVE_SINE: sine wave
 * @GSTBT_OSC_SYNTH_WAVE_SQUARE: square wave
 * @GSTBT_OSC_SYNTH_WAVE_SAW: saw wave
 * @GSTBT_OSC_SYNTH_WAVE_TRIANGLE: triangle wave
 * @GSTBT_OSC_SYNTH_WAVE_S_AND_H: sample and hold. Create an random value and
 *  hold it for a time specified by #GstBtOscSynth:frequency.
 * @GSTBT_OSC_SYNTH_WAVE_SPIKES: spikes. Create a random level spikes at a rate
 *  specified by #GstBtOscSynth:frequency.
 * @GSTBT_OSC_SYNTH_WAVE_S_AND_G: sample and glide. Create an random values and
 *  blend between them over a time specified by #GstBtOscSynth:frequency.
 *
 * Tonal oscillator wave forms from #GstBtOscSynthWave.
 */
#if 0
typedef enum
{
  GSTBT_OSC_SYNTH_WAVE_SILENCE,
  GSTBT_OSC_SYNTH_WAVE_SINE,
  GSTBT_OSC_SYNTH_WAVE_SQUARE,
  GSTBT_OSC_SYNTH_WAVE_SAW,
  GSTBT_OSC_SYNTH_WAVE_TRIANGLE,
  GSTBT_OSC_SYNTH_WAVE_S_AND_H,
  GSTBT_OSC_SYNTH_WAVE_SPIKES,
  GSTBT_OSC_SYNTH_WAVE_S_AND_G
} GstBtOscSynthTonalWave;
#endif

/**
 * GstBtOscSynthNoiseWave:
 * @GSTBT_OSC_SYNTH_WAVE_SILENCE: silence
 * @GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE: white noise
 * @GSTBT_OSC_SYNTH_WAVE_PINK_NOISE: pink noise
 * @GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE: white (zero mean) Gaussian noise;
 *   volume sets the standard deviation of the noise in units of the range of
 *   values of the sample type, e.g. volume=0.1 produces noise with a standard
 *   deviation of 0.1*32767=3277 with 16-bit integer samples, or 0.1*1.0=0.1
 *   with floating-point samples.
 * @GSTBT_OSC_SYNTH_WAVE_RED_NOISE: red (brownian) noise
 * @GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE: spectraly inverted pink noise
 * @GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE: spectraly inverted red (brownian) noise
 *
 * Noise oscillator wave forms from #GstBtOscSynthWave.
 */
#if 0
typedef enum
{
  GSTBT_OSC_SYNTH_WAVE_SILENCE,
  GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE,
  GSTBT_OSC_SYNTH_WAVE_PINK_NOISE,
  GSTBT_OSC_SYNTH_WAVE_GAUSSIAN_WHITE_NOISE,
  GSTBT_OSC_SYNTH_WAVE_RED_NOISE,
  GSTBT_OSC_SYNTH_WAVE_BLUE_NOISE,
  GSTBT_OSC_SYNTH_WAVE_VIOLET_NOISE
} GstBtOscSynthNoiseWave;
#endif

GType gstbt_osc_synth_wave_get_type(void);
GType gstbt_osc_synth_tonal_wave_get_type(void);
GType gstbt_osc_synth_noise_wave_get_type(void);

#define _PINK_MAX_RANDOM_ROWS   (30)

typedef struct
{
  glong rows[_PINK_MAX_RANDOM_ROWS];
  glong running_sum;            /* Used to optimize summing of generators. */
  gint index;                   /* Incremented each sample. */
  gint index_mask;              /* Index wrapped by ANDing with this mask. */
  gfloat scalar;                /* Used to scale within range of -1.0 to +1.0 */
} GstBtPinkNoise;

typedef struct
{
  gdouble state;                /* noise state */
} GstBtRedNoise;

typedef struct
{
  gint count;                   /* count down */
  gdouble smpl, next;           /* current and next sample */
} GstBtSampleAndHold;

#define GSTBT_TYPE_OSC_SYNTH            (gstbt_osc_synth_get_type())
#define GSTBT_OSC_SYNTH(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_OSC_SYNTH,GstBtOscSynth))
#define GSTBT_IS_OSC_SYNTH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_OSC_SYNTH))
#define GSTBT_OSC_SYNTH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_OSC_SYNTH,GstBtOscSynthClass))
#define GSTBT_IS_OSC_SYNTH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_OSC_SYNTH))
#define GSTBT_OSC_SYNTH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_OSC_SYNTH,GstBtOscSynthClass))

typedef struct _GstBtOscSynth GstBtOscSynth;
typedef struct _GstBtOscSynthClass GstBtOscSynthClass;

/**
 * GstBtOscSynth:
 *
 * Class instance data.
 */
struct _GstBtOscSynth {
  GstObject parent;

  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */
  /* parameters */
  gint samplerate;
  GstBtOscSynthWave wave;
  gdouble vol, freq;

  /* oscillator state */
  guint64 offset;
  gdouble accumulator;          /* phase angle */
  gdouble period;
  gdouble flip;
  GstBtPinkNoise pink;
  GstBtRedNoise red;
  GstBtSampleAndHold sh;

  /* < private > */
  void (*process) (GstBtOscSynth *, guint, gint16 *);
};

struct _GstBtOscSynthClass {
  GstObjectClass parent_class;
};

GType gstbt_osc_synth_get_type(void);

GstBtOscSynth *gstbt_osc_synth_new(void);
void gstbt_osc_synth_trigger(GstBtOscSynth *self);
void gstbt_osc_synth_process(GstBtOscSynth *self, guint size, gint16 *data);

G_END_DECLS
#endif /* __GSTBT_OSC_SYNTH_H__ */
