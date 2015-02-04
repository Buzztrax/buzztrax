/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * simsyn.h: simple audio synthesizer for gstreamer
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

#ifndef __GSTBT_SIM_SYN_H__
#define __GSTBT_SIM_SYN_H__

#include <gst/gst.h>
#include "gst/audiosynth.h"
#include "gst/envelope-d.h"
#include "gst/filter-svf.h"
#include "gst/osc-synth.h"
#include "gst/toneconversion.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_SIM_SYN            (gstbt_sim_syn_get_type())
#define GSTBT_SIM_SYN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_SIM_SYN,GstBtSimSyn))
#define GSTBT_IS_SIM_SYN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_SIM_SYN))
#define GSTBT_SIM_SYN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_SIM_SYN,GstBtSimSynClass))
#define GSTBT_IS_SIM_SYN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_SIM_SYN))
#define GSTBT_SIM_SYN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_SIM_SYN,GstBtSimSynClass))

typedef struct _GstBtSimSyn GstBtSimSyn;
typedef struct _GstBtSimSynClass GstBtSimSynClass;

/**
 * GstBtSimSyn:
 *
 * Class instance data.
 */
struct _GstBtSimSyn
{
  GstBtAudioSynth parent;

  /* < private > */
  gboolean dispose_has_run;     /* validate if dispose has run */

  /* parameters */
  GstBtNote note;
 
  GstBtToneConversion *n2f;
  GstBtEnvelopeD *volenv;
  GstBtOscSynth *osc;  
  GstBtFilterSVF *filter;
};

struct _GstBtSimSynClass
{
  GstBtAudioSynthClass parent_class;
};

GType gstbt_sim_syn_get_type (void);

G_END_DECLS
#endif /* __GSTBT_SIM_SYN_H__ */
