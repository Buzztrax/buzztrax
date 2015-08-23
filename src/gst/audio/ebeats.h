/* GStreamer
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
 *
 * ebeats.h: electric drum synthesizer for gstreamer
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

#ifndef __GSTBT_E_BEATS_H__
#define __GSTBT_E_BEATS_H__

#include <gst/gst.h>
#include "gst/audiosynth.h"
#include "gst/combine.h"
#include "gst/envelope-d.h"
#include "gst/filter-svf.h"
#include "gst/osc-synth.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_E_BEATS_FILTER_ROUTING_TYPE (gstbt_e_beats_filter_routing_get_type())

/**
 * GstBtEBeatsFilterRouting:
 * @GSTBT_E_BEATS_FILTER_ROUTING_T_N: apply filter at the end to both tonal and
 *   noise parts
 * @GSTBT_E_BEATS_FILTER_ROUTING_T: apply filter only to tonal parts
 * @GSTBT_E_BEATS_FILTER_ROUTING_N: apply filter only to noise parts
 *
 * The filter routing modes configure to which parts of the signal to apply the
 * filter.
 */
typedef enum
{
  GSTBT_E_BEATS_FILTER_ROUTING_T_N,
  GSTBT_E_BEATS_FILTER_ROUTING_T,
  GSTBT_E_BEATS_FILTER_ROUTING_N,
} GstBtEBeatsFilterRouting;

#define GSTBT_TYPE_E_BEATS            (gstbt_e_beats_get_type())
#define GSTBT_E_BEATS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_E_BEATS,GstBtEBeats))
#define GSTBT_IS_E_BEATS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_E_BEATS))
#define GSTBT_E_BEATS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_E_BEATS,GstBtEBeatsClass))
#define GSTBT_IS_E_BEATS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_E_BEATS))
#define GSTBT_E_BEATS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_E_BEATS,GstBtEBeatsClass))

typedef struct _GstBtEBeats GstBtEBeats;
typedef struct _GstBtEBeatsClass GstBtEBeatsClass;

/**
 * GstBtEBeats:
 *
 * Class instance data.
 */
struct _GstBtEBeats
{
  GstBtAudioSynth parent;

  /* < private > */
  gdouble volume;
  GstBtEnvelopeD *volenv_t, *volenv_n, *freqenv_t1, *freqenv_t2, *fltenv;
  GstBtOscSynth *osc_t1, *osc_t2, *osc_n;  
  GstBtFilterSVF *filter;
  GstBtCombine *mix;
  GstBtEBeatsFilterRouting flt_routing;
};

struct _GstBtEBeatsClass
{
  GstBtAudioSynthClass parent_class;
};

GType gstbt_e_beats_get_type (void);

G_END_DECLS
#endif /* __GSTBT_E_BEATS_H__ */
