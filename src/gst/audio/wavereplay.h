/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * wavereplay.h: wavetable player
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

#ifndef __GSTBT_WAVE_REPLAY_H__
#define __GSTBT_WAVE_REPLAY_H__

#include <gst/gst.h>
#include "gst/audiosynth.h"
#include "gst/osc-wave.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_WAVE_REPLAY            (gstbt_wave_replay_get_type())
#define GSTBT_WAVE_REPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_WAVE_REPLAY,GstBtWaveReplay))
#define GSTBT_IS_WAVE_REPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_WAVE_REPLAY))
#define GSTBT_WAVE_REPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_WAVE_REPLAY,GstBtWaveReplayClass))
#define GSTBT_IS_WAVE_REPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_WAVE_REPLAY))
#define GSTBT_WAVE_REPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_WAVE_REPLAY,GstBtWaveReplayClass))

typedef struct _GstBtWaveReplay GstBtWaveReplay;
typedef struct _GstBtWaveReplayClass GstBtWaveReplayClass;

/**
 * GstBtWaveReplay:
 *
 * Class instance data.
 */
struct _GstBtWaveReplay
{
  GstBtAudioSynth parent;

  /* < private > */
  GstBtOscWave *osc;  
};

struct _GstBtWaveReplayClass
{
  GstBtAudioSynthClass parent_class;
};

GType gstbt_wave_replay_get_type (void);

G_END_DECLS
#endif /* __GSTBT_WAVE_REPLAY_H__ */
