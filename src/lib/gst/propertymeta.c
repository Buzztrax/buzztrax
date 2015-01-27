/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * propertymeta.c: helper interface for extended gstreamer element meta data
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
 * SECTION:propertymeta
 * @title: GstBtPropertyMeta
 * @include: libgstbuzztrax/propertymeta.h
 * @short_description: helper interface for extended gstreamer element meta data
 *
 * This interface standardises some additional meta-data that is attached to
 * #GObject properties.
 *
 * Furthermore it adds the gstbt_property_meta_describe_property() method that
 * builds a string description of a property value.
 */

#include "propertymeta.h"

//-- the iface

G_DEFINE_INTERFACE (GstBtPropertyMeta, gstbt_property_meta, 0);


/**
 * gstbt_property_meta_quark:
 *
 * Only if this is set to TRUE, there is property meta data for this property..
 */
GQuark gstbt_property_meta_quark;
/**
 * gstbt_property_meta_quark_min_val:
 *
 * Minimum property value (excluding default and no-value).
 */
GQuark gstbt_property_meta_quark_min_val = 0;
/**
 * gstbt_property_meta_quark_max_val:
 *
 * Maximum property value (excluding default and no-value).
 */
GQuark gstbt_property_meta_quark_max_val = 0;
/**
 * gstbt_property_meta_quark_def_val:
 *
 * Default property value (used initialy).
 */
GQuark gstbt_property_meta_quark_def_val = 0;
/**
 * gstbt_property_meta_quark_no_val:
 *
 * Property value (used in trigger style properties, when there is no current
 * value)
 */
GQuark gstbt_property_meta_quark_no_val = 0;
/**
 * gstbt_property_meta_quark_flags:
 *
 * Application specific flags giving more hints about the property.
 */
GQuark gstbt_property_meta_quark_flags = 0;

/**
 * gstbt_property_meta_describe_property:
 * @self: a #GObject that implements #GstBtPropertyMeta
 * @property_id: the property index
 * @value: the current property value
 *
 * Formats the gives value as a human readable string. The method is useful to
 * pretty print a property value to be shown in a user interface.
 * It provides a default implementation.
 *
 * Returns: a string with the value in human readable form, free memory when
 * done
 */
 // TODO(ensonic): make index generic (property name)
gchar *
gstbt_property_meta_describe_property (GstBtPropertyMeta * self,
    guint property_id, const GValue * value)
{
  g_return_val_if_fail (GSTBT_IS_PROPERTY_META (self), FALSE);

  GstBtPropertyMetaInterface *iface = GSTBT_PROPERTY_META_GET_INTERFACE (self);
  if (iface->describe_property) {
    return (iface->describe_property (self, property_id, value));
  } else {
    return (g_strdup_value_contents (value));
  }
}

static void
gstbt_property_meta_default_init (GstBtPropertyMetaInterface * iface)
{
  gstbt_property_meta_quark = g_quark_from_string ("GstBtPropertyMeta::");
  gstbt_property_meta_quark_min_val =
      g_quark_from_string ("GstBtPropertyMeta::min-val");
  gstbt_property_meta_quark_max_val =
      g_quark_from_string ("GstBtPropertyMeta::max-val");
  gstbt_property_meta_quark_def_val =
      g_quark_from_string ("GstBtPropertyMeta::def-val");
  gstbt_property_meta_quark_no_val =
      g_quark_from_string ("GstBtPropertyMeta::no-val");
  gstbt_property_meta_quark_flags =
      g_quark_from_string ("GstBtPropertyMeta::flags");
}
