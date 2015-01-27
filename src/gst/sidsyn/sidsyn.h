/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * sidsyn.h: c64 sid synthesizer
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

#ifndef __GSTBT_SID_SYN_H__
#define __GSTBT_SID_SYN_H__

#include <gst/gst.h>
#include "gst/audiosynth.h"
#include "gst/toneconversion.h"

#include "sidsynv.h"
#ifndef __GTK_DOC_IGNORE__
#include "sidemu.h"
#define NUM_VOICES 3
#define NUM_REGS 29
#else
typedef struct {
} SID;
typedef enum chip_model {
  MOS
} chip_model;
#endif

G_BEGIN_DECLS
#define GSTBT_TYPE_SID_SYN            (gstbt_sid_syn_get_type())
#define GSTBT_SID_SYN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_SID_SYN,GstBtSidSyn))
#define GSTBT_IS_SID_SYN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_SID_SYN))
#define GSTBT_SID_SYN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_SID_SYN,GstBtSidSynClass))
#define GSTBT_IS_SID_SYN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_SID_SYN))
#define GSTBT_SID_SYN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_SID_SYN,GstBtSidSynClass))

typedef struct _GstBtSidSyn GstBtSidSyn;
typedef struct _GstBtSidSynClass GstBtSidSynClass;

/**
 * GstBtSidSyn:
 *
 * Class instance data.
 */
struct _GstBtSidSyn
{
  GstBtAudioSynth parent;

  /* < private > */
  /* parameters */
  gboolean dispose_has_run;     /* validate if dispose has run */

	gint clockrate;
	SID *emu;
	chip_model chip;

	/* states */
	gint regs[NUM_REGS];
	GstBtSidSynV *voices[NUM_VOICES];
	gint cutoff, resonance, volume;
	gboolean filter_low_pass;
	gboolean filter_band_pass;
	gboolean filter_hi_pass;
	gboolean voice_3_off;
	GstBtToneConversionTuning tuning;
	GstBtToneConversion *n2f;
};

struct _GstBtSidSynClass
{
  GstBtAudioSynthClass parent_class;
};

GType gstbt_sid_syn_get_type (void);

G_END_DECLS
#endif /* __GSTBT_SID_SYN_H__ */
