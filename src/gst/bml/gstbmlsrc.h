/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * gstbmlsrc.h: Header for BML source plugin
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


#ifndef __GST_BML_SRC_H__
#define __GST_BML_SRC_H__

#include "gstbml.h"

G_BEGIN_DECLS

// that can not work here, as we register a dozen types at once
//#define GST_TYPE_BML_SRC          (gst_bml_src_get_type ())

// this is a bit weak, but better that nothing
#define GST_BML_SRC(obj)            ((GstBMLSrc *)     G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_BASE_SRC, GstBaseSrc))
#define GST_BML_SRC_CLASS(klass)    ((GstBMLSrcClass *)G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_BASE_SRC, GstBaseSrcClass))
#define GST_IS_BML_SRC(obj)         (                  G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_BASE_SRC))
#define GST_IS_BML_SRC_CLASS(klass) (                  G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_BASE_SRC))
#define GST_BML_SRC_GET_CLASS(obj)  ((GstBMLSrcClass *)G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_BASE_SRC, GstBaseSrcClass))

typedef struct _GstBMLSrc GstBMLSrc;
typedef struct _GstBMLSrcClass GstBMLSrcClass;

struct _GstBMLSrc {
  GstBaseSrc parent;
  GstBML bml;
};

struct _GstBMLSrcClass {
  GstBaseSrcClass parent_class;
  GstBMLClass bml_class;
};

extern GType bml(src_get_type(const char *name, gboolean is_polyphonic));

G_END_DECLS

#endif /* __GST_BML_SRC_H__ */
