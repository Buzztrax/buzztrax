/* Buzztrax
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
 *
 * ebeats.c: electric drum synthesizer for gstreamer
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
 * SECTION:ebeats
 * @title: GstBtEBeats
 * @short_description: electric drum audio synthesizer
 *
 * A drum synthesizer with two tonal and one noise oscillator (#GstBtOscSynth),
 * plus decay envelopes (#GstBtEnvelopeD) for tonal transitions and volumes.
 * The tonal oscillators can be mixed through various #GstBtCombine:combine
 * modes. The noise part is then mixed with the tonal parts.
 *
 * Finally one can apply a filter (#GstBtFilterSVF) to either the tonal mix, the
 * noise or both. The #GstBtFilterSVF:cut-off is also controlled by a decay
 * envelope and the decay is the same as the one from the tonal osc, noise osc
 * or the max of both depending on the (#GstBtEBeats:filter-routing).
 *
 * The synthesizer uses a trigger parameter (#GstBtEBeats:volume) to be start a
 * tone that also controls the overall volume.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 ebeats num-buffers=10 volume=100 ! autoaudiosink
 * ]| Render a drum tone.
 * </refsect2>
 */
/* TODO: add a delay line for metalic effects */
/* TODO: add a boolean 'reverse 'parameter
 * - can be a pattern only trigger param to set time to max and time-inc * -1
 * - for that the envelopes need a 'reverse' param too, or we need to check if
 *   we can count 'ct' backwards
 */
/* TODO: have presets for:
 * kick
 * snare
 * - http://www.soundonsound.com/sos/apr02/articles/synthsecrets0402.asp
 * - two sin + (noise ! filter)
 * open-hihat
 * closed-hihat
 * claps
 * cymbal
 * rimshot
 * toms
 * cowbell
 * - http://www.soundonsound.com/sos/sep02/articles/synthsecrets09.asp
 * - two sqr/tri: 587Hz and 845Hz (frequency ratio of 1:1.44)
 * claves
 *   - osc ! bandpass ! vca
 *
 * http://www.soundonsound.com/sos/1996_articles/apr96/analoguedrums.html
 * http://www.soundonsound.com/search?page=2&Keyword=%22synth%20secrets%22&Words=All&Summary=Yes
 *   http://www.soundonsound.com/sos/Feb02/articles/synthsecrets0202.asp
 *
 * http://www.heise.de/ct/artikel/Tutorial-Einstieg-in-die-Drum-Synthese-mit-Waldorf-Attack-2799668.html
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "plugin.h"
#include "ebeats.h"

#ifdef HAVE_GST_CONTROL_BINDING_ABS
#include <gst/controller/gstdirectcontrolbinding.h>
#else
#include "gst/gstdirectcontrolbinding.h"
#endif

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  // dynamic class properties
  PROP_VOLUME = 1,
  // tonal
  PROP_T1_WAVE, PROP_T1_FREQ_START, PROP_T1_FREQ_END, PROP_T1_FREQ_CURVE,
  PROP_T2_WAVE, PROP_T2_FREQ_START, PROP_T2_FREQ_END, PROP_T2_FREQ_CURVE,
  PROP_T_VOLUME, PROP_T_DECAY, PROP_T_VOL_CURVE,
  PROP_T_COMBINE,
  // noise
  PROP_N_WAVE, PROP_N_VOLUME, PROP_N_DECAY, PROP_N_VOL_CURVE,
  // filter
  PROP_FILTER_ROUTING, PROP_FILTER, PROP_CUTOFF_START, PROP_CUTOFF_END,
  PROP_CUTOFF_CURVE, PROP_RESONANCE,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

//-- the class

G_DEFINE_TYPE (GstBtEBeats, gstbt_e_beats, GSTBT_TYPE_AUDIO_SYNTH);

//-- enums

GType
gstbt_e_beats_filter_routing_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_E_BEATS_FILTER_ROUTING_T_N, "Tonal+Noise",
        "both tonal and noise parts"},
    {GSTBT_E_BEATS_FILTER_ROUTING_T, "Tonal", "only tonal parts"},
    {GSTBT_E_BEATS_FILTER_ROUTING_N, "Noise", "only noise parts"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtEBeatsFilterRouting", enums);
  }
  return type;
}

//-- helper methods

static void
gstbt_e_beats_update_filter_decay (GstBtEBeats * src)
{
  GValue value = { 0, };
  gdouble tv, nv;

  g_value_init (&value, G_TYPE_DOUBLE);
  switch (src->flt_routing) {
    case GSTBT_E_BEATS_FILTER_ROUTING_T_N:
      g_object_get_property ((GObject *) (src->volenv_t), "decay", &value);
      tv = g_value_get_double (&value);
      g_object_get_property ((GObject *) (src->volenv_n), "decay", &value);
      nv = g_value_get_double (&value);
      g_value_set_double (&value, MAX (tv, nv));
      break;
    case GSTBT_E_BEATS_FILTER_ROUTING_T:
      g_object_get_property ((GObject *) (src->volenv_t), "decay", &value);
      break;
    case GSTBT_E_BEATS_FILTER_ROUTING_N:
      g_object_get_property ((GObject *) (src->volenv_n), "decay", &value);
      break;
  }

  g_object_set_property ((GObject *) (src->fltenv), "decay", &value);
}

//-- audiosynth vmethods

static void
gstbt_e_beats_negotiate (GstBtAudioSynth * base, GstCaps * caps)
{
  gint i, n = gst_caps_get_size (caps);

  for (i = 0; i < n; i++) {
    gst_structure_fixate_field_nearest_int (gst_caps_get_structure (caps, i),
        "channels", 1);
  }
}

static void
gstbt_e_beats_setup (GstBtAudioSynth * base, GstAudioInfo * info)
{
  GstBtEBeats *src = ((GstBtEBeats *) base);

  g_object_set (src->osc_t1, "sample-rate", info->rate, NULL);
  g_object_set (src->osc_t2, "sample-rate", info->rate, NULL);
  g_object_set (src->osc_n, "sample-rate", info->rate, NULL);
}

static void
gstbt_e_beats_reset (GstBtAudioSynth * base)
{
  GstBtEBeats *src = ((GstBtEBeats *) base);

  src->volume = 0.0;
  gstbt_envelope_reset ((GstBtEnvelope *) src->volenv_t);
  gstbt_envelope_reset ((GstBtEnvelope *) src->volenv_n);
}

static gboolean
gstbt_e_beats_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  GstBtEBeats *src = ((GstBtEBeats *) base);
  gboolean env_t = gstbt_envelope_is_running ((GstBtEnvelope *) src->volenv_t,
      src->osc_t1->offset);
  gboolean env_n = gstbt_envelope_is_running ((GstBtEnvelope *) src->volenv_n,
      src->osc_n->offset);

  GST_DEBUG_OBJECT (src, "play vol=%lf, env_t=%d, env_n=%d", src->volume,
      env_t, env_n);

  if (src->volume && (env_t || env_n)) {
    gint16 *d1 = (gint16 *) info->data;
    guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
    gint16 *d2 = g_new0 (gint16, ct);
    gint16 *d3 = g_new0 (gint16, ct);
    guint i;
    glong mix;
    gdouble v = (1.0 / 3.0) * src->volume;

    /* Tonal oscs */
    if (env_t) {
      gstbt_osc_synth_process (src->osc_t1, ct, d1);
      gstbt_osc_synth_process (src->osc_t2, ct, d2);
      gstbt_combine_process (src->mix, ct, d1, d2);
      if (src->flt_routing == GSTBT_E_BEATS_FILTER_ROUTING_T) {
        gstbt_filter_svf_process (src->filter, ct, d1);
      }
    } else {
      memset (d1, 0, ct * sizeof (gint16));
    }
    /* Noise osc */
    if (env_n) {
      gstbt_osc_synth_process (src->osc_n, ct, d3);
      if (src->flt_routing == GSTBT_E_BEATS_FILTER_ROUTING_N) {
        gstbt_filter_svf_process (src->filter, ct, d3);
      }
    }
    /* Mix */
    for (i = 0; i < ct; i++) {
      mix = (glong) (v * ((glong) d1[i] + (glong) d3[i]));
      d1[i] = (gint16) CLAMP (mix, G_MININT16, G_MAXINT16);
    }
    if (src->flt_routing == GSTBT_E_BEATS_FILTER_ROUTING_T_N) {
      gstbt_filter_svf_process (src->filter, ct, d1);
    }

    g_free (d2);
    g_free (d3);
    return TRUE;
  }
  return FALSE;
}

//-- gobject vmethods

static void
gstbt_e_beats_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEBeats *src = GSTBT_E_BEATS (object);

  switch (prop_id) {
    case PROP_VOLUME:{
      guint vol = g_value_get_uint (value);
      if (vol) {
        gint samplerate = ((GstBtAudioSynth *) src)->info.rate;
        src->volume = (gdouble) vol / 128.0;
        GST_DEBUG_OBJECT (object, "trigger -> %d = %lf", vol, src->volume);
        gstbt_osc_synth_trigger (src->osc_t1);
        gstbt_osc_synth_trigger (src->osc_t2);
        gstbt_osc_synth_trigger (src->osc_n);
        gstbt_envelope_d_setup (src->freqenv_t1, samplerate);
        gstbt_envelope_d_setup (src->freqenv_t2, samplerate);
        gstbt_envelope_d_setup (src->volenv_t, samplerate);
        gstbt_envelope_d_setup (src->volenv_n, samplerate);
        gstbt_envelope_d_setup (src->fltenv, samplerate);
        gstbt_filter_svf_trigger (src->filter);
      }
      break;
    }
    case PROP_T1_WAVE:
      g_object_set_property ((GObject *) (src->osc_t1), "wave", value);
      break;
    case PROP_T1_FREQ_START:
      g_object_set_property ((GObject *) (src->freqenv_t1), "peak-level",
          value);
      break;
    case PROP_T1_FREQ_END:
      g_object_set_property ((GObject *) (src->freqenv_t1), "floor-level",
          value);
      break;
    case PROP_T1_FREQ_CURVE:
      g_object_set_property ((GObject *) (src->freqenv_t1), "curve", value);
      break;
    case PROP_T2_WAVE:
      g_object_set_property ((GObject *) (src->osc_t2), "wave", value);
      break;
    case PROP_T2_FREQ_START:
      g_object_set_property ((GObject *) (src->freqenv_t2), "peak-level",
          value);
      break;
    case PROP_T2_FREQ_END:
      g_object_set_property ((GObject *) (src->freqenv_t2), "floor-level",
          value);
      break;
    case PROP_T2_FREQ_CURVE:
      g_object_set_property ((GObject *) (src->freqenv_t2), "curve", value);
      break;
    case PROP_T_VOLUME:
      g_object_set_property ((GObject *) (src->volenv_t), "peak-level", value);
      break;
    case PROP_T_DECAY:
      g_object_set_property ((GObject *) (src->volenv_t), "decay", value);
      g_object_set_property ((GObject *) (src->freqenv_t1), "decay", value);
      g_object_set_property ((GObject *) (src->freqenv_t2), "decay", value);
      gstbt_e_beats_update_filter_decay (src);
      break;
    case PROP_T_VOL_CURVE:
      g_object_set_property ((GObject *) (src->volenv_t), "curve", value);
      break;
    case PROP_T_COMBINE:
      g_object_set_property ((GObject *) (src->mix), pspec->name, value);
      break;
    case PROP_N_WAVE:
      g_object_set_property ((GObject *) (src->osc_n), "wave", value);
      break;
    case PROP_N_VOLUME:
      g_object_set_property ((GObject *) (src->volenv_n), "peak-level", value);
      break;
    case PROP_N_DECAY:
      g_object_set_property ((GObject *) (src->volenv_n), "decay", value);
      gstbt_e_beats_update_filter_decay (src);
      break;
    case PROP_N_VOL_CURVE:
      g_object_set_property ((GObject *) (src->volenv_n), "curve", value);
      break;
    case PROP_CUTOFF_START:
      g_object_set_property ((GObject *) (src->fltenv), "peak-level", value);
      break;
    case PROP_CUTOFF_END:
      g_object_set_property ((GObject *) (src->fltenv), "floor-level", value);
      break;
    case PROP_CUTOFF_CURVE:
      g_object_set_property ((GObject *) (src->fltenv), "curve", value);
      break;
    case PROP_FILTER_ROUTING:
      src->flt_routing = g_value_get_enum (value);
      gstbt_e_beats_update_filter_decay (src);
      break;
    case PROP_FILTER:
    case PROP_RESONANCE:
      g_object_set_property ((GObject *) (src->filter), pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_e_beats_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEBeats *src = GSTBT_E_BEATS (object);

  switch (prop_id) {
    case PROP_T1_WAVE:
      g_object_get_property ((GObject *) (src->osc_t1), "wave", value);
      break;
    case PROP_T1_FREQ_START:
      g_object_get_property ((GObject *) (src->freqenv_t1), "peak-level",
          value);
      break;
    case PROP_T1_FREQ_END:
      g_object_get_property ((GObject *) (src->freqenv_t1), "floor-level",
          value);
      break;
    case PROP_T1_FREQ_CURVE:
      g_object_get_property ((GObject *) (src->freqenv_t1), "curve", value);
      break;
    case PROP_T2_WAVE:
      g_object_get_property ((GObject *) (src->osc_t2), "wave", value);
      break;
    case PROP_T2_FREQ_START:
      g_object_get_property ((GObject *) (src->freqenv_t2), "peak-level",
          value);
      break;
    case PROP_T2_FREQ_END:
      g_object_get_property ((GObject *) (src->freqenv_t2), "floor-level",
          value);
      break;
    case PROP_T2_FREQ_CURVE:
      g_object_get_property ((GObject *) (src->freqenv_t2), "curve", value);
      break;
    case PROP_T_VOLUME:
      g_object_get_property ((GObject *) (src->volenv_t), "peak-level", value);
      break;
    case PROP_T_DECAY:
      g_object_get_property ((GObject *) (src->volenv_t), "decay", value);
      break;
    case PROP_T_VOL_CURVE:
      g_object_get_property ((GObject *) (src->volenv_t), "curve", value);
      break;
    case PROP_T_COMBINE:
      g_object_get_property ((GObject *) (src->mix), pspec->name, value);
      break;
    case PROP_N_WAVE:
      g_object_get_property ((GObject *) (src->osc_n), "wave", value);
      break;
    case PROP_N_VOLUME:
      g_object_get_property ((GObject *) (src->volenv_n), "peak-level", value);
      break;
    case PROP_N_DECAY:
      g_object_get_property ((GObject *) (src->volenv_n), "decay", value);
      break;
    case PROP_N_VOL_CURVE:
      g_object_get_property ((GObject *) (src->volenv_n), "curve", value);
      break;
    case PROP_CUTOFF_START:
      g_object_get_property ((GObject *) (src->fltenv), "peak-level", value);
      break;
    case PROP_CUTOFF_END:
      g_object_get_property ((GObject *) (src->fltenv), "floor-level", value);
      break;
    case PROP_CUTOFF_CURVE:
      g_object_get_property ((GObject *) (src->fltenv), "curve", value);
      break;
    case PROP_FILTER_ROUTING:
      g_value_set_enum (value, src->flt_routing);
      break;
    case PROP_FILTER:
    case PROP_RESONANCE:
      g_object_get_property ((GObject *) (src->filter), pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_e_beats_dispose (GObject * object)
{
  GstBtEBeats *src = GSTBT_E_BEATS (object);

  g_clear_object (&src->volenv_t);
  g_clear_object (&src->volenv_n);
  g_clear_object (&src->freqenv_t1);
  g_clear_object (&src->freqenv_t2);
  g_clear_object (&src->fltenv);
  g_clear_object (&src->osc_t1);
  g_clear_object (&src->osc_t2);
  g_clear_object (&src->mix);
  g_clear_object (&src->osc_n);
  g_clear_object (&src->filter);

  G_OBJECT_CLASS (gstbt_e_beats_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_e_beats_init (GstBtEBeats * src)
{
  /* synth components */
  src->osc_t1 = gstbt_osc_synth_new ();
  src->osc_t2 = gstbt_osc_synth_new ();
  src->osc_n = gstbt_osc_synth_new ();
  src->volenv_t = gstbt_envelope_d_new ();
  src->volenv_n = gstbt_envelope_d_new ();
  src->freqenv_t1 = gstbt_envelope_d_new ();
  src->freqenv_t2 = gstbt_envelope_d_new ();
  src->fltenv = gstbt_envelope_d_new ();
  src->filter = gstbt_filter_svf_new ();
  src->mix = gstbt_combine_new ();
  src->flt_routing = GSTBT_E_BEATS_FILTER_ROUTING_T_N;

  g_object_set (src->osc_t1, "wave", GSTBT_OSC_SYNTH_WAVE_SINE, NULL);
  g_object_set (src->osc_t2, "wave", GSTBT_OSC_SYNTH_WAVE_SINE, NULL);
  g_object_set (src->osc_n, "wave", GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE, NULL);
  g_object_set (src->volenv_t, "peak-level", 0.8, NULL);
  g_object_set (src->volenv_n, "peak-level", 0.8, NULL);
  g_object_set (src->freqenv_t1, "peak-level", 200.0, "floor-level", 10.0,
      NULL);
  g_object_set (src->freqenv_t2, "peak-level", 210.0, "floor-level", 30.0,
      NULL);
  g_object_set (src->fltenv, "peak-level", 0.8, "floor-level", 0.2, NULL);

  gst_object_add_control_binding ((GstObject *) src->osc_t1,
      gst_direct_control_binding_new_absolute ((GstObject *) src->osc_t1,
          "volume", (GstControlSource *) src->volenv_t));
  gst_object_add_control_binding ((GstObject *) src->osc_t1,
      gst_direct_control_binding_new_absolute ((GstObject *) src->osc_t1,
          "frequency", (GstControlSource *) src->freqenv_t1));
  gst_object_add_control_binding ((GstObject *) src->osc_t2,
      gst_direct_control_binding_new_absolute ((GstObject *) src->osc_t2,
          "volume", (GstControlSource *) src->volenv_t));
  gst_object_add_control_binding ((GstObject *) src->osc_t2,
      gst_direct_control_binding_new_absolute ((GstObject *) src->osc_t2,
          "frequency", (GstControlSource *) src->freqenv_t2));
  gst_object_add_control_binding ((GstObject *) src->osc_n,
      gst_direct_control_binding_new_absolute ((GstObject *) src->osc_n,
          "volume", (GstControlSource *) src->volenv_n));
  gst_object_add_control_binding ((GstObject *) src->filter,
      gst_direct_control_binding_new_absolute ((GstObject *) src->filter,
          "cut-off", (GstControlSource *) src->fltenv));
}

static void
gstbt_e_beats_class_init (GstBtEBeatsClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  GObjectClass *component, *env_component;

  audio_synth_class->process = gstbt_e_beats_process;
  audio_synth_class->reset = gstbt_e_beats_reset;
  audio_synth_class->negotiate = gstbt_e_beats_negotiate;
  audio_synth_class->setup = gstbt_e_beats_setup;

  gobject_class->set_property = gstbt_e_beats_set_property;
  gobject_class->get_property = gstbt_e_beats_get_property;
  gobject_class->dispose = gstbt_e_beats_dispose;

  // describe us
  gst_element_class_set_static_metadata (element_class,
      "EBeats", "Source/Audio",
      "Electric drum synthesizer", "Stefan Sauer <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "-gst" G_DIR_SEPARATOR_S "GstBtEBeats.html");

  // register properties
  PROP (VOLUME) = g_param_spec_uint ("volume", "Tone volume", "Tone volume",
      0, 255, 0,
      G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  env_component = g_type_class_ref (GSTBT_TYPE_ENVELOPE_D);

  PROP (T1_WAVE) = g_param_spec_enum ("t1-wave", "Tonal waveform 1",
      "Tonal oscillator waveform",
      GSTBT_TYPE_OSC_SYNTH_TONAL_WAVE, GSTBT_OSC_SYNTH_WAVE_SINE,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (T1_FREQ_START) = g_param_spec_double ("t1-freq-start", "T1 Start Freq.",
      "Initial frequency of tone", 1.0, 10000.0, 200.0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (T1_FREQ_END) = g_param_spec_double ("t1-freq-end", "T1 End Freq.",
      "Final frequency of tone", 1.0, 1000.0, 10.0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (T1_FREQ_CURVE) = bt_g_param_spec_clone_as (env_component, "curve",
      "t1-freq-curve", "T1 Freq. Curve", NULL);

  PROP (T2_WAVE) = g_param_spec_enum ("t2-wave", "Tonal waveform 2",
      "Tonal oscillator waveform",
      GSTBT_TYPE_OSC_SYNTH_TONAL_WAVE, GSTBT_OSC_SYNTH_WAVE_SINE,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (T2_FREQ_START) = g_param_spec_double ("t2-freq-start", "T2 Start Freq.",
      "Initial frequency of tone", 1.0, 10000.0, 210.0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (T2_FREQ_END) = g_param_spec_double ("t2-freq-end", "T2 End Freq.",
      "Final frequency of tone", 1.0, 1000.0, 30.0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (T2_FREQ_CURVE) = bt_g_param_spec_clone_as (env_component, "curve",
      "t2-freq-curve", "T2 Freq. Curve", NULL);

  PROP (T_VOLUME) = bt_g_param_spec_clone_as (env_component, "peak-level",
      "t-volume", "T Volume", NULL);
  bt_g_param_spec_override_range (GParamSpecDouble, PROP (T_VOLUME), 0.0, 1.0,
      0.8);
  PROP (T_DECAY) =
      bt_g_param_spec_clone_as (env_component, "decay", "t-decay", "T Decay",
      NULL);
  PROP (T_VOL_CURVE) =
      bt_g_param_spec_clone_as (env_component, "curve", "t-vol-curve",
      "T Vol. Curve", NULL);

  component = g_type_class_ref (GSTBT_TYPE_COMBINE);
  PROP (T_COMBINE) = bt_g_param_spec_clone (component, "combine");
  g_type_class_unref (component);

  PROP (N_WAVE) = g_param_spec_enum ("n-wave", "Noise waveform",
      "Noise oscillator waveform",
      GSTBT_TYPE_OSC_SYNTH_NOISE_WAVE, GSTBT_OSC_SYNTH_WAVE_WHITE_NOISE,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (N_VOLUME) =
      bt_g_param_spec_clone_as (env_component, "peak-level", "n-volume",
      "N Volume", NULL);
  bt_g_param_spec_override_range (GParamSpecDouble, PROP (N_VOLUME), 0.0, 1.0,
      0.8);
  PROP (N_DECAY) =
      bt_g_param_spec_clone_as (env_component, "decay", "n-decay", "N Decay",
      NULL);
  PROP (N_VOL_CURVE) =
      bt_g_param_spec_clone_as (env_component, "curve", "n-vol-curve",
      "N Vol. Curve", NULL);

  component = g_type_class_ref (GSTBT_TYPE_FILTER_SVF);
  PROP (FILTER_ROUTING) = g_param_spec_enum ("filter-routing", "Filterrouting",
      "Configuration to which parts of the signal to apply the filter",
      GSTBT_TYPE_E_BEATS_FILTER_ROUTING_TYPE, GSTBT_E_BEATS_FILTER_ROUTING_T_N,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (FILTER) = bt_g_param_spec_clone (component, "filter");
  PROP (CUTOFF_START) = g_param_spec_double ("cutoff-start", "Start Cut-Off",
      "Initial audio filter cut-off frequency", 0.0, 1.0, 0.8,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (CUTOFF_END) = g_param_spec_double ("cutoff-end", "End Cut-Off",
      "Final audio filter cut-off frequency", 0.0, 1.0, 0.2,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  PROP (CUTOFF_CURVE) = bt_g_param_spec_clone_as (env_component, "curve",
      "cufoff-curve", "Cut-Off Curve", NULL);
  PROP (RESONANCE) = bt_g_param_spec_clone (component, "resonance");
  g_type_class_unref (component);

  g_type_class_unref (env_component);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
