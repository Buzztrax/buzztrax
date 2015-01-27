/* GStreamer
 * Copyright (C) 2004 Stefan Kost <ensonic at user.sf.net>
 *
 * gstbml.h: Header for BML plugin
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


#ifndef __GST_BMLV_H__
#define __GST_BMLV_H__

#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

// this is a bit weak, but better that nothing
#define GST_BMLV(obj) ((GstBMLV *)obj)
#define GST_BMLV_CLASS(klass) ((GstBMLVClass *)klass)
#define GST_BMLV_GET_CLASS(obj)  ((GstBMLVClass *)G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_OBJECT, GstObjectClass))


typedef struct _GstBMLV GstBMLV;
typedef struct _GstBMLVClass GstBMLVClass;

struct _GstBMLV {
  GstObject object;

  gboolean dispose_has_run;

  // the buzz machine handle (to use with libbml API)
  gpointer bm;

  // the parent gst-element
  //GstElement *parent;
  // the voice number
  guint voice;

  // array with an entry for each parameter
  // flags that a g_object_set has set a value for a trigger param
  gint * volatile triggers_changed;
};

struct _GstBMLVClass {
  GstObjectClass parent_class;

  // the buzz machine handle (to use with libbml API)
  gpointer bmh;
  
  gint numtrackparams;

  // param specs
  GParamSpec **track_property;
};

extern GType bml(v_get_type(const gchar *name));

G_END_DECLS

#endif /* __GST_BMLV_H__ */
