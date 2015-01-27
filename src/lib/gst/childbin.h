/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * childbin.h: helper interface header for multi child gstreamer elements
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

#ifndef __GSTBT_CHILD_BIN_H__
#define __GSTBT_CHILD_BIN_H__

#include <glib-object.h>
#include <gst/gst.h>
#include <gst/gstchildproxy.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_CHILD_BIN               (gstbt_child_bin_get_type())
#define GSTBT_CHILD_BIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTBT_TYPE_CHILD_BIN, GstBtChildBin))
#define GSTBT_IS_CHILD_BIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTBT_TYPE_CHILD_BIN))
#define GSTBT_CHILD_BIN_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSTBT_TYPE_CHILD_BIN, GstBtChildBinInterface))


typedef struct _GstBtChildBin GstBtChildBin; /* dummy object */
typedef struct _GstBtChildBinInterface GstBtChildBinInterface;

/**
 * GstBtChildBin:
 *
 * Opaque interface handle.
 */
/**
 * GstBtChildBinInterface:
 * @parent: parent type
 * @add_child: vmethod for adding a child to the bin
 * @remove_child: vmethod for removing a child from the bin
 *
 * Interface structure.
 */
struct _GstBtChildBinInterface
{
  GTypeInterface parent;

  gboolean (*add_child) (GstBtChildBin *self, GstObject *child);
  gboolean (*remove_child) (GstBtChildBin *self, GstObject *child);
};

GType gstbt_child_bin_get_type(void);

gboolean gstbt_child_bin_add_child (GstBtChildBin *self, GstObject *child);
gboolean gstbt_child_bin_remove_child (GstBtChildBin *self, GstObject *child);

G_END_DECLS

#endif /* __GSTBT_CHILD_BIN_H__ */
