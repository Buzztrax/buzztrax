/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * propertymeta.h: helper interface header for extendet gstreamer element meta data
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

#ifndef __GSTBT_PROPERTY_META_H__
#define __GSTBT_PROPERTY_META_H__

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_PROPERTY_META               (gstbt_property_meta_get_type())
#define GSTBT_PROPERTY_META(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTBT_TYPE_PROPERTY_META, GstBtPropertyMeta))
#define GSTBT_IS_PROPERTY_META(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTBT_TYPE_PROPERTY_META))
#define GSTBT_PROPERTY_META_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSTBT_TYPE_PROPERTY_META, GstBtPropertyMetaInterface))

typedef struct _GstBtPropertyMeta GstBtPropertyMeta; /* dummy object */
typedef struct _GstBtPropertyMetaInterface GstBtPropertyMetaInterface;

/**
 * GstBtPropertyMeta:
 *
 * Opaque interface handle.
 */
/**
 * GstBtPropertyMetaInterface:
 * @parent: parent type
 * @describe_property: vmethod for returning a human readable property value
 *                     description
 *
 * Interface structure.
 */
struct _GstBtPropertyMetaInterface
{
  GTypeInterface parent;

  gchar *(*describe_property) (GstBtPropertyMeta *self, guint property_id, const GValue *value);
};

GType gstbt_property_meta_get_type(void);

gchar *gstbt_property_meta_describe_property (GstBtPropertyMeta *self, guint property_id, const GValue *value);

extern GQuark gstbt_property_meta_quark;
extern GQuark gstbt_property_meta_quark_min_val;
extern GQuark gstbt_property_meta_quark_max_val;
extern GQuark gstbt_property_meta_quark_def_val;
extern GQuark gstbt_property_meta_quark_no_val;
extern GQuark gstbt_property_meta_quark_flags;

G_END_DECLS

/**
 * GstBtPropertyMetaFlags:
 * @GSTBT_PROPERTY_META_NONE: no special treatment needed
 * @GSTBT_PROPERTY_META_WAVE: parameter value references a wavetable slot
 * @GSTBT_PROPERTY_META_STATE: parameter is continuously changing (not used for notes and triggers)
 * @GSTBT_PROPERTY_META_TICK_ON_EDIT: need to call tick after editing it
 *
 * Parameter flags to describe their behaviour.
 */
/* FIXME(ensonic): eliminate this
 * GSTBT_PROPERTY_META_WAVE -> use a enum type for those properties
 * GSTBT_PROPERTY_META_STATE -> GParamSpec->flags&G_PARAM_READABLE
 * GSTBT_PROPERTY_META_TICK_ON_EDIT -> ignore as we do now
 */
typedef enum {
  GSTBT_PROPERTY_META_NONE=0,
  GSTBT_PROPERTY_META_WAVE=1,		/* parameter value references a wavetable slot */
  GSTBT_PROPERTY_META_STATE=2,		/* parameter is continuously changing (not used for notes and triggers) */
  GSTBT_PROPERTY_META_TICK_ON_EDIT=4 /* causes the host to send you a Tick() message immediatly after the user enters a value in the pattern or machine editor */
} GstBtPropertyMetaFlags;

#endif /* __GSTBT_PROPERTY_META_H__ */
