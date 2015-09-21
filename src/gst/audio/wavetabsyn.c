/* Buzztrax
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * wavetabsyn.c: wavetable synthesizer
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
 * SECTION:wavetabsyn
 * @title: GstBtWaveTabSyn
 * @short_description: wavetable synthesizer
 *
 * A synth that uses the wavetable osc. I picks a cycle from the selected
 * wavetable entry and repeats it as a osc. The offset parameter allows scanning
 * though the waveform.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "plugin.h"
#include "wavetabsyn.h"

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_WAVE_CALLBACKS = 1,
  PROP_TUNING,
  // dynamic class properties
  PROP_NOTE, PROP_NOTE_LENGTH, PROP_WAVE, PROP_OFFSET, PROP_ATTACK,
  PROP_PEAK_VOLUME, PROP_DECAY, PROP_SUSTAIN_VOLUME, PROP_RELEASE,
  N_PROPERTIES
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

#define INNER_LOOP 64

//-- the class

G_DEFINE_TYPE (GstBtWaveTabSyn, gstbt_wave_tab_syn, GSTBT_TYPE_AUDIO_SYNTH);

//-- audiosynth vmethods

static void
gstbt_wave_tab_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstBtWaveTabSyn *src = ((GstBtWaveTabSyn *) base);
  gint i, n = gst_caps_get_size (caps);

  gstbt_osc_wave_setup (src->osc);
  for (i = 0; i < n; i++) {
    gst_structure_fixate_field_nearest_int (gst_caps_get_structure (caps, i),
        "channels", src->osc->channels);
  }
  src->note = GSTBT_NOTE_OFF;
  gstbt_envelope_reset ((GstBtEnvelope *) src->volenv);
  GST_DEBUG_OBJECT (src, "reset");
}

static gboolean
gstbt_wave_tab_syn_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  GstBtWaveTabSyn *src = ((GstBtWaveTabSyn *) base);

  if (src->osc->process && src->note != GSTBT_NOTE_OFF &&
      gstbt_envelope_is_running ((GstBtEnvelope *) src->volenv, src->offset)) {
    gint16 *d = (gint16 *) info->data;
    guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
    gint ch = ((GstBtAudioSynth *) src)->channels;
    guint sz = src->cycle_size;
    guint pos = src->cycle_pos;
    guint p = 0;                // work pos in buffer
    guint64 offset = src->offset;
    guint64 off = src->wt_offset * (src->duration - src->cycle_size) / 0xFFFF;
    GstControlSource *volenv = (GstControlSource *) src->volenv;

    GST_DEBUG_OBJECT (src, "processing %d sampels with %d channels", ct, ch);

    // do we have a unfinished cycle?
    if (pos > 0) {
      guint new_pos;
      p = sz - pos;
      if (p > ct) {
        p = ct;
        new_pos = pos + ct;
      } else {
        new_pos = 0;
      }
      src->osc->process (src->osc, off + pos, p, &d[0]);
      ct -= p;
      pos = new_pos;
    }
    // do full cycles
    while (ct >= sz) {
      src->osc->process (src->osc, off, sz, &d[p * ch]);
      ct -= sz;
      p += sz;
    }
    // fill buffer with partial cycle
    if (ct > 0) {
      src->osc->process (src->osc, off, ct, &d[p * ch]);
      pos += ct;
    }
    src->cycle_pos = pos;
    // apply volume envelope
    ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
    if (ch == 1) {
      gdouble amp;
      guint i = 0, j;
      while (i < ct) {
        j = ct - i;
        gst_control_source_get_value (volenv, offset + i, &amp);
        for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
          d[i] *= amp;
        }
      }
    } else if (ch == 2) {
      gdouble amp;
      guint i = 0, j;
      while (i < ct) {
        j = ct - i;
        gst_control_source_get_value (volenv, offset + i, &amp);
        for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
          d[(i << 1)] *= amp;
          d[(i << 1) + 1] *= amp;
        }
      }
    }
    src->offset += ct;
    return TRUE;
  }
  return FALSE;
}

//-- interfaces

//-- gobject vmethods

static void
gstbt_wave_tab_syn_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtWaveTabSyn *src = GSTBT_WAVE_TAB_SYN (object);

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
    case PROP_WAVE:
      g_object_set_property ((GObject *) (src->osc), pspec->name, value);
      break;
    case PROP_TUNING:
      g_object_set_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_NOTE:
      if ((src->note = g_value_get_enum (value))) {
        gdouble freq =
            gstbt_tone_conversion_translate_from_number (src->n2f, src->note);

        g_object_set (src->osc, "frequency", freq, NULL);
        g_object_get (src->osc, "duration", &src->duration, NULL);

        // this is the chunk that we need to repeat for the selected tone
        src->cycle_size = ((GstBtAudioSynth *) src)->samplerate / freq;
        src->cycle_pos = 0;

        src->offset = 0;
        gstbt_envelope_adsr_setup (src->volenv,
            ((GstBtAudioSynth *) src)->samplerate,
            ((GstBtAudioSynth *) src)->ticktime);
      }
      break;
    case PROP_OFFSET:
      src->wt_offset = g_value_get_uint (value);
      break;
    case PROP_NOTE_LENGTH:
    case PROP_ATTACK:
    case PROP_DECAY:
    case PROP_RELEASE:
      g_object_set_property ((GObject *) (src->volenv), pspec->name, value);
      break;
    case PROP_PEAK_VOLUME:
      g_object_set_property ((GObject *) (src->volenv), "peak-level", value);
      break;
    case PROP_SUSTAIN_VOLUME:
      g_object_set_property ((GObject *) (src->volenv), "sustain-level", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_tab_syn_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtWaveTabSyn *src = GSTBT_WAVE_TAB_SYN (object);

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), pspec->name, value);
      break;
    case PROP_TUNING:
      g_object_get_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_OFFSET:
      g_value_set_uint (value, src->wt_offset);
      break;
    case PROP_NOTE_LENGTH:
    case PROP_ATTACK:
    case PROP_DECAY:
    case PROP_RELEASE:
      g_object_get_property ((GObject *) (src->volenv), pspec->name, value);
      break;
    case PROP_PEAK_VOLUME:
      g_object_get_property ((GObject *) (src->volenv), "peak-level", value);
      break;
    case PROP_SUSTAIN_VOLUME:
      g_object_get_property ((GObject *) (src->volenv), "sustain-level", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_tab_syn_dispose (GObject * object)
{
  GstBtWaveTabSyn *src = GSTBT_WAVE_TAB_SYN (object);

  g_clear_object (&src->n2f);
  g_clear_object (&src->osc);
  g_clear_object (&src->volenv);

  G_OBJECT_CLASS (gstbt_wave_tab_syn_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_wave_tab_syn_init (GstBtWaveTabSyn * src)
{
  src->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);

  /* synth components */
  src->osc = gstbt_osc_wave_new ();
  src->volenv = gstbt_envelope_adsr_new ();
  g_object_set (src->volenv, "peak-level", 0.8, "sustain-level", 0.4, NULL);
}

static void
gstbt_wave_tab_syn_class_init (GstBtWaveTabSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  GObjectClass *component;

  audio_synth_class->process = gstbt_wave_tab_syn_process;
  audio_synth_class->setup = gstbt_wave_tab_syn_setup;

  gobject_class->set_property = gstbt_wave_tab_syn_set_property;
  gobject_class->get_property = gstbt_wave_tab_syn_get_property;
  gobject_class->dispose = gstbt_wave_tab_syn_dispose;

  // describe us
  gst_element_class_set_static_metadata (element_class,
      "Wavetable Synth",
      "Source/Audio",
      "Wavetable synthesizer", "Stefan Sauer <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "-gst" G_DIR_SEPARATOR_S
      "GstBtWaveTabSyn.html");

  // register properties
  PROP (WAVE_CALLBACKS) = g_param_spec_pointer ("wave-callbacks",
      "Wavetable Callbacks", "The wave-table access callbacks",
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  component = g_type_class_ref (GSTBT_TYPE_TONE_CONVERSION);
  PROP (TUNING) = bt_g_param_spec_clone (component, "tuning");
  g_type_class_unref (component);

  PROP (NOTE) = g_param_spec_enum ("note", "Musical note",
      "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
      G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  component = g_type_class_ref (GSTBT_TYPE_OSC_WAVE);
  PROP (WAVE) = bt_g_param_spec_clone (component, "wave");
  g_type_class_unref (component);

  PROP (OFFSET) = g_param_spec_uint ("offset", "Offset", "Wave table offset", 0,
      0xFFFF, 0,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  component = g_type_class_ref (GSTBT_TYPE_ENVELOPE_ADSR);
  PROP (NOTE_LENGTH) = bt_g_param_spec_clone (component, "length");
  PROP (ATTACK) = bt_g_param_spec_clone (component, "attack");
  PROP (PEAK_VOLUME) =
      bt_g_param_spec_clone_as (component, "peak-level", "peak-volume",
      "Peak Volume", NULL);
  bt_g_param_spec_override_range (GParamSpecDouble, PROP (PEAK_VOLUME), 0.0,
      1.0, 0.8);
  PROP (DECAY) = bt_g_param_spec_clone (component, "decay");
  PROP (SUSTAIN_VOLUME) =
      bt_g_param_spec_clone_as (component, "sustain-level", "sustain-volume",
      "Sustain Volume", NULL);
  bt_g_param_spec_override_range (GParamSpecDouble, PROP (SUSTAIN_VOLUME), 0.0,
      1.0, 0.4);
  PROP (RELEASE) = bt_g_param_spec_clone (component, "release");
  g_type_class_unref (component);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
