/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * envelope.h: envelope generator for gstreamer
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

#ifndef __GSTBT_ENVELOPE_H__
#define __GSTBT_ENVELOPE_H__

#include <gst/gst.h>
#include <gst/controller/gstinterpolationcontrolsource.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_ENVELOPE            (gstbt_envelope_get_type())
#define GSTBT_ENVELOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_ENVELOPE,GstBtEnvelope))
#define GSTBT_IS_ENVELOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_ENVELOPE))
#define GSTBT_ENVELOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_ENVELOPE,GstBtEnvelopeClass))
#define GSTBT_IS_ENVELOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_ENVELOPE))
#define GSTBT_ENVELOPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_ENVELOPE,GstBtEnvelopeClass))

typedef struct _GstBtEnvelope GstBtEnvelope;
typedef struct _GstBtEnvelopeClass GstBtEnvelopeClass;

/**
 * GstBtEnvelope:
 * @value: current envelope value
 *
 * Class instance data.
 */
struct _GstBtEnvelope {
  GObject parent;
  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */

  /* < public > */
  /* parameters */
  gdouble value;

  /* < private > */
  GstTimedValueControlSource *cs;
  guint64 offset, length;
};

struct _GstBtEnvelopeClass {
  GObjectClass parent_class;
};

GType gstbt_envelope_get_type (void);

gdouble gstbt_envelope_get (GstBtEnvelope *self, guint offset);
gboolean gstbt_envelope_is_running (GstBtEnvelope *self);

G_END_DECLS

#endif /* __GSTBT_ENVELOPE_H__ */
