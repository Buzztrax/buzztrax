/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * wavetabsyn.h: wavetable synthesizer
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

#ifndef __GSTBT_WAVE_TAB_SYN_H__
#define __GSTBT_WAVE_TAB_SYN_H__

#include <gst/gst.h>
#include "gst/audiosynth.h"
#include "gst/envelope-adsr.h"
#include "gst/osc-wave.h"
#include "gst/toneconversion.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_WAVE_TAB_SYN            (gstbt_wave_tab_syn_get_type())
#define GSTBT_WAVE_TAB_SYN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_WAVE_TAB_SYN,GstBtWaveTabSyn))
#define GSTBT_IS_WAVE_TAB_SYN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_WAVE_TAB_SYN))
#define GSTBT_WAVE_TAB_SYN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_WAVE_TAB_SYN,GstBtWaveTabSynClass))
#define GSTBT_IS_WAVE_TAB_SYN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_WAVE_TAB_SYN))
#define GSTBT_WAVE_TAB_SYN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_WAVE_TAB_SYN,GstBtWaveTabSynClass))

typedef struct _GstBtWaveTabSyn GstBtWaveTabSyn;
typedef struct _GstBtWaveTabSynClass GstBtWaveTabSynClass;

/**
 * GstBtWaveTabSyn:
 *
 * Class instance data.
 */
struct _GstBtWaveTabSyn
{
  GstBtAudioSynth parent;
  
  /* < private > */
  GstBtNote note;
  guint offset;

  guint cycle_pos, cycle_size;
  guint64 duration;
  GstBtToneConversion *n2f;
  GstBtEnvelopeADSR *volenv;
  GstBtOscWave *osc;
};

struct _GstBtWaveTabSynClass
{
  GstBtAudioSynthClass parent_class;
};

GType gstbt_wave_tab_syn_get_type (void);

G_END_DECLS
#endif /* __GSTBT_WAVE_TAB_SYN_H__ */
