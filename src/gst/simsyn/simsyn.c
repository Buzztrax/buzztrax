/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * simsyn.c: simple audio synthesizer for gstreamer
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
 * SECTION:simsyn
 * @title: GstBtSimSyn
 * @short_description: simple monophonic audio synthesizer
 *
 * Simple monophonic audio synthesizer with a decay envelope and a
 * state-variable filter.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch simsyn num-buffers=1000 note="c-4" ! autoaudiosink
 * ]| Render a sine wave tone.
 * </refsect2>
 */
/* TODO(ensonic): improvements
 * - implement property-meta iface (see gstbml) - why actually?
 *   - we could pretty print filter cut-off
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "simsyn.h"

#define GST_CAT_DEFAULT sim_syn_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_TUNING = 1,
  // dynamic class properties
  PROP_NOTE,
  PROP_WAVE,
  PROP_VOLUME,
  PROP_DECAY,
  PROP_FILTER,
  PROP_CUTOFF,
  PROP_RESONANCE
};

//-- the class

G_DEFINE_TYPE (GstBtSimSyn, gstbt_sim_syn, GSTBT_TYPE_AUDIO_SYNTH);

//-- audiosynth vmethods

static gboolean
gstbt_sim_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstStructure *structure;
  gint i, n = gst_caps_get_size (caps);

  for (i = 0; i < n; i++) {
    structure = gst_caps_get_structure (caps, i);
    gst_structure_fixate_field_nearest_int (structure, "channels", 1);
  }
  return TRUE;
}

static gboolean
gstbt_sim_syn_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  GstBtSimSyn *src = ((GstBtSimSyn *) base);

  if ((src->note != GSTBT_NOTE_OFF)
      && gstbt_envelope_is_running ((GstBtEnvelope *) src->volenv)) {
    gint16 *d = (gint16 *) info->data;
    guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;

    src->osc->process (src->osc, ct, d);
    if (src->filter->process)
      src->filter->process (src->filter, ct, d);
    return TRUE;
  }
  return FALSE;
}

//-- gobject vmethods

static void
gstbt_sim_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_TUNING:
      g_object_set_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_NOTE:
      if ((src->note = g_value_get_enum (value))) {
        GST_DEBUG ("new note -> '%d'", src->note);
        gdouble freq =
            gstbt_tone_conversion_translate_from_number (src->n2f, src->note);
        gstbt_envelope_d_setup (src->volenv,
            ((GstBtAudioSynth *) src)->samplerate, src->decay, src->volume);
        g_object_set (src->osc, "frequency", freq, NULL);
      }
      break;
    case PROP_WAVE:
      g_object_set_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_VOLUME:
      src->volume = g_value_get_double (value);
      break;
    case PROP_DECAY:
      src->decay = g_value_get_double (value);
      break;
    case PROP_FILTER:
    case PROP_CUTOFF:
    case PROP_RESONANCE:
      g_object_set_property ((GObject *) (src->filter), pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_sim_syn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_TUNING:
      g_object_get_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_VOLUME:
      g_value_set_double (value, src->volume);
      break;
    case PROP_DECAY:
      g_value_set_double (value, src->decay);
      break;
    case PROP_FILTER:
    case PROP_CUTOFF:
    case PROP_RESONANCE:
      g_object_get_property ((GObject *) (src->filter), pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_sim_syn_dispose (GObject * object)
{
  GstBtSimSyn *src = GSTBT_SIM_SYN (object);

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  if (src->n2f)
    g_object_unref (src->n2f);
  if (src->volenv)
    g_object_unref (src->volenv);
  if (src->osc)
    g_object_unref (src->osc);
  if (src->filter)
    g_object_unref (src->filter);

  G_OBJECT_CLASS (gstbt_sim_syn_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_sim_syn_init (GstBtSimSyn * src)
{
  /* set base parameters */
  src->volume = 0.8;
  src->decay = 0.5;

  src->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);

  /* synth components */
  src->osc = gstbt_osc_synth_new ();
  src->volenv = gstbt_envelope_d_new ();
  src->filter = gstbt_filter_svf_new ();
  g_object_set (src->osc, "volume-envelope", src->volenv, NULL);
}

static void
gstbt_sim_syn_class_init (GstBtSimSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;

  audio_synth_class->process = gstbt_sim_syn_process;
  audio_synth_class->setup = gstbt_sim_syn_setup;

  gobject_class->set_property = gstbt_sim_syn_set_property;
  gobject_class->get_property = gstbt_sim_syn_get_property;
  gobject_class->dispose = gstbt_sim_syn_dispose;

  // describe us
  gst_element_class_set_static_metadata (element_class,
      "Simple Synth",
      "Source/Audio",
      "Simple audio synthesizer", "Stefan Kost <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtSimSyn.html");

  // register own properties
  g_object_class_install_property (gobject_class, PROP_TUNING,
      g_param_spec_enum ("tuning", "Tuning", "Harmonic tuning",
          GSTBT_TYPE_TONE_CONVERSION_TUNING,
          GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GSTBT_TYPE_OSC_SYNTH_WAVE, GSTBT_OSC_SYNTH_WAVE_SINE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of tone", 0.0, 1.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DECAY,
      g_param_spec_double ("decay", "Decay",
          "Volume decay of the tone in seconds", 0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FILTER,
      g_param_spec_enum ("filter", "Filtertype", "Type of audio filter",
          GSTBT_TYPE_FILTER_SVF_TYPE, GSTBT_FILTER_SVF_LOWPASS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CUTOFF,
      g_param_spec_double ("cut-off", "Cut-Off",
          "Audio filter cut-off frequency", 0.0, 1.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RESONANCE,
      g_param_spec_double ("resonance", "Resonance", "Audio filter resonance",
          0.7, 25.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  /* it does not show up :/  
     GObjectClass * filter_klass = g_type_class_ref (GSTBT_TYPE_FILTER_SVF);
     g_object_class_install_property (gobject_class, PROP_RESONANCE,
     g_param_spec_override ("resonance", 
     g_object_class_find_property (filter_klass, "resonance")));
     g_type_class_unref (filter_klass);
   */
  /* the paramspec is not reusable, we need a g_param_spec_clone()
     GObjectClass * filter_klass = g_type_class_ref (GSTBT_TYPE_FILTER_SVF);
     g_object_class_install_property (gobject_class, PROP_RESONANCE, 
     g_param_spec_ref (
     g_object_class_find_property (filter_klass, "resonance")));
     g_type_class_unref (filter_klass);
   */
  /* we could define an interface for each component and have the component
   * implement the iface, here we also implement the iface and thus inherit the
   * properties. This unfortunately won't allow us to rename properties. If
   * e.g. we use two Osc components, we want to prefix them with e.g. 'Osc1-'
   * and 'Osc2-'. What we'd like to have is:
   *
   * g_object_class_proxy_property (gobject_class, "resonance", filter_klass,
   *   "resonance"); 
   */
}
