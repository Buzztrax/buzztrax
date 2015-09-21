/* Buzztrax
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * sidsync.h: c64 sid synthesizer voice
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
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

#ifndef __GSTBT_SID_SYNV_V_H__
#define __GSTBT_SID_SYNV_V_H__

#include <gst/gst.h>
#include "gst/musicenums.h"

G_BEGIN_DECLS
#define GSTBT_TYPE_SID_SYNV            (gstbt_sid_synv_get_type())
#define GSTBT_SID_SYNV(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_SID_SYNV,GstBtSidSynV))
#define GSTBT_IS_SID_SYNV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_SID_SYNV))
#define GSTBT_SID_SYNV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_SID_SYNV,GstBtSidSynVClass))
#define GSTBT_IS_SID_SYNV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_SID_SYNV))
#define GSTBT_SID_SYNV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_SID_SYNV,GstBtSidSynVClass))

/**
 * GstBtSidSynWave:
 * @GSTBT_SID_SYN_WAVE_TRIANGLE: triangle wave
 * @GSTBT_SID_SYN_WAVE_SAW: saw wave
 * @GSTBT_SID_SYN_WAVE_SAW_TRIANGLE: saw + triangle wave 
 * @GSTBT_SID_SYN_WAVE_PULSE: square wave with pulse width modulation
 * @GSTBT_SID_SYN_WAVE_PULSE_TRIANGLE: square + triangle wave
 * @GSTBT_SID_SYN_WAVE_PULSE_SAW: square + saw wave
 * @GSTBT_SID_SYN_WAVE_PULSE_SAW_TRIANGLE: square + saw + triangle wave
 * @GSTBT_SID_SYN_WAVE_NOISE: noise
 *
 * Oscillator wave forms.
 */
typedef enum
{
  GSTBT_SID_SYN_WAVE_TRIANGLE = (1<<0),
  GSTBT_SID_SYN_WAVE_SAW = (1<<1),
  GSTBT_SID_SYN_WAVE_SAW_TRIANGLE = ((1<<1)|(1<<0)),
  GSTBT_SID_SYN_WAVE_PULSE = (1<<2),
  GSTBT_SID_SYN_WAVE_PULSE_TRIANGLE = ((1<<2)|(1<<0)),
  GSTBT_SID_SYN_WAVE_PULSE_SAW = ((1<<2)|(1<<1)),
  GSTBT_SID_SYN_WAVE_PULSE_SAW_TRIANGLE = ((1<<2)|(1<<1)|(1<<0)),
  GSTBT_SID_SYN_WAVE_NOISE = (1<<3)
} GstBtSidSynWave;

/**
 * GstBtSidSynEffect:
 * @GSTBT_SID_SYN_EFFECT_ARPEGGIO: arpeggio
 * @GSTBT_SID_SYN_EFFECT_PORTAMENTO_UP: portamento up
 * @GSTBT_SID_SYN_EFFECT_PORTAMENTO_DOWN: portamento down
 * @GSTBT_SID_SYN_EFFECT_PORTAMENTO: portamento
 * @GSTBT_SID_SYN_EFFECT_VIBRATO: vibrato
 * @GSTBT_SID_SYN_EFFECT_GLISSANDO_CONTROL: glissando control
 * @GSTBT_SID_SYN_EFFECT_VIBRATO_TYPE: vibrato type
 * @GSTBT_SID_SYN_EFFECT_FINETUNE: finetune
 * @GSTBT_SID_SYN_EFFECT_NONE: none
 *
 * Track effects.
 */
typedef enum
{
  GSTBT_SID_SYN_EFFECT_ARPEGGIO = 0,
  GSTBT_SID_SYN_EFFECT_PORTAMENTO_UP = 1,
  GSTBT_SID_SYN_EFFECT_PORTAMENTO_DOWN = 2,
  GSTBT_SID_SYN_EFFECT_PORTAMENTO = 3,
  GSTBT_SID_SYN_EFFECT_VIBRATO = 4,
  GSTBT_SID_SYN_EFFECT_GLISSANDO_CONTROL = 0xE3,
  GSTBT_SID_SYN_EFFECT_VIBRATO_TYPE = 0xE4,
  GSTBT_SID_SYN_EFFECT_FINETUNE = 0xE5,
  GSTBT_SID_SYN_EFFECT_NONE = 0xFF,
} GstBtSidSynEffect;


typedef struct _GstBtSidSynV GstBtSidSynV;
typedef struct _GstBtSidSynVClass GstBtSidSynVClass;

/**
 * GstBtSidSynV:
 *
 * Class instance data.
 */
struct _GstBtSidSynV
{
  GstObject parent;

  /* < private > */
  /* parameters */
  GstBtNote note, prev_note;
  gboolean gate, sync, ringmod, test, filter;
  GstBtSidSynWave wave;
  guint pulse_width;
  guint attack, decay, sustain, release;
  GstBtSidSynEffect effect_type;
  guint effect_value;
  
  /* state */
  gboolean note_set, effect_set;
  gdouble freq;
  gdouble prev_freq, want_freq;                      // for portamento
  gdouble arp_freq[3];                               // for arpeggio
  gdouble vib_pos, vib_depth, vib_speed, vib_center; // for vibrato
  guint vib_type;
  gdouble finetune, portamento;
  gboolean quantize_freq;                            // glissando control
  gulong fx_ticks_remain;
};

struct _GstBtSidSynVClass
{
  GstObjectClass parent_class;
};

GType gstbt_sid_synv_get_type (void);

G_END_DECLS
#endif /* __GSTBT_SID_SYN_H_V__ */
