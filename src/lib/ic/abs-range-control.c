/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:bticabsrangecontrol
 * @short_description: buzztraxxs interaction controller single absolute range control
 *
 * Absolute value-range control. The state of the hardware control can be read from
 * BtIcAbsRangeControl:value.
 */
#define BTIC_CORE
#define BTIC_ABS_RANGE_CONTROL_C

#include "ic_private.h"

enum
{
  ABS_RANGE_CONTROL_VALUE = 1,
  ABS_RANGE_CONTROL_MIN,
  ABS_RANGE_CONTROL_MAX,
  ABS_RANGE_CONTROL_DEF
};

struct _BtIcAbsRangeControlPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  glong value;
  glong min, max, def;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtIcAbsRangeControl, btic_abs_range_control, BTIC_TYPE_CONTROL,
    G_ADD_PRIVATE(BtIcAbsRangeControl));

//-- helper

//-- handler

//-- constructor methods

/**
 * btic_abs_range_control_new:
 * @device: the device it belongs to
 * @name: human readable name
 * @id: unique identifier per device
 * @min: minimum value
 * @max: maximum value
 * @def: default value
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcAbsRangeControl *
btic_abs_range_control_new (const BtIcDevice * device, const gchar * name,
    guint id, glong min, glong max, glong def)
{
  BtIcAbsRangeControl *self =
      BTIC_ABS_RANGE_CONTROL (g_object_new (BTIC_TYPE_ABS_RANGE_CONTROL,
          "device", device, "name", name, "id", id, "min", min, "max", max,
          "def", def, NULL));
  if (!self) {
    goto Error;
  }
  // register myself with the device
  btic_device_add_control (device, BTIC_CONTROL (self));
  return self;
Error:
  g_object_try_unref (self);
  return NULL;
}


//-- methods

//-- wrapper

//-- class internals

static void
btic_abs_range_control_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  const BtIcAbsRangeControl *const self = BTIC_ABS_RANGE_CONTROL (object);
  return_if_disposed ();
  switch (property_id) {
    case ABS_RANGE_CONTROL_VALUE:
      g_value_set_long (value, self->priv->value);
      break;
    case ABS_RANGE_CONTROL_MIN:
      g_value_set_long (value, self->priv->min);
      break;
    case ABS_RANGE_CONTROL_MAX:
      g_value_set_long (value, self->priv->max);
      break;
    case ABS_RANGE_CONTROL_DEF:
      g_value_set_long (value, self->priv->def);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_abs_range_control_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  const BtIcAbsRangeControl *const self = BTIC_ABS_RANGE_CONTROL (object);
  return_if_disposed ();
  switch (property_id) {
    case ABS_RANGE_CONTROL_VALUE:
      self->priv->value = g_value_get_long (value);
      break;
    case ABS_RANGE_CONTROL_MIN:
      self->priv->min = g_value_get_long (value);
      break;
    case ABS_RANGE_CONTROL_MAX:
      self->priv->max = g_value_get_long (value);
      break;
    case ABS_RANGE_CONTROL_DEF:
      self->priv->def = g_value_get_long (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_abs_range_control_dispose (GObject * const object)
{
  const BtIcAbsRangeControl *const self = BTIC_ABS_RANGE_CONTROL (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_abs_range_control_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_abs_range_control_init (BtIcAbsRangeControl * self)
{
  self->priv = btic_abs_range_control_get_instance_private(self);
}

static void
btic_abs_range_control_class_init (BtIcAbsRangeControlClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = btic_abs_range_control_set_property;
  gobject_class->get_property = btic_abs_range_control_get_property;
  gobject_class->dispose = btic_abs_range_control_dispose;

  g_object_class_install_property (gobject_class, ABS_RANGE_CONTROL_VALUE,
      g_param_spec_long ("value", "value prop", "control value", G_MINLONG,
          G_MAXLONG, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ABS_RANGE_CONTROL_MIN,
      g_param_spec_long ("min", "min prop", "minimum control value", G_MINLONG,
          G_MAXLONG, G_MINLONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ABS_RANGE_CONTROL_MAX,
      g_param_spec_long ("max", "max prop", "maximum control value", G_MINLONG,
          G_MAXLONG, G_MAXLONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ABS_RANGE_CONTROL_DEF,
      g_param_spec_long ("def", "def prop", "default control value", G_MINLONG,
          G_MAXLONG, 0,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
