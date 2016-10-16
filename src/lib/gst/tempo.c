/* Buzztrax
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * tempo.c: helper interface for tempo synced gstreamer elements
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
 * SECTION:tempo
 * @title: GstBtTempo
 * @include: gst/tempo.h
 * @short_description: helper for tempo synced gstreamer elements
 *
 * Methods to work with a #GstContext that distributes tempo and timing
 * information.
 *
 * Applications use gstbt_audio_tempo_context_new() to create and initialize the
 * context and gst_element_set_context() to set it on the top-level pipeline.
 * The context is persistent and therefore can be set at any time.
 *
 * #GstElements that implement the #GstElementClass::set_context() vmethod can
 * use gstbt_audio_tempo_context_get_tempo() to retrieve the values. Based on
 * the parameters they can adjust their gst_object_sync_values() cycle.
 *
 * The difference between the tempo context and %GST_TAG_BEATS_PER_MINUTE is
 * that the tag describes the overall tempo, but the context allows to change
 * the tempo during playback and offers more finegrained control.
 */
/* offer upstream?
 * if this is in gstreamer we could enhance the following elements:
 * jacksink: jack_set_timebase_callback()
 * - see jack_mclk_dump,jack_midi_clock
 *
 * lv2, ladspa and all kind of audio-sources: gst_base_src_set_blocksize()
 *
 * alsamidisink: could send "MIDI beat clock" + "MIDI timecode"
 * alsamidisrc: could provide a clock based based on incomming midi
 */

#include "tempo.h"

//-- the context

/**
 * gstbt_audio_tempo_context_new:
 * @bpm: beats per minute
 * @tpb: ticks per beat
 * @stpb: sub-ticks per beat
 *
 * The values of @bpm and @tpb define the meassure and speed of an audio track.
 * Elements (mostly sources) use these value to configure how the quantise
 * controller events and size their buffers. Elements can also use this for
 * tempo synced effects.
 *
 * When using controllers one can also use @stpb to achieve smoother modulation.
 *
 * Returns: a new persistent context that can be send to the pipeline
 */
GstContext *
gstbt_audio_tempo_context_new (guint bpm, guint tpb, guint stpb)
{
  GstContext *ctx = gst_context_new (GSTBT_AUDIO_TEMPO_TYPE, TRUE);
  GstStructure *s = gst_context_writable_structure (ctx);
  gst_structure_set (s,
      "beats-per-minute", G_TYPE_UINT, bpm,
      "ticks-per-beat", G_TYPE_UINT, tpb,
      "subticks-per-beat", G_TYPE_UINT, stpb, NULL);
  return ctx;
}

/**
 * gstbt_audio_tempo_context_get_tempo:
 * @ctx: the context
 * @bpm: beats per minute
 * @tpb: ticks per beat
 * @stpb: sub-ticks per beat
 *
 * Get values from the audio-tempo context. Pointers can be %NULL to skip them.
 *
 * Returns: %FALSE if the context if of the wrong type or any of the values were
 * not stored in the context.
 */
gboolean
gstbt_audio_tempo_context_get_tempo (GstContext * ctx, guint * bpm, guint * tpb,
    guint * stpb)
{
  const GstStructure *s;
  gboolean res = TRUE;

  if (!ctx)
    return FALSE;

  if (!gst_context_has_context_type (ctx, GSTBT_AUDIO_TEMPO_TYPE))
    return FALSE;

  s = gst_context_get_structure (ctx);
  if (bpm)
    res &= gst_structure_get_uint (s, "beats-per-minute", bpm);
  if (tpb)
    res &= gst_structure_get_uint (s, "ticks-per-beat", tpb);
  if (stpb)
    res &= gst_structure_get_uint (s, "subticks-per-beat", stpb);
  return res;
}

#if 0
// extra value calculated in the app from latency in ms
guint stpb = (glong) ((GST_SECOND * 60) / (bpm * tpb * latency * GST_MSECOND));
stpb = MAX (1, stpb);

// extra values calculated in plugins based on tempo values and samplerate
// stored values: subticktime (as ticktime), samples_per_buffer

gdouble tpm = (gdouble) (bpm * tpb);
gdouble div = 60.0 / stpb;
GstClockTime ticktime = (GstClockTime) (0.5 + ((GST_SECOND * 60.0) / tpm));
GstClockTime subticktime = (GstClockTime) (0.5 + ((GST_SECOND * div) / tpm));

gdouble samples_per_buffer = ((samplerate * div) / tpm);
guint generate_samples_per_buffer = (guint) (0.5 + samples_per_buffer);

// music apps quantize trigger events (notes) to ticks and not subticks
// we need to compensate for the rounding errors
// subticks are used to smooth modulation effects and lower live-latency
gdouble ticktime_err =
    ((gdouble) ticktime - (gdouble) (stpb * ticktime)) / (gdouble) stpb;

// the values are use like this in sources:
gst_base_src_set_blocksize (GST_BASE_SRC (self),
    channels * generate_samples_per_buffer * sizeof (gint16));
#endif
