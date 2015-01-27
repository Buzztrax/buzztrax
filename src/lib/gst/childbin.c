/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * childbin.c: helper interface for multi child gstreamer elements
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
/**
 * SECTION:childbin
 * @title: GstBtChildBin
 * @include: libgstbuzztrax/childbin.h
 * @short_description: helper interface for multi child gstreamer elements
 *
 * This interface provides an extension to the #GstChildProxy interface, which
 * is useful for classes that have identical children.
 *
 * The interface provides a #GstBtChildBin:children property for the number of
 * children as well as two methods to add and remove children.
 */

#include "childbin.h"

//-- the iface

G_DEFINE_INTERFACE (GstBtChildBin, gstbt_child_bin, GST_TYPE_CHILD_PROXY);


/**
 * gstbt_child_bin_add_child:
 * @self: a #GObject that implements #GstBtChildBin
 * @child: the #GstObject to add as a child
 *
 * Add the given child to the list of children.
 *
 * Returns: %TRUE for success
 */
gboolean
gstbt_child_bin_add_child (GstBtChildBin * self, GstObject * child)
{
  g_return_val_if_fail (GSTBT_IS_CHILD_BIN (self), FALSE);

  return (GSTBT_CHILD_BIN_GET_INTERFACE (self)->add_child (self, child));
}

/**
 * gstbt_child_bin_remove_child:
 * @self: a #GObject that implements #GstBtChildBin
 * @child: the #GstObject to remove from the children
 *
 * Remove the given child from the list of children.
 *
 * Returns: %TRUE for success
 */
gboolean
gstbt_child_bin_remove_child (GstBtChildBin * self, GstObject * child)
{
  g_return_val_if_fail (GSTBT_IS_CHILD_BIN (self), FALSE);

  return (GSTBT_CHILD_BIN_GET_INTERFACE (self)->remove_child (self, child));
}

static void
gstbt_child_bin_default_init (GstBtChildBinInterface * iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_ulong ("children",
          "children count property",
          "the number of children this element uses",
          1, G_MAXULONG, 1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
