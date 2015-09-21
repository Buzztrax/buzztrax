/* Buzztrax
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * osc-wave.h: wavetable oscillator
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

#ifndef __GSTBT_OSC_WAVE_H__
#define __GSTBT_OSC_WAVE_H__

#include <gst/gst.h>
#include "gst/toneconversion.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_OSC_WAVE            (gstbt_osc_wave_get_type())
#define GSTBT_OSC_WAVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_OSC_WAVE,GstBtOscWave))
#define GSTBT_IS_OSC_WAVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_OSC_WAVE))
#define GSTBT_OSC_WAVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_OSC_WAVE,GstBtOscWaveClass))
#define GSTBT_IS_OSC_WAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_OSC_WAVE))
#define GSTBT_OSC_WAVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_OSC_WAVE,GstBtOscWaveClass))

typedef struct _GstBtOscWave GstBtOscWave;
typedef struct _GstBtOscWaveClass GstBtOscWaveClass;

/**
 * GstBtOscWave:
 *
 * Class instance data.
 */
struct _GstBtOscWave {
  GObject parent;

  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */
  /* parameters */
  gpointer *wave_callbacks;
  guint wave, wave_level;
  gdouble freq;

  /* oscillator state */
  GstBtToneConversion *n2f;
  GstBuffer *data;
  GstMapInfo map_info;
  gint channels;
  gdouble rate;
  guint64 duration;

  /* < private > */
  gboolean (*process) (GstBtOscWave *, guint64, guint, gint16 *);  
};

struct _GstBtOscWaveClass {
  GObjectClass parent_class;
};

void gstbt_osc_wave_setup(GstBtOscWave * self);

GType gstbt_osc_wave_get_type(void);

GstBtOscWave *gstbt_osc_wave_new(void);

G_END_DECLS
#endif /* __GSTBT_OSC_WAVE_H__ */
