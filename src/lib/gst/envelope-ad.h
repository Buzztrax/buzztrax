/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-ad.h: attack-decay envelope generator
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

#ifndef __GSTBT_ENVELOPE_AD_H__
#define __GSTBT_ENVELOPE_AD_H__

#include <gst/gst.h>
#include "gst/envelope.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_ENVELOPE_AD            (gstbt_envelope_ad_get_type())
#define GSTBT_ENVELOPE_AD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_ENVELOPE_AD,GstBtEnvelopeAD))
#define GSTBT_IS_ENVELOPE_AD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_ENVELOPE_AD))
#define GSTBT_ENVELOPE_AD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_ENVELOPE_AD,GstBtEnvelopeADClass))
#define GSTBT_IS_ENVELOPE_AD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_ENVELOPE_AD))
#define GSTBT_ENVELOPE_AD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_ENVELOPE_AD,GstBtEnvelopeADClass))

typedef struct _GstBtEnvelopeAD GstBtEnvelopeAD;
typedef struct _GstBtEnvelopeADClass GstBtEnvelopeADClass;

/**
 * GstBtEnvelopeAD:
 *
 * Class instance data.
 */
struct _GstBtEnvelopeAD {
  GstBtEnvelope parent;
  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */
  
  /* parameters */
  gdouble attack, peak_level, decay, floor_level;
};

struct _GstBtEnvelopeADClass {
  GstBtEnvelopeClass parent_class;
};

GType gstbt_envelope_ad_get_type (void);

GstBtEnvelopeAD *gstbt_envelope_ad_new (void);
void gstbt_envelope_ad_setup (GstBtEnvelopeAD *self, gint samplerate);

G_END_DECLS

#endif /* __GSTBT_ENVELOPE_AD_H__ */
