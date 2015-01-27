/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * osc-wave.c: wavetable oscillator
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
 * SECTION:osc-wave
 * @title: GstBtOscWave
 * @include: libgstbuzztrax/osc-wave.h
 * @short_description: wavetable oscillator
 *
 * An audio waveform generator that read from the applications wave-table.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "osc-wave.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // static class properties
  PROP_WAVE_CALLBACKS = 1,
  // dynamic class properties
  PROP_WAVE,
  PROP_WAVE_LEVEL,
  PROP_FREQUENCY,
  // readable class properties
  PROP_DURATION
};

//-- the class

G_DEFINE_TYPE (GstBtOscWave, gstbt_osc_wave, G_TYPE_OBJECT);

//-- constructor methods

/**
 * gstbt_osc_wave_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
GstBtOscWave *
gstbt_osc_wave_new (void)
{
  return GSTBT_OSC_WAVE (g_object_new (GSTBT_TYPE_OSC_WAVE, NULL));
}

//-- private methods

static gboolean
gstbt_osc_wave_create_mono (GstBtOscWave * self, guint64 off, guint ct,
    gint16 * dst)
{
  if (!self->data) {
    GST_DEBUG ("no wave buffer");
    return FALSE;
  }
  const guint ss = sizeof (gint16);
  guint size = self->map_info.size;
  if (off * ss >= size) {
    GST_DEBUG ("beyond size");
    return FALSE;
  }

  gint16 *src = (gint16 *) self->map_info.data;

  if ((off + ct) * ss >= size) {
    guint ct2 = (size / ss) - off;
    // clear end of buffer
    memset (&dst[ct2], 0, (ct - ct2) * ss);
    ct = ct2;
  }
  // copy from data[off] ... data[off+ct]
  memcpy (dst, &src[off], ct * ss);

  return TRUE;
}

static gboolean
gstbt_osc_wave_create_stereo (GstBtOscWave * self, guint64 off, guint ct,
    gint16 * dst)
{
  if (!self->data) {
    GST_DEBUG ("no wave buffer");
    return FALSE;
  }
  const guint ss = 2 * sizeof (gint16);
  guint size = self->map_info.size;
  if (off * ss >= size) {
    GST_DEBUG ("beyond size");
    return FALSE;
  }

  gint16 *src = (gint16 *) self->map_info.data;

  if ((off + ct) * ss >= size) {
    guint ct2 = (size / ss) - off;
    // clear end of buffer
    memset (&dst[ct2 * 2], 0, (ct - ct2) * ss);
    ct = ct2;
  }
  // copy from data[off] ... data[off+ct]
  memcpy (dst, &src[off * 2], ct * ss);

  return TRUE;
}

static gboolean
gstbt_osc_wave_create_mono_resampled (GstBtOscWave * self, guint64 off,
    guint ct, gint16 * dst)
{
  if (!self->data) {
    GST_DEBUG ("no wave buffer");
    return FALSE;
  }
  const guint ss = sizeof (gint16);
  guint size = self->map_info.size;
  if (off * ss >= size) {
    GST_DEBUG ("beyond size");
    return FALSE;
  }

  gint16 *src = (gint16 *) self->map_info.data;
  gdouble rate = self->rate;
  guint64 s, d;

  for (d = 0; d < ct; d++) {
    s = (off + d) * rate;
    dst[d] = (s * ss < size) ? src[s] : 0;
  }

  return TRUE;
}

static gboolean
gstbt_osc_wave_create_stereo_resampled (GstBtOscWave * self, guint64 off,
    guint ct, gint16 * dst)
{
  if (!self->data) {
    GST_DEBUG ("no wave buffer");
    return FALSE;
  }
  const guint ss = 2 * sizeof (gint16);
  guint size = self->map_info.size;
  if (off * ss >= size) {
    GST_DEBUG ("beyond size");
    return FALSE;
  }

  gint16 *src = (gint16 *) self->map_info.data;
  gdouble rate = self->rate;
  guint64 s, d;

  for (d = 0; d < ct; d++) {
    s = (off + d) * rate;
    if (s * ss < size) {
      dst[d << 1] = src[(s << 1)];
      dst[(d << 1) + 1] = src[(s << 1) + 1];
    } else {
      dst[d << 1] = 0;
      dst[(d << 1) + 1] = 0;
    }
  }

  return TRUE;
}

/**
 * gstbt_osc_wave_setup:
 * @self: the oscillator
 *
 * Prepare the oscillator. Should be called before first use to ensure it is
 * configured for the default parameters.
 */
void
gstbt_osc_wave_setup (GstBtOscWave * self)
{
  gpointer *cb = self->wave_callbacks;
  GstStructure *(*get_wave_buffer) (gpointer, guint, guint);
  GstStructure *s;
  GstBtNote root_note;

  if (!cb) {
    return;
  }
  if (self->data) {
    gst_buffer_unmap (self->data, &self->map_info);
    gst_buffer_unref (self->data);
    self->data = NULL;
  }
  get_wave_buffer = cb[1];
  if (!(s = get_wave_buffer (cb[0], self->wave, self->wave_level)))
    return;

  gst_structure_get (s,
      "channels", G_TYPE_INT, &self->channels,
      "root-note", GSTBT_TYPE_NOTE, &root_note,
      "buffer", GST_TYPE_BUFFER, &self->data, NULL);

  if (!self->data) {
    return;
  }

  if (!gst_buffer_map (self->data, &self->map_info, GST_MAP_READ)) {
    GST_WARNING_OBJECT (self, "unable to map buffer for read");
    return;
  }

  if (self->freq > 0.0) {
    gdouble freq =
        gstbt_tone_conversion_translate_from_number (self->n2f, root_note);
    self->rate = freq / self->freq;
  } else {
    self->rate = 1.0;
  }

  GST_WARNING ("got wave with %d channels", self->channels);

  self->duration = self->map_info.size / (self->rate * sizeof (gint16));

  switch (self->channels) {
    case 1:
      if (self->rate == 1.0) {
        self->process = gstbt_osc_wave_create_mono;
      } else {
        self->process = gstbt_osc_wave_create_mono_resampled;
      }
      break;
    case 2:
      self->duration >>= 1;
      if (self->rate == 1.0) {
        self->process = gstbt_osc_wave_create_stereo;
      } else {
        self->process = gstbt_osc_wave_create_stereo_resampled;
      }
      break;
    default:
      GST_ERROR ("unsupported number of channels: %d", self->channels);
      break;
  }

  GST_WARNING ("duration at rate %lf is %" G_GUINT64_FORMAT, self->rate,
      self->duration);
}

//-- public methods

//-- virtual methods

static void
gstbt_osc_wave_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtOscWave *self = GSTBT_OSC_WAVE (object);

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
      self->wave_callbacks = g_value_get_pointer (value);
      gstbt_osc_wave_setup (self);
      break;
    case PROP_WAVE:
      //GST_INFO("change wave %u -> %u",g_value_get_uint (value),self->wave);
      self->wave = g_value_get_uint (value);
      gstbt_osc_wave_setup (self);
      break;
    case PROP_WAVE_LEVEL:
      //GST_INFO("change wave-level %u -> %u",g_value_get_uint (value),self->wave_level);
      self->wave_level = g_value_get_uint (value);
      gstbt_osc_wave_setup (self);
      break;
    case PROP_FREQUENCY:
      //GST_INFO("change frequency %lf -> %lf",g_value_get_double (value),self->freq);
      self->freq = g_value_get_double (value);
      gstbt_osc_wave_setup (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_osc_wave_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtOscWave *self = GSTBT_OSC_WAVE (object);

  switch (prop_id) {
    case PROP_WAVE_CALLBACKS:
      g_value_set_pointer (value, self->wave_callbacks);
      break;
    case PROP_WAVE:
      g_value_set_uint (value, self->wave);
      break;
    case PROP_WAVE_LEVEL:
      g_value_set_uint (value, self->wave_level);
      break;
    case PROP_FREQUENCY:
      g_value_set_double (value, self->freq);
      break;
    case PROP_DURATION:
      g_value_set_uint64 (value, self->duration);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_osc_wave_dispose (GObject * object)
{
  GstBtOscWave *self = GSTBT_OSC_WAVE (object);

  if (self->dispose_has_run)
    return;
  self->dispose_has_run = TRUE;

  if (self->n2f)
    g_object_unref (self->n2f);
  if (self->data) {
    gst_buffer_unmap (self->data, &self->map_info);
    gst_buffer_unref (self->data);
  }

  G_OBJECT_CLASS (gstbt_osc_wave_parent_class)->dispose (object);
}

static void
gstbt_osc_wave_init (GstBtOscWave * self)
{
  self->wave = 1;
  self->freq = 0.0;
  self->rate = 1.0;
  self->n2f =
      gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
}

static void
gstbt_osc_wave_class_init (GstBtOscWaveClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "osc-wave",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "wavetable oscillator");

  gobject_class->set_property = gstbt_osc_wave_set_property;
  gobject_class->get_property = gstbt_osc_wave_get_property;
  gobject_class->dispose = gstbt_osc_wave_dispose;

  // register own properties
  g_object_class_install_property (gobject_class, PROP_WAVE_CALLBACKS,
      g_param_spec_pointer ("wave-callbacks", "Wavetable Callbacks",
          "The wave-table access callbacks",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_uint ("wave", "Wave", "Wave index", 1, 200, 1,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE_LEVEL,
      g_param_spec_uint ("wave-level", "Wavelevel", "Wave level index",
          0, 100, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FREQUENCY,
      g_param_spec_double ("frequency", "Frequency",
          "Frequency of tone (0.0 for original)", 0.0, G_MAXDOUBLE, 0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DURATION,
      g_param_spec_uint64 ("duration", "Duration",
          "Duration in samples at the given rate", 0, G_MAXUINT64, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
