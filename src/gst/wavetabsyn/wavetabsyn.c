/* GStreamer
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
#include "gst/propertymeta.h"
#include "wavetabsyn.h"

#define GST_CAT_DEFAULT wave_tab_syn_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_WAVE_CALLBACKS = 1,
  PROP_TUNING,
  // dynamic class properties
  PROP_NOTE,
  PROP_NOTE_LENGTH,
  PROP_WAVE,
  PROP_OFFSET,
  PROP_ATTACK,
  PROP_PEAK_VOLUME,
  PROP_DECAY,
  PROP_SUSTAIN_VOLUME,
  PROP_RELEASE
};

#define INNER_LOOP 64

//-- the class

G_DEFINE_TYPE_WITH_CODE (GstBtWaveTabSyn, gstbt_wave_tab_syn,
    GSTBT_TYPE_AUDIO_SYNTH, G_IMPLEMENT_INTERFACE (GSTBT_TYPE_PROPERTY_META,
        NULL));

//-- audiosynth vmethods

static gboolean
gstbt_wave_tab_syn_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstBtWaveTabSyn *src = ((GstBtWaveTabSyn *) base);
  GstStructure *structure;
  gint i, n = gst_caps_get_size (caps), c = src->osc->channels;

  gstbt_osc_wave_setup (src->osc);

  for (i = 0; i < n; i++) {
    structure = gst_caps_get_structure (caps, i);
    gst_structure_fixate_field_nearest_int (structure, "channels", c);
  }
  return TRUE;
}

static gboolean
gstbt_wave_tab_syn_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  GstBtWaveTabSyn *src = ((GstBtWaveTabSyn *) base);

  if (src->osc->process) {
    gint16 *d = (gint16 *) info->data;
    guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
    gint ch = ((GstBtAudioSynth *) src)->channels;
    guint sz = src->cycle_size;
    guint pos = src->cycle_pos;
    guint p = 0;
    guint64 off = src->offset * (src->duration - src->cycle_size) / 0xFFFF;

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
        amp = gstbt_envelope_get ((GstBtEnvelope *) src->volenv,
            MIN (INNER_LOOP, j));
        for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
          d[i] *= amp;
        }
      }
    } else if (ch == 2) {
      gdouble amp;
      guint i = 0, j;
      while (i < ct) {
        j = ct - i;
        amp = gstbt_envelope_get ((GstBtEnvelope *) src->volenv,
            MIN (INNER_LOOP, j));
        for (j = 0; ((j < INNER_LOOP) && (i < ct)); j++, i++) {
          d[(i << 1)] *= amp;
          d[(i << 1) + 1] *= amp;
        }
      }
    }
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

  if (src->dispose_has_run)
    return;

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
        GST_DEBUG ("new note -> '%d'", src->note);
        gdouble freq =
            gstbt_tone_conversion_translate_from_number (src->n2f, src->note);

        g_object_set (src->osc, "frequency", freq, NULL);
        g_object_get (src->osc, "duration", &src->duration, NULL);

        // this is the chunk that we need to repeat for the selected tone
        src->cycle_size = ((GstBtAudioSynth *) src)->samplerate / freq;
        src->cycle_pos = 0;

        gstbt_envelope_adsr_setup (src->volenv,
            ((GstBtAudioSynth *) src)->samplerate,
            ((GstBtAudioSynth *) src)->ticktime);
      }
      break;
    case PROP_OFFSET:
      src->offset = g_value_get_uint (value);
      break;
    case PROP_NOTE_LENGTH:
    case PROP_ATTACK:
    case PROP_PEAK_VOLUME:
    case PROP_DECAY:
    case PROP_SUSTAIN_VOLUME:
    case PROP_RELEASE:
      g_object_set_property ((GObject *) (src->volenv), pspec->name, value);
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

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
    case PROP_WAVE:
      g_object_get_property ((GObject *) (src->osc), pspec->name, value);
      break;
    case PROP_TUNING:
      g_object_get_property ((GObject *) (src->n2f), "tuning", value);
      break;
    case PROP_OFFSET:
      g_value_set_uint (value, src->offset);
      break;
    case PROP_NOTE_LENGTH:
    case PROP_ATTACK:
    case PROP_PEAK_VOLUME:
    case PROP_DECAY:
    case PROP_SUSTAIN_VOLUME:
    case PROP_RELEASE:
      g_object_get_property ((GObject *) (src->volenv), pspec->name, value);
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

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  if (src->n2f)
    g_object_unref (src->n2f);
  if (src->osc)
    g_object_unref (src->osc);
  if (src->volenv)
    g_object_unref (src->volenv);

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
}

static void
gstbt_wave_tab_syn_class_init (GstBtWaveTabSynClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  GParamSpec *pspec;

  audio_synth_class->process = gstbt_wave_tab_syn_process;
  audio_synth_class->setup = gstbt_wave_tab_syn_setup;

  gobject_class->set_property = gstbt_wave_tab_syn_set_property;
  gobject_class->get_property = gstbt_wave_tab_syn_get_property;
  gobject_class->dispose = gstbt_wave_tab_syn_dispose;

  // describe us
  gst_element_class_set_static_metadata (element_class,
      "WaveTabSyn",
      "Source/Audio",
      "Wavetable synthesizer", "Stefan Sauer <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtWaveTabSyn.html");

  // register own properties
  g_object_class_install_property (gobject_class, PROP_WAVE_CALLBACKS,
      g_param_spec_pointer ("wave-callbacks", "Wavetable Callbacks",
          "The wave-table access callbacks",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TUNING,
      g_param_spec_enum ("tuning", "Tuning", "Harmonic tuning",
          GSTBT_TYPE_TONE_CONVERSION_TUNING,
          GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE_LENGTH,
      g_param_spec_uint ("length", "Note length", "Note length in ticks",
          1, 255, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  pspec = g_param_spec_uint ("wave", "Wave", "Wave index", 1, 200, 1,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  g_param_spec_set_qdata (pspec, gstbt_property_meta_quark,
      GUINT_TO_POINTER (1));
  g_param_spec_set_qdata (pspec, gstbt_property_meta_quark_flags,
      GUINT_TO_POINTER (GSTBT_PROPERTY_META_WAVE));
  g_object_class_install_property (gobject_class, PROP_WAVE, pspec);

  g_object_class_install_property (gobject_class, PROP_OFFSET,
      g_param_spec_uint ("offset", "Offset", "Wave table offset", 0, 0xFFFF, 0,
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
      g_param_spec_double ("release", "RELEASE",
          "Volume release of the tone in seconds", 0.001, 4.0, 0.5,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

//-- plugin

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "wavetabsyn",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "wavetable synthesizer");

  return gst_element_register (plugin, "wavetabsyn", GST_RANK_NONE,
      GSTBT_TYPE_WAVE_TAB_SYN);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    wavetabsyn,
    "Wavetable synthesizer",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, PACKAGE_ORIGIN);
