/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * wavereplay.c: wavetable player
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
 * SECTION:wavereplay
 * @title: GstBtWaveReplay
 * @short_description: wavetable player
 *
 * Plays wavetable assets pre-loaded by the application. Unlike in tracker
 * machines, the wave is implicitly triggered at the start and one can seek in
 * the song without loosing the audio.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gst/propertymeta.h"
#include "wavereplay.h"

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_WAVE_CALLBACKS = 1,
  // dynamic class properties
  PROP_WAVE,
  PROP_WAVE_LEVEL
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (GstBtWaveReplay, gstbt_wave_replay,
    GSTBT_TYPE_AUDIO_SYNTH, G_IMPLEMENT_INTERFACE (GSTBT_TYPE_PROPERTY_META,
        NULL));

//-- audiosynth vmethods

static gboolean
gstbt_wave_replay_setup (GstBtAudioSynth * base, GstPad * pad, GstCaps * caps)
{
  GstBtWaveReplay *src = ((GstBtWaveReplay *) base);
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
gstbt_wave_replay_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  GstBtWaveReplay *src = ((GstBtWaveReplay *) base);

  if (src->osc->process) {
    gint16 *d = (gint16 *) info->data;
    guint ct = ((GstBtAudioSynth *) src)->generate_samples_per_buffer;
    guint64 off = gst_util_uint64_scale_round (GST_BUFFER_TIMESTAMP (data),
        base->samplerate, GST_SECOND);

    return src->osc->process (src->osc, off, ct, d);
  }
  return FALSE;
}

//-- interfaces

//-- gobject vmethods

static void
gstbt_wave_replay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtWaveReplay *src = GSTBT_WAVE_REPLAY (object);

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
    case PROP_WAVE:
    case PROP_WAVE_LEVEL:
      g_object_set_property ((GObject *) (src->osc), pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_replay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtWaveReplay *src = GSTBT_WAVE_REPLAY (object);

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
    case PROP_WAVE:
    case PROP_WAVE_LEVEL:
      g_object_get_property ((GObject *) (src->osc), pspec->name, value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_wave_replay_dispose (GObject * object)
{
  GstBtWaveReplay *src = GSTBT_WAVE_REPLAY (object);

  if (src->osc)
    g_object_unref (src->osc);

  G_OBJECT_CLASS (gstbt_wave_replay_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_wave_replay_init (GstBtWaveReplay * src)
{
  /* synth components */
  src->osc = gstbt_osc_wave_new ();
}

static void
gstbt_wave_replay_class_init (GstBtWaveReplayClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;
  GParamSpec *pspec;

  audio_synth_class->process = gstbt_wave_replay_process;
  audio_synth_class->setup = gstbt_wave_replay_setup;

  gobject_class->set_property = gstbt_wave_replay_set_property;
  gobject_class->get_property = gstbt_wave_replay_get_property;
  gobject_class->dispose = gstbt_wave_replay_dispose;

  // describe us
  gst_element_class_set_static_metadata (element_class,
      "Wave Replay",
      "Source/Audio",
      "Wavetable player", "Stefan Sauer <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtWaveReplay.html");

  // register own properties
  g_object_class_install_property (gobject_class, PROP_WAVE_CALLBACKS,
      g_param_spec_pointer ("wave-callbacks", "Wavetable Callbacks",
          "The wave-table access callbacks",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  pspec = g_param_spec_uint ("wave", "Wave", "Wave index", 1, 200, 1,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  g_param_spec_set_qdata (pspec, gstbt_property_meta_quark,
      GUINT_TO_POINTER (1));
  g_param_spec_set_qdata (pspec, gstbt_property_meta_quark_flags,
      GUINT_TO_POINTER (GSTBT_PROPERTY_META_WAVE));
  g_object_class_install_property (gobject_class, PROP_WAVE, pspec);

  g_object_class_install_property (gobject_class, PROP_WAVE_LEVEL,
      g_param_spec_uint ("wave-level", "Wavelevel", "Wave level index",
          0, 100, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}
