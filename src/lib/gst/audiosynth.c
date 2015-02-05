/* GStreamer
 *
 * audiosynth.c: base audio synthesizer
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
 * SECTION:audiosynth
 * @title: GstBtAudioSynth
 * @include: libgstbuzztrax/audiosynth.h
 * @short_description: base audio synthesizer
 *
 * Base audio synthesizer to use as a foundation for new synthesizers. Handles
 * tempo, seeking, trick mode playback and format negotiation.
 * The pure virtual process and setup methods must be implemented by the child
 * class. The setup vmethod provides the caps to negotiate. Form them the elemnt
 * can take parameters such as sampling rate or data format.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <gst/audio/audio.h>

#include "gst/tempo.h"

#include "audiosynth.h"

#define GST_CAT_DEFAULT audiosynth_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // tempo interface
  PROP_BPM = 1,
  PROP_TPB,
  PROP_STPT,
};

static GstStaticPadTemplate gstbt_audio_synth_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [1, 2]")
    );

//-- the class

static void gstbt_audio_synth_tempo_interface_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GstBtAudioSynth, gstbt_audio_synth,
    GST_TYPE_BASE_SRC, G_IMPLEMENT_INTERFACE (GST_TYPE_PRESET, NULL)
    G_IMPLEMENT_INTERFACE (GSTBT_TYPE_TEMPO,
        gstbt_audio_synth_tempo_interface_init));

//--

static guint
gstbt_audio_synth_calculate_buffer_size (GstBtAudioSynth * self)
{
  return self->channels * self->generate_samples_per_buffer * sizeof (gint16);
}

static void
gstbt_audio_synth_calculate_buffer_frames (GstBtAudioSynth * self)
{
  const gdouble ticks_per_minute =
      (gdouble) (self->beats_per_minute * self->ticks_per_beat);
  const gdouble div = 60.0 / self->subticks_per_tick;
  const gdouble subticktime = ((GST_SECOND * div) / ticks_per_minute);
  GstClockTime ticktime =
      (GstClockTime) (0.5 + ((GST_SECOND * 60.0) / ticks_per_minute));

  self->samples_per_buffer = ((self->samplerate * div) / ticks_per_minute);
  self->ticktime = (GstClockTime) (0.5 + subticktime);
  GST_DEBUG ("samples_per_buffer=%lf", self->samples_per_buffer);
  gst_base_src_set_blocksize (GST_BASE_SRC (self),
      gstbt_audio_synth_calculate_buffer_size (self));
  // the sequence is quantized to ticks and not subticks
  // we need to compensate for the rounding errors :/
  self->ticktime_err =
      ((gdouble) ticktime -
      (gdouble) (self->subticks_per_tick * self->ticktime)) /
      (gdouble) self->subticks_per_tick;
  GST_DEBUG ("ticktime err=%lf", self->ticktime_err);
}

//-- audiosynth implementation

static GstCaps *
gstbt_audio_synth_fixate (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  GstBtAudioSynthClass *klass = GSTBT_AUDIO_SYNTH_GET_CLASS (src);
  GstCaps *res = gst_caps_copy (caps);
  gint i, n = gst_caps_get_size (res);

  GST_INFO_OBJECT (src, "fixate");

  for (i = 0; i < n; i++) {
    gst_structure_fixate_field_nearest_int (gst_caps_get_structure (res, i),
        "rate", src->samplerate);
  }
  GST_INFO_OBJECT (src, "fixated to %" GST_PTR_FORMAT, res);

  if (klass->setup) {
    klass->setup (src, GST_BASE_SRC_PAD (basesrc), res);
  }
  GST_INFO_OBJECT (src, "fixated to %" GST_PTR_FORMAT, res);

  return GST_BASE_SRC_CLASS (gstbt_audio_synth_parent_class)->fixate (basesrc,
      res);
}

static gboolean
gstbt_audio_synth_set_caps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  const GstStructure *structure = gst_caps_get_structure (caps, 0);
  gboolean ret;

  ret = gst_structure_get_int (structure, "rate", &src->samplerate);
  ret &= gst_structure_get_int (structure, "channels", &src->channels);
  if (ret) {
    gst_base_src_set_blocksize (basesrc,
        gstbt_audio_synth_calculate_buffer_size (src));
  }
  return ret;
}

static gboolean
gstbt_audio_synth_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (src_fmt == dest_fmt) {
        dest_val = src_val;
        goto done;
      }

      switch (src_fmt) {
        case GST_FORMAT_DEFAULT:
          switch (dest_fmt) {
            case GST_FORMAT_TIME:
              /* samples to time */
              dest_val = gst_util_uint64_scale_int (src_val, GST_SECOND,
                  src->samplerate);
              break;
            default:
              goto error;
          }
          break;
        case GST_FORMAT_TIME:
          switch (dest_fmt) {
            case GST_FORMAT_DEFAULT:
              /* time to samples */
              dest_val = gst_util_uint64_scale_int (src_val, src->samplerate,
                  GST_SECOND);
              break;
            default:
              goto error;
          }
          break;
        default:
          goto error;
      }
    done:
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      res = TRUE;
      break;
    }
    default:
      res =
          GST_BASE_SRC_CLASS (gstbt_audio_synth_parent_class)->query (basesrc,
          query);
      break;
  }

  return res;
  /* ERROR */
error:
  {
    GST_DEBUG_OBJECT (src, "query failed");
    return FALSE;
  }
}

static gboolean
gstbt_audio_synth_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  GstClockTime time;

  time = segment->position;
  src->reverse = (segment->rate < 0.0);
  src->running_time = time;
  src->ticktime_err_accum = 0.0;

  /* now move to the time indicated */
  src->n_samples =
      gst_util_uint64_scale_int (time, src->samplerate, GST_SECOND);

  if (!src->reverse) {
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      segment->time = segment->start;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      time = segment->stop;
      src->n_samples_stop = gst_util_uint64_scale_int (time, src->samplerate,
          GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
    src->subtick_count = src->subticks_per_tick;
  } else {
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      segment->time = segment->stop;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      time = segment->start;
      src->n_samples_stop = gst_util_uint64_scale_int (time, src->samplerate,
          GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
    src->subtick_count = 1;
  }
  src->eos_reached = FALSE;

  GST_DEBUG_OBJECT (src,
      "seek from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT " cur %"
      GST_TIME_FORMAT " rate %lf", GST_TIME_ARGS (segment->start),
      GST_TIME_ARGS (segment->stop), GST_TIME_ARGS (segment->position),
      segment->rate);

  return TRUE;
}

static gboolean
gstbt_audio_synth_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static gboolean
gstbt_audio_synth_start (GstBaseSrc * basesrc)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);

  src->n_samples = G_GINT64_CONSTANT (0);
  src->running_time = G_GUINT64_CONSTANT (0);
  src->ticktime_err_accum = 0.0;

  return TRUE;
}

static GstFlowReturn
gstbt_audio_synth_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  GstBtAudioSynthClass *klass = GSTBT_AUDIO_SYNTH_GET_CLASS (src);
  GstFlowReturn res;
  GstBuffer *buf;
  GstMapInfo info;
  GstClockTime next_running_time;
  gint64 n_samples;
  gdouble samples_done;
  guint samples_per_buffer;
  gboolean partial_buffer = FALSE;

  if (G_UNLIKELY (src->eos_reached)) {
    GST_WARNING_OBJECT (src, "EOS reached");
    return GST_FLOW_EOS;
  }
  // the amount of samples to produce (handle rounding errors by collecting left over fractions)
  samples_done =
      (gdouble) src->running_time * (gdouble) src->samplerate /
      (gdouble) GST_SECOND;
  if (!src->reverse) {
    samples_per_buffer =
        (guint) (src->samples_per_buffer + (samples_done -
            (gdouble) src->n_samples));
  } else {
    samples_per_buffer =
        (guint) (src->samples_per_buffer + ((gdouble) src->n_samples -
            samples_done));
  }

  /*
     GST_DEBUG_OBJECT(src,"samples_done=%lf, src->n_samples=%lf, samples_per_buffer=%u",
     samples_done,(gdouble)src->n_samples,samples_per_buffer);
     GST_DEBUG("  samplers-per-buffer = %7d (%8.3lf), length = %u",samples_per_buffer,src->samples_per_buffer,length);
   */

  /* check for eos */
  if (src->check_eos) {
    if (!src->reverse) {
      partial_buffer = ((src->n_samples_stop >= src->n_samples) &&
          (src->n_samples_stop < src->n_samples + samples_per_buffer));
    } else {
      partial_buffer = ((src->n_samples_stop < src->n_samples) &&
          (src->n_samples_stop >= src->n_samples - samples_per_buffer));
    }
  }

  if (G_UNLIKELY (partial_buffer)) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer =
        (guint) (src->n_samples_stop - src->n_samples);
    GST_INFO_OBJECT (src, "partial buffer: %u",
        src->generate_samples_per_buffer);
    if (G_UNLIKELY (!src->generate_samples_per_buffer)) {
      GST_WARNING_OBJECT (src, "0 samples left -> EOS reached");
      src->eos_reached = TRUE;
      return GST_FLOW_EOS;
    }
    n_samples = src->n_samples_stop;
    src->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples_per_buffer;
    n_samples =
        src->n_samples +
        (src->reverse ? (-samples_per_buffer) : samples_per_buffer);
  }
  next_running_time =
      src->running_time + (src->reverse ? (-src->ticktime) : src->ticktime);
  src->ticktime_err_accum =
      src->ticktime_err_accum +
      (src->reverse ? (-src->ticktime_err) : src->ticktime_err);

  res = GST_BASE_SRC_GET_CLASS (basesrc)->alloc (basesrc, src->n_samples,
      gstbt_audio_synth_calculate_buffer_size (src), &buf);
  if (G_UNLIKELY (res != GST_FLOW_OK)) {
    return res;
  }

  if (!src->reverse) {
    GST_BUFFER_TIMESTAMP (buf) =
        src->running_time + (GstClockTime) src->ticktime_err_accum;
    GST_BUFFER_DURATION (buf) = next_running_time - src->running_time;
    GST_BUFFER_OFFSET (buf) = src->n_samples;
    GST_BUFFER_OFFSET_END (buf) = n_samples;
  } else {
    GST_BUFFER_TIMESTAMP (buf) =
        next_running_time + (GstClockTime) src->ticktime_err_accum;
    GST_BUFFER_DURATION (buf) = src->running_time - next_running_time;
    GST_BUFFER_OFFSET (buf) = n_samples;
    GST_BUFFER_OFFSET_END (buf) = src->n_samples;
  }

  if (src->subtick_count >= src->subticks_per_tick) {
    src->subtick_count = 1;
  } else {
    src->subtick_count++;
  }

  gst_object_sync_values (GST_OBJECT (src), GST_BUFFER_TIMESTAMP (buf));

  GST_DEBUG ("n_samples %12" G_GUINT64_FORMAT ", d_samples %6u running_time %"
      GST_TIME_FORMAT ", next_time %" GST_TIME_FORMAT ", duration %"
      GST_TIME_FORMAT, src->n_samples, src->generate_samples_per_buffer,
      GST_TIME_ARGS (src->running_time), GST_TIME_ARGS (next_running_time),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)));

  src->running_time = next_running_time;
  src->n_samples = n_samples;

  if (gst_buffer_map (buf, &info, GST_MAP_WRITE)) {
    if (!klass->process (src, buf, &info)) {
      memset (info.data, 0, info.size);
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_GAP);
    }
    gst_buffer_unmap (buf, &info);
  } else {
    GST_WARNING_OBJECT (src, "unable to map buffer for write");
  }
  *buffer = buf;

  return GST_FLOW_OK;
}

//-- interfaces

static void
gstbt_audio_synth_tempo_change_tempo (GstBtTempo * tempo,
    glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick)
{
  GstBtAudioSynth *self = GSTBT_AUDIO_SYNTH (tempo);
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
    GST_DEBUG ("changing tempo to %lu BPM  %lu TPB  %lu STPT",
        self->beats_per_minute, self->ticks_per_beat, self->subticks_per_tick);
    gstbt_audio_synth_calculate_buffer_frames (self);
  }
}

static void
gstbt_audio_synth_tempo_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstBtTempoInterface *iface = g_iface;
  GST_INFO ("initializing interface");

  iface->change_tempo = gstbt_audio_synth_tempo_change_tempo;
}

//-- gobject vmethods

static void
gstbt_audio_synth_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
      /* tempo interface */
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
gstbt_audio_synth_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (object);

  if (src->dispose_has_run)
    return;

  switch (prop_id) {
      /* tempo interface */
    case PROP_BPM:
      g_value_set_ulong (value, src->beats_per_minute);
      break;
    case PROP_TPB:
      g_value_set_ulong (value, src->ticks_per_beat);
      break;
    case PROP_STPT:
      g_value_set_ulong (value, src->subticks_per_tick);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_audio_synth_dispose (GObject * object)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (object);

  if (src->dispose_has_run)
    return;
  src->dispose_has_run = TRUE;

  G_OBJECT_CLASS (gstbt_audio_synth_parent_class)->dispose (object);
}

//-- gobject type methods

static void
gstbt_audio_synth_init (GstBtAudioSynth * src)
{
  src->samplerate = GST_AUDIO_DEF_RATE;
  src->beats_per_minute = 120;
  src->ticks_per_beat = 4;
  src->subticks_per_tick = 1;
  gstbt_audio_synth_calculate_buffer_frames (src);
  src->generate_samples_per_buffer = (guint) (0.5 + src->samples_per_buffer);

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);
}

static void
gstbt_audio_synth_class_init (GstBtAudioSynthClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *gstbasesrc_class = (GstBaseSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "audiosynth",
      GST_DEBUG_FG_BLUE | GST_DEBUG_BG_BLACK, "Base Audio synthesizer");

  gobject_class->set_property = gstbt_audio_synth_set_property;
  gobject_class->get_property = gstbt_audio_synth_get_property;
  gobject_class->dispose = gstbt_audio_synth_dispose;

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gstbt_audio_synth_set_caps);
  gstbasesrc_class->fixate = GST_DEBUG_FUNCPTR (gstbt_audio_synth_fixate);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gstbt_audio_synth_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gstbt_audio_synth_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gstbt_audio_synth_query);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gstbt_audio_synth_start);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gstbt_audio_synth_create);

  /* make process and setup method pure virtual */
  klass->process = NULL;
  klass->setup = NULL;

  /* override interface properties */
  g_object_class_override_property (gobject_class, PROP_BPM,
      "beats-per-minute");
  g_object_class_override_property (gobject_class, PROP_TPB, "ticks-per-beat");
  g_object_class_override_property (gobject_class, PROP_STPT,
      "subticks-per-tick");

  /* add the pad */
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gstbt_audio_synth_src_template));
}
