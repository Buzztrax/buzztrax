/* Buzztrax
 * Copyright (C) 2007 Josh Green <josh@users.sf.net>
 *
 * Adapted from simsyn synthesizer plugin in gst-buzztrax source.
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * gstfluidsynth.h: GStreamer wrapper for FluidSynth
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

#ifndef __GSTBT_FLUID_SYNTH_H__
#define __GSTBT_FLUID_SYNTH_H__

#include <gst/gst.h>
#include "gst/audiosynth.h"
#include "gst/envelope.h"
#include "gst/toneconversion.h"
#include <fluidsynth.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_FLUID_SYNTH            (gstbt_fluid_synth_get_type())
#define GSTBT_FLUID_SYNTH(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_FLUID_SYNTH,GstBtFluidSynth))
#define GSTBT_IS_FLUID_SYNTH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_FLUID_SYNTH))
#define GSTBT_FLUID_SYNTH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_FLUID_SYNTH,GstBtFluidSynthClass))
#define GSTBT_IS_FLUID_SYNTH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_FLUID_SYNTH))
#define GSTBT_FLUID_SYNTH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_FLUID_SYNTH,GstBtFluidSynthClass))

/**
 * GstBtFluidSynthInterpolationMode:
 * @GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_NONE: no interpolation
 * @GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_LINEAR: linear interpolation
 * @GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_4THORDER: 4th order interpolation
 * @GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_7THORDER: 7th order interpolation
 *
 * Synthesis engine interpolation mode.
 */
typedef enum
{
  GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_NONE = FLUID_INTERP_NONE,
  GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_LINEAR = FLUID_INTERP_LINEAR,
  GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_4THORDER = FLUID_INTERP_4THORDER,
  GSTBT_FLUID_SYNTH_INTERPOLATION_MODE_7THORDER = FLUID_INTERP_7THORDER
} GstBtFluidSynthInterpolationMode;

/**
 * GstBtFluidSynthChorusWaveform:
 * @GSTBT_FLUID_SYNTH_CHORUS_MOD_SINE: sine wave
 * @GSTBT_FLUID_SYNTH_CHORUS_MOD_TRIANGLE: triangle wave
 *
 * Modulation waveform for the chorus effect.
 */
typedef enum
{
  GSTBT_FLUID_SYNTH_CHORUS_MOD_SINE = FLUID_CHORUS_MOD_SINE,
  GSTBT_FLUID_SYNTH_CHORUS_MOD_TRIANGLE = FLUID_CHORUS_MOD_TRIANGLE
} GstBtFluidSynthChorusWaveform;

typedef struct _GstBtFluidSynth GstBtFluidSynth;
typedef struct _GstBtFluidSynthClass GstBtFluidSynthClass;

/**
 * GstBtFluidSynth:
 *
 * Class instance data.
 */
struct _GstBtFluidSynth {
  GstBtAudioSynth parent;

  /* < private > */
  /* parameters */
  gboolean dispose_has_run;         /* validate if dispose has run */

  GstBtNote note;
  gint key;
  gint velocity;
  glong cur_note_length,note_length;
  gint program;

  fluid_synth_t *fluid;			        /* the FluidSynth handle */
  fluid_settings_t *settings;       /* to free on close */
  fluid_midi_driver_t *midi;		    /* FluidSynth MIDI driver */
  fluid_midi_router_t *midi_router; /* FluidSynth MIDI router */
  fluid_cmd_handler_t *cmd_handler;

  gchar *instrument_patch_path;
  gint instrument_patch;

  GstBtFluidSynthInterpolationMode interp; /* interpolation type */

  gboolean reverb_enable;		        /* reverb enable */
  gdouble reverb_room_size;		      /* reverb room size */
  gdouble reverb_damp;			        /* reverb damping */
  gdouble reverb_width;			        /* reverb width */
  gdouble reverb_level;			        /* reverb level */
  gboolean reverb_update;

  gboolean chorus_enable;		        /* chorus enable */
  gint chorus_count;			          /* chorus voice count */
  gdouble chorus_level;			        /* chorus level */
  gdouble chorus_freq;			        /* chorus freq */
  gdouble chorus_depth;			        /* chorus depth */
  GstBtFluidSynthChorusWaveform chorus_waveform; /* chorus waveform */
  gboolean chorus_update;
};

struct _GstBtFluidSynthClass {
  GstBtAudioSynthClass parent_class;
};

GType gstbt_fluid_synth_get_type(void);

G_END_DECLS

#endif /* __GSTBT_FLUID_SYNTH_H__ */
