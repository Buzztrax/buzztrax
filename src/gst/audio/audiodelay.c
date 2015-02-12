/*
 * GStreamer             
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
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
 * SECTION:audiodelay
 * @title: GstBtAudioDelay
 * @short_description: audio echo effect
 *
 * <refsect2>
 * Echo effect with controllable effect-ratio, delay-time and feedback.
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch filesrc location="melo1.ogg" ! decodebin ! audioconvert ! audiodelay drywet=50 delaytime=25 feedback=75 ! autoaudiosink
 * gst-launch autoaudiosrc ! audiodelay delaytime=25 feedback=75 ! autoaudiosink
 * </programlisting>
 * In the latter example the echo is applied to the input signal of the
 * soundcard (like a microphone).
 * </para>
 * </refsect2>
 */
/* FIXME: delay-time should be in ticks, thats what the tempo iface is good for
 * here (see self->ticktime), we could add another property called 'delay' that
 * is in ticks and/or remove 'delaytime'.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>

#include "gst/tempo.h"

#include "plugin.h"
#include "audiodelay.h"

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  // static class properties
  // dynamic class properties
  PROP_DRYWET = 1, PROP_FEEDBACK, PROP_DELAYTIME,
  N_PROPERTIES,
  // tempo iface
  PROP_BPM = N_PROPERTIES, PROP_TPB, PROP_STPT,
};
static GParamSpec *properties[N_PROPERTIES] = { NULL, };

#define PROP(name) properties[PROP_##name]

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 1")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 1")
    );

//-- the class

static void gstbt_audio_delay_tempo_interface_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GstBtAudioDelay, gstbt_audio_delay,
    GST_TYPE_BASE_TRANSFORM, G_IMPLEMENT_INTERFACE (GSTBT_TYPE_TEMPO,
        gstbt_audio_delay_tempo_interface_init));

//-- basetransform vmethods

static gboolean
gstbt_audio_delay_set_caps (GstBaseTransform * base, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (base);
  const GstStructure *structure = gst_caps_get_structure (incaps, 0);

  return gst_structure_get_int (structure, "rate", &self->samplerate);
}

static gboolean
gstbt_audio_delay_start (GstBaseTransform * base)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (base);

  gstbt_delay_start (self->delay, self->samplerate);
  return TRUE;
}

static GstFlowReturn
gstbt_audio_delay_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (base);
  GstBtDelay *delay = self->delay;
  GstMapInfo info;
  GstClockTime timestamp;
  gdouble feedback, dry, wet;
  gint16 *data;
  gdouble val_dry, val_fx;
  glong val, sum_fx = 0;
  guint i, num_samples, rb_in, rb_out;

  if (!gst_buffer_map (outbuf, &info, GST_MAP_READ | GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base, "unable to map buffer for read & write");
    return GST_FLOW_ERROR;
  }
  data = (gint16 *) info.data;
  num_samples = info.size / sizeof (gint16);

  /* flush ring_buffer on DISCONT */
  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_DISCONT)) {
    gstbt_delay_flush (delay);
  }

  timestamp = gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (outbuf));
  if (GST_CLOCK_TIME_IS_VALID (timestamp))
    gst_object_sync_values (GST_OBJECT (self), timestamp);

  feedback = (gdouble) self->feedback / 100.0;
  wet = (gdouble) self->drywet / 100.0;
  dry = 1.0 - wet;
  GSTBT_DELAY_BEFORE (delay, rb_in, rb_out);

  if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP) ||
          gst_base_transform_is_passthrough (base))) {
    /* input is silence */
    for (i = 0; i < num_samples; i++) {
      GSTBT_DELAY_READ (delay, rb_out, val_fx);
      val = (glong) (val_fx * feedback);
      GSTBT_DELAY_WRITE (delay, rb_in, CLAMP (val, G_MININT16, G_MAXINT16));
      val = (glong) (wet * val_fx);
      sum_fx += abs (val);
      *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
    }
  } else {
    for (i = 0; i < num_samples; i++) {
      GSTBT_DELAY_READ (delay, rb_out, val_fx);
      val_dry = (gdouble) * data;
      val = (glong) (val_fx * feedback + val_dry);
      GSTBT_DELAY_WRITE (delay, rb_in, CLAMP (val, G_MININT16, G_MAXINT16));
      val = (glong) (wet * val_fx + dry * val_dry);
      sum_fx += abs (val);
      *data++ = (gint16) CLAMP (val, G_MININT16, G_MAXINT16);
    }
  }
  GSTBT_DELAY_AFTER (delay, rb_in, rb_out);

  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP) && sum_fx) {
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_GAP);
  }

  gst_buffer_unmap (outbuf, &info);

  return GST_FLOW_OK;
}

static gboolean
gstbt_audio_delay_stop (GstBaseTransform * base)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (base);

  if (self->delay)
    gstbt_delay_stop (self->delay);

  return TRUE;
}

//-- interfaces

static void
gstbt_audio_delay_calculate_tick_time (GstBtAudioDelay * self)
{
  self->ticktime =
      ((GST_SECOND * 60) / (GstClockTime) (self->beats_per_minute *
          self->ticks_per_beat));
}

static void
gstbt_audio_delay_tempo_change_tempo (GstBtTempo * tempo,
    glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (tempo);
  gboolean changed = FALSE;

  if (beats_per_minute >= 0) {
    if (self->beats_per_minute != beats_per_minute) {
      self->beats_per_minute = (gulong) beats_per_minute;
      g_object_notify (G_OBJECT (self), "beats-per-minute");
      changed = TRUE;
    }
  }
  if (ticks_per_beat >= 0) {
    if (self->ticks_per_beat != ticks_per_beat) {
      self->ticks_per_beat = (gulong) ticks_per_beat;
      g_object_notify (G_OBJECT (self), "ticks-per-beat");
      changed = TRUE;
    }
  }
  if (subticks_per_tick >= 0) {
    if (self->subticks_per_tick != subticks_per_tick) {
      self->subticks_per_tick = (gulong) subticks_per_tick;
      g_object_notify (G_OBJECT (self), "subticks-per-tick");
      changed = TRUE;
    }
  }
  if (changed) {
    GST_DEBUG ("changing tempo to %ld BPM  %ld TPB  %ld STPT",
        self->beats_per_minute, self->ticks_per_beat, self->subticks_per_tick);
    gstbt_audio_delay_calculate_tick_time (self);
  }
}

static void
gstbt_audio_delay_tempo_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstBtTempoInterface *iface = g_iface;

  GST_INFO ("initializing iface");
  iface->change_tempo = gstbt_audio_delay_tempo_change_tempo;
}

//-- gobject vmethods

static void
gstbt_audio_delay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (object);

  switch (prop_id) {
    case PROP_DRYWET:
      self->drywet = g_value_get_uint (value);
      break;
    case PROP_FEEDBACK:
      self->feedback = g_value_get_uint (value);
      break;
    case PROP_DELAYTIME:
      g_object_set_property ((GObject *) (self->delay), pspec->name, value);
      break;
      // tempo iface
    case PROP_BPM:
    case PROP_TPB:
    case PROP_STPT:
      GST_WARNING ("use gstbt_tempo_change_tempo()");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_audio_delay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (object);

  switch (prop_id) {
    case PROP_DRYWET:
      g_value_set_uint (value, self->drywet);
      break;
    case PROP_FEEDBACK:
      g_value_set_uint (value, self->feedback);
      break;
    case PROP_DELAYTIME:
      g_object_get_property ((GObject *) (self->delay), pspec->name, value);
      break;
      // tempo iface
    case PROP_BPM:
      g_value_set_ulong (value, self->beats_per_minute);
      break;
    case PROP_TPB:
      g_value_set_ulong (value, self->ticks_per_beat);
      break;
    case PROP_STPT:
      g_value_set_ulong (value, self->subticks_per_tick);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_audio_delay_dispose (GObject * object)
{
  GstBtAudioDelay *self = GSTBT_AUDIO_DELAY (object);

  g_clear_object (&self->delay);

  G_OBJECT_CLASS (gstbt_audio_delay_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_audio_delay_init (GstBtAudioDelay * self)
{
  self->drywet = 50;
  self->feedback = 50;

  self->samplerate = GST_AUDIO_DEF_RATE;
  self->beats_per_minute = 120;
  self->ticks_per_beat = 4;
  self->subticks_per_tick = 1;
  gstbt_audio_delay_calculate_tick_time (self);

  /* effect components */
  self->delay = gstbt_delay_new ();

  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (self), TRUE);
}

static void
gstbt_audio_delay_class_init (GstBtAudioDelayClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseTransformClass *gstbasetransform_class =
      (GstBaseTransformClass *) klass;
  GObjectClass *component;

  gobject_class->set_property = gstbt_audio_delay_set_property;
  gobject_class->get_property = gstbt_audio_delay_get_property;
  gobject_class->dispose = gstbt_audio_delay_dispose;

  gstbasetransform_class->set_caps =
      GST_DEBUG_FUNCPTR (gstbt_audio_delay_set_caps);
  gstbasetransform_class->start = GST_DEBUG_FUNCPTR (gstbt_audio_delay_start);
  gstbasetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gstbt_audio_delay_transform_ip);
  gstbasetransform_class->stop = GST_DEBUG_FUNCPTR (gstbt_audio_delay_stop);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_set_static_metadata (element_class,
      "AudioDelay", "Filter/Effect/Audio",
      "Add echos to audio streams", "Stefan Kost <ensonic@users.sf.net>");
  gst_element_class_add_metadata (element_class, GST_ELEMENT_METADATA_DOC_URI,
      "file://" DATADIR "" G_DIR_SEPARATOR_S "gtk-doc" G_DIR_SEPARATOR_S "html"
      G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "GstBtAudioDelay.html");

  // override interface properties
  g_object_class_override_property (gobject_class, PROP_BPM,
      "beats-per-minute");
  g_object_class_override_property (gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property (gobject_class, PROP_STPT,
      "subticks-per-tick");

  // register own properties
  PROP (DRYWET) = g_param_spec_uint ("drywet", "Dry-Wet",
      "Intensity of effect (0 none -> 100 full)", 0, 100, 50,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  PROP (FEEDBACK) = g_param_spec_uint ("feedback", "Fedback",
      "Echo feedback in percent", 0, 99, 50,
      G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  component = g_type_class_ref (GSTBT_TYPE_DELAY);
  PROP (DELAYTIME) = bt_g_param_spec_clone (component, "delaytime");
  g_type_class_unref (component);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, properties);
}
