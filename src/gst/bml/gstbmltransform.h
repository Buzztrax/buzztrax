/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * gstbmltransform.h: Header for BML transform plugin
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


#ifndef __GST_BML_TRANSFORM_H__
#define __GST_BML_TRANSFORM_H__

#include "gstbml.h"
#include "gstbmlorc.h"

G_BEGIN_DECLS

// that can not work here, as we register a dozen types at once
//#define GST_TYPE_BML                    (gst_bml_transform_get_type ())

// this is a bit weak, but better that nothing
#define GST_BML_TRANSFORM(obj)            ((GstBMLTransform *)     G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_BASE_TRANSFORM, GstBaseTransform))
#define GST_BML_TRANSFORM_CLASS(klass)    ((GstBMLTransformClass *)G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_BASE_TRANSFORM, GstBaseTransformClass))
#define GST_IS_BML_TRANSFORM(obj)         (                        G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_BASE_TRANSFORM))
#define GST_IS_BML_TRANSFORM_CLASS(klass) (                        G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_BASE_TRANSFORM))
#define GST_BML_TRANSFORM_GET_CLASS(obj)  ((GstBMLTransformClass *)G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_BASE_TRANSFORM, GstBaseTransformClass))

typedef struct _GstBMLTransform GstBMLTransform;
typedef struct _GstBMLTransformClass GstBMLTransformClass;

struct _GstBMLTransform {
  GstBaseTransform parent;
  GstBML bml;
};

struct _GstBMLTransformClass {
  GstBaseTransformClass parent_class;
  GstBMLClass bml_class;
};

extern GType bml(transform_get_type(const char *name, gboolean is_polyphonic));

G_END_DECLS

#endif /* __GST_BML_TRANSFORM_H__ */
