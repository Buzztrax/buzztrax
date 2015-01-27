/* GStreamer
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

#ifndef __GSTBT_AUDIO_SYNTH_H__
#define __GSTBT_AUDIO_SYNTH_H__

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS
#define GSTBT_TYPE_AUDIO_SYNTH			        (gstbt_audio_synth_get_type())
#define GSTBT_AUDIO_SYNTH(obj)			        (G_TYPE_CHECK_INSTANCE_CAST((obj) ,GSTBT_TYPE_AUDIO_SYNTH,GstBtAudioSynth))
#define GSTBT_IS_AUDIO_SYNTH(obj)		        (G_TYPE_CHECK_INSTANCE_TYPE((obj) ,GSTBT_TYPE_AUDIO_SYNTH))
#define GSTBT_AUDIO_SYNTH_CLASS(klass)	    (G_TYPE_CHECK_CLASS_CAST((klass)  ,GSTBT_TYPE_AUDIO_SYNTH,GstBtAudioSynthClass))
#define GSTBT_IS_AUDIO_SYNTH_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass)  ,GSTBT_TYPE_AUDIO_SYNTH))
#define GSTBT_AUDIO_SYNTH_GET_CLASS(obj)	  (G_TYPE_INSTANCE_GET_CLASS((obj)  ,GSTBT_TYPE_AUDIO_SYNTH,GstBtAudioSynthClass))
typedef struct _GstBtAudioSynth GstBtAudioSynth;
typedef struct _GstBtAudioSynthClass GstBtAudioSynthClass;


/**
 * GstBtAudioSynth:
 *
 * Instance data.
 */
struct _GstBtAudioSynth
{
  GstBaseSrc parent;

  /* < private > */
  /* parameters */
  gdouble samples_per_buffer;

  gboolean dispose_has_run;     /* validate if dispose has run */

  gint samplerate;
  gint channels;
  GstClockTime running_time;    /* total running time */
  gint64 n_samples;             /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_eos;
  gboolean eos_reached;
  guint generate_samples_per_buffer;    /* generate a partial buffer */
  gboolean reverse;             /* play backwards */

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  gulong subtick_count;
  GstClockTime ticktime;
  gdouble ticktime_err, ticktime_err_accum;
};

/**
 * GstBtAudioSynthClass:
 * @parent_class: parent type
 * @process: vmethod for generating a block of audio, return false to indicate
 * that a GAP buffer should be sent
 * @setup: vmethod for initial processign setup
 *
 * Class structure.
 */
struct _GstBtAudioSynthClass
{
  GstBaseSrcClass parent_class;

  /* virtual functions */
  gboolean (*process) (GstBtAudioSynth * src, GstBuffer * data, GstMapInfo *info);
  gboolean (*setup) (GstBtAudioSynth * src,GstPad * pad, GstCaps * caps);
};

GType gstbt_audio_synth_get_type (void);

G_END_DECLS
#endif /* __GSTBT_AUDIO_SYNTH_H__ */
