/* Buzztrax
 *
 * Copyright (C) 2011 Stefan Sauer <ensonic@users.sf.net>
 *
 * gstdirectcontrolbinding.h: Direct attachment for control sources
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GSTBT_DIRECT_CONTROL_BINDING_H__
#define __GSTBT_DIRECT_CONTROL_BINDING_H__

#include <gst/gstcontrolsource.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_DIRECT_CONTROL_BINDING \
  (gstbt_direct_control_binding_get_type())
#define GSTBT_DIRECT_CONTROL_BINDING(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_DIRECT_CONTROL_BINDING,GstBtDirectControlBinding))
#define GSTBT_DIRECT_CONTROL_BINDING_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GSTBT_TYPE_DIRECT_CONTROL_BINDING,GstBtDirectControlBindingClass))
#define GSTBT_IS_DIRECT_CONTROL_BINDING(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_DIRECT_CONTROL_BINDING))
#define GSTBT_IS_DIRECT_CONTROL_BINDING_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GSTBT_TYPE_DIRECT_CONTROL_BINDING))
#define GSTBT_DIRECT_CONTROL_BINDING_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSTBT_TYPE_CONTOL_SOURCE, GstBtDirectControlBindingClass))

typedef struct _GstBtDirectControlBinding GstBtDirectControlBinding;
typedef struct _GstBtDirectControlBindingClass GstBtDirectControlBindingClass;

/**
 * GstBtDirectControlBindingConvertValue:
 * @self: the #GstBtDirectControlBinding instance
 * @src_value: the value returned by the cotnrol source
 * @dest_value: the target location
 *
 * Function to map a control-value to the target plain data type.
 */
typedef void (* GstBtDirectControlBindingConvertValue) (GstBtDirectControlBinding *self, gdouble src_value, gpointer dest_value);

/**
 * GstBtDirectControlBindingConvertGValue:
 * @self: the #GstBtDirectControlBinding instance
 * @src_value: the value returned by the cotnrol source
 * @dest_value: the target GValue
 *
 * Function to map a control-value to the target GValue.
 */
typedef void (* GstBtDirectControlBindingConvertGValue) (GstBtDirectControlBinding *self, gdouble src_value, GValue *dest_value);

/**
 * GstBtDirectControlBinding:                                   
 * @name: name of the property of this binding
 *
 * The instance structure of #GstBtDirectControlBinding.
 */
struct _GstBtDirectControlBinding {
  GstControlBinding parent;
  
  /*< private >*/
  GstControlSource *cs;    /* GstControlSource for this property */
  GValue cur_value;
  gdouble last_value;
  gint byte_size;

  GstBtDirectControlBindingConvertValue convert_value;
  GstBtDirectControlBindingConvertGValue convert_g_value;

  union {
    gpointer _gstbt_reserved[GST_PADDING];
    struct {
      gboolean want_absolute;
    } abi;
  } ABI;
};

/**
 * GstBtDirectControlBindingClass:
 * @parent_class: Parent class
 * @convert: Class method to convert control-values
 *
 * The class structure of #GstBtDirectControlBinding.
 */

struct _GstBtDirectControlBindingClass
{
  GstControlBindingClass parent_class;

  /*< private >*/
  gpointer _gstbt_reserved[GST_PADDING];
};

GType gstbt_direct_control_binding_get_type (void);

/* Functions */

GstControlBinding * gst_direct_control_binding_new_absolute (GstObject * object, const gchar * property_name, 
                                                    GstControlSource * cs);

G_END_DECLS

#endif /* __GSTBT_DIRECT_CONTROL_BINDING_H__ */
