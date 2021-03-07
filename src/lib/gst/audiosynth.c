/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 *
 * There are a few virtual methods that can subclasses will implement:
 * The negotiate vmethod provides the #GstCaps to negotiate. Once the caps
 * are negotiated, setup is called. There the element can take parameters such
 * as sampling rate or data format from the #GstAudioInfo parameter.
 * The reset method, if implemented, is called on discontinuities - e.g. after
 * seeking. It can be used to e.g. cut off playing notes.
 * Finally the process method is where the audio generation is going to be
 * implemented.
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

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GstBtAudioSynth, gstbt_audio_synth,
    GST_TYPE_BASE_SRC, G_IMPLEMENT_INTERFACE (GST_TYPE_PRESET, NULL));

//--

static guint
gstbt_audio_synth_calculate_buffer_size (GstBtAudioSynth * self)
{
  return self->info.bpf * self->generate_samples_per_buffer;
}

static void
gstbt_audio_synth_calculate_buffer_frames (GstBtAudioSynth * self)
{
  const gdouble ticks_per_minute =
      (gdouble) (self->beats_per_minute * self->ticks_per_beat);
  const gdouble div = 60.0 / self->subticks_per_beat;
  const GstClockTime ticktime =
      (GstClockTime) (0.5 + ((GST_SECOND * 60.0) / ticks_per_minute));

  self->ticktime =
      (GstClockTime) (0.5 + ((GST_SECOND * div) / ticks_per_minute));
  self->samples_per_buffer = ((self->info.rate * div) / ticks_per_minute);
  GST_DEBUG ("samples_per_buffer=%lf", self->samples_per_buffer);
  self->generate_samples_per_buffer = (guint) (0.5 + self->samples_per_buffer);
  gst_base_src_set_blocksize (GST_BASE_SRC (self),
      gstbt_audio_synth_calculate_buffer_size (self));
  // the sequence is quantized to ticks and not subticks
  // we need to compensate for the rounding errors :/
  self->ticktime_err =
      ((gdouble) ticktime -
      (gdouble) (self->subticks_per_beat * self->ticktime)) /
      (gdouble) self->subticks_per_beat;
  GST_DEBUG ("ticktime err=%lf", self->ticktime_err);
}

//-- audiosynth implementation

static void
gstbt_audio_synth_set_context (GstElement * element, GstContext * context)
{
  GstBtAudioSynth *self = GSTBT_AUDIO_SYNTH (element);
  guint bpm, tpb, stpb;

  if (gstbt_audio_tempo_context_get_tempo (context, &bpm, &tpb, &stpb)) {
    if (self->beats_per_minute != bpm ||
        self->ticks_per_beat != tpb || self->subticks_per_beat != stpb) {
      self->beats_per_minute = bpm;
      self->ticks_per_beat = tpb;
      self->subticks_per_beat = stpb;

      GST_INFO_OBJECT (self, "audio tempo context: bmp=%u, tpb=%u, stpb=%u",
          bpm, tpb, stpb);

      gstbt_audio_synth_calculate_buffer_frames (self);
    }
  }
#if GST_CHECK_VERSION (1,8,0)
  GST_ELEMENT_CLASS (gstbt_audio_synth_parent_class)->set_context (element,
      context);
#else
  if (GST_ELEMENT_CLASS (gstbt_audio_synth_parent_class)->set_context) {
    GST_ELEMENT_CLASS (gstbt_audio_synth_parent_class)->set_context (element,
        context);
  }
#endif
}

static GstCaps *
gstbt_audio_synth_fixate (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstBtAudioSynth *self = GSTBT_AUDIO_SYNTH (basesrc);
  GstBtAudioSynthClass *klass = GSTBT_AUDIO_SYNTH_GET_CLASS (self);
  gint i, n = gst_caps_get_size (caps);

  GST_INFO_OBJECT (self, "fixate");

  caps = gst_caps_make_writable (caps);
  for (i = 0; i < n; i++) {
    gst_structure_fixate_field_nearest_int (gst_caps_get_structure (caps, i),
        "rate", self->info.rate);
  }
  GST_INFO_OBJECT (self, "fixated to %" GST_PTR_FORMAT, caps);

  if (klass->negotiate) {
    klass->negotiate (self, caps);
  }
  GST_INFO_OBJECT (self, "fixated to %" GST_PTR_FORMAT, caps);

  return GST_BASE_SRC_CLASS (gstbt_audio_synth_parent_class)->fixate (basesrc,
      caps);
}

static gboolean
gstbt_audio_synth_set_caps (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstBtAudioSynth *self = GSTBT_AUDIO_SYNTH (basesrc);
  GstBtAudioSynthClass *klass = GSTBT_AUDIO_SYNTH_GET_CLASS (self);
  gboolean ret;

  GST_INFO_OBJECT (self, "set_caps");

  if ((ret = gst_audio_info_from_caps (&self->info, caps))) {
    if (klass->setup) {
      klass->setup (self, &self->info);
    }
    gst_base_src_set_blocksize (basesrc,
        gstbt_audio_synth_calculate_buffer_size (self));
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
      if ((res = gst_audio_info_convert (&src->info, src_fmt, src_val, dest_fmt,
                  &dest_val))) {
        gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      }
      break;
    }
    default:
      res =
          GST_BASE_SRC_CLASS (gstbt_audio_synth_parent_class)->query (basesrc,
          query);
      break;
  }

  return res;
}

static gboolean
gstbt_audio_synth_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  GstBtAudioSynth *src = GSTBT_AUDIO_SYNTH (basesrc);
  GstClockTime time = segment->position;

  src->reverse = (segment->rate < 0.0);
  src->running_time = time;
  src->ticktime_err_accum = 0.0;
  /* Assume that seeks in < PAUSED configure the playback segment. Don't
   * generate disconts on them as there is nothing to reset.
   * Doing needless resets breaks comamndline usage, where we'd reset the
   * parameters set on element creation.
   */
  if (GST_STATE (src) >= GST_STATE_PAUSED) {
    src->discont = TRUE;
  }

  /* now move to the time indicated */
  src->n_samples = gst_util_uint64_scale_int (time, src->info.rate, GST_SECOND);

  if (!src->reverse) {
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      segment->time = segment->start;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      src->stop_time = segment->stop;
      src->n_samples_stop = gst_util_uint64_scale_int (src->stop_time,
          src->info.rate, GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
    src->subtick_count = src->subticks_per_beat;
  } else {
    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      segment->time = segment->stop;
    }
    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      src->stop_time = segment->start;
      src->n_samples_stop = gst_util_uint64_scale_int (src->stop_time,
          src->info.rate, GST_SECOND);
      src->check_eos = TRUE;
    } else {
      src->check_eos = FALSE;
    }
    src->subtick_count = 1;
  }
  src->eos_reached = FALSE;

  GST_DEBUG_OBJECT (src,
      "seek from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT " pos %"
      GST_TIME_FORMAT " == pos %" GST_TIME_FORMAT " rate %lf",
      GST_TIME_ARGS (segment->start), GST_TIME_ARGS (segment->stop),
      GST_TIME_ARGS (segment->position), GST_TIME_ARGS (segment->time),
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
  src->discont = FALSE;

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
  GstClockTime next_running_time, ticktime;
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
      (gdouble) src->running_time * (gdouble) src->info.rate /
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

  GST_DEBUG_OBJECT (src,
      "samples_done=%lf, src->n_samples=%" G_GUINT64_FORMAT
      ", src->n_samples_stop=%" G_GUINT64_FORMAT,
      samples_done, src->n_samples, src->n_samples_stop);
  GST_DEBUG_OBJECT (src, "samples-per-buffer=%7u (%8.3lf), length=%u",
      samples_per_buffer, src->samples_per_buffer, length);

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
    if (!src->reverse) {
      src->generate_samples_per_buffer =
          (guint) (src->n_samples_stop - src->n_samples);
      ticktime = src->stop_time - src->running_time;
    } else {
      src->generate_samples_per_buffer =
          (guint) (src->n_samples - src->n_samples_stop);
      ticktime = src->running_time - src->stop_time;
    }
    if (G_UNLIKELY (!src->generate_samples_per_buffer)) {
      GST_WARNING_OBJECT (src, "0 samples left -> EOS reached");
      src->eos_reached = TRUE;
      return GST_FLOW_EOS;
    }
    n_samples = src->n_samples_stop;
    src->eos_reached = TRUE;
    GST_INFO_OBJECT (src, "partial buffer: %u, ticktime: %" GST_TIME_FORMAT,
        src->generate_samples_per_buffer, GST_TIME_ARGS (ticktime));
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = samples_per_buffer;
    n_samples =
        src->n_samples +
        (src->reverse ? (-samples_per_buffer) : samples_per_buffer);
    ticktime = src->ticktime;
  }
  next_running_time =
      src->running_time + (src->reverse ? (-ticktime) : ticktime);
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

  if (src->subtick_count >= src->subticks_per_beat) {
    src->subtick_count = 1;
  } else {
    src->subtick_count++;
  }

  if (src->discont) {
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
    if (klass->reset) {
      klass->reset (src);
    }
    src->discont = FALSE;
  }

  gst_object_sync_values (GST_OBJECT (src), GST_BUFFER_TIMESTAMP (buf));

  GST_DEBUG_OBJECT (src, "generate_samples %6u, offset %12" G_GUINT64_FORMAT
      ", offset_end %12" G_GUINT64_FORMAT " timestamp %" GST_TIME_FORMAT
      ", duration %" GST_TIME_FORMAT, src->generate_samples_per_buffer,
      GST_BUFFER_OFFSET (buf),
      GST_BUFFER_OFFSET_END (buf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)));

  src->running_time = next_running_time;
  src->n_samples = n_samples;

  if (gst_buffer_map (buf, &info, GST_MAP_WRITE)) {
    if (klass->process && !klass->process (src, buf, &info)) {
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

static gboolean
gstbt_audio_synth_stop (GstBaseSrc * basesrc)
{
  GstBtAudioSynth *self = GSTBT_AUDIO_SYNTH (basesrc);

  gst_audio_info_init (&self->info);

  return TRUE;
}

//-- gobject type methods

static void
gstbt_audio_synth_init (GstBtAudioSynth * self)
{
  gst_audio_info_init (&self->info);
  self->info.rate = GST_AUDIO_DEF_RATE;
  self->beats_per_minute = 120;
  self->ticks_per_beat = 4;
  self->subticks_per_beat = 1;
  gstbt_audio_synth_calculate_buffer_frames (self);

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (self), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (self), FALSE);
}

static void
gstbt_audio_synth_class_init (GstBtAudioSynthClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *gstbasesrc_class = (GstBaseSrcClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "audiosynth",
      GST_DEBUG_FG_BLUE | GST_DEBUG_BG_BLACK, "Base Audio synthesizer");

  element_class->set_context =
      GST_DEBUG_FUNCPTR (gstbt_audio_synth_set_context);

  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gstbt_audio_synth_set_caps);
  gstbasesrc_class->fixate = GST_DEBUG_FUNCPTR (gstbt_audio_synth_fixate);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gstbt_audio_synth_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gstbt_audio_synth_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gstbt_audio_synth_query);
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gstbt_audio_synth_start);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gstbt_audio_synth_create);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gstbt_audio_synth_stop);

  /* make process and setup method pure virtual */
  klass->process = NULL;
  klass->setup = NULL;

  /* add the pad */
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gstbt_audio_synth_src_template));
}
