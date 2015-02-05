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

#ifndef __GSTBT_AUDIO_DELAY_H__
#define __GSTBT_AUDIO_DELAY_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gst/delay.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_AUDIO_DELAY            (gstbt_audio_delay_get_type())
#define GSTBT_AUDIO_DELAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_AUDIO_DELAY,GstBtAudioDelay))
#define GSTBT_IS_AUDIO_DELAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_AUDIO_DELAY))
#define GSTBT_AUDIO_DELAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_AUDIO_DELAY,GstBtAudioDelayClass))
#define GSTBT_IS_AUDIO_DELAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_AUDIO_DELAY))
#define GSTBT_AUDIO_DELAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_AUDIO_DELAY,GstBtAudioDelayClass))

typedef struct _GstBtAudioDelay      GstBtAudioDelay;
typedef struct _GstBtAudioDelayClass GstBtAudioDelayClass;

/**
 * GstBtAudioDelay:
 *
 * Class instance data.
 */
struct _GstBtAudioDelay {
  GstBaseTransform parent;

  /* < private > */
  /* properties */
  guint drywet;
  guint feedback;

  gint samplerate;
  GstBtDelay *delay;

  /* tempo handling */
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  GstClockTime ticktime;
};

struct _GstBtAudioDelayClass {
  GstBaseTransformClass parent_class;
};

GType gstbt_audio_delay_get_type (void);

G_END_DECLS

#endif /* __GSTBT_AUDIO_DELAY_H__ */
