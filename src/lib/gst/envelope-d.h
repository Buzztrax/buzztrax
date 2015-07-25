/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * envelope-d.h: decay envelope generator
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

#ifndef __GSTBT_ENVELOPE_D_H__
#define __GSTBT_ENVELOPE_D_H__

#include <gst/gst.h>
#include "gst/envelope.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_ENVELOPE_D            (gstbt_envelope_d_get_type())
#define GSTBT_ENVELOPE_D(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_ENVELOPE_D,GstBtEnvelopeD))
#define GSTBT_IS_ENVELOPE_D(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_ENVELOPE_D))
#define GSTBT_ENVELOPE_D_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_ENVELOPE_D,GstBtEnvelopeDClass))
#define GSTBT_IS_ENVELOPE_D_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_ENVELOPE_D))
#define GSTBT_ENVELOPE_D_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_ENVELOPE_D,GstBtEnvelopeDClass))

typedef struct _GstBtEnvelopeD GstBtEnvelopeD;
typedef struct _GstBtEnvelopeDClass GstBtEnvelopeDClass;

/**
 * GstBtEnvelopeD:
 *
 * Class instance data.
 */
struct _GstBtEnvelopeD {
  GstBtEnvelope parent;
  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */
  
  /* parameters */
  gdouble peak_level, decay, floor_level;
};

struct _GstBtEnvelopeDClass {
  GstBtEnvelopeClass parent_class;
};

GType gstbt_envelope_d_get_type (void);

GstBtEnvelopeD *gstbt_envelope_d_new (void);
void gstbt_envelope_d_setup (GstBtEnvelopeD *self, gint samplerate);

G_END_DECLS

#endif /* __GSTBT_ENVELOPE_D_H__ */
