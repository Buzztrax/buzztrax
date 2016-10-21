/* Buzztrax
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
 * Simple monophonic audio synthesizer with a single oscillator (#GstBtOscSynth)
 * , an attack-decay envelope (#GstBtEnvelopeAD) and a state-variable filter
 * (#GstBtFilterSVF).
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 simsyn num-buffers=10 note="c-4" ! autoaudiosink
 * ]| Render a sine wave tone.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "plugin.h"
#include "simsyn.h"

#ifdef HAVE_GST_CONTROL_BINDING_ABS
#include <gst/controller/gstdirectcontrolbinding.h>
#else
#include "gst/gstdirectcontrolbinding.h"
#endif

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_TUNING = 1,
  // dynamic class properties
  PROP_NOTE, PROP_WAVE, PROP_VOLUME, PROP_ATTACK, PROP_DECAY, PROP_FILTER,
  PROP_CUTOFF, PROP_RESONANCE,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

//-- the class

G_DEFINE_TYPE (GstBtSimSyn, gstbt_sim_syn, GSTBT_TYPE_AUDIO_SYNTH);

//-- audiosynth vmethods

static void
gstbt_sim_syn_negotiate (GstBtAudioSynth * base, GstCaps * caps)
{
  gint i, n = gst_caps_get_size (caps);

  for (i = 0; i < n; i++) {
    gst_structure_fixate_field_nearest_int (gst_caps_get_structure (caps, i),
        "channels", 1);
  }
}

static void
gstbt_sim_syn_setup (GstBtAudioSynth * base, GstAudioInfo * info)
{
  GstBtSimSyn *src = ((GstBtSimSyn *) base);

  g_object_set (src->osc, "sample-rate", info->rate, NULL);
}

static void
gstbt_sim_syn_reset (GstBtAudioSynth * base)
{
  GstBtSimSyn *src = ((GstBtSimSyn *) base);

  src->note = GSTBT_NOTE_OFF;
  gstbt_envelope_reset ((GstBtEnvelope *) src->volenv);
}

static gboolean
gstbt_sim_syn_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  GstBtSimSyn *src = ((GstBtSimSyn *) base);

  if ((src->note != GSTBT_NOTE_OFF)
      && gstbt_envelope_is_running ((GstBtEnvelope *) src->volenv,
          src->osc->offset)) {
    gint16 *d = (gint16 *) info->data;
    guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;

    gstbt_osc_synth_process (src->osc, ct, d);
    gstbt_filter_svf_process (src->filter, ct, d);
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

  switch (prop_id) {
    case PROP_TUNING:
      g_object_set_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_NOTE:
      if ((src->note = g_value_get_enum (value))) {
        GST_INFO_OBJECT (object, "new note -> '%d'", src->note);
        if (src->note != GSTBT_NOTE_OFF) {
          gdouble freq =
              gstbt_tone_conversion_translate_from_number (src->n2f, src->note);
          gstbt_envelope_ad_setup (src->volenv,
              ((GstBtAudioSynth *) src)->samplerate);
          g_object_set (src->osc, "frequency", freq, NULL);
          gstbt_osc_synth_trigger (src->osc);
          gstbt_filter_svf_trigger (src->filter);
        }
      }
      break;
    case PROP_WAVE:
      g_object_set_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_VOLUME:
      g_object_set_property ((GObject *) (src->volenv), "peak-level", value);
      break;
    case PROP_ATTACK:
    case PROP_DECAY:
      g_object_set_property ((GObject *) (src->volenv), pspec->name, value);
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

  switch (prop_id) {
    case PROP_TUNING:
      g_object_get_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), "wave", value);
      break;
    case PROP_VOLUME:
      g_object_get_property ((GObject *) (src->volenv), "peak-level", value);
      break;
    case PROP_ATTACK:
    case PROP_DECAY:
      g_object_get_property ((GObject *) (src->volenv), pspec->name, value);
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

  g_clear_object (&src->n2f);
  g_clear_object (&src->volenv);
  g_clear_object (&src->osc);
  g_clear_object (&src->filter);

  G_OBJECT_CLASS (gstbt_sim_syn_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_sim_syn_init (GstBtSimSyn * src)
{
  src->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);

  /* synth components */
  src->osc = gstbt_osc_synth_new ();
  src->volenv = gstbt_envelope_ad_new ();
  src->filter = gstbt_filter_svf_new ();
  gst_object_add_control_binding ((GstObject *) src->osc,
      gst_direct_control_binding_new_absolute ((GstObject *) src->osc, "volume",
          (GstControlSource *) src->volenv));
  g_object_set (src->volenv, "peak-level", 0.8, NULL);
}

static void
gstbt_sim_syn_class_init (GstBtSimSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  GObjectClass *component;

  audio_synth_class->process = gstbt_sim_syn_process;
  audio_synth_class->reset = gstbt_sim_syn_reset;
  audio_synth_class->negotiate = gstbt_sim_syn_negotiate;
  audio_synth_class->setup = gstbt_sim_syn_setup;

  gobject_class->set_property = gstbt_sim_syn_set_property;
  gobject_class->get_property = gstbt_sim_syn_get_property;
  gobject_class->dispose = gstbt_sim_syn_dispose;

  // describe us
  gst_element_class_set_static_metadata (element_class,
      "Simple Synth", "Source/Audio",
      "Simple audio synthesizer", "Stefan Kost <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "-gst" G_DIR_SEPARATOR_S "GstBtSimSyn.html");

  // register properties
  component = g_type_class_ref (GSTBT_TYPE_TONE_CONVERSION);
  PROP (TUNING) = bt_g_param_spec_clone (component, "tuning");
  g_type_class_unref (component);

  PROP (NOTE) = g_param_spec_enum ("note", "Musical note",
      "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
      G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  component = g_type_class_ref (GSTBT_TYPE_OSC_SYNTH);
  PROP (WAVE) = bt_g_param_spec_clone (component, "wave");
  g_type_class_unref (component);

  component = g_type_class_ref (GSTBT_TYPE_ENVELOPE_AD);
  PROP (VOLUME) =
      bt_g_param_spec_clone_as (component, "peak-level", "volume", "Volume",
      NULL);
  bt_g_param_spec_override_range (GParamSpecDouble, PROP (VOLUME), 0.0, 1.0,
      0.8);
  PROP (ATTACK) = bt_g_param_spec_clone (component, "attack");
  PROP (DECAY) = bt_g_param_spec_clone (component, "decay");
  g_type_class_unref (component);

  component = g_type_class_ref (GSTBT_TYPE_FILTER_SVF);
  PROP (FILTER) = bt_g_param_spec_clone (component, "filter");
  PROP (CUTOFF) = bt_g_param_spec_clone (component, "cut-off");
  PROP (RESONANCE) = bt_g_param_spec_clone (component, "resonance");
  g_type_class_unref (component);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
